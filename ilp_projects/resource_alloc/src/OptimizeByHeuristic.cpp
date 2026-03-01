#include "OptimizeByHeuristic.h"

#include "DataStructures.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers  (shared formulas from the ILP section of the README)
// ─────────────────────────────────────────────────────────────────────────────
namespace {

/// S_RT_i = N - ceil(N / i)
int timeSaving(int N, int i)
{
    return N - static_cast<int>(std::ceil(static_cast<double>(N) / static_cast<double>(i)));
}

/// C_RT_i = ceil(N/i) * C_R * (i - 1)
double extraCost(int N, int i, double cR)
{
    if (i <= 1) return 0.0;
    const double duration = std::ceil(static_cast<double>(N) / static_cast<double>(i));
    return duration * cR * static_cast<double>(i - 1);
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// buildCandidates
// ─────────────────────────────────────────────────────────────────────────────
std::vector<OptimizeByHeuristic::Candidate> OptimizeByHeuristic::buildCandidates() const
{
    const DataStore& ds = DataStore::getInstance();
    std::vector<Candidate> candidates;

    for (const auto& [pid, pPtr] : ds.getProjects())
    {
        for (const auto& [rid, unitsDouble] : pPtr->getResourceCosts())
        {
            const Resource* r = ds.getResourceById(rid);
            if (!r) continue;

            const int    N  = static_cast<int>(unitsDouble);
            const double cR = r->getCost();

            // Enumerate every X_RT_i for i = 2..N
            for (int i = 2; i <= N; ++i)
            {
                const int    S    = timeSaving(N, i);
                if (S == 0) continue;  // no saving at this level

                const double cost  = extraCost(N, i, cR);
                const double ratio = cost / static_cast<double>(S);

                Candidate c;
                c.resourceId  = rid;
                c.projectId   = pid;
                c.i           = i;
                c.N           = N;
                c.costPerUnit = cR;
                c.cost        = cost;
                c.saving      = S;
                c.ratio       = ratio;
                candidates.push_back(c);
            }
        }
    }

    // Sort by ratio ascending; ties broken by cost ascending
    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& a, const Candidate& b)
              {
                  if (a.ratio != b.ratio)
                      return a.ratio < b.ratio;
                  return a.cost < b.cost;
              });

    return candidates;
}

// ─────────────────────────────────────────────────────────────────────────────
// solve
// ─────────────────────────────────────────────────────────────────────────────
void OptimizeByHeuristic::solve() const
{
    const DataStore& ds     = DataStore::getInstance();
    double           budget = ds.getAdditionalBudget();

    const auto candidates = buildCandidates();

    // Initialise allocation state for every (R,T) pair at baseline i=1
    std::map<PairKey, AllocResult> state;
    for (const auto& [pid, pPtr] : ds.getProjects())
    {
        for (const auto& [rid, unitsDouble] : pPtr->getResourceCosts())
        {
            const Resource* r = ds.getResourceById(rid);
            if (!r) continue;

            PairKey key{rid, pid};
            AllocResult ar;
            ar.resourceId     = rid;
            ar.projectId      = pid;
            ar.N              = static_cast<int>(unitsDouble);
            ar.costPerUnit    = r->getCost();
            ar.selectedI      = 1;
            ar.selectedCost   = 0.0;
            ar.selectedSaving = 0;
            state[key]        = ar;
        }
    }

    // Greedy selection: iterate sorted candidates in ratio order.
    // For each X_RT_{i_new}:
    //   incremental_cost = C_RT_{i_new} - C_RT_{current level for (R,T)}
    //   Only accept if i_new > current level (strictly better allocation)
    //   and incremental_cost fits in remaining budget.
    bool progress = true;
    while (progress)
    {
        progress = false;
        for (const auto& c : candidates)
        {
            PairKey key{c.resourceId, c.projectId};
            AllocResult& ar = state.at(key);

            if (ar.selectedI > 1)
                continue;  // already selected a non-default value for this (R,T) pair based on a better ratio

            const double incremental = c.cost - ar.selectedCost;
            if (incremental > budget)  continue;  // can't afford

            // Upgrade this (R,T) pair to level i_new
            budget         -= incremental;
            ar.selectedI      = c.i;
            ar.selectedCost   = c.cost;
            ar.selectedSaving = c.saving;
            progress = true;
        }
    }

    printResults(state, ds.getAdditionalBudget());
}

// ─────────────────────────────────────────────────────────────────────────────
// printResults
// ─────────────────────────────────────────────────────────────────────────────
void OptimizeByHeuristic::printResults(
    const std::map<PairKey, AllocResult>& results,
    double                                 cBudget) const
{
    const DataStore& ds = DataStore::getInstance();
    const int W = 24;

    // Group by project for printing
    std::map<int, std::vector<const AllocResult*>> byProject;
    for (const auto& [key, ar] : results)
        byProject[ar.projectId].push_back(&ar);

    int    totalTimeSaving = 0;
    double totalExtraCost  = 0.0;

    std::cout << "\n=== Heuristic Results ===\n"
              << "Additional budget: $" << static_cast<long long>(cBudget) << '\n';

    for (const auto& [pid, list] : byProject)
    {
        std::cout << "\n--- Project " << pid << " ---\n";

        int    projSaving    = 0;
        double projExtraCost = 0.0;

        for (const AllocResult* ar : list)
        {
            const Resource*   r     = ds.getResourceById(ar->resourceId);
            const std::string rName = r ? r->getName()
                                        : ("resource_" + std::to_string(ar->resourceId));

            std::cout << "  "
                      << std::left  << std::setw(W) << rName
                      << ": " << std::right << std::setw(3) << ar->selectedI << " unit(s) allocated"
                      << "  |  time saving: " << std::setw(3) << ar->selectedSaving << " week(s)"
                      << "  |  extra cost: $" << static_cast<long long>(ar->selectedCost) << '\n';

            projSaving    += ar->selectedSaving;
            projExtraCost += ar->selectedCost;
        }

        std::cout << "  " << std::string(68, '-') << '\n'
                  << "  Project time saving : " << projSaving << " week(s)\n"
                  << "  Project extra cost  : $"
                  << static_cast<long long>(projExtraCost) << '\n';

        totalTimeSaving += projSaving;
        totalExtraCost  += projExtraCost;
    }

    std::cout << '\n' << std::string(70, '=') << '\n'
              << "Total time saving    : " << totalTimeSaving << " week(s)\n"
              << "Total additional cost: $" << static_cast<long long>(totalExtraCost) << '\n';
}

