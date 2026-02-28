/* -------- Hybrid Chatbot Implementation --------- */

// A Hybrid Chatbot = Rule-Based + Retrieval-Based

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <algorithm>

using namespace std;

enum class Intent {
    DEFINE,
    EXPLAIN,
    TERM,
    GENERAL
};

class HybridChatbot {
private:
    // =========================
    // --- Datastorage ---
    // =========================

    vector<pair<string, string>> data;      // Dataset Q&A
    unordered_map<string, double> idf;      // IDF values
    unordered_map<string, string> rules;    // Rule-based responses

    // ==========================
    // --- Text utilities ---
    // ==========================

    // Converts text to lowercase for uniform processing
    void toLower(string &text) {
        transform(text.begin(), text.end(), text.begin(), ::tolower);
    }

    // Splits the input text into individual lowercase words (tokens)
    // Removes punctuation and prepares text for TF-IDF processing
    vector<string> tokenize(const string &text) {
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
    unordered_map<string, double> computeTF(const vector<string> &tokens) {
        unordered_map<string, double> tf;
        for (const auto &word : tokens)
            tf[word]++;

        for (auto &item : tf)
            item.second /= tokens.size();

        return tf;
    }

    // Calculates cosine similarity between two TF-IDF vectors
    // Measures how similar two texts are based on the angle between vectors
    // Returns a value between 0 (no similarity) and 1 (identical)
    double cosineSimilarity(
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
    Intent detectPromptType(const string &input) {
        if (input.rfind("define:", 0) == 0) 
            return Intent::DEFINE;

        if (input.rfind("explain:", 0) == 0) 
            return Intent::EXPLAIN;

        if (input.rfind("ml term:", 0) == 0) 
            return Intent::TERM;

        return Intent::GENERAL;
    }

    // Removes intent prefixes like "define:", "explain:", etc.
    // Returns only the core user query for similarity matching
    string stripPromptPrefix(const string &input) {
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
    // --- Rule Initialization ---
    // ==============================

    // Loads predefined rule-based responses
    void loadRules(const string &filename) {

    ifstream file(filename);
    if (!file)
        return;

    string line, key, value;

    while (getline(file, line)) {

        stringstream ss(line);

        if (getline(ss, key, '|') && getline(ss, value)) {

            // Remove leading spaces
            key.erase(0, key.find_first_not_of(" "));
            value.erase(0, value.find_first_not_of(" "));

            toLower(key);

            rules[key] = value;
        }
    }
}




public:
    // Constructor: initializes the chatbot by loading predefined rule-based responses
    // Ensures the chatbot object is ready to respond immediately after creation
    HybridChatbot() {
    loadRules("rules.txt");
}

    // Loads chatbot question-answer pairs from dataset file
    // Each line follows the format: question | response
    bool loadDataset(const string &filename) {
        ifstream file(filename);
        if (!file) return false;

        data.clear();
        idf.clear();    

        string line, q, a;

        while (getline(file, line)) {

            stringstream ss(line);

            if (getline(ss, q, '|') && getline(ss, a)) {

                // Remove leading spaces
                q.erase(0, q.find_first_not_of(" "));
                a.erase(0, a.find_first_not_of(" "));

                toLower(q);

                data.push_back({q, a});
            }
        }
        computeIDF();
        return true;
    }

    // Computes Inverse Document Frequency (IDF) for all words
    // IDF measures how important a word is across the dataset
    void computeIDF() {
        unordered_map<string, int> docCount;
        int N = data.size();

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
    unordered_map<string, double> buildVector(const string &text) {
        vector<string> tokens = tokenize(text);
        auto tf = computeTF(tokens);

        unordered_map<string, double> tfidf;
        for (const auto &item : tf)
            tfidf[item.first] = item.second * idf[item.first];

        return tfidf;
    }

    // Generates chatbot response for user input
    // Handles intent detection, exact matching, TF-IDF similarity,
    // applies similarity threshold, and returns best response
    string reply(const string &input) {
        string userInput = input;
        toLower(userInput);

        // Detect intent (Prompt Type)
        Intent intent = detectPromptType(userInput);

        // Rule-Based Handling
        if (rules.count(userInput)) {
            return rules[userInput];
        }

        //Check prompt misuse
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

        // Exact match (Important!)
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

            // Intent-based score boost
            if (intent == Intent::DEFINE && pair.first.find("what is") != string::npos)
            score += 0.05;

            if (score > bestScore) {
                bestScore = score;
                bestAnswer = pair.second;
            }
        }

        // Similarity threshold (Important)
        if (bestScore < 0.15) {
            return "Sorry, I don't understand that.";
        }

        return bestAnswer;
    }
};

int main() {
    HybridChatbot bot;

    if (!bot.loadDataset("dataset.txt")) {
        cerr << "Failed to load dataset.\n";
        return 1;
    }

    cout << "ChatBot (Hybrid): Hello! Type 'exit' to quit.\n";

    string input;
    while (true) {
        cout << "You: ";
        getline(cin, input);
        if (input == "exit") break;
        cout << "ChatBot: " << bot.reply(input) << endl;
    }

    return 0;
}
