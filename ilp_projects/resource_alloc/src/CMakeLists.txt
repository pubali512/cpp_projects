# ── Source targets ─────────────────────────────────────────────────────────────

add_executable(resource_alloc
    main.cpp
    DataStructures.cpp
)

target_include_directories(resource_alloc
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)
