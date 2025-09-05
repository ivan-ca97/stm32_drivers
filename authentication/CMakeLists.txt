cmake_minimum_required(VERSION 3.15)
project(authentication LANGUAGES CXX)

add_library(authentication
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/session.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/user_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sources/user.cpp
)

target_include_directories(authentication PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)