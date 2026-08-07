cmake_minimum_required(VERSION 3.15)
project(reactor_parameters LANGUAGES CXX)

# Header-only domain model: no dependencies, so computation modules can use the
# calibration without pulling in the command protocol.
add_library(reactor_parameters INTERFACE)

target_include_directories(reactor_parameters INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)
