cmake_minimum_required(VERSION 3.15)
project(set LANGUAGES CXX)

add_library(set INTERFACE)

target_include_directories(set INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)