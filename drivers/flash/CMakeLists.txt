cmake_minimum_required(VERSION 3.15)
project(flash_driver LANGUAGES CXX)

add_library(flash_driver
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/flash.cpp
)

target_include_directories(flash_driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(flash_driver
    ${STM32_BASE_LIBRARIES}
)
