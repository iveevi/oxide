cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)

project(oxidius CXX)

add_executable(oxidius
	source/formalism.cpp
	source/format.cpp
	source/hash.cpp
	source/lex.cpp
	source/main.cpp
	source/match.cpp
	source/memory.cpp
	source/parse.cpp)

include_directories(.)

target_compile_options(oxidius PRIVATE
	-Wall
	-fno-rtti
	-fsanitize=address
	-fconcepts-diagnostics-depth=2)

target_link_libraries(oxidius PRIVATE fmt
	-fsanitize=address)
