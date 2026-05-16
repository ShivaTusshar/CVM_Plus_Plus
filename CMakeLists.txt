cmake_minimum_required(VERSION 3.14)
project(cvmpp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

add_executable(cvmpp main.cpp)

target_include_directories(cvmpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS cvmpp RUNTIME DESTINATION bin)
