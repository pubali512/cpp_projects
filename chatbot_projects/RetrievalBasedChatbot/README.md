# Retrieval-Based Chatbot

## Project Overview

A command-line chatbot that uses a purely **retrieval-based** approach to answer user queries. It computes **TF-IDF (Term Frequency–Inverse Document Frequency)** vectors for both the user's input and a corpus of Q&A pairs, then uses **cosine similarity** to find and return the most relevant answer.

### Main Features

- **TF-IDF retrieval engine** — Builds IDF scores across all questions in the dataset and computes TF-IDF vectors for similarity matching
- **Cosine similarity matching** — Finds the closest matching question in the dataset using cosine similarity with a 0.15 threshold
- **Intent detection** — Supports intent prefixes (`define:`, `explain:`, `ml term:`) with score boosting for definitional queries
- **ML/AI knowledge base** — Ships with 39 Q&A pairs covering machine learning and artificial intelligence topics
- **Single-file implementation** — Self-contained `TFIDFChatbot` class using only the C++ Standard Library

## Technologies Used & Installation Instructions

### Technologies

- **C++** (C++11 or later)
- Standard Template Library (STL) only — no external dependencies

### Prerequisites

- A C++ compiler (GCC, Clang, or MSVC)

### Build

```bash
cd RetrievalBasedChatbot
g++ -o chatbot chatbot.cpp
```

Or with Clang:

```bash
clang++ -o chatbot chatbot.cpp
```

## Usage Instructions

### Running the Chatbot

Run the executable from the project directory (the dataset file must be in the working directory):

```bash
./chatbot
```

Type your questions at the prompt and press Enter. Type `exit` to quit.

### Example Session

```
You: what is deep learning
Bot: Deep learning is a subset of machine learning that uses neural networks with many layers...

You: define: gradient descent
Bot: Gradient descent is an optimization algorithm used to minimize the loss function...

You: exit
```

### Intent Prefixes

| Prefix      | Effect                                               |
|-------------|------------------------------------------------------|
| `define:`   | Boosts scores for "what is" questions in the dataset  |
| `explain:`  | Standard retrieval with prefix stripped               |
| `ml term:`  | Standard retrieval with prefix stripped               |

### Data Files

| File          | Format                      | Description                                        |
|---------------|-----------------------------|----------------------------------------------------|
| `dataset.txt` | `question\|answer` per line | 39 Q&A pairs on ML/AI topics for TF-IDF retrieval  |
