cmake_minimum_required(VERSION 3.15)
project(adc LANGUAGES CXX)

add_library(adc INTERFACE)
target_include_directories(adc INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)
target_link_libraries(adc INTERFACE
    result
)
