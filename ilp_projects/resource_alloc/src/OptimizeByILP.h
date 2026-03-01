#pragma once

#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// OptimizeByILP
//
// Reads the populated DataStore singleton and writes an LP_Solve-compatible
// .lp file.
//
// LP formulation  (README – "Formulation of the ILP Problem")
// ─────────────────────────────────────────────────────────────
// For each (project T, resource R) with N_RT base units required:
//   Decision variables : X_RT_i ∈ {0,1}  for i = 1 … N_RT
//     X_RT_i = 1  iff  i units of resource R are allocated to project T
//
//   Time saving  : S_RT_i = N_RT − ⌈N_RT / i⌉
//   Extra cost   : C_RT_i = C_R · (i − 1)
//   Minimum cost : C_min  = Σ_{R,T} N_RT · C_R   (one resource per (R,T))
//
//   Objective              : maximise  Σ_{R,T,i}  S_RT_i · X_RT_i
//   Non-budget constraints : Σ_i X_RT_i = 1  for every (R,T)  [exactly one level]
//   Budget constraint      : Σ_{R,T,i} C_RT_i · X_RT_i  ≤  C_budget − C_min
// ─────────────────────────────────────────────────────────────────────────────
class OptimizeByILP
{
public:
    OptimizeByILP() = default;

    /// Build all sections and write the complete .lp file to @p outputPath.
    /// Throws std::runtime_error if the file cannot be created.
    void generateLPFile(const std::string& outputPath) const;

private:
    // ── Internal variable descriptor ──────────────────────────────────────────

    /// One entry in the flat variable table.
    struct AllocVar
    {
        int    resourceId;  ///< Resource R id
        int    projectId;   ///< Project  T id
        int    i;           ///< Allocation level (1 … N_RT)
        int    N;           ///< Base units required (N_RT)
        double costPerUnit; ///< C_R
        std::string name;   ///< LP variable name, e.g. "x_r1_p2_3"
    };

    /// Build the flat list of AllocVar descriptors from the DataStore.
    std::vector<AllocVar> buildVarTable() const;

    // ── Section generators ────────────────────────────────────────────────────

    /// Returns the LP_Solve objective-function section.
    ///   max: S_RT_i x_r1_p1_2 + ...;
    std::string generateObjectiveFunction() const;

    /// Returns one equality constraint per (R,T) pair.
    ///   eq_r1_p1: x_r1_p1_1 + x_r1_p1_2 + ... = 1;
    std::string generateNonBudgetConstraints() const;

    /// Returns the extra-budget inequality.
    ///   budget: C_RT_i x_r1_p1_2 + ... <= C_budget - C_min;
    std::string generateBudgetConstraint() const;

    /// Returns the binary-variable declaration section.
    ///   bin x_r1_p1_1, x_r1_p1_2, ...;
    std::string generateBinaryDeclarations() const;
};
