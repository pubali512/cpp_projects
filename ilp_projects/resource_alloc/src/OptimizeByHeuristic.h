#pragma once

#include "Optimize.h"

#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// OptimizeByHeuristic
//
// Greedy heuristic (README – "Formulation of a greedy heuristic").
// Inherits AllocResult, PairKey, m_results, printResults and formula helpers
// from Optimize.
//
// Algorithm
// ─────────
//  1. Enumerate all X_RT_i for i = 2..N across every (R,T) pair.
//     Discard entries where S_RT_i = 0.
//  2. Sort by ratio = C_RT_i / S_RT_i ascending; ties broken by C_RT_i.
//  3. Initialise m_results for every (R,T) at baseline level i=1.
//  4. Greedy loop over sorted candidates:
//       incremental_cost = C_RT_{i_new} - C_RT_{currently_selected}
//       If i_new > current level AND incremental_cost <= remaining budget:
//         upgrade (R,T) to i_new, deduct incremental_cost.
//  5. Call printResults("Heuristic Results") when done.
// ─────────────────────────────────────────────────────────────────────────────
class OptimizeByHeuristic : public Optimize
{
public:
    OptimizeByHeuristic() = default;

    /// Run the greedy heuristic; populates m_results and prints to stdout.
    void solve() override;

private:
    // ── X_RT_i variable descriptor (heuristic-specific) ───────────────────
    /// One entry per (R, T, i) where i >= 2 and S_RT_i > 0.
    struct Candidate
    {
        int    resourceId;
        int    projectId;
        int    i;           ///< Allocation level
        int    N;           ///< Base weeks N_RT
        double costPerUnit; ///< C_R
        double cost;        ///< C_RT_i = ceil(N/i) * C_R * (i-1)
        int    saving;      ///< S_RT_i = N - ceil(N/i)
        double ratio;       ///< C_RT_i / S_RT_i
    };

    /// Build sorted candidate list from DataStore (i >= 2, S > 0).
    std::vector<Candidate> buildCandidates() const;
};
