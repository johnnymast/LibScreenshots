
# PipeWire.cmake - Find PipeWire libraries



if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND PIPEWIRE)
    message(STATUS "✅ Configuring LibScreenshots to use Pipewrite backend")

    find_package(PkgConfig REQUIRED)

    pkg_check_modules(PIPEWIRE libpipewire-0.3)
    pkg_check_modules(SPA libspa-0.2)
    pkg_check_modules(GIO REQUIRED gio-2.0)
    pkg_check_modules(GLIB REQUIRED glib-2.0)

    if(PIPEWIRE_FOUND AND SPA_FOUND)
        message(STATUS "✅ PipeWire found: ${PIPEWIRE_VERSION}")

        target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_PIPEWIRE)

        target_include_directories(${PROJECT_NAME} PRIVATE
                ${PIPEWIRE_INCLUDE_DIRS}
                ${SPA_INCLUDE_DIRS}
                ${GIO_INCLUDE_DIRS}
                ${GLIB_INCLUDE_DIRS}
        )

        target_link_libraries(${PROJECT_NAME} PRIVATE
                ${PIPEWIRE_LIBRARIES}
                ${SPA_LIBRARIES}
                ${GIO_LIBRARIES}
                ${GLIB_LIBRARIES}
        )

        target_compile_options(${PROJECT_NAME} PRIVATE
                ${PIPEWIRE_CFLAGS_OTHER}
                ${SPA_CFLAGS_OTHER}
                ${GIO_CFLAGS_OTHER}
                ${GLIB_CFLAGS_OTHER}
        )
    else()
        message(WARNING "⚠️  PipeWire not found - falling back to X11/Wayland portal")
        set(PIPEWIRE OFF)
    endif()
endif()