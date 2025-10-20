cmake_minimum_required(VERSION 3.5)
project(mapache64emu)
add_executable(${PROJECT_NAME})
set(CMAKE_BUILD_TYPE "Debug")

add_subdirectory(raylib)
add_subdirectory(vrEmu6502)

target_sources(${PROJECT_NAME} PRIVATE
    main.c
    monitor.c
)
target_link_libraries(${PROJECT_NAME}
    raylib
    vrEmu6502
)
target_include_directories(${PROJECT_NAME} PUBLIC
    raylib/src
    vrEmu6502/src
)
target_compile_definitions(${PROJECT_NAME} PUBLIC
    VR_EMU_6502_STATIC
)

