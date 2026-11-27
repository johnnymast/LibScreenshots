message(STATUS "  🔨 Linking Windows")
if (WINDOWS_SCREENSHOTS)
    message(STATUS "✅  Configuring LibScreenshots to use Windows backend")

    find_package(OpenCV REQUIRED)

    target_include_directories(LibScreenshots
            PUBLIC
            ${OpenCV_INCLUDE_DIRS}
    )

    target_link_directories(LibScreenshots
            PUBLIC
            ${OpenCV_LIBRARY_DIRS}
    )

    target_compile_definitions(LibScreenshots
            PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _WIN32_WINNT=0x0A00
    )

    target_link_libraries(LibScreenshots
            PRIVATE
            ${OpenCV_LIBS}
            d3d11
            dxgi
    )
endif ()
