cmake_minimum_required (VERSION 3.9)
project (LANK)

set (LANK_VERSION_MAJOR 1)
set (LANK_VERSION_MINOR 0)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS_RELEASE "-O3 -s -fno-ident -march=native -flto -DNDEBUG")

set(CMAKE_C_STANDARD 11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}")

add_definitions(-D_POSIX_C_SOURCE=200809L)
add_definitions(-D_DEFAULT_SOURCE)

SET(MAIN
    "src/main.c"
    )

add_executable(lank ${MAIN})

set_property(TARGET lank PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(lank rt Threads::Threads)
