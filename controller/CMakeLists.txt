cmake_minimum_required(VERSION 3.15)

project(controller LANGUAGES CXX)

# Header-only generic controller (register-mailbox slave protocol).
add_library(controller INTERFACE)

target_include_directories(controller INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(controller INTERFACE
    i2c_driver
)
