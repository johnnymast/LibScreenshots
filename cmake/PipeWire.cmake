
# PipeWire.cmake - Find PipeWire libraries

option(PIPEWIRE "Enable PipeWire support for screen capture" ON)

if(PIPEWIRE AND PLATFORM_LINUX)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(PIPEWIRE libpipewire-0.3)
    pkg_check_modules(SPA libspa-0.2)

    if(PIPEWIRE_FOUND AND SPA_FOUND)
        message(STATUS "✅ PipeWire found: ${PIPEWIRE_VERSION}")

        target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_PIPEWIRE)
        target_include_directories(${PROJECT_NAME} PRIVATE
                ${PIPEWIRE_INCLUDE_DIRS}
                ${SPA_INCLUDE_DIRS}
        )
        target_link_libraries(${PROJECT_NAME} PRIVATE
                ${PIPEWIRE_LIBRARIES}
                ${SPA_LIBRARIES}
        )
    else()
        message(WARNING "⚠️  PipeWire not found - falling back to X11/Wayland portal")
        set(PIPEWIRE OFF)
    endif()
endif()