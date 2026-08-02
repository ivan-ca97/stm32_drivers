cmake_minimum_required(VERSION 3.15)
project(auth_command_handler LANGUAGES CXX)

add_library(auth_command_handler
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/auth_command_handler.cpp
)

target_include_directories(auth_command_handler PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(auth_command_handler PUBLIC
    authentication
    controller
)
