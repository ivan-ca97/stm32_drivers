cmake_minimum_required(VERSION 3.15)
project(measurement_calculator LANGUAGES CXX)

add_library(measurement_calculator
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/measurement_calculator.cpp
)

target_include_directories(measurement_calculator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(measurement_calculator PUBLIC
    reactor_parameters
    reactor_measurements
    clock
)
