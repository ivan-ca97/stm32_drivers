cmake_minimum_required(VERSION 3.15)
project(clock LANGUAGES CXX)

add_library(clock INTERFACE)

target_include_directories(clock INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)
