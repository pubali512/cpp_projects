#include <iostream>
#include <stdexcept>

#include "DataStructures.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: resource_alloc <input_file>\n";
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
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
