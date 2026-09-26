cmake_minimum_required(VERSION 3.20)
project(ExampleOS C)

set(CMAKE_C_STANDARD 99)

include_directories(src/kernel/include)
add_compile_options(-m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -Wall -Wextra)

file(GLOB_RECURSE C_SOURCES "src/kernel/*.c")
file(GLOB_RECURSE HEADERS "src/kernel/include/*.h")
file(GLOB_RECURSE ASM_SOURCES "src/*.asm")

# Target for CLion code insight, autocomplete, and navigation
add_executable(ExampleOS ${C_SOURCES} ${HEADERS} ${ASM_SOURCES})
