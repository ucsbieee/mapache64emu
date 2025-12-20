cmake_minimum_required(VERSION 3.10)
project(mapache64emu)
add_executable(${PROJECT_NAME})
set(CMAKE_BUILD_TYPE "Debug")

add_subdirectory(vrEmu6502)
add_subdirectory(SDL)

set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

target_sources(${PROJECT_NAME} PRIVATE
    main.c
    monitor.c
)
target_link_libraries(${PROJECT_NAME}
    m
    vrEmu6502
    SDL3::SDL3
)
target_include_directories(${PROJECT_NAME} PUBLIC
    vrEmu6502/src
    SDL/include/
)
target_compile_definitions(${PROJECT_NAME} PUBLIC
    VR_EMU_6502_STATIC
)

