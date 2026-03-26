# Hybrid Chatbot

## Project Overview

A command-line chatbot that combines **rule-based** and **retrieval-based** approaches to answer user queries. The chatbot first attempts to match user input against predefined keyword-response rules for quick answers. If no rule matches, it falls back to a **TF-IDF + cosine similarity** retrieval engine that searches a Q&A dataset of machine learning and AI topics.

### Main Features

- **Rule-based matching** — Instant responses for common greetings and commands (hi, hello, bye, help, thanks)
- **TF-IDF retrieval** — Computes term frequency–inverse document frequency vectors and uses cosine similarity to find the most relevant answer from the dataset
- **Intent detection** — Supports intent prefixes (`define:`, `explain:`, `ml term:`) that boost scores for definitional questions
- **Similarity threshold** — Uses a 0.15 threshold to avoid returning irrelevant answers
- **Single-file implementation** — Self-contained `HybridChatbot` class using only the C++ Standard Library

## Technologies Used & Installation Instructions

### Technologies

- **C++** (C++11 or later)
- Standard Template Library (STL) only — no external dependencies

### Prerequisites

- A C++ compiler (GCC, Clang, or MSVC)

### Build

```bash
cd HybridChatbot
g++ -o chatbot chatbot.cpp
```

Or with Clang:

```bash
clang++ -o chatbot chatbot.cpp
```

## Usage Instructions

### Running the Chatbot

Run the executable from the project directory (the data files must be in the working directory):

```bash
./chatbot
```

Type your questions at the prompt and press Enter. Type `exit` to quit.

### Example Session

```
You: hello
Bot: Hey there! How can I help you?

You: define: neural network
Bot: A neural network is a series of algorithms that endeavors to recognize underlying relationships in a set of data...

You: explain: overfitting
Bot: Overfitting occurs when a model learns the training data too well, including its noise...

You: exit
```

### Intent Prefixes

| Prefix      | Effect                                              |
|-------------|-----------------------------------------------------|
| `define:`   | Boosts scores for "what is" questions in the dataset |
| `explain:`  | Standard retrieval with prefix stripped              |
| `ml term:`  | Standard retrieval with prefix stripped              |

### Data Files

| File          | Format                   | Description                                        |
|---------------|--------------------------|----------------------------------------------------|
| `dataset.txt` | `question\|answer` per line | 39 Q&A pairs on ML/AI topics for TF-IDF retrieval |
| `rules.txt`   | `keyword\|response` per line | 5 keyword-response pairs for instant matching     |
