message(STATUS "  🔨 Linking Windows")
if (WINDOWS_SCREENSHOTS)
    message(STATUS "✅  Configuring LibScreenshots to use Windows backend")

    target_compile_definitions(LibScreenshots
            PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _WIN32_WINNT=0x0A00
    )

    target_link_libraries(LibScreenshots
            PRIVATE
            d3d11
            dxgi
    )


endif ()
