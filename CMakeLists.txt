cmake_minimum_required(VERSION 3.15)

if(NOT DEFINED STM32_BASE_LIBRARIES)
    message(FATAL_ERROR "Variable STM32_BASE_LIBRARIES not set.")
endif()

project(stm32_drivers LANGUAGES CXX)

add_subdirectory(drivers/i2c)
add_subdirectory(drivers/timer)
add_subdirectory(lib/queue)
add_subdirectory(lib/custom_exception)

target_link_libraries(${CMAKE_PROJECT_NAME}
    custom_exception
    queue
    timer_driver
    i2c_driver
)