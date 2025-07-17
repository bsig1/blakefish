cmake_minimum_required(VERSION 3.30)
project(blakefish C)

set(CMAKE_C_STANDARD 11)

# Default to Release if no build type was specified
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# Set global flags for each configuration
set(CMAKE_C_FLAGS_DEBUG          "-g -O0")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -O2")
set(CMAKE_C_FLAGS_MINSIZEREL     "-Os")
set(CMAKE_C_FLAGS_RELEASE        "-O3 -march=native -flto -DNDEBUG")

# Enable link-time optimization in Release
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto")

# Your sources
add_executable(blakefish
		main.c
		gameboard.c
		movegen.c
		magic.c
		game_tree.c
		eval.c
		eval.h
		bitboards.c
		bitboards.h
)

# (Optional) Modern target-specific approach instead of global flags:
# target_compile_options(blakefish PRIVATE
#   $<$<CONFIG:Release>:-O3 -march=native -flto -DNDEBUG>
#   $<$<CONFIG:Debug>:-g -O0>
# )
# target_link_options(blakefish PRIVATE
#   $<$<CONFIG:Release>:-flto>
# )
