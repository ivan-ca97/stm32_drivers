cmake_minimum_required(VERSION 3.15)

project(reactor LANGUAGES CXX)

# Domain models first: the protocol components depend on them, never the reverse.
add_subdirectory(reactor_parameters)
add_subdirectory(reactor_measurements)
add_subdirectory(operating_mode)

# Both faces of the link between the boards: what each publishes and how it reads
# the other's.
add_subdirectory(peer)

add_subdirectory(measurement_calculator)
add_subdirectory(parameter_command_handler)

add_subdirectory(safety)

# Top of the application: composes everything above.
add_subdirectory(reactor)

target_link_libraries(${CMAKE_PROJECT_NAME}
    reactor_core
    safety
)
