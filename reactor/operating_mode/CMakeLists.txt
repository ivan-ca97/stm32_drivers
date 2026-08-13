cmake_minimum_required(VERSION 3.15)
project(operating_mode LANGUAGES CXX)

add_library(operating_mode INTERFACE)

target_include_directories(operating_mode INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)
