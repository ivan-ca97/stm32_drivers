cmake_minimum_required(VERSION 3.15)
project(ads1115_driver LANGUAGES CXX)

add_library(ads1115_driver
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/ads1115.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/ads1115_config.cpp
)

target_include_directories(ads1115_driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(ads1115_driver PUBLIC
    i2c_driver
    adc
)
