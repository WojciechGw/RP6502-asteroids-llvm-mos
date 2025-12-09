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
    src/asteroid.c
    src/bullet.c
    src/ship.c
    src/sin_cos_tables.c
    src/main.c
    # src/mandelbrot.c
)
