cmake_minimum_required(VERSION 3.17)
project(coroutine-sort)

string(APPEND CMAKE_EXE_LINKER_FLAGS "-fsanitize=address")

add_executable(coroutine-sort)
target_compile_definitions(coroutine-sort PRIVATE -D CMAKE_C_COMPILER=clang CMAKE_C_STANDARD=11 CMAKE_C_STANDARD_REQUIRED ON _DEFAULT_SOURCE)
target_compile_options(coroutine-sort PRIVATE -g -O1 -lm -lrt -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls)
target_sources(coroutine-sort PRIVATE main.c merge_sort.h merge_sort.c coro.h coro.c coro_data.h elem.h errors.h dynamic_memory_management.h dynamic_memory_management.c)
target_include_directories(coroutine-sort PRIVATE "${PROJECT_SOURCE_DIR}")
target_link_libraries(coroutine-sort PRIVATE m rt)
