cmake_minimum_required(VERSION 3.28.3)

project(Tidy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tidy)
add_executable(tidyd)

include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG 9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03 #RELEASE 3.11.3
)

FetchContent_MakeAvailable(nlohmann_json)
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
target_link_libraries(tidyd PRIVATE TidyCommon TidyDaemon nlohmann_json::nlohmann_json)