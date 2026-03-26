#pragma once

#include <map>
#include <string>
#include <utility>

// ─────────────────────────────────────────────────────────────────────────────
// Optimize  –  abstract base class for resource-allocation optimizers
//
// Provides:
//   • AllocResult  – per-(resource, project) allocation outcome
//   • PairKey      – (resourceId, projectId) map key
//   • m_results    – the allocation map populated by solve()
//   • printResults – common human-readable console printer
//   • timeSaving / extraCost – shared formula helpers (protected static)
//
// Formulas (README "Formulation of the optimization Problem"):
//   S_RT_i = N - ceil(N / i)                (time saving at level i)
//   C_RT_i = ceil(N/i) * C_R * (i - 1)     (additional cost at level i)
// ─────────────────────────────────────────────────────────────────────────────
class Optimize
{
public:
    virtual ~Optimize() = default;

    // ── Common result type ─────────────────────────────────────────────────

    /// Tracks the chosen allocation level for one (resource R, project T) pair.
    struct AllocResult
    {
        int    resourceId;
        int    projectId;
        int    N;               ///< Base time units required  (N_RT)
        double costPerUnit;     ///< Cost rate C_R
        int    selectedI{1};    ///< Chosen level  (baseline = 1, no extra cost)
        double selectedCost{0.0};   ///< C_RT_selectedI
        int    selectedSaving{0};   ///< S_RT_selectedI
    };

    /// Composite key (resourceId, projectId) for the result map.
    using PairKey = std::pair<int, int>;

    // ── Interface ──────────────────────────────────────────────────────────

    /// Run the optimization.  Implementations must populate m_results.
    virtual void solve() = 0;

    /// Print m_results in a human-readable table.
    /// @param title  Section heading, e.g. "Heuristic Results" or "ILP Results"
    void printResults(const std::string& title) const;

protected:
    /// Populated by each concrete solve() implementation.
    std::map<PairKey, AllocResult> m_results;

    // ── Shared formula helpers ─────────────────────────────────────────────

    /// S_RT_i = N − ceil(N / i)
    static int timeSaving(int N, int i);

    /// C_RT_i = ceil(N / i) × C_R × (i − 1)
    static double extraCost(int N, int i, double cR);
};
