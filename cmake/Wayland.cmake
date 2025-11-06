if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND WAYLAND)
    message(STATUS "✅ Configuring LibScreenshots to use Wayland backend via DBus portal")

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DBUS REQUIRED dbus-1)
    pkg_check_modules(GIO REQUIRED gio-2.0)
    pkg_check_modules(GLIB REQUIRED glib-2.0)

    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_WAYLAND)

    target_include_directories(${PROJECT_NAME} PRIVATE
            ${DBUS_INCLUDE_DIRS}
            ${GIO_INCLUDE_DIRS}
            ${GLIB_INCLUDE_DIRS}
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
            ${DBUS_LIBRARIES}
            ${GIO_LIBRARIES}
            ${GLIB_LIBRARIES}
    )

    target_compile_options(${PROJECT_NAME} PRIVATE
            ${DBUS_CFLAGS_OTHER}
            ${GIO_CFLAGS_OTHER}
            ${GLIB_CFLAGS_OTHER}
    )
endif()
