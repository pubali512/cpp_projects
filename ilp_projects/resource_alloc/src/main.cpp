#include <iostream>
#include <stdexcept>

#include "DataStructures.h"
#include "OptimizeByHeuristic.h"
#include "OptimizeByILP.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: resource_alloc <input_file> [timeout_seconds]\n"
                  << "  timeout_seconds  default: 300\n";
        return 1;
    }

    const int timeout = (argc >= 3) ? std::stoi(argv[2]) : 300;

    try
    {
        DataStore& ds = DataStore::getInstance();
        ds.loadFromFile(argv[1]);

        // ── Print Resources ───────────────────────────────────────────────────
        std::cout << "=== Resources (" << ds.getResources().size() << ") ===\n";
        for (const auto& [id, r] : ds.getResources())
            std::cout << "  [" << id << "] " << r->getName()
                      << "  cost=$" << r->getCost() << "/week\n";

        // ── Print Projects ────────────────────────────────────────────────────
        std::cout << "\n=== Projects (" << ds.getProjects().size() << ") ===\n";
        for (const auto& [pid, p] : ds.getProjects())
        {
            std::cout << "  Project [" << pid << "]:\n";
            for (const auto& [rid, units] : p->getResourceCosts())
            {
                const auto* r = ds.getResourceById(rid);
                if (r)
                    std::cout << "    " << r->getName() << " -> " << units << " weeks\n";
            }
        }
        std::cout << "\nAdditional budget: $" << ds.getAdditionalBudget() << '\n';

        // ── Heuristic ────────────────────────────────────────────────────────
        OptimizeByHeuristic heuristic;
        heuristic.solve();

        // ── ILP (exact) ──────────────────────────────────────────────────────
        OptimizeByILP optimizer;
        optimizer.solve(timeout);
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
