#include "Optimize.h"

#include "DataStructures.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Formula helpers
// ─────────────────────────────────────────────────────────────────────────────

int Optimize::timeSaving(int N, int i)
{
    return N - static_cast<int>(
        std::ceil(static_cast<double>(N) / static_cast<double>(i)));
}

double Optimize::extraCost(int N, int i, double cR)
{
    if (i <= 1) return 0.0;
    const double duration =
        std::ceil(static_cast<double>(N) / static_cast<double>(i));
    return duration * cR * static_cast<double>(i - 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// Optimize::printResults
// ─────────────────────────────────────────────────────────────────────────────
void Optimize::printResults(const std::string& title) const
{
    const DataStore& ds     = DataStore::getInstance();
    const double     budget = ds.getAdditionalBudget();
    constexpr int    W      = 24; // column width for resource names

    // ── Pre-pass: aggregate totals ─────────────────────────────────────────
    int    totalTimeSaving = 0;
    double totalExtraCost  = 0.0;
    for (const auto& [key, ar] : m_results)
    {
        totalTimeSaving += ar.selectedSaving;
        totalExtraCost  += ar.selectedCost;
    }

    // ── Header ────────────────────────────────────────────────────────────
    std::cout << "\n=== " << title << " ===\n"
              << "Additional budget  : $" << static_cast<long long>(budget)       << '\n'
              << "Total time saving  : "  << totalTimeSaving                      << " week(s)\n"
              << "Total extra cost   : $" << static_cast<long long>(totalExtraCost) << '\n';

    if (m_results.empty())
        return;

    // ── Group by project ──────────────────────────────────────────────────
    std::map<int, std::vector<const AllocResult*>> byProject;
    for (const auto& [key, ar] : m_results)
        byProject[ar.projectId].push_back(&ar);

    // ── Per-project breakdown ─────────────────────────────────────────────
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
                      << ": " << std::right << std::setw(3) << ar->N
                      << " week(s) work planned"
                      << "  | " << std::right << std::setw(3) << ar->selectedI
                      << " unit(s) allocated"
                      << "  |  time saving: " << std::setw(3) << ar->selectedSaving
                      << " week(s)"
                      << "  |  extra cost: $"
                      << static_cast<long long>(ar->selectedCost) << '\n';

            projSaving    += ar->selectedSaving;
            projExtraCost += ar->selectedCost;
        }

        std::cout << "  " << std::string(68, '-') << '\n'
                  << "  Project time saving : " << projSaving << " week(s)\n"
                  << "  Project extra cost  : $"
                  << static_cast<long long>(projExtraCost) << '\n';
    }
}
