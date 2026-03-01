#include "OptimizeByHeuristic.h"

#include "DataStructures.h"

#include <algorithm>
#include <vector>

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
void OptimizeByHeuristic::solve()
{
    const DataStore& ds     = DataStore::getInstance();
    double           budget = ds.getAdditionalBudget();

    const auto candidates = buildCandidates();

    // Initialise m_results for every (R,T) pair at baseline level i=1
    m_results.clear();
    for (const auto& [pid, pPtr] : ds.getProjects())
    {
        for (const auto& [rid, unitsDouble] : pPtr->getResourceCosts())
        {
            const Resource* r = ds.getResourceById(rid);
            if (!r) continue;

            PairKey     key{rid, pid};
            AllocResult ar;
            ar.resourceId     = rid;
            ar.projectId      = pid;
            ar.N              = static_cast<int>(unitsDouble);
            ar.costPerUnit    = r->getCost();
            ar.selectedI      = 1;
            ar.selectedCost   = 0.0;
            ar.selectedSaving = 0;
            m_results[key]    = ar;
        }
    }

    // Greedy selection: iterate sorted candidates in ratio order.
    // For each X_RT_{i_new}:
    //   incremental_cost = C_RT_{i_new} - C_RT_{current level for (R,T)}
    //   Only accept if i_new > current level AND incremental_cost <= budget.
    bool progress = true;
    while (progress)
    {
        progress = false;
        for (const auto& c : candidates)
        {
            PairKey      key{c.resourceId, c.projectId};
            AllocResult& ar = m_results.at(key);

            if (ar.selectedI > 1)
                continue;  // already selected a non-default value for this (R,T) pair based on a better ratio

            const double incremental = c.cost - ar.selectedCost;
            if (incremental > budget) continue;  // can't afford

            budget            -= incremental;
            ar.selectedI       = c.i;
            ar.selectedCost    = c.cost;
            ar.selectedSaving  = c.saving;
            progress = true;
        }
    }

    printResults("Heuristic Results");
}

