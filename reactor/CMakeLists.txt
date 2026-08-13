cmake_minimum_required(VERSION 3.15)

project(reactor LANGUAGES CXX)

# Domain models first: the protocol components depend on them, never the reverse.
add_subdirectory(reactor_parameters)
add_subdirectory(reactor_measurements)
add_subdirectory(operating_mode)

# Wire contract, owned by neither of its consumers.
add_subdirectory(measurement_map)

add_subdirectory(measurement_calculator)
add_subdirectory(parameter_command_handler)
add_subdirectory(safety_command_handler)
add_subdirectory(measurement_publisher)
add_subdirectory(peer_measurement_reader)
add_subdirectory(peer_monitor)

target_link_libraries(${CMAKE_PROJECT_NAME}
    parameter_command_handler
    measurement_publisher
    measurement_calculator
    peer_monitor
)
