cmake_minimum_required(VERSION 3.15)

project(custom_exception LANGUAGES CXX)

add_library(custom_exception
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/custom_exception.cpp
)

target_include_directories(custom_exception PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)