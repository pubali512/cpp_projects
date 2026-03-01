#pragma once

#include <map>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// Resource
//
// Represents a single resource (e.g. "developer", "tester").
// Attributes parsed from the file are stored in a generic string->string map so
// that any future attribute can be added without touching this header.
// A unique integer ID is auto-assigned on construction.
// ─────────────────────────────────────────────────────────────────────────────
class Resource
{
public:
    /// Construct from a map of parsed attribute key/value pairs.
    explicit Resource(std::map<std::string, std::string> attributes);

    int         getId()           const { return m_id; }

    /// Convenience accessor for the "name" attribute.
    std::string getName()         const;

    /// Convenience accessor for the "cost" attribute (returns 0.0 if absent).
    double      getCost()         const;

    /// Full attribute map (read-only).
    const std::map<std::string, std::string>& getAttributes() const { return m_attributes; }

private:
    static int m_nextId;

    int         m_id;
    std::map<std::string, std::string> m_attributes;
};

// ─────────────────────────────────────────────────────────────────────────────
// Project
//
// Represents a project that consumes one or more resources.
// Internally stores a map of  resource-ID → required units  so that look-ups
// are O(log n) by ID.  A unique integer ID is auto-assigned on construction.
// ─────────────────────────────────────────────────────────────────────────────
class Project
{
public:
    /// Construct with a (possibly empty) map of resource costs.
    explicit Project(std::map<int, double> resourceCosts = {});

    int getId() const { return m_id; }

    /// resource ID → required units for this project.
    const std::map<int, double>& getResourceCosts() const { return m_resourceCosts; }

    /// Add or overwrite the required units for a given resource ID.
    void addResourceCost(int resourceId, double units);

private:
    static int m_nextId;

    int                  m_id;
    std::map<int, double> m_resourceCosts;
};

// ─────────────────────────────────────────────────────────────────────────────
// DataStore  (singleton)
//
// Owns all Resource* and Project* objects created during parsing.
// Call loadFromFile() with a path to an EXAMPLE.md-formatted file.
// Subsequent calls to loadFromFile() ADD to the existing collections.
// ─────────────────────────────────────────────────────────────────────────────
class DataStore
{
public:
    static DataStore& getInstance();

    /// Parse an EXAMPLE.md-style file and populate resources / projects.
    /// Throws std::runtime_error if the file cannot be opened.
    void loadFromFile(const std::string& filepath);

    /// Clears all stored resources and projects (frees memory).
    void clear();

    const std::map<int, Resource*>& getResources()      const { return m_resources;       }
    const std::map<int, Project*>&  getProjects()        const { return m_projects;         }
    double                          getAdditionalBudget() const { return m_additionalBudget; }

    Resource* getResourceById(int resourceId) const;
    Project*  getProjectById(int projectId) const;

    // Non-copyable / non-movable
    DataStore(const DataStore&)            = delete;
    DataStore& operator=(const DataStore&) = delete;
    DataStore(DataStore&&)                 = delete;
    DataStore& operator=(DataStore&&)      = delete;

    ~DataStore();

private:
    DataStore() = default;

    /// Returns the resource ID whose "name" attribute equals @p name,
    /// or -1 if not found.
    int findResourceIdByName(const std::string& name) const;

    std::map<int, Resource*> m_resources;
    std::map<int, Project*>  m_projects;
    double                   m_additionalBudget{0.0};
};
