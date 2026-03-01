#include <iostream>
#include <stdexcept>

#include "DataStructures.h"
#include "OptimizeByILP.h"

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: resource_alloc <input_file> <output_lp_file>\n";
        return 1;
    }

    try
    {
        DataStore& ds = DataStore::getInstance();
        ds.loadFromFile(argv[1]);

        // ── Print Resources ───────────────────────────────────────────────────
        std::cout << "=== Resources (" << ds.getResources().size() << ") ===\n";
        for (const auto& [id, r] : ds.getResources())
        {
            std::cout << "  [" << id << "] name=" << r->getName()
                      << "  cost=" << r->getCost() << '\n';
        }

        // ── Print Projects ────────────────────────────────────────────────────
        std::cout << "\n=== Projects (" << ds.getProjects().size() << ") ===\n";
        for (const auto& [pid, p] : ds.getProjects())
        {
            std::cout << "  Project [" << pid << "]:\n";
            for (const auto& [rid, units] : p->getResourceCosts())
            {
                const auto* r = ds.getResourceById(rid);
                if (r)
                {
                    std::cout << "    resource " << r->getName() << " -> " << units << " units\n";
                }
            }
        }

        std::cout << "\nAdditional budget: " << ds.getAdditionalBudget() << '\n';

        // ── Generate .lp file ─────────────────────────────────────────────────
        OptimizeByILP optimizer;
        optimizer.generateLPFile(argv[2]);
        std::cout << "\nLP file written to: " << argv[2] << '\n';
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
