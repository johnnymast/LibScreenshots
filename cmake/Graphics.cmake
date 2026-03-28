set(LIBGRAPHICS_ENABLE_TESTS OFF CACHE BOOL "Disable LibGraphics tests")

include(FetchContent)

FetchContent_Declare(LibGraphics
        GIT_REPOSITORY https://github.com/johnnymast/LibGraphics.git
        GIT_TAG master
)

FetchContent_MakeAvailable(LibGraphics)

target_link_libraries(LibScreenshots PUBLIC LibGraphics)

target_include_directories(LibScreenshots PUBLIC ${LIBGRAPHICS_INCLUDE_DIRS})