#pragma once

#include <map>
#include <utility>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// OptimizeByHeuristic
//
// Greedy heuristic (README – "Formulation of a greedy heuristic")
//
// Uses the same X_RT_i / C_RT_i / S_RT_i notation as the ILP formulation:
//   S_RT_i = N - ceil(N/i)                      (time saving)
//   C_RT_i = ceil(N/i) * C_R * (i - 1)          (additional cost)
//   ratio   = C_RT_i / S_RT_i                   (cost per week saved)
//
// Algorithm
// ─────────
//  1. Enumerate all X_RT_i for i = 2..N across every (R,T) pair.
//     Discard entries where S_RT_i = 0.
//  2. Sort by ratio ascending; ties broken by C_RT_i ascending.
//  3. For each (R,T) pair track the currently selected level (starts at i=1).
//  4. Greedy loop over sorted candidates:
//       incremental_cost = C_RT_{i_new} - C_RT_{currently_selected_for_(R,T)}
//       If i_new > current_level AND incremental_cost <= remaining budget:
//         upgrade (R,T) to i_new, deduct incremental_cost.
//  5. Continue until budget is exhausted or no candidate is affordable.
// ─────────────────────────────────────────────────────────────────────────────
class OptimizeByHeuristic
{
public:
    OptimizeByHeuristic() = default;

    /// Run the greedy heuristic and print human-readable results to stdout.
    void solve() const;

private:
    // ── X_RT_i variable descriptor ──────────────────────────────────────────
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

    // ── Per-(R,T) allocation state ─────────────────────────────────────────
    /// Tracks the chosen X_RT_i level for one (R,T) pair.
    struct AllocResult
    {
        int    resourceId;
        int    projectId;
        int    N;
        double costPerUnit;
        int    selectedI{1};       ///< Active level (baseline = 1)
        double selectedCost{0.0};  ///< C_RT for selectedI
        int    selectedSaving{0};  ///< S_RT for selectedI
    };

    using PairKey = std::pair<int,int>; ///< (resourceId, projectId)

    /// Build sorted candidate list from DataStore (i >= 2, S > 0).
    std::vector<Candidate> buildCandidates() const;

    /// Print results from the final AllocResult map.
    void printResults(const std::map<PairKey, AllocResult>& results,
                      double                                 cBudget) const;
};
