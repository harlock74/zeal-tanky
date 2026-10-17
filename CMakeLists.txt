cmake_minimum_required(VERSION 3.16)
set(target_name ztank)
option(TANK_PROFILE "Build a headless native-emulator frame benchmark" OFF)
set(game_main src/main.c)
if(TANK_PROFILE)
    set(target_name ztank_profile)
    set(game_main tests/z80_profile.c)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)
set(ZOS_TOOLCHAIN sdcc)
include($ENV{ZOS_PATH}/cmake/zos_init.cmake)
project(${target_name} C ASM)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
zos_use(coreutils)
find_package(COREUTILS REQUIRED)
zos_use(zgdk)
find_package(ZGDK REQUIRED)
zos_use(zvb)
find_package(ZVB_SDK REQUIRED)
add_executable(${target_name} ${game_main} src/timing.c src/input.c src/gameplay.c src/ai.c src/arena.c src/physics.c src/render.c src/audio.c src/splash.c src/assets.c)
if(TANK_PROFILE)
    target_compile_definitions(${target_name} PRIVATE TANK_PROFILE)
    target_sources(${target_name} PRIVATE tests/z80_regressions.c)
endif()
target_link_libraries(${target_name} PUBLIC core zgdk zvb_gfx zvb_sound)
target_compile_options(${target_name} PRIVATE $<$<COMPILE_LANGUAGE:C>:--opt-code-speed>)
target_include_directories(${target_name} PRIVATE src ${CMAKE_BINARY_DIR}/generated)
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/generated/arenas.generated.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_arenas.py ${CMAKE_SOURCE_DIR}/assets/arenas.tmx ${CMAKE_BINARY_DIR}/generated/arenas.generated.h
    DEPENDS tools/gen_arenas.py assets/arenas.tmx VERBATIM)
add_custom_target(${target_name}_arenas DEPENDS ${CMAKE_BINARY_DIR}/generated/arenas.generated.h)
add_dependencies(${target_name} ${target_name}_arenas)
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/generated/splash.generated.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_splash.py ${CMAKE_SOURCE_DIR}/assets/splash.tmx ${CMAKE_BINARY_DIR}/generated/splash.generated.h
    DEPENDS tools/gen_splash.py assets/splash.tmx assets/tiles.tsx VERBATIM)
add_custom_target(${target_name}_splash DEPENDS ${CMAKE_BINARY_DIR}/generated/splash.generated.h)
add_dependencies(${target_name} ${target_name}_splash)
add_custom_target(${target_name}_art_check
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/export_art.py VERBATIM)
gif2zeal(${target_name} FILES assets/tiles.gif BIT 4 COLORS 16)
add_dependencies(${target_name}_tiles_asset ${target_name}_art_check)
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/generated/tiles.generated.h ${CMAKE_BINARY_DIR}/generated/tiles.rle
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_tiles.py ${CMAKE_BINARY_DIR}/assets/tiles.zts ${CMAKE_BINARY_DIR}/generated/tiles.generated.h ${CMAKE_BINARY_DIR}/generated/tiles.rle
    DEPENDS ${CMAKE_BINARY_DIR}/assets/tiles.zts tools/gen_tiles.py VERBATIM)
add_custom_target(${target_name}_tile_payload DEPENDS ${CMAKE_BINARY_DIR}/generated/tiles.generated.h ${CMAKE_BINARY_DIR}/generated/tiles.rle)
add_dependencies(${target_name} ${target_name}_tile_payload)
# Embedded assembly includes are invisible to the C compiler's dependency scan.
set_source_files_properties(src/assets.c PROPERTIES OBJECT_DEPENDS "${CMAKE_BINARY_DIR}/generated/tiles.rle;${CMAKE_BINARY_DIR}/assets/tiles.ztp")
zgdk_add_outputs(${target_name})
add_custom_target(${target_name}_memory_check ALL
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/check_link_map.py ${CMAKE_SOURCE_DIR}/debug/${target_name}.map
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/export_art.py --packed ${CMAKE_BINARY_DIR}/assets/tiles.zts
    DEPENDS ${target_name}_bin VERBATIM)
