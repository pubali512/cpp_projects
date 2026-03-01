#include "OptimizeByHeuristic.h"

#include "DataStructures.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
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

            Candidate c;
            c.resourceId  = rid;
            c.projectId   = pid;
            c.N           = N;
            c.costPerUnit = cR;
            c.baseCost    = static_cast<double>(N) * cR;
            c.upgradeCost = cR * static_cast<double>(N - 1); // extra cost vs baseline
            c.benefit     = N - 1;                           // weeks saved vs 1 resource
            c.upgraded    = false;
            candidates.push_back(c);
        }
    }

    // Sort: base_cost ascending; ties broken by benefit descending
    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& a, const Candidate& b)
              {
                  if (a.baseCost != b.baseCost)
                      return a.baseCost < b.baseCost;
                  return a.benefit > b.benefit;
              });

    return candidates;
}

// ─────────────────────────────────────────────────────────────────────────────
// solve
// ─────────────────────────────────────────────────────────────────────────────
void OptimizeByHeuristic::solve() const
{
    const DataStore& ds     = DataStore::getInstance();
    double           cExtra = ds.getAdditionalBudget();

    auto candidates = buildCandidates();

    // Greedy pass: iterate sorted candidates, upgrade if affordable.
    // Run multiple passes in case a later (cheaper-to-upgrade) item becomes
    // the only remaining affordable one after skipping earlier ones.
    bool progress = true;
    while (progress && cExtra > 0.0)
    {
        progress = false;
        for (auto& c : candidates)
        {
            if (c.upgraded)       continue; // already selected
            if (c.benefit == 0)   continue; // N=1: no gain possible
            if (c.upgradeCost > cExtra) continue; // can't afford

            c.upgraded = true;
            cExtra -= c.upgradeCost;
            progress = true;
        }
    }

    printResults(candidates, ds.getAdditionalBudget());
}

// ─────────────────────────────────────────────────────────────────────────────
// printResults
// ─────────────────────────────────────────────────────────────────────────────
void OptimizeByHeuristic::printResults(const std::vector<Candidate>& candidates,
                                       double                         cBudget) const
{
    const DataStore& ds = DataStore::getInstance();
    const int W = 24; // column width for resource name

    // Group by project
    std::map<int, std::vector<const Candidate*>> byProject;
    for (const auto& c : candidates)
        byProject[c.projectId].push_back(&c);

    double totalExtraCost  = 0.0;
    int    totalTimeSaving = 0;

    std::cout << "\n=== Heuristic Results ===\n"
              << "Additional budget: $" << static_cast<long long>(cBudget) << '\n';

    for (const auto& [pid, list] : byProject)
    {
        std::cout << "\n--- Project " << pid << " ---\n";

        int    projSaving    = 0;
        double projExtraCost = 0.0;

        for (const Candidate* c : list)
        {
            const Resource* r     = ds.getResourceById(c->resourceId);
            const std::string rName = r ? r->getName()
                                        : ("resource_" + std::to_string(c->resourceId));

            const int    allocated = c->upgraded ? c->N : 1;
            const int    saving    = c->upgraded ? c->benefit : 0;
            const double extra     = c->upgraded ? c->upgradeCost : 0.0;

            projSaving    += saving;
            projExtraCost += extra;

            std::cout << "  "
                      << std::left  << std::setw(W) << rName
                      << ": " << std::right << std::setw(3) << allocated << " unit(s) allocated"
                      << "  |  time saving: " << std::setw(3) << saving    << " week(s)"
                      << "  |  extra cost: $" << static_cast<long long>(extra) << '\n';
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
