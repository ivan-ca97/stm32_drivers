cmake_minimum_required(VERSION 3.15)
project(application LANGUAGES CXX)

add_library(application
    ${CMAKE_CURRENT_SOURCE_DIR}/src/run.cpp
)

target_include_directories(application PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(application PRIVATE
    ads1115_driver
    auth_command_handler
    authentication
    base
    clock
    controller
    flash_driver
    flash_log
    gpio_driver
    i2c_driver
    measurement_calculator
    operating_mode
    parameter_command_handler
    peer
    persistent_storage
    reactor_core
    reactor_parameters
    reactor_storage
    safety
    stm32f4
    timer_driver
    watchdog_driver
    ${STM32_BASE_LIBRARIES}
)

# Until the address strap is wired, each board gets its own build.
set(APPLICATION_BOARD "First" CACHE STRING "Board this build is for: First (0x42) or Second (0x43)")
set_property(CACHE APPLICATION_BOARD PROPERTY STRINGS First Second)
if(APPLICATION_BOARD STREQUAL "Second")
    target_compile_definitions(application PRIVATE APPLICATION_BOARD_SECOND=1)
elseif(NOT APPLICATION_BOARD STREQUAL "First")
    message(FATAL_ERROR "APPLICATION_BOARD must be First or Second, not '${APPLICATION_BOARD}'")
endif()
