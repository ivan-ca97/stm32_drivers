cmake_minimum_required(VERSION 3.15)
project(stm32f4 LANGUAGES CXX)

add_library(stm32f4
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flash_pages.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sys_tick_clock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/unique_id_token_generator.cpp
)

target_include_directories(stm32f4 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(stm32f4
    PUBLIC
        authentication
        base
        clock
        flash_driver
        flash_log
    PRIVATE
        ${STM32_BASE_LIBRARIES}
)
