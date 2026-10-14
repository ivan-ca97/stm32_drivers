cmake_minimum_required(VERSION 3.15)
project(reactor_storage LANGUAGES CXX)

add_library(reactor_storage
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flash_calibration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flash_upkeep.cpp
)

target_include_directories(reactor_storage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(reactor_storage PUBLIC
    base
    flash_log
    reactor_parameters
    safety
)
