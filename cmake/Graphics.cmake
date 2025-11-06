set(LIBGRAPHICS_ENABLE_TESTS OFF CACHE BOOL "Disable LibGraphics tests")

include(FetchContent)

FetchContent_Declare(LibGraphics
        GIT_REPOSITORY https://github.com/johnnymast/LibGraphics.git
        GIT_TAG master
)

FetchContent_MakeAvailable(LibGraphics)

target_link_libraries(LibScreenshots PUBLIC LibGraphics)

get_target_property(LIBGRAPHICS_INCLUDE_DIRS LibGraphics INTERFACE_INCLUDE_DIRECTORIES)
if(NOT LIBGRAPHICS_INCLUDE_DIRS)
    set(LIBGRAPHICS_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../Graphics/include")
endif()
target_include_directories(LibScreenshots PUBLIC ${LIBGRAPHICS_INCLUDE_DIRS})