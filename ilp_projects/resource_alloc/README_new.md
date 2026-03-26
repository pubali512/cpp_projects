# Resource Allocation Optimizer

## Project Overview

A C++17 resource allocation optimizer that minimizes total project duration within a given additional budget. The tool solves the optimization problem using two independent approaches and compares the results:

1. **Greedy Heuristic** — Enumerates all possible (resource, project, allocation-level) triples, sorts them by cost-effectiveness ratio, and greedily assigns resources within the budget.
2. **Integer Linear Programming (ILP)** — Generates an LP-format problem file with binary decision variables, an objective function maximizing time savings, equality and budget constraints, and solves it using the external `lp_solve` solver.

### Problem Description

A company executes a set of sequential projects, each requiring different resource types (e.g., system architects, developers, testers) with per-week costs. Adding duplicate resources shortens task duration ($\lceil N/i \rceil$ weeks for $i$ resources) but each additional hire costs double. Given an additional budget, the goal is to find the optimal resource allocation that minimizes total project duration.

This formulation is generalizable — for example, it can also model hardware module design where resources are hardware components (adders, multipliers, ALUs) with gate-count costs, and the goal is to minimize execution time within a gate budget.

### Main Features

- **Dual optimization** — Runs both a greedy heuristic and an exact ILP solver, allowing comparison of approximate vs. optimal solutions
- **ILP formulation** — Binary decision variables $X_{RT_i}$, objective to maximize $\sum S_{RT_i} \cdot X_{RT_i}$, with equality and budget constraints
- **Greedy heuristic** — Cost-benefit ratio sorting with greedy selection
- **Configurable input** — Custom Markdown-like input format with `# GENERAL`, `# RESOURCE`, `# PROJECT`, and `# ADDITIONAL_BUDGET` sections
- **Detailed output** — Per-project breakdowns showing resource allocation, time savings, and costs
- **Cross-platform** — Windows native process management (Win32 API) with POSIX fallback

### ILP Formulation

For a given task $T$ and resource type $R$, binary variables $X_{RT_i}$ indicate whether $i$ resources of type $R$ are allocated to task $T$:

- **Objective:** Maximize $\sum S_{RT_i} \cdot X_{RT_i}$ where $S_{RT_i} = N - \lceil N/i \rceil$
- **Equality constraint:** $\sum_i X_{RT_i} = 1$ for each $(R, T)$ pair
- **Budget constraint:** $\sum C_{RT_i} \cdot X_{RT_i} \leq C_{\text{additionalBudget}}$ where $C_{RT_i} = \lceil N/i \rceil \times C_R \times (i - 1)$

## Technologies Used & Installation Instructions

### Technologies

- **C++17**
- **CMake** (>= 3.25) with **Ninja** generator
- **Clang** compiler (LLVM or VS-bundled) — configured via CMake presets
- **lp_solve** — External ILP solver binary (expected at `external/lp_solve/lp_solve.exe`)
- **Win32 API** — For process management of lp_solve with timeout (POSIX fallback available)

### Prerequisites

- C++17-compatible compiler (Clang recommended, MSVC also supported)
- CMake (>= 3.25) and Ninja
- `lp_solve` binary placed at `external/lp_solve/lp_solve.exe`

### Build

Using CMake presets (recommended):

```powershell
cmake --preset llvm-clang-debug
cmake --build --preset llvm-clang-debug
```

Or manually:

```powershell
cmake -S . -B build -G Ninja
cmake --build build --config Release
```

The executable is produced at `build/<preset>/src/resource_alloc(.exe)`.

### Available CMake Presets

| Preset               | Compiler   | Config  | Generator |
|----------------------|------------|---------|-----------|
| `llvm-clang-debug`   | LLVM Clang | Debug   | Ninja     |
| `llvm-clang-release` | LLVM Clang | Release | Ninja     |
| `vs-clang-debug`     | VS Clang   | Debug   | Ninja     |
| `vs-clang-release`   | VS Clang   | Release | Ninja     |

## Usage Instructions

### Running the Optimizer

```powershell
.\build\llvm-clang-debug\src\resource_alloc.exe <input_file> [timeout_seconds]
```

- `input_file` — Path to the input file in the custom Markdown-like format
- `timeout_seconds` — Optional timeout for the ILP solver (default: 300)

The program runs both the greedy heuristic and the ILP solver, printing results for each to stdout.

### Example

```powershell
# Software project optimization (4 resource types, 4 projects, $25,000 budget)
.\build\llvm-clang-debug\src\resource_alloc.exe .\EXAMPLE.md

# Hardware design optimization (gate-based resources, 25,000 gate budget)
.\build\llvm-clang-debug\src\resource_alloc.exe .\EXAMPLE_hwdesign.md 600
```

### Input File Format

Input files use a custom section-based format with four sections:

```markdown
# GENERAL
time_unit = week
cost_unit = $

# RESOURCE
| Name             | Cost |
| system_architect | 600  |
| developer        | 400  |

# PROJECT
| Project Name | system_architect | developer |
| Project_1    | 5                | 10        |
| Project_2    | 3                | 8         |

# ADDITIONAL_BUDGET
25000
```

### Example Input Files

| File                  | Description                                                                                       |
|-----------------------|---------------------------------------------------------------------------------------------------|
| `EXAMPLE.md`          | Software project: 4 resource types (system_architect, software_architect, developer, tester), 4 projects, $25,000 budget |
| `EXAMPLE_hwdesign.md` | Hardware design: resources are divider/multiplier/ALU/shifter with gate costs, 25,000 gate budget |

### Project Structure

```
resource_alloc/
├── CMakeLists.txt          # Top-level build configuration (C++17, Clang warnings)
├── CMakePresets.json        # Build presets (LLVM/VS Clang, Debug/Release)
├── EXAMPLE.md              # Sample input: software project optimization
├── EXAMPLE_hwdesign.md     # Sample input: hardware design optimization
├── external/
│   ├── CMakeLists.txt      # Placeholder for third-party deps
│   └── lp_solve/           # Expected location for lp_solve binary
├── src/
│   ├── CMakeLists.txt      # Executable target, bakes LP_SOLVE_EXE path
│   ├── main.cpp            # CLI entry point
│   ├── DataStructures.h/cpp # Input parser, Resource/Project/DataStore models
│   ├── Optimize.h/cpp      # Abstract base class with shared formulas and result printer
│   ├── OptimizeByHeuristic.h/cpp  # Greedy heuristic implementation
│   └── OptimizeByILP.h/cpp       # ILP file generation, solver invocation, output parsing
```

### Notes

- If `lp_solve` cannot be found at the baked-in path, place `lp_solve.exe` at `external/lp_solve/lp_solve.exe` or update the path in `src/CMakeLists.txt` and reconfigure.
- Temporary files created during ILP solve (`_resource_alloc_tmp.lp` and `_resource_alloc_tmp_out.txt`) are left in the working directory for inspection.
