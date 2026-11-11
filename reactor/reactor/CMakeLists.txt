cmake_minimum_required(VERSION 3.15)
project(reactor_core LANGUAGES CXX)

add_library(reactor_core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/reactor.cpp
)

target_include_directories(reactor_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(reactor_core PUBLIC
    adc
    base
    controller
    operating_mode
    reactor_parameters
    reactor_measurements
    measurement_calculator
    peer
    parameter_command_handler
    gpio_driver
    watchdog_driver
)
