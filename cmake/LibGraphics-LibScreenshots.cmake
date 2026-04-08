message(STATUS "✅  Configuring LibScreenshots to use LibGraphics")

if (WIN32)
    list(APPEND CMAKE_PREFIX_PATH "C:/Program Files/LibGraphics")
endif()

find_package(LibGraphics REQUIRED)

