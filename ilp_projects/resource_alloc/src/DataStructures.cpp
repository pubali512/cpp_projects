#include "DataStructures.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────
namespace {

/// Trim leading/trailing ASCII whitespace from @p s.
std::string trim(const std::string& s)
{
    auto first = s.begin();
    while (first != s.end() && std::isspace(static_cast<unsigned char>(*first)))
        ++first;

    auto last = s.end();
    while (last != first && std::isspace(static_cast<unsigned char>(*(last - 1))))
        --last;

    return {first, last};
}

/// Split @p line on the first '=' and return {trimmed_key, trimmed_value}.
/// Returns {empty, empty} when no '=' is present.
std::pair<std::string, std::string> splitKeyValue(const std::string& line)
{
    const auto eq = line.find('=');
    if (eq == std::string::npos)
        return {};

    return {trim(line.substr(0, eq)), trim(line.substr(eq + 1))};
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Resource
// ─────────────────────────────────────────────────────────────────────────────
int Resource::m_nextId = 1;

Resource::Resource(std::map<std::string, std::string> attributes)
    : m_id(m_nextId++)
    , m_attributes(std::move(attributes))
{}

std::string Resource::getName() const
{
    const auto it = m_attributes.find("name");
    return it != m_attributes.end() ? it->second : std::string{};
}

double Resource::getCost() const
{
    const auto it = m_attributes.find("cost");
    if (it == m_attributes.end())
        return 0.0;
    try   { return std::stod(it->second); }
    catch (...) { return 0.0; }
}

// ─────────────────────────────────────────────────────────────────────────────
// Project
// ─────────────────────────────────────────────────────────────────────────────
int Project::m_nextId = 1;

Project::Project(std::map<int, double> resourceCosts)
    : m_id(m_nextId++)
    , m_resourceCosts(std::move(resourceCosts))
{}

void Project::addResourceCost(int resourceId, double units)
{
    m_resourceCosts[resourceId] = units;
}

// ─────────────────────────────────────────────────────────────────────────────
// DataStore
// ─────────────────────────────────────────────────────────────────────────────
DataStore& DataStore::getInstance()
{
    static DataStore instance;
    return instance;
}

DataStore::~DataStore()
{
    clear();
}

void DataStore::clear()
{
    for (auto& [id, ptr] : m_resources) delete ptr;
    m_resources.clear();

    for (auto& [id, ptr] : m_projects) delete ptr;
    m_projects.clear();

    m_additionalBudget = 0.0;
    m_timeUnit = "week";
    m_costUnit = "dollar";
}

void DataStore::loadFromFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
        throw std::runtime_error("DataStore::loadFromFile – cannot open file: " + filepath);

    // ── Section state ─────────────────────────────────────────────────────────
    enum class Section { None, General, Resource, Project, AdditionalBudget };

    Section current = Section::None;

    // Buffers for the section being parsed
    std::map<std::string, std::string> resourceAttribs;
    std::map<std::string, double>      projectEntries;   // resource-name → units

    // ── Lambda: commit the current section buffer ──────────────────────────────
    auto finalizeSection = [&]()
    {
        if (current == Section::Resource && !resourceAttribs.empty())
        {
            auto* r = new Resource(resourceAttribs);
            m_resources[r->getId()] = r;
            resourceAttribs.clear();
        }
        else if (current == Section::Project && !projectEntries.empty())
        {
            std::map<int, double> costs;
            for (const auto& [name, units] : projectEntries)
            {
                const int rid = findResourceIdByName(name);
                if (rid != -1)
                    costs[rid] = units;
                // Unknown resource names are silently skipped.
            }
            auto* p = new Project(std::move(costs));
            m_projects[p->getId()] = p;
            projectEntries.clear();
        }
    };

    // ── Main parse loop ───────────────────────────────────────────────────────
    std::string line;
    while (std::getline(file, line))
    {
        const std::string trimmed = trim(line);
        if (trimmed.empty())
            continue;

        // Section headers
        if (trimmed == "# GENERAL")
        {
            finalizeSection();
            current = Section::General;
            continue;
        }
        if (trimmed == "# RESOURCE")
        {
            finalizeSection();
            current = Section::Resource;
            continue;
        }
        if (trimmed == "# PROJECT")
        {
            finalizeSection();
            current = Section::Project;
            continue;
        }
        if (trimmed == "# ADDITIONAL_BUDGET")
        {
            finalizeSection();
            current = Section::AdditionalBudget;
            continue;
        }

        // Attribute / entry line
        auto [key, val] = splitKeyValue(trimmed);
        if (key.empty() || val.empty())
            continue;

        if (current == Section::General)
        {
            if (key == "time_unit") m_timeUnit = val;
            else if (key == "cost_unit") m_costUnit = val;
        }
        else if (current == Section::Resource)
        {
            resourceAttribs[key] = val;
        }
        else if (current == Section::Project)
        {
            try   { projectEntries[key] = std::stod(val); }
            catch (...) { /* malformed numeric – skip */ }
        }
        else if (current == Section::AdditionalBudget && key == "value")
        {
            try   { m_additionalBudget = std::stod(val); }
            catch (...) { /* malformed numeric – skip */ }
        }
    }

    // Commit the final section that has no following header
    finalizeSection();
}

int DataStore::findResourceIdByName(const std::string& name) const
{
    for (const auto& [id, ptr] : m_resources)
        if (ptr->getName() == name)
            return id;

    return -1;
}

Resource* DataStore::getResourceById(int resourceId) const
{
    const auto it = m_resources.find(resourceId);
    return it != m_resources.end() ? it->second : nullptr;
}   

Project* DataStore::getProjectById(int projectId) const
{
    const auto it = m_projects.find(projectId);
    return it != m_projects.end() ? it->second : nullptr;
}
