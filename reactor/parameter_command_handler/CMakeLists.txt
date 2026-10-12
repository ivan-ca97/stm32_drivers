cmake_minimum_required(VERSION 3.15)
project(parameter_command_handler LANGUAGES CXX)

add_library(parameter_command_handler
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/parameter_handler.cpp
)

target_include_directories(parameter_command_handler PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(parameter_command_handler PUBLIC
    reactor_parameters
    controller
    authentication
    peer
)
