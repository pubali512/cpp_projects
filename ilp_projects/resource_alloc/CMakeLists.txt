cmake_minimum_required(VERSION 3.25)

project(
    resource_alloc
    VERSION 0.1.0
    DESCRIPTION "Resource Allocation ILP Project"
    LANGUAGES CXX
)

# ── Global C++ standard ────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Export compile commands (useful for clangd / IDEs) ────────────────────────
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Compiler diagnostics ──────────────────────────────────────────────────────
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
    )
endif()

# ── Subdirectories ────────────────────────────────────────────────────────────
add_subdirectory(external)
add_subdirectory(src)
