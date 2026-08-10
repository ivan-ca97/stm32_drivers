cmake_minimum_required(VERSION 3.15)
project(measurement_map LANGUAGES CXX)

add_library(measurement_map INTERFACE)

target_include_directories(measurement_map INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)
