cmake_minimum_required(VERSION 3.15)
project(serial_authentication LANGUAGES CXX)

add_library(serial_authentication
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/serial_authentication.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/serial_operations.cpp
)

target_include_directories(serial_authentication PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(serial_authentication PUBLIC
    authentication
)