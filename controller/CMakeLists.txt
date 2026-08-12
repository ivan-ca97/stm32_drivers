cmake_minimum_required(VERSION 3.15)

project(controller LANGUAGES CXX)

# Generic controller (register-mailbox slave protocol).
add_library(controller
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/controller_slave.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/register_bank.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/register_window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/router.cpp
)

target_include_directories(controller PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(controller PUBLIC
    i2c_driver
    result
    clock
)
