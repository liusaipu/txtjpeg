cmake_minimum_required(VERSION 3.5)
project(txtjpeg C)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(JPEG REQUIRED)

add_executable(txtjpeg
    main.c
    parser.c
    format.c
    shape.c
    normalize.c
    jpeg_codec.c
    container.c
    compress.c
    decompress.c
)

target_link_libraries(txtjpeg PRIVATE JPEG::JPEG)

if(MSVC)
    target_compile_definitions(txtjpeg PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(txtjpeg PRIVATE /W3)
endif()

if(UNIX)
    target_link_libraries(txtjpeg PRIVATE m)
endif()
