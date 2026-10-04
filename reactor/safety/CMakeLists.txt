cmake_minimum_required(VERSION 3.15)
project(safety LANGUAGES CXX)

add_library(safety
    ${CMAKE_CURRENT_SOURCE_DIR}/src/handler.cpp
)

target_include_directories(safety PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(safety PUBLIC
    authentication
    base
    controller
    operating_mode
    peer
    reactor_parameters
)
