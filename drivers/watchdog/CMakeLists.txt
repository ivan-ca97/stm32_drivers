cmake_minimum_required(VERSION 3.15)
project(watchdog_driver LANGUAGES CXX)

add_library(watchdog_driver
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/watchdog.cpp
)

target_include_directories(watchdog_driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(watchdog_driver
    base
    ${STM32_BASE_LIBRARIES}
)
