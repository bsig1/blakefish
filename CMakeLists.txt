cmake_minimum_required(VERSION 3.14)
project(blakefish C)

set(CMAKE_C_STANDARD 11)

# Add source files
add_subdirectory(src)

# Add Google Test
enable_testing()
add_subdirectory(tests)
