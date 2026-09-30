cmake_minimum_required(VERSION 3.15)
project(safety_command_handler LANGUAGES CXX)

# The wire contract plus the peer read it needs; the handler that serves it is not
# written yet.
add_library(safety_command_handler
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/peer_parameter_reader.cpp
)

target_include_directories(safety_command_handler PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(safety_command_handler PUBLIC
    parameter_command_handler
    reactor_parameters
    i2c_driver
    base
)
