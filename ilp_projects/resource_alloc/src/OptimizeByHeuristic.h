#pragma once

#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// OptimizeByHeuristic
//
// Implements the greedy heuristic described in README "Formulation of heuristic"
//
// Algorithm
// ─────────
//   For every (resource R, project T) pair with N_RT base weeks required:
//     base_cost   = N_RT · C_R          (cost of the baseline 1-resource allocation)
//     upgrade_cost = C_R · (N_RT − 1)   (extra spend needed to complete in 1 week)
//     benefit      = N_RT − 1           (weeks saved by upgrading to 1 week)
//
//   Sort candidates by base_cost ↑; ties by benefit ↓.
//   Greedily upgrade each candidate (allocate all N_RT resources → 1 week)
//   if upgrade_cost ≤ remaining C_extra budget.
//   Continue until C_extra is exhausted or no candidate is affordable.
//
//   Results are printed via printResults().
// ─────────────────────────────────────────────────────────────────────────────
class OptimizeByHeuristic
{
public:
    OptimizeByHeuristic() = default;

    /// Run the greedy heuristic and print human-readable results to stdout.
    void solve() const;

private:
    /// One candidate (resource, project) entry.
    struct Candidate
    {
        int    resourceId;
        int    projectId;
        int    N;           ///< Base units (weeks) required
        double costPerUnit; ///< C_R
        double baseCost;    ///< N · C_R
        double upgradeCost; ///< C_R · (N − 1): extra spend to finish in 1 week
        int    benefit;     ///< N − 1: weeks saved

        bool   upgraded{false}; ///< set to true when selected by the heuristic
    };

    /// Build the candidate list from the DataStore (one entry per (R,T) pair).
    std::vector<Candidate> buildCandidates() const;

    /// Print results in the same style as OptimizeByILP::parseAndPrintResults.
    void printResults(const std::vector<Candidate>& candidates,
                      double                         cExtra) const;
};
