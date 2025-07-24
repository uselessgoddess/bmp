cmake_minimum_required(VERSION 3.30)
project(bmp)

set(CMAKE_CXX_STANDARD 23)

add_library(bmp INTERFACE)
target_include_directories(bmp INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_executable(cli src/main.cpp)
target_link_libraries(cli bmp)
