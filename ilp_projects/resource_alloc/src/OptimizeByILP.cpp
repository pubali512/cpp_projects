#include "OptimizeByILP.h"

#include "DataStructures.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────
namespace {

/// Format a double: use integer representation when there is no fractional part.
std::string fmtDouble(double v)
{
    const long long iv = static_cast<long long>(v);
    if (static_cast<double>(iv) == v)
        return std::to_string(iv);
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

/// Time saving when allocating i units to a task that originally needs N units.
/// S = N - ceil(N / i)
int timeSaving(int N, int i)
{
    return N - static_cast<int>(std::ceil(static_cast<double>(N) / static_cast<double>(i)));
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// OptimizeByILP – buildVarTable
// ─────────────────────────────────────────────────────────────────────────────
std::vector<OptimizeByILP::AllocVar> OptimizeByILP::buildVarTable() const
{
    const DataStore& ds = DataStore::getInstance();
    std::vector<AllocVar> table;

    for (const auto& [pid, pPtr] : ds.getProjects())
    {
        for (const auto& [rid, unitsDouble] : pPtr->getResourceCosts())
        {
            const Resource* r = ds.getResourceById(rid);
            if (!r) continue;

            const int    N    = static_cast<int>(unitsDouble); // base weeks required
            const double cR   = r->getCost();                  // cost per unit

            for (int i = 1; i <= N; ++i)
            {
                AllocVar v;
                v.resourceId  = rid;
                v.projectId   = pid;
                v.i           = i;
                v.N           = N;
                v.costPerUnit = cR;
                v.name        = "x_r" + std::to_string(rid)
                              + "_p" + std::to_string(pid)
                              + "_"  + std::to_string(i);
                table.push_back(std::move(v));
            }
        }
    }
    return table;
}

// ─────────────────────────────────────────────────────────────────────────────
// OptimizeByILP – public
// ─────────────────────────────────────────────────────────────────────────────
void OptimizeByILP::generateLPFile(const std::string& outputPath) const
{
    std::ofstream out(outputPath);
    if (!out.is_open())
        throw std::runtime_error("OptimizeByILP::generateLPFile – cannot create file: " + outputPath);

    out << generateObjectiveFunction()    << '\n'
        << generateNonBudgetConstraints() << '\n'
        << generateBudgetConstraint()     << '\n'
        << generateBinaryDeclarations();
}

// ─────────────────────────────────────────────────────────────────────────────
// OptimizeByILP – private section generators
// ─────────────────────────────────────────────────────────────────────────────

// Objective: maximise Σ S_RT_i · X_RT_i
// Terms with S=0 (i=1) are omitted for clarity but do no harm if included.
std::string OptimizeByILP::generateObjectiveFunction() const
{
    const auto vars = buildVarTable();

    std::ostringstream oss;
    oss << "/* Objective function\n"
        << "   Maximise total time saved across all (resource, project) pairs.\n"
        << "   S_RT_i = (N_RT - ceil(N_RT / i))*/\n";

    if (vars.empty())
    {
        oss << "max: 0;\n";
        return oss.str();
    }

    oss << "max: ";

    bool first = true;
    for (const auto& v : vars)
    {
        const int S = timeSaving(v.N, v.i);
        if (S == 0) continue;          // i=1 always gives S=0; skip zero terms

        if (!first) oss << "\n    + ";
        oss << S << ' ' << v.name;
        first = false;
    }

    if (first)                         // every saving is zero (degenerate input)
        oss << "0";

    oss << ";\n";
    return oss.str();
}

// Non-budget constraints: Σ_i X_RT_i = 1 for each (R, T) pair.
std::string OptimizeByILP::generateNonBudgetConstraints() const
{
    const auto vars = buildVarTable();

    std::ostringstream oss;
    oss << "/* Non-budget constraints\n"
        << "   For each (resource, project) pair exactly one allocation level\n"
        << "   must be chosen: sum of all X_RT_i for that pair = 1 */\n";

    // Group variables by (resourceId, projectId)
    // Iterate in order: the table is already ordered by project then resource.
    int  curRid = -1, curPid = -1;
    bool firstInGroup = true;

    for (const auto& v : vars)
    {
        if (v.resourceId != curRid || v.projectId != curPid)
        {
            // Close previous group
            if (curRid != -1)
                oss << " = 1;\n";

            // Open new group
            curRid = v.resourceId;
            curPid = v.projectId;
            firstInGroup = true;

            oss << "eq_r" << curRid << "_p" << curPid << ": ";
        }

        if (!firstInGroup) oss << " + ";
        oss << v.name;
        firstInGroup = false;
    }

    if (curRid != -1)   // close last group
        oss << " = 1;\n";

    return oss.str();
}

// Budget constraint: Σ C_R·(i-1)·X_RT_i  ≤  C_additionalBudget
// C_RT_i = C_R · (i-1)          (extra cost beyond baseline)
// Terms with i=1 give coefficient 0 and are omitted.
std::string OptimizeByILP::generateBudgetConstraint() const
{
    const DataStore& ds   = DataStore::getInstance();
    const auto       vars = buildVarTable();

    // Compute C_min
    double cMin = 0.0;
    for (const auto& [pid, pPtr] : ds.getProjects())
        for (const auto& [rid, unitsDouble] : pPtr->getResourceCosts())
            if (const Resource* r = ds.getResourceById(rid))
                cMin += unitsDouble * r->getCost();

    const double cBudget = ds.getAdditionalBudget();
    const double cExtra  = cBudget - cMin;

    std::ostringstream oss;
    oss << "/* Budget constraint\n"
        << "   C_min (baseline cost, one resource per phase) = " << fmtDouble(cMin)     << "\n"
        << "   C_budget (total available)                   = " << fmtDouble(cBudget)   << "\n"
        << "   C_extra  (budget available to speed up)      = " << fmtDouble(cExtra)    << "\n"
        << "   Extra cost per variable: C_RT_i = C_R * (i - 1) */\n";

    bool anyTerm = false;
    std::ostringstream lhs;
    for (const auto& v : vars)
    {
        if (v.i == 1) continue;        // coefficient = C_R*(1-1) = 0

        const double coeff = v.costPerUnit * static_cast<double>(v.i - 1);
        if (!anyTerm)
            lhs << "budget: ";
        else
            lhs << "\n       + ";

        lhs << fmtDouble(coeff) << ' ' << v.name;
        anyTerm = true;
    }

    if (!anyTerm)
        oss << "budget: 0 <= " << fmtDouble(cExtra) << ";\n";
    else
        oss << lhs.str() << "\n       <= " << fmtDouble(cExtra) << ";\n";

    return oss.str();
}

// Binary declarations for every X_RT_i variable.
std::string OptimizeByILP::generateBinaryDeclarations() const
{
    const auto vars = buildVarTable();

    std::ostringstream oss;
    oss << "/* Binary variable declarations */\n";

    if (vars.empty())
        return oss.str();

    oss << "bin ";
    bool first = true;
    for (const auto& v : vars)
    {
        if (!first) oss << ", ";
        oss << v.name;
        first = false;
    }
    oss << ";\n";

    return oss.str();
}
