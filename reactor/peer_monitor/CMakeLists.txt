cmake_minimum_required(VERSION 3.15)
project(peer_monitor LANGUAGES CXX)

add_library(peer_monitor
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/peer_monitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/peer_monitor_builder.cpp
)

target_include_directories(peer_monitor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(peer_monitor PUBLIC
    peer_measurement_reader
    reactor_measurements
)
