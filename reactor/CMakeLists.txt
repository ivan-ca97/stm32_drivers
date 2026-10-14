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

# Persistence of the reactor's data, over the generic flash log.
add_subdirectory(storage)

# Top of the application: composes everything above.
add_subdirectory(reactor)

# The composition root: builds the system on this board and runs it.
add_subdirectory(application)

target_link_libraries(${CMAKE_PROJECT_NAME}
    application
    reactor_core
    safety
)
