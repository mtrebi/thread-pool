# CMake entry point
cmake_minimum_required (VERSION 3.5)
project (thread-pool)
 
## Add headers files
include_directories (
	include/
)
 
set(HEADERS include/SafeQueue.h include/ThreadPool.h)
 
set(SOURCES src/main.cpp)

SET(CMAKE_CXX_FLAGS -pthread)
 
add_compile_options(
	-std=c++11
#        -D_DEBUG
#        -g
) 
 
# main.cpp
add_executable(main ${HEADERS} ${SOURCES})
target_link_libraries(main)
