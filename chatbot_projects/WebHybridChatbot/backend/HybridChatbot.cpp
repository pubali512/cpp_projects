/* -------- Hybrid Chatbot Implementation --------- */

// A Hybrid Chatbot = Rule-Based + Retrieval-Based

#include "HybridChatbot.h"
#include "crow.h"           // Needed for crow::json::load
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <cctype>           // For tolower

using namespace std;


// Constructor
HybridChatbot::HybridChatbot() {
    loadRules("rules.json");

    if (loadDataset("dataset.json")) {
        std::cout << "Successfully loaded dataset.json with " << data.size() << " entries." << std::endl;
    } 
    else {
        std::cout << "ERROR: Failed to load dataset.json!" << std::endl;
    }
}

// ==========================
// --- Text utilities ---
// ==========================

// Converts text to lowercase for uniform processing
void HybridChatbot::toLower(string &text) {
    transform(text.begin(), text.end(), text.begin(), ::tolower);
}

// Splits the input text into individual lowercase words (tokens)
// Removes punctuation and prepares text for TF-IDF processing
vector<string> HybridChatbot::tokenize(const string &text) {
    vector<string> tokens;
    string word;
    stringstream ss(text);
    while (ss >> word) 
        tokens.push_back(word);
    return tokens;
}

// ==========================
// --- TF-IDF Helpers ---
// ==========================

// Computes Term Frequency (TF) for each word
// TF = (count of word in input) / (total number of words)
unordered_map<string, double> HybridChatbot::computeTF(const vector<string> &tokens) {
    unordered_map<string, double> tf;
    if (tokens.empty()) return tf;

    for (const auto &word : tokens)
        tf[word]++;

    for (auto &item : tf)
        item.second /= tokens.size();

    return tf;
}

// Calculates cosine similarity between two TF-IDF vectors
// Measures how similar two texts are based on the angle between vectors
// Returns a value between 0 (no similarity) and 1 (identical)
double HybridChatbot::cosineSimilarity(
    const unordered_map<string, double> &a,
    const unordered_map<string, double> &b) {

    double dot = 0.0, magA = 0.0, magB = 0.0;

    for (const auto &item : a) {
        magA += item.second * item.second;
        if (b.count(item.first))
            dot += item.second * b.at(item.first);
    }

    for (const auto &item : b)
        magB += item.second * item.second;

    if (magA == 0 || magB == 0)
        return 0.0;

    return dot / (sqrt(magA) * sqrt(magB));
}

// ===========================
// --- Intent Helpers ---
// ===========================

// Detects the intent type based on prompt prefix
// Example intents: define, explain, term, or general
Intent HybridChatbot::detectPromptType(const string &input) {
    string temp = input; 
    
    // Convert the copy to lowercase
    std::transform(temp.begin(), temp.end(), temp.begin(), 
                   [](unsigned char c){ return std::tolower(c); });

    if (temp.rfind("define:", 0) == 0) 
        return Intent::DEFINE;

    if (temp.rfind("explain:", 0) == 0) 
        return Intent::EXPLAIN;

    if (temp.rfind("ml term:", 0) == 0) 
        return Intent::TERM;

    return Intent::GENERAL;
}

// Removes intent prefixes like "define:", "explain:", etc.
// Returns only the core user query for similarity matching
string HybridChatbot::stripPromptPrefix(const string &input) {
    size_t pos = input.find(":");
    if (pos != string::npos) {  
        string result = input.substr(pos + 1);

        // remove leading spaces
        result.erase(0, result.find_first_not_of(" "));
        return result;
    }
    return input;
}

// ==============================
// --- JSON Data Loading ---
// ==============================

// Loads predefined rule-based responses
void HybridChatbot::loadRules(const string &filename) {

    rules.clear();             // Wipe old memory before loading new file
    data.clear();

    ifstream file(filename);
    if (!file)
    return;

    stringstream buffer;
    buffer << file.rdbuf();

    auto json_data = crow::json::load(buffer.str());
    if (!json_data) return;

    // Expected rules.json: {"hello": "Hi there!", "exit": "Goodbye!"}
    for (auto& key : json_data.keys()) {
        string lowerKey = key;
        toLower(lowerKey);
        rules[lowerKey] = json_data[key].s();
    }
}

