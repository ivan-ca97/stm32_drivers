cmake_minimum_required(VERSION 3.15)
project(peer_measurement_reader LANGUAGES CXX)

add_library(peer_measurement_reader
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/peer_measurement_reader.cpp
)

target_include_directories(peer_measurement_reader PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(peer_measurement_reader PUBLIC
    reactor_measurements
    measurement_map
    i2c_driver
    result
)
