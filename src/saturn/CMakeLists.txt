add_library(saturn STATIC
    libretro_host.cpp
    libretro_host.h
    saturn_core.cpp
    saturn_core.h
)

target_include_directories(saturn
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(saturn
    PRIVATE
        ${CMAKE_SOURCE_DIR}/ext/beetle-saturn/libretro-common/include
)

target_compile_features(saturn PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(saturn PRIVATE
        /W4
        /permissive-
        /EHsc
    )
endif()