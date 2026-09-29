cmake_minimum_required(VERSION 3.15)
project(base LANGUAGES CXX)

add_library(base INTERFACE)

target_include_directories(base INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
