/*----- Simple Chatbot Impementation -----*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <algorithm>

using namespace std;

class ChatBot {
private:
    unordered_map<string, string> responses;

    // Convert string to lowercase
    void toLower(string &text) {
        transform(text.begin(), text.end(), text.begin(), ::tolower);
    }

public:

    // Load responses from responses.txt
    bool loadResponses(const string &filename) {
        ifstream file(filename);

        if (!file) return false;

        string line, key, value;

        while (getline(file, line)) {
            stringstream ss(line);
            
            if (getline(ss, key, ':') && getline(ss, value)) {
                toLower(key);
                responses[key] = value;
            }
        }
        return true;
    }

    // Generate chatbot response
    string reply(const string &userInput) {
        string copy = userInput;
        toLower(copy);

        for(const auto &item : responses){
            if (copy.find(item.first) != string::npos)
                return item.second;
        }
        return "Sorry, I don't understand that.";
    }
    
};

int main() {
    ChatBot bot;

    if (!bot.loadResponses("responses.txt")) {
        cerr << "Failed to load responses.\n";
        return 1;
    }

    string userInput;
    cout << "ChatBot: Hello! Type 'exit' to quit.\n";

    while (true) {
        cout << "You: ";
        getline(cin, userInput);
        
        if (userInput == "exit") {
            cout << "ChatBot: Goodbye!\n";
            break;
        }

        string botReply = bot.reply(userInput);
        cout << "ChatBot: " << botReply << "\n";
    }

    return 0;
}


