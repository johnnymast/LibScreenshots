if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT WAYLAND)
    message(STATUS "Configuring X11 and OpenCV for Linux (X11 backend)")

    find_package(OpenCV REQUIRED)
    find_package(X11 REQUIRED)

    target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS} X11 Xrandr)
endif()
