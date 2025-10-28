if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT WAYLAND)
    find_package(OpenCV REQUIRED)
    find_package(X11 REQUIRED)

    target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS} X11 Xrandr)
endif()
