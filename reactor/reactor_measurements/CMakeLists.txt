cmake_minimum_required(VERSION 3.15)
project(reactor_measurements LANGUAGES CXX)

# Header-only domain type: no dependencies (see reactor_parameters).
add_library(reactor_measurements INTERFACE)

target_include_directories(reactor_measurements INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)
