cmake_minimum_required(VERSION 3.20)
project(Sodium LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Static libraries (.a / .lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Shared libraries (.so / .dll)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Library source dir
add_subdirectory(src)

# Server executable source dir
add_subdirectory(apps/server)

# Client executable source dir
add_subdirectory(apps/client)


