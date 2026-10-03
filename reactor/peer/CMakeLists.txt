cmake_minimum_required(VERSION 3.15)
project(peer LANGUAGES CXX)

add_library(peer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/link.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/measurement_publisher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/measurement_reader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/monitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/parameter_publisher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/parameter_reader.cpp
)

target_include_directories(peer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(peer PUBLIC
    base
    clock
    controller
    i2c_driver
    reactor_measurements
    reactor_parameters
)
