# CMake entry point
cmake_minimum_required (VERSION 3.5)
project (thread-pool)

## Add headers files
include_directories (
	include/
)


set(SOURCES src/main.cpp)

add_compile_options(
	-std=c++11
//	-g
) 

add_executable(main ${SOURCES})
target_link_libraries(main)