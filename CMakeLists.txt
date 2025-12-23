cmake_minimum_required(VERSION 3.18)
add_subdirectory(tools)
set(LLVM_MOS_PLATFORM rp6502)
find_package(llvm-mos-sdk REQUIRED)
project(MY-RP6502-PROJECT)
add_executable(asteroids)
rp6502_executable(asteroids)
target_sources(asteroids PRIVATE
    src/colors.c
    src/bitmap_graphics_db.c
    src/sound.c
    src/main.c
    src/ezpsg.c
    # src/psg_sound_tester.c
)

set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Flags used by the C compiler for Release builds." FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Flags used by the C++ compiler for Release builds." FORCE)
