cmake_minimum_required(VERSION 3.17)
project(task-1)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

add_executable(task-1)
target_sources(task-1 PUBLIC main.c)
