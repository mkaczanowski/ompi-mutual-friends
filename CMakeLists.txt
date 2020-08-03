project(friends)
cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "mrmpi/src/*.cpp")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(MPI)
find_package(Libmemcached)
include_directories(SYSTEM ${MPI_C_INCLUDE_DIRS} ${LIBMEMCACHED_INCLUDE_DIR} mrmpi/src)

set(CMAKE_CXX_STANDARD 14)

add_compile_options(-Wall -Wextra)

add_executable(
    friends
	main.cpp
	memcached.cpp
	mfriends.cpp
    ${SOURCES}
)

target_link_libraries(
    friends
	${MPI_C_LIBRARIES}
	${MPI_CXX_LIBRARIES}
	${LIBMEMCACHED_LIBRARY}
)
