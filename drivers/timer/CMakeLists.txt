cmake_minimum_required(VERSION 3.15)

project(timer_driver LANGUAGES CXX)

add_library(timer_driver
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/timer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/timer_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/timer_interrupt_handlers.cpp
)

target_compile_options(timer_driver PUBLIC
    $<$<COMPILE_LANGUAGE:CXX>:-fexceptions>
)

target_include_directories(timer_driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

target_link_libraries(timer_driver
    ${STM32_BASE_LIBRARIES}
)