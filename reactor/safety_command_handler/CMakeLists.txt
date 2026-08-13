cmake_minimum_required(VERSION 3.15)
project(safety_command_handler LANGUAGES CXX)

# Only the wire contract so far; the handler that serves it is not written yet.
add_library(safety_command_handler INTERFACE)

target_include_directories(safety_command_handler INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)
