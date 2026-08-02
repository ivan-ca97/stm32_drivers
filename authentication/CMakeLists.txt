cmake_minimum_required(VERSION 3.15)

project(reactor LANGUAGES CXX)

add_subdirectory(authentication)
add_subdirectory(auth_command_handler)

target_link_libraries(${CMAKE_PROJECT_NAME}
    authentication
    auth_command_handler
)