cmake_minimum_required(VERSION 3.15)
project(gpio_driver LANGUAGES CXX)

add_library(gpio_driver
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/gpio_pin.cpp
)

target_include_directories(gpio_driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(gpio_driver
    ${STM32_BASE_LIBRARIES}
)
