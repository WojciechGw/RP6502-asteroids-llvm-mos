cmake_minimum_required(VERSION 3.18)
add_subdirectory(tools)
set(LLVM_MOS_PLATFORM rp6502)
find_package(llvm-mos-sdk REQUIRED)
project(MY-RP6502-PROJECT)
add_executable(asteroids)
rp6502_executable(asteroids)
target_sources(asteroids PRIVATE
    src/colors.c
    src/bitmap_graphics_db_gemini3.c
    src/ezpsg.c
    src/sound.c
    src/main_claude.c
    # src/poprock.c
    # src/psg_sound_tester.c
)
