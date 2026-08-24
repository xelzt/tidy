cmake_minimum_required(VERSION 3.28.3)

project(Tidy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tidy)
add_executable(tidyd)

target_sources(tidy
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tidy/main.cpp)

target_sources(tidyd
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tidyd/main.cpp)   
