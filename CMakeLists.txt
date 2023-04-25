cmake_minimum_required (VERSION 3.1)
project("ls" VERSION "0.0.1" LANGUAGES C)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Default build type: Debug" FORCE)
endif()

file(GLOB_RECURSE LS_SRC "source/*.h" "source/*.c")
add_executable(ls ${LS_SRC})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(ls PRIVATE _DEBUG)
else()
    target_compile_definitions(ls PRIVATE NDEBUG)
endif()
