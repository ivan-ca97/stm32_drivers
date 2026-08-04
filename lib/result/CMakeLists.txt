cmake_minimum_required(VERSION 3.15)
project(result LANGUAGES CXX)

add_library(result INTERFACE)

target_include_directories(result INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)
