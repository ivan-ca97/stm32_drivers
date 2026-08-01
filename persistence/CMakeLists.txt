cmake_minimum_required(VERSION 3.15)
project(persistence LANGUAGES CXX)

# The PersistentStorage abstraction (header-only). Components depend only on this.
add_library(persistent_storage INTERFACE)
target_include_directories(persistent_storage INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

# Flash-backed implementation (header-only template). The app links this.
add_library(flash_store INTERFACE)
target_include_directories(flash_store INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)
target_link_libraries(flash_store INTERFACE
    persistent_storage
    flash_driver
)
