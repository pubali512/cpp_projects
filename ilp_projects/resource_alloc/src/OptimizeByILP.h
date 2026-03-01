#pragma once

#include "Optimize.h"

#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// OptimizeByILP
//
// Reads the populated DataStore singleton, writes an LP_Solve-compatible
// .lp file, invokes lp_solve, and populates the inherited m_results map.
// Inherits AllocResult, PairKey, m_results, printResults and formula helpers
// from Optimize.
//
// LP formulation  (README – "Formulation of the ILP Problem")
// ─────────────────────────────────────────────────────────────
//   Decision variables : X_RT_i ∈ {0,1}  for i = 1 … N_RT
//   Objective          : maximise  Σ S_RT_i · X_RT_i
//   Equality           : Σ_i X_RT_i = 1  for every (R,T)
//   Budget             : Σ C_RT_i · X_RT_i ≤ C_additionalBudget
//   S_RT_i = N − ⌈N/i⌉
//   C_RT_i = ⌈N/i⌉ × C_R × (i−1)
// ─────────────────────────────────────────────────────────────────────────────
class OptimizeByILP : public Optimize
{
public:
    OptimizeByILP() = default;

    /// Build all sections and write the complete .lp file to @p outputPath.
    /// Throws std::runtime_error if the file cannot be created.
    void generateLPFile(const std::string& outputPath) const;

    /// Set the solver timeout (seconds) used by solve().  Default: 300 s.
    void setTimeoutSeconds(int t) { m_timeoutSeconds = t; }

    /// Generate a temporary .lp file, run lp_solve, wait up to the configured
    /// timeout, populate m_results, and print human-readable results to stdout.
    void solve() override;

private:
    int m_timeoutSeconds{300};

    // ── Internal LP variable descriptor ───────────────────────────────────
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

    // ── LP section generators ──────────────────────────────────────────────
    std::string generateObjectiveFunction()    const;
    std::string generateNonBudgetConstraints() const;
    std::string generateBudgetConstraint()     const;
    std::string generateBinaryDeclarations()   const;

    /// Parse raw lp_solve output and populate m_results.
    /// Returns true if a feasible (optimal) solution was found.
    bool parseResults(const std::string& solverOutput);
};
