cmake_minimum_required(VERSION 3.13)

project(networked)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(project_options INTERFACE)
add_library(project_warnings INTERFACE)

add_subdirectory(3rdParty)

add_subdirectory(w2)
add_subdirectory(w4)
add_subdirectory(w5)


