#set(LIBGRAPHICS_ENABLE_TESTS OFF CACHE BOOL "Disable LibGraphics tests")
#
## Add LibGraphics once
#add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../Graphics" Graphics_screenshots_build)
#
## Optionally expose headers (if not already PUBLIC in LibGraphics)
#message(STATUS "Including LibGraphics headers at ${CMAKE_CURRENT_SOURCE_DIR}/../Graphics/include")
#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../Graphics/include")
#
#
#target_link_libraries(Screenshots PUBLIC Graphics)

set(LIBGRAPHICS_ENABLE_TESTS OFF CACHE BOOL "Disable LibGraphics tests")

# Use LibGraphics directly from GitHub if not installed locally
include(FetchContent)

FetchContent_Declare(LibGraphics
        GIT_REPOSITORY https://github.com/johnnymast/LibGraphics.git
        GIT_TAG master
)

# Downloaded content will be available to the current CMake project
FetchContent_MakeAvailable(LibGraphics)

# LibGraphics provides a target named LibGraphics
# Link Screenshots against LibGraphics instead of a local subdirectory
# (The previous add_subdirectory approach is replaced with FetchContent)
# Optionally, LibGraphics headers are automatically included when the target is linked.
target_link_libraries(Screenshots PUBLIC Graphics)


# Ensure LibGraphics include directories are visible to Screenshots
get_target_property(LIBGRAPHICS_INCLUDE_DIRS LibGraphics INTERFACE_INCLUDE_DIRECTORIES)
if(NOT LIBGRAPHICS_INCLUDE_DIRS)
    # Fallback in case LibGraphics doesn't export include dirs
    set(LIBGRAPHICS_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../Graphics/include")
endif()
target_include_directories(Screenshots PUBLIC ${LIBGRAPHICS_INCLUDE_DIRS})