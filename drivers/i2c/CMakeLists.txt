cmake_minimum_required(VERSION 3.15)

project(i2c_driver LANGUAGES CXX)

add_compile_definitions(USE_FULL_LL_DRIVER)

add_library(i2c_driver
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/i2c_bus.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/i2c_bus_master_events.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/i2c_bus_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/i2c_device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/i2c_driver_exceptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/i2c_interrupt_handlers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/i2c_transaction.cpp
)

target_compile_options(i2c_driver PUBLIC
    $<$<COMPILE_LANGUAGE:CXX>:-fexceptions>
)

target_include_directories(i2c_driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(i2c_driver
    ${STM32_BASE_LIBRARIES}
    timer_driver
    custom_exception
    queue
    set
)