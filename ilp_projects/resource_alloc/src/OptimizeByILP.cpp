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
    const double cExtra   = ds.getAdditionalBudget();

    std::ostringstream oss;
    oss << "/* Budget constraint\n"
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

// ─────────────────────────────────────────────────────────────────────────────
// OptimizeByILP::solve
// ─────────────────────────────────────────────────────────────────────────────
void OptimizeByILP::solve(int timeoutSeconds) const
{
    const std::string lpPath  = "_resource_alloc_tmp.lp";
    const std::string outPath = "_resource_alloc_tmp_out.txt";

    // Delete temp files if they already exist
    std::remove(lpPath.c_str());
    std::remove(outPath.c_str());

    std::cout << "Generating LP file: " << lpPath << " ...\n";
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

    std::cout << "Running lp_solve (timeout: " << timeoutSeconds << "s) ...\n";

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

    parseAndPrintResults(content);
}

// ─────────────────────────────────────────────────────────────────────────────
// OptimizeByILP::parseAndPrintResults
// ─────────────────────────────────────────────────────────────────────────────
void OptimizeByILP::parseAndPrintResults(const std::string& solverOutput) const
{
    // ── Infeasibility check ───────────────────────────────────────────────────
    std::string lower = solverOutput;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

    if (lower.find("infeasible") != std::string::npos)
    {
        std::cout << "\n=== LP Solver Result: INFEASIBLE ===\n"
                  << "No feasible solution exists within the given constraints.\n";
        return;
    }

    // ── Parse objective value ───────────────────────────────────────────────
    double objValue = 0.0;
    {
        const std::string tok = "Value of objective function: ";
        const auto pos = solverOutput.find(tok);
        if (pos == std::string::npos)
        {
            std::cout << "\n=== Raw solver output ===\n" << solverOutput << '\n';
            return;
        }
        try { objValue = std::stod(solverOutput.substr(pos + tok.size())); }
        catch (...) {}
    }

    // ── Build name → AllocVar* lookup ─────────────────────────────────────────
    const auto allVars = buildVarTable();
    std::map<std::string, const AllocVar*> varLookup;
    for (const auto& v : allVars)
        varLookup[v.name] = &v;

    // ── Parse active (=1) variables from solver output ────────────────────
    // lp_solve only prints non-zero variables; each line: "<name>    <value>"
    std::vector<const AllocVar*> active;
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
                if (it != varLookup.end())
                    active.push_back(it->second);
            }
        }
    }

    // ── Group active vars by project ──────────────────────────────────────
    std::map<int, std::vector<const AllocVar*>> byProject;
    for (const AllocVar* v : active)
        byProject[v->projectId].push_back(v);

    // ── Print ─────────────────────────────────────────────────────────────────────
    const DataStore& ds = DataStore::getInstance();
    const int W = 24; // column width for resource names

    std::cout << "\n=== LP Solver Results ===\n"
              << "Status  : OPTIMAL\n"
              << "Objective (total time saving): "
              << static_cast<long long>(objValue) << " week(s)\n";

    double grandExtraCost  = 0.0;

    for (const auto& [pid, allocList] : byProject)
    {
        std::cout << "\n--- Project " << pid << " ---\n";

        int    projSaving   = 0;
        double projExtraCost = 0.0;

        for (const AllocVar* v : allocList)
        {
            const Resource* r = ds.getResourceById(v->resourceId);
            const std::string rName = r ? r->getName()
                                        : ("resource_" + std::to_string(v->resourceId));

            const int    S     = timeSaving(v->N, v->i);
            const double extra = v->costPerUnit * static_cast<double>(v->i - 1);

            projSaving    += S;
            projExtraCost += extra;

            std::cout << "  "
                      << std::left << std::setw(W) << rName
                      << ": " << std::right << std::setw(3) << v->i << " unit(s) allocated"
                      << "  |  time saving: " << std::setw(3) << S << " week(s)"
                      << "  |  extra cost: $" << static_cast<long long>(extra) << "\n";
        }

        std::cout << "  " << std::string(68, '-') << "\n"
                  << "  Project time saving : " << projSaving   << " week(s)\n"
                  << "  Project extra cost  : $"
                  << static_cast<long long>(projExtraCost) << "\n";

        grandExtraCost  += projExtraCost;
    }

    std::cout << "\n" << std::string(70, '=') << "\n"
              << "Total time saving    : " << static_cast<long long>(objValue) << " week(s)\n"
              << "Total additional cost: $" << static_cast<long long>(grandExtraCost) << "\n";
}