// Loads chatbot question-answer pairs from dataset file
bool HybridChatbot::loadDataset(const string &filename) {
    ifstream file(filename);
    if (!file) 
        return false;

    stringstream buffer;
    buffer << file.rdbuf();

    auto json_data = crow::json::load(buffer.str());
    // Expected dataset.json: [{"q": "What is AI?", "a": "AI is..."}, {...}]
    if (!json_data || json_data.t() != crow::json::type::List) 
        return false;

    data.clear();
    idf.clear();    

    for (auto& item : json_data) {
        if (item.has("q") && item.has("a")) {
            string question = item["q"].s();
            toLower(question);
            data.push_back({question, item["a"].s()});
        }
    }
    computeIDF();
    return true;
}

// Computes Inverse Document Frequency (IDF) for all words
// IDF measures how important a word is across the dataset
void HybridChatbot::computeIDF() {
    unordered_map<string, int> docCount;
    int N = data.size();
    if (N == 0) 
        return;

    for (const auto &pair : data) {
        unordered_set<string> seen;
        for (const auto &word : tokenize(pair.first))
            seen.insert(word);

        for (const auto &word : seen)
            docCount[word]++;
    }

    for (const auto &item : docCount)
        idf[item.first] = log((double)N / item.second);
}

// Builds the TF-IDF vector for the given input text
// Combines Term Frequency (TF) and Inverse Document Frequency (IDF)
unordered_map<string, double> HybridChatbot::buildVector(const string &text) {
    vector<string> tokens = tokenize(text);
    auto tf = computeTF(tokens);

    unordered_map<string, double> tfidf;
    for (const auto &item : tf) {
        if (idf.count(item.first)) 
            tfidf[item.first] = item.second * idf[item.first];
        
    }
    return tfidf;
}


// ==============================
// --- Main Logic ---
// ==============================

// Generates chatbot response for user input
// Handles intent detection, exact matching, TF-IDF similarity,
// applies similarity threshold, and returns best response
string HybridChatbot::getResponse(const string &input) {
    string userInput = input;
    toLower(userInput);

    // Detect intent (Prompt Type)
    Intent intent = detectPromptType(userInput);

    // Rule-Based Handling
    if (rules.count(userInput)) {
        return rules[userInput];
    }   

    // Prefix Validation
    if ((intent == Intent::DEFINE || intent == Intent::EXPLAIN || intent == Intent::TERM)
        && userInput.find(":") == string::npos) {
        return "Usage: define: <topic> | explain: <topic> | term: <topic>";
    }

    // Remove prompt prefix
    string processedInput = stripPromptPrefix(userInput);

    // After stripping prefix
    if (processedInput.empty()) {
        return "Please provide a term after the prompt (e.g., define: machine learning).";
    }

    // Exact match check (Important!)
    for (const auto &pair : data) {
        if (processedInput == pair.first)
            return pair.second;
        }

    // Build TF-IDF vector for user input
    auto userVector = buildVector(processedInput);
    double bestScore = 0.0;

    string bestAnswer = "Sorry, I don't understand that.";

    // Compare with dataset
    for (const auto &pair : data) {
        auto questionVector = buildVector(pair.first);
        double score = cosineSimilarity(userVector, questionVector);
        // Debugging score output
        // std::cout << "Comparing against: " << pair.first << " | Score: " << score << std::endl;

        // Intent-based score boost
        if (intent == Intent::DEFINE && pair.first.find("what is") != string::npos)
            score += 0.05;

        if (score > bestScore) {
            bestScore = score;
            bestAnswer = pair.second;
        }
    }

    // Similarity threshold (Important)
    if (bestScore < 0.05) {
        return "Sorry, I don't understand that.";
    }

    return bestAnswer;
}



