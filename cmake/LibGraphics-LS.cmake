if (WIN32)
    list(APPEND CMAKE_PREFIX_PATH "C:/Program Files/LibGraphics")
endif()

find_package(LibGraphics REQUIRED)
target_link_libraries(LibScreenshots PUBLIC LibGraphics::LibGraphics)
