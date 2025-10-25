if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "🖥️ Configuring Windows backend")

    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WINDOWS)
    target_link_libraries(${PROJECT_NAME} PRIVATE user32 gdi32)
endif()
