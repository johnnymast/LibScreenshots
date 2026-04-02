message(STATUS "🔧 Loading Wayland backend configuration")

find_package(PkgConfig REQUIRED)

pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)

find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

set(WLR_SCREENCOPY_XML
        ${CMAKE_CURRENT_SOURCE_DIR}/protocols/wlr-screencopy-unstable-v1.xml
)

set(GENERATED_PROTO_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_PROTO_DIR})

set(WLR_SCREENCOPY_HEADER ${GENERATED_PROTO_DIR}/wlr-screencopy-unstable-v1-client-protocol.h)
set(WLR_SCREENCOPY_CODE   ${GENERATED_PROTO_DIR}/wlr-screencopy-unstable-v1-client-protocol.c)

# Genereer header
add_custom_command(
        OUTPUT ${WLR_SCREENCOPY_HEADER}
        COMMAND ${WAYLAND_SCANNER} client-header
        ${WLR_SCREENCOPY_XML}
        ${WLR_SCREENCOPY_HEADER}
        DEPENDS ${WLR_SCREENCOPY_XML}
)

# Genereer implementatie
add_custom_command(
        OUTPUT ${WLR_SCREENCOPY_CODE}
        COMMAND ${WAYLAND_SCANNER} private-code
        ${WLR_SCREENCOPY_XML}
        ${WLR_SCREENCOPY_CODE}
        DEPENDS ${WLR_SCREENCOPY_XML}
)

# Kleine library voor het protocol
add_library(wlr_screencopy_proto STATIC
        ${WLR_SCREENCOPY_HEADER}
        ${WLR_SCREENCOPY_CODE}
)

target_include_directories(wlr_screencopy_proto PUBLIC ${GENERATED_PROTO_DIR})
target_link_libraries(wlr_screencopy_proto PUBLIC ${WAYLAND_CLIENT_LIBRARIES})

# Backend define
target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_WAYLAND_SCREEN_COPY)

# Include dirs
target_include_directories(${PROJECT_NAME} PRIVATE
        ${WAYLAND_CLIENT_INCLUDE_DIRS}
        ${GENERATED_PROTO_DIR}
)

# Link libs
target_link_libraries(${PROJECT_NAME} PRIVATE
        ${WAYLAND_CLIENT_LIBRARIES}
        wlr_screencopy_proto
)

# Extra compile flags
target_compile_options(${PROJECT_NAME} PRIVATE
        ${WAYLAND_CLIENT_CFLAGS_OTHER}
)
