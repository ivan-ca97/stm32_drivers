#include <cstdint>
#include <string_view>

#include "ads1115.hpp"
#include "application/layout.h"
#include "application/run.h"
#include "auth_command_handler.hpp"
#include "authentication.hpp"
#include "base/result.h"
#include "board.h"
#include "flash.hpp"
#include "flash_log/region.h"
#include "flash_log/store.h"
#include "gpio_pin.hpp"
#include "i2c_bus_builder.hpp"
#include "i2c_bus_static.hpp"
#include "i2c_device.hpp"
#include "measurement_calculator.hpp"
#include "operating_mode.hpp"
#include "parameter_handler.hpp"
#include "peer/link.h"
#include "peer/measurement_publisher.h"
#include "peer/measurement_window.h"
#include "peer/monitor.h"
#include "peer/parameter_publisher.h"
#include "peer/parameter_window.h"
#include "reactor/reactor.h"
#include "reactor_parameters.hpp"
#include "register_bank.hpp"
#include "register_window.hpp"
#include "safety/handler.h"
#include "stm32f4/flash_pages.h"
#include "stm32f4/sys_tick_clock.h"
#include "stm32f4/unique_id_token_generator.h"
#include "storage/flash_calibration.h"
#include "storage/flash_upkeep.h"
#include "timer.hpp"
#include "timer_builder.hpp"
#include "watchdog.hpp"
#include "watchdog_stretcher.h"

// Which board this is. 1 reads it from the strap pin; 0 takes it from the build
// (APPLICATION_BOARD_SECOND), until the strap is wired on both boards.
#ifndef APPLICATION_ADDRESS_STRAP
#define APPLICATION_ADDRESS_STRAP 0
#endif

#ifndef APPLICATION_BOARD_SECOND
#define APPLICATION_BOARD_SECOND 0
#endif

