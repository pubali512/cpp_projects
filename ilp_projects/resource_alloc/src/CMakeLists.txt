# ── Source targets ─────────────────────────────────────────────────────────────

add_executable(resource_alloc
    main.cpp
    DataStructures.cpp
    Optimize.cpp
    OptimizeByILP.cpp
    OptimizeByHeuristic.cpp
)

target_include_directories(resource_alloc
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# Bake the absolute path to lp_solve.exe into the binary at compile time.
target_compile_definitions(resource_alloc
    PRIVATE
        LP_SOLVE_EXE="${CMAKE_SOURCE_DIR}/external/lp_solve/lp_solve.exe"
)
