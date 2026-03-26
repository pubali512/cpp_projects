# Rule-Based Chatbot

## Project Overview

A simple command-line chatbot that uses **keyword-based pattern matching** to generate responses. The chatbot loads keyword-response pairs from a configuration file and scans user input for matching keywords, returning the first match found. This is the simplest chatbot in the chatbot projects series, demonstrating basic text matching without any machine learning or statistical techniques.

### Main Features

- **Keyword matching** — Converts user input to lowercase and checks for substring matches against predefined keywords
- **Configurable responses** — All keyword-response pairs are loaded from an external file, making it easy to customize behavior without code changes
- **Default fallback** — Returns a polite fallback message when no keyword matches
- **Single-file implementation** — Self-contained `ChatBot` class using only the C++ Standard Library

## Technologies Used & Installation Instructions

### Technologies

- **C++** (C++11 or later)
- Standard Template Library (STL) only — no external dependencies

### Prerequisites

- A C++ compiler (GCC, Clang, or MSVC)

### Build

```bash
cd RuleBasedChatbot
g++ -o chatbot chatbot.cpp
```

Or with Clang:

```bash
clang++ -o chatbot chatbot.cpp
```

## Usage Instructions

### Running the Chatbot

Run the executable from the project directory (the responses file must be in the working directory):

```bash
./chatbot
```

Type your messages at the prompt and press Enter. Type `exit` to quit.

### Example Session

```
You: hello
Bot: Hi there! How can I help you?

You: what is your name
Bot: I'm a simple rule-based chatbot!

You: how are you
Bot: I'm doing great, thanks for asking!

You: goodbye
Bot: Sorry, I don't understand that.

You: bye
Bot: Goodbye! Have a nice day!
```

### Data Files

| File            | Format                   | Description                                      |
|-----------------|--------------------------|--------------------------------------------------|
| `responses.txt` | `keyword:response` per line | 5 keyword-response pairs (hello, how are you, what is your name, bye, thanks) |

### Customizing Responses

To add or modify responses, edit `responses.txt`. Each line follows the format:

```
keyword:response text
```

The chatbot checks if the keyword appears anywhere in the user's input (case-insensitive).
