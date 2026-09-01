cmake_minimum_required(VERSION 3.25)

project(SaturnX
    VERSION 0.1.0
    DESCRIPTION "SEGA Saturn Emulator"
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

add_subdirectory(src/saturn)

add_executable(SaturnX WIN32
    src/main.cpp

    src/frontend/win32.cpp
    src/frontend/win32.h

    src/vulkan/vulkan_renderer.cpp
    src/vulkan/vulkan_renderer.h

    src/saturn/libretro_host.cpp
    src/saturn/libretro_host.h

    src/saturn/saturn_core.cpp
    src/saturn/saturn_core.h
)

target_include_directories(saturn
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/ext/beetle-saturn/libretro-common/include
)

target_link_libraries(SaturnX PRIVATE
    Vulkan::Vulkan
    saturn
)

if(MSVC)
    target_compile_options(SaturnX PRIVATE
        /W4
        /permissive-
        /EHsc
    )
else()
    target_compile_options(SaturnX PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

set_target_properties(SaturnX PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

file(MAKE_DIRECTORY
    "${CMAKE_BINARY_DIR}/bin"
)

if(EXISTS "${CMAKE_SOURCE_DIR}/runtime/cores/beetle-saturn.dll")
    add_custom_command(TARGET SaturnX POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/runtime/cores/beetle-saturn.dll"
            "$<TARGET_FILE_DIR:SaturnX>/beetle-saturn.dll"
    )
endif()