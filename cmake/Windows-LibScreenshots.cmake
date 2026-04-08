if(WINDOWS_SCREENSHOTS)
    message(STATUS "✅  Configuring LibScreenshots to use Windows backend")
    target_link_libraries(${PROJECT_NAME} PRIVATE user32 gdi32)
endif()
