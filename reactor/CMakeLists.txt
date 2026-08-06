cmake_minimum_required(VERSION 3.15)

project(reactor LANGUAGES CXX)

# Domain models first: the protocol components depend on them, never the reverse.
add_subdirectory(reactor_measurements)

add_subdirectory(measurement_publisher)

target_link_libraries(${CMAKE_PROJECT_NAME}
    measurement_publisher
)
