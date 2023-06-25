cmake_minimum_required(VERSION 3.16.0)

set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    message("Setting default build type to Release")
endif()

project(zedis VERSION 0.0.1 LANGUAGES C CXX)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(./src)
add_subdirectory(./test)
