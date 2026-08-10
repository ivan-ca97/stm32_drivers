cmake_minimum_required(VERSION 3.15)
project(measurement_publisher LANGUAGES CXX)

add_library(measurement_publisher
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/measurement_publisher.cpp
)

target_include_directories(measurement_publisher PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(measurement_publisher PUBLIC
    reactor_measurements
    measurement_map
    controller
)
