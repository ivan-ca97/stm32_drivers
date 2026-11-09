cmake_minimum_required(VERSION 3.15)
project(adc LANGUAGES CXX)

add_library(adc
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/adc.cpp
)
target_include_directories(adc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)
target_link_libraries(adc PUBLIC
    base
)
