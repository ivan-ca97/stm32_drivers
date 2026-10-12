cmake_minimum_required(VERSION 3.15)
project(flash_log LANGUAGES CXX)

add_library(flash_log
    ${CMAKE_CURRENT_SOURCE_DIR}/src/region.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/store.cpp
)

target_include_directories(flash_log PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(flash_log PUBLIC
    base
    persistent_storage
)
