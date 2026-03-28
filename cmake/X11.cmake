if(X11_SCREENSHOTS)
    message(STATUS "✅ Configuring LibScreenshots to use X11 backend")

    find_package(PkgConfig REQUIRED)
    find_package(OpenCV REQUIRED)
    find_package(X11 REQUIRED)

    target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS} X11 Xrandr)
endif()
