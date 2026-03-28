if(X11)
    message(STATUS "✅ Configuring LibScreenshots to use X11 backend")

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(OpenCV REQUIRED opencv4)

    find_package(X11 REQUIRED)

    target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS} X11 Xrandr)
endif()