namespace application {
namespace {

// Pass period of the reactor, and the publishing rate of its measurements.
constexpr uint32_t passPeriodMs = 50;

// Against the 50 ms loop. The LSI is specified 17-47 kHz around 32, so 500 ms nominal
// is at least ~340 ms real.
constexpr uint32_t watchdogTimeoutMs = 500;

// While a flash erase stalls the CPU. At x8 parallelism a 16 KB sector takes up to
// ~800 ms (datasheet); 2000 ms nominal is at least ~1360 ms real.
constexpr uint32_t stallWatchdogTimeoutMs = 2000;

// Keys of the persistent storage.
constexpr uint8_t usersKey = 0;
constexpr uint32_t usersCapacity = 512;
constexpr uint8_t calibrationKey = 1;

// Room required to leave maintenance: the calibration, and a few full rewrites of the
// users, which normal operation can still make without compacting.
constexpr uint32_t minimumFreeBytes = 2048;

// A configuration that cannot be built. SCRAM was never released, and the watchdog is
// not running yet, so the board stays here.
[[noreturn]] void stop()
{
    while (true) {
    }
}

enum class Board {
    First,
    Second,
};

struct Addresses {
    uint8_t own;
    uint8_t peer;
};

Board identifyBoard()
{
#if APPLICATION_ADDRESS_STRAP
    GpioInput strap(board::addressStrap.port, board::addressStrap.number, GpioPin::Pull::Up);
    if (strap.get()) {
        return Board::First;
    }
    return Board::Second;
#else
    if (APPLICATION_BOARD_SECOND) {
        return Board::Second;
    }
    return Board::First;
#endif
}

Addresses addressesOf(Board board)
{
    Addresses addresses = {
        .own = layout::firstBoardAddress,
        .peer = layout::secondBoardAddress,
    };
    if (board == Board::Second) {
        addresses = {
            .own = layout::secondBoardAddress,
            .peer = layout::firstBoardAddress,
        };
    }
    return addresses;
}

} // namespace

void run()
{
    Addresses addresses = addressesOf(identifyBoard());

    StaticRegisterBank<128, 4> bank;

    Watchdog watchdog;
    WatchdogStretcher stretcher(watchdog, watchdogTimeoutMs, stallWatchdogTimeoutMs);

    Flash flash;
    base::Result<stm32f4::FlashPages> pages = stm32f4::FlashPages::Builder()
        .flash(flash)
        .sectors(board::firstStoragePage, board::secondStoragePage, board::storagePageSize)
        .stallListener(stretcher)
        .build();
    if (!pages.has_value()) {
        stop();
    }
    base::Result<flash_log::Store> store = flash_log::Store::Builder()
        .pages(*pages)
        .build();
    if (!store.has_value()) {
        stop();
    }
    flash_log::Region userStorage(*store, usersKey, usersCapacity);
    storage::FlashCalibration calibrationStorage(*store, calibrationKey);
    storage::FlashUpkeep storageUpkeep(*store);

    stm32f4::SysTickClock clock;
    stm32f4::UniqueIdTokenGenerator tokenGenerator;
    StaticUserManager<8, 16, 16, 16> users(&userStorage);
    StaticSessionManager<4> sessions(clock, tokenGenerator);
    Authentication authentication(users, sessions);

    if (!users.getUser(std::string_view("admin"))) {
        (void)users.createUser(Permission::Superuser, "admin", "admin", "Administrator");
    }

    base::Result<RegisterWindow> parameterWindow = RegisterWindow::claim(
        bank,
        layout::parametersBase,
        peer::parameter_window::length
    );
    base::Result<RegisterWindow> measurementWindow = RegisterWindow::claim(
        bank,
        layout::measurementsBase,
        peer::measurement_window::length
    );
    if (!parameterWindow.has_value() || !measurementWindow.has_value()) {
        stop();
    }

    ReactorParams parameters;
    OperatingMode mode;

    Timer retryTimer;
    Timer::Builder()
        .timerSelection(board::retryTimer)
        .setFrequency(1000000)
        .buildIn(retryTimer);

    I2cBusStatic<64, 8> sensorBus = I2cBus::Builder()
        .setName("ADC/Peer")
        .withBusSelection(board::sensorBus)
        .setBusSpeed(board::busSpeedHz)
        .withTimer(retryTimer)
        .setRetryIntervalMs(10)
        .buildConfig();

    I2cDevice adcDevice(board::adcAddress, &sensorBus, "ADS1115");
    Ads1115 adc(adcDevice);
    adc.setGain(Ads1115::Gain::Fsr4_096V);   // covers a 3.3 V rail

    I2cDevice peerDevice(addresses.peer, &sensorBus, "peer");

    // The peer lays its bank out the same way.
    base::Result<peer::Link> peerLink = peer::Link::Builder()
        .device(peerDevice)
        .measurementWindow(layout::measurementsBase)
        .parameterWindow(layout::parametersBase)
        .build();
    if (!peerLink.has_value()) {
        stop();
    }

    base::Result<peer::Monitor> peerMonitor = peer::Monitor::Builder()
        .link(*peerLink)
        .clock(clock)
        .consecutiveFailures(3)
        .tolerance(0.05f)
        .build();
    if (!peerMonitor.has_value()) {
        stop();
    }

    peer::MeasurementPublisher measurementPublisher(*measurementWindow);
    peer::ParameterPublisher parameterPublisher(*parameterWindow);

    AuthCommandHandler authenticationHandler(authentication);
    ParameterHandler parameterHandler(
        parameterPublisher,
        authentication,
        parameters,
        calibrationStorage
    );
    base::Result<safety::Handler> safetyHandler = safety::Handler::Builder()
        .authentication(authentication)
        .mode(mode)
        .calibration(parameters, calibrationStorage)
        .peer(*peerLink, *peerMonitor)
        .storage(storageUpkeep, minimumFreeBytes)
        .build();
    if (!safetyHandler.has_value()) {
        stop();
    }

    MeasurementCalculator calculator(parameters, clock);

    GpioOutput scramPin(board::scram.port, board::scram.number, false);   // starts asserted

    base::Result<reactor::Reactor> core = reactor::Reactor::Builder()
        .bank(bank)
        .clock(clock)
        .mode(mode)
        .calibration(parameters)
        .measurements(calculator, measurementPublisher)
        .peerMonitor(*peerMonitor)
        .scramOutput(scramPin)
        .watchdog(watchdog)
        .section(0x01, 0x3F, authenticationHandler)
        .section(0x40, 0x5F, *safetyHandler)
        .parameterSection(0x80, 0xEF, parameterHandler)
        .build();
    if (!core.has_value()) {
        stop();
    }

    I2cBusStatic<64, 8> commandBus = I2cBus::Builder()
        .setName("Commands")
        .withBusSelection(board::commandBus)
        .setBusSpeed(board::busSpeedHz)
        .enableSlave(addresses.own, core->commandSlave())
        .buildConfig();

    const Ads1115::Channel channels[] = {Ads1115::Channel::Ain0, Ads1115::Channel::Ain1};
    uint8_t nextChannel = 0;
    uint32_t lastPass = 0;

    base::Status supervised = watchdog.start(watchdogTimeoutMs);
    if (!supervised.has_value()) {
        stop();
    }

    while (true) {
        adc.service();
        if (!adc.busy()) {
            (void)adc.startSample(channels[nextChannel]);
            nextChannel = (nextChannel + 1) % (sizeof(channels) / sizeof(channels[0]));
        }

        if (clock.millisecondsSince(lastPass) < passPeriodMs) {
            continue;
        }
        lastPass = clock.milliseconds();

        // Channel 0 is the logarithmic one, 1 the linear one. Without a reading yet, a
        // channel enters as zero.
        Adc::MicroVolts logarithmic = adc.readLatestMicroVolts(0);
        Adc::MicroVolts linear = adc.readLatestMicroVolts(1);

        core->service(logarithmic.value_or(0), linear.value_or(0));
    }
}

} // namespace application
