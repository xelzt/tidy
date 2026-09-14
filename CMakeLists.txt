cmake_minimum_required(VERSION 3.28.3)

project(Tidy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tidy)
add_executable(tidyd)

add_library(TidyDaemon INTERFACE)
target_include_directories(TidyDaemon INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(TidyCommon STATIC src/common/TidyDescriptor.cpp
                              src/common/InotifyDescriptor.cpp)
target_include_directories(TidyCommon PUBLIC 
                                          ${CMAKE_CURRENT_SOURCE_DIR}/include)


target_sources(tidy
PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/src/tidy/main.cpp)

target_sources(tidyd
PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/src/tidyd/main.cpp)   

target_link_libraries(tidy PRIVATE TidyCommon)
target_link_libraries(tidyd PRIVATE TidyCommon TidyDaemon)