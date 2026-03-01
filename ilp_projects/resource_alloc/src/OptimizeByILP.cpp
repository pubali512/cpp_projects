#include "OptimizeByILP.h"

#include "DataStructures.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_WIN32)
#  include <windows.h>
#endif

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers  (LP-file generation only)
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

// Budget constraint: Σ C_RT_i · X_RT_i  ≤  C_additionalBudget
// New cost formula (README "Cost calculation"):
//   C_RT_i = ceil(N/i) * C_R * (i - 1)
//   i.e. the task finishes in ceil(N/i) weeks; (i-1) extra resources hired at rate C_R.
// Terms with i=1 give coefficient 0 and are omitted.
std::string OptimizeByILP::generateBudgetConstraint() const
{
    const DataStore& ds   = DataStore::getInstance();
    const auto       vars = buildVarTable();
    const double cExtra   = ds.getAdditionalBudget();

    std::ostringstream oss;
    oss << "/* Budget constraint\n"
        << "   C_extra (additional budget available)   = " << fmtDouble(cExtra) << "\n"
        << "   C_RT_i = ceil(N/i) * C_R * (i - 1) */\n";

    bool anyTerm = false;
    std::ostringstream lhs;
    for (const auto& v : vars)
    {
        if (v.i == 1) continue;        // coefficient = 0

        const double coeff = extraCost(v.N, v.i, v.costPerUnit);
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

// ─────────────────────────────────────────────────────────────────────────────
// OptimizeByILP::solve
// ─────────────────────────────────────────────────────────────────────────────
void OptimizeByILP::solve()
{
    const int timeoutSeconds = m_timeoutSeconds;
    const std::string lpPath  = "_resource_alloc_tmp.lp";
    const std::string outPath = "_resource_alloc_tmp_out.txt";

    // Delete temp files if they already exist
    std::remove(lpPath.c_str());
    std::remove(outPath.c_str());

    // std::cout << "Generating LP file: " << lpPath << " ...\n";
    generateLPFile(lpPath);

#if defined(_WIN32)
    // Create the output file with an inheritable handle so CreateProcess can
    // redirect lp_solve's stdout/stderr into it.
    SECURITY_ATTRIBUTES sa{};
    sa.nLength        = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;

    HANDLE hOut = CreateFileA(
        outPath.c_str(),
        GENERIC_WRITE, FILE_SHARE_READ, &sa,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr
    );
    if (hOut == INVALID_HANDLE_VALUE)
        throw std::runtime_error("solve: cannot create output file: " + outPath);

    // Build command line: "<exe>" -lp "<lpfile>"
    const std::string cmd =
        "\"" LP_SOLVE_EXE "\" -lp \"" + lpPath + "\"";
    std::vector<char> cmdBuf(cmd.begin(), cmd.end());
    cmdBuf.push_back('\0');

    STARTUPINFOA si{};
    si.cb         = sizeof(STARTUPINFOA);
    si.dwFlags    = STARTF_USESTDHANDLES;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = hOut;
    si.hStdError  = hOut;

    PROCESS_INFORMATION pi{};
    const BOOL launched = CreateProcessA(
        nullptr, cmdBuf.data(),
        nullptr, nullptr,
        TRUE,   // bInheritHandles
        0, nullptr, nullptr,
        &si, &pi
    );
    CloseHandle(hOut); // child has its own copy; we don't need ours

    if (!launched)
        throw std::runtime_error(
            "solve: failed to launch lp_solve (WinError " +
            std::to_string(GetLastError()) + ")");

    // std::cout << "Running lp_solve (timeout: " << timeoutSeconds << "s) ...\n";

    const DWORD waitMs     = static_cast<DWORD>(timeoutSeconds) * 1000u;
    const DWORD waitResult = WaitForSingleObject(pi.hProcess, waitMs);

    if (waitResult == WAIT_TIMEOUT)
    {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        throw std::runtime_error(
            "lp_solve timed out after " + std::to_string(timeoutSeconds) + "s");
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

#else
    // POSIX fallback: shell redirect
    const std::string cmd =
        std::string(LP_SOLVE_EXE) +
        " -lp \"" + lpPath + "\" > \"" + outPath + "\" 2>&1";
    const int ret = std::system(cmd.c_str());
    (void)ret;
#endif

    // Read solver output into a string
    std::ifstream outFile(outPath);
    if (!outFile.is_open())
        throw std::runtime_error("solve: cannot read solver output: " + outPath);

    const std::string content(
        std::istreambuf_iterator<char>(outFile),
        std::istreambuf_iterator<char>{}
    );

    if (parseResults(content))
        printResults("LP Solver Results");
}

// ─────────────────────────────────────────────────────────────────────────────
// OptimizeByILP::parseResults
//
// Parse raw lp_solve output and populate m_results.
// Returns true if a feasible (optimal) solution was found.
// ─────────────────────────────────────────────────────────────────────────────
bool OptimizeByILP::parseResults(const std::string& solverOutput)
{
    m_results.clear();

    // ── Infeasibility check ───────────────────────────────────────────────────
    std::string lower = solverOutput;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

    if (lower.find("infeasible") != std::string::npos)
    {
        std::cout << "\n=== LP Solver Result: INFEASIBLE ===\n"
                  << "No feasible solution exists within the given constraints.\n";
        return false;
    }

    // ── Verify objective value token is present ────────────────────────────
    {
        const std::string tok = "Value of objective function: ";
        if (solverOutput.find(tok) == std::string::npos)
        {
            std::cout << "\n=== Raw solver output ===\n" << solverOutput << '\n';
            return false;
        }
    }

    // ── Build name → AllocVar* lookup ─────────────────────────────────────────
    const auto allVars = buildVarTable();
    std::map<std::string, const AllocVar*> varLookup;
    for (const auto& v : allVars)
        varLookup[v.name] = &v;

    // ── Pre-initialise m_results for every (R,T) pair at baseline i=1 ─────
    const DataStore& ds = DataStore::getInstance();
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

    // ── Parse active (=1) variables from solver output ────────────────────
    // lp_solve only prints non-zero variables; each line: "<name>    <value>"
    {
        std::istringstream ss(solverOutput);
        std::string line;
        while (std::getline(ss, line))
        {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.size() < 3 || line[0] != 'x') continue;

            std::istringstream ls(line);
            std::string name; double val = 0.0;
            ls >> name >> val;
            if (val > 0.5)
            {
                const auto it = varLookup.find(name);
                if (it == varLookup.end()) continue;

                const AllocVar* v   = it->second;
                PairKey         key{v->resourceId, v->projectId};
                auto            rit = m_results.find(key);
                if (rit == m_results.end()) continue;

                AllocResult& ar    = rit->second;
                ar.selectedI       = v->i;
                ar.selectedCost    = extraCost(v->N, v->i, v->costPerUnit);
                ar.selectedSaving  = timeSaving(v->N, v->i);
            }
        }
    }

    return true;
}
