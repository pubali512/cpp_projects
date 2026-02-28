#pragma once      // To prevent the file from being included multiple times

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>

// Definition of the Intent enum
enum class Intent { DEFINE, EXPLAIN, TERM, GENERAL };

class HybridChatbot {
private:
    // Data storage
    std::vector<std::pair<std::string, std::string>> data;
    std::unordered_map<std::string, double> idf;
    std::unordered_map<std::string, std::string> rules;

    // Private helper declarations
    void toLower(std::string &text);
    std::vector<std::string> tokenize(const std::string &text);
    std::unordered_map<std::string, double> computeTF(const std::vector<std::string> &tokens);
    double cosineSimilarity(const std::unordered_map<std::string, double> &a, 
                            const std::unordered_map<std::string, double> &b);
    Intent detectPromptType(const std::string &input);
    std::string stripPromptPrefix(const std::string &input);
    void loadRules(const std::string &filename);
    void computeIDF();
    std::unordered_map<std::string, double> buildVector(const std::string &text);

public:
    HybridChatbot();
    bool loadDataset(const std::string &filename);
    std::string getResponse(const std::string &input); 
    void reloadData() {
        loadRules("rules.json");
        loadDataset("dataset.json");
        std::cout << "Memory reloaded: Rules and Dataset updated." << std::endl;

        // DEBUG
        // std::cout << "DEBUG: Reloaded " << rules.size() << " rules." << std::endl;
        // if (rules.count("hello")) {
        // std::cout << "DEBUG: Current 'hello' rule: " << rules["hello"] << std::endl;
        // }
    }
};