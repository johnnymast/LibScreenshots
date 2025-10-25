# cmake/LibGraphics.cmake

# Add LibGraphics once
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../Graphics" Graphics_build)

# Optionally expose headers (if not already PUBLIC in LibGraphics)
message(STATUS "Including LibGraphics headers at ${CMAKE_CURRENT_SOURCE_DIR}/../Graphics/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../Graphics/include")

target_link_libraries(LibScreenshots PUBLIC LibGraphics)