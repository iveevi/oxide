cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)

project(oxide CXX)

add_executable(oxide
	source/main.cpp)

include_directories(.)

target_compile_options(oxide PRIVATE -fconcepts-diagnostics-depth=2)

target_link_libraries(oxide PRIVATE fmt)
