# Web Hybrid Chatbot

## Project Overview

A full-stack web chatbot application with a **C++ backend** powered by the [Crow](https://github.com/CrowCpp/Crow) HTTP framework and a **React frontend** styled with Tailwind CSS. The backend combines rule-based and TF-IDF retrieval-based approaches (hybrid chatbot) to answer user queries about machine learning and AI topics. The frontend provides an animated chat interface with a dark theme.

### Main Features

- **Hybrid chatbot engine** — Combines rule-based keyword matching with TF-IDF + cosine similarity retrieval for intelligent responses
- **REST API** — Crow-based HTTP server with endpoints for chat, health check, and live data reload
- **CORS support** — Built-in CORS middleware for cross-origin frontend-backend communication
- **Hot reload** — Reload rules and dataset JSON files without restarting the server
- **Animated chat UI** — React frontend with Framer Motion animations for message bubbles
- **Dark theme** — Tailwind CSS v4 styled interface with Lucide icons
- **Intent detection** — Supports `define:`, `explain:`, and `ml term:` prefixes for query refinement

## Technologies Used & Installation Instructions 

### Technologies

#### Backend
- **C++17**
- **[Crow](https://github.com/CrowCpp/Crow)** — Lightweight C++ HTTP framework (included in `external/crow/`)
- **CMake** — Build system
- **JSON** — Crow's built-in JSON parser for data files and API payloads

#### Frontend
- **React 19** with Vite 7
- **Tailwind CSS v4** (via `@tailwindcss/vite` plugin)
- **Framer Motion** — Animation library for chat message transitions
- **Lucide React** — Icon library
- **ESLint** — Code linting

### Prerequisites

- **Backend:** C++17 compatible compiler (GCC, Clang, or MSVC), CMake (>= 3.10)
- **Frontend:** Node.js (>= 18), npm

### Installation

#### Backend Setup

1. Clone the Crow library into the `external/` directory (if not already present):
   ```bash
   cd external
   git clone https://github.com/CrowCpp/Crow.git crow
   ```

2. Build the backend with CMake:
   ```bash
   cd WebHybridChatbot
   cmake -S . -B build
   cmake --build build
   ```
   The build process automatically copies `rules.json` and `dataset.json` into the build directory.

#### Frontend Setup

```bash
cd frontend
npm install
```

## Usage Instructions

### Starting the Application

1. **Start the backend server:**
   ```bash
   ./build/chatbot
   ```
   The server starts on port **18080**.

2. **Start the frontend dev server:**
   ```bash
   cd frontend
   npm run dev
   ```
   Vite starts on `http://localhost:5173` and proxies API calls (`/chat`, `/reload`) to `http://localhost:18080`.

3. Open a browser and navigate to the URL shown by Vite (typically `http://localhost:5173`).

### Using the Chatbot

- Type a message in the input field and press Enter or click Send
- The chatbot responds using rule matching or TF-IDF retrieval
- Use intent prefixes for targeted queries:
  - `define: neural network` — Boosts definitional answers
  - `explain: overfitting` — Standard retrieval with prefix stripped
  - `ml term: backpropagation` — Standard retrieval with prefix stripped
- Click the **Reload** button to hot-reload the backend's rules and dataset files without restarting the server

### API Endpoints

| Method | Endpoint   | Description                                | Request Body              | Response                  |
|--------|------------|--------------------------------------------|---------------------------|---------------------------|
| GET    | `/`        | Health check                               | —                         | `"Hello, World!"`         |
| POST   | `/chat`    | Send a message and get a chatbot response  | `{"message": "..."}` | `{"reply": "..."}`   |
| GET    | `/reload`  | Hot-reload rules and dataset JSON files    | —                         | `"Data reloaded"`         |

### Data Files

| File               | Format                                     | Description                                   |
|--------------------|--------------------------------------------|-----------------------------------------------|
| `backend/dataset.json` | JSON array of `{"q": "...", "a": "..."}` | 20 ML/AI Q&A pairs for TF-IDF retrieval      |
| `backend/rules.json`   | JSON object `{"keyword": "response"}`     | 5 rule-based instant responses                |

### Project Structure

```
WebHybridChatbot/
├── CMakeLists.txt            # Backend build configuration
├── instructions.md           # Original setup notes
├── backend/
│   ├── main.cpp              # Crow HTTP server with routes and CORS
│   ├── HybridChatbot.h       # Chatbot class declaration
│   ├── HybridChatbot.cpp     # TF-IDF engine + rule matching implementation
│   ├── dataset.json          # Q&A retrieval corpus
│   └── rules.json            # Keyword-response rules
├── external/
│   └── crow/                 # Crow HTTP framework (header-only)
└── frontend/
    ├── package.json          # Node.js dependencies
    ├── vite.config.js        # Vite config with API proxy
    ├── tailwind.config.js    # Tailwind CSS configuration
    ├── index.html            # HTML entry point
    └── src/
        ├── App.jsx           # Chat UI component
        ├── main.jsx          # React entry point
        └── index.css         # Tailwind v4 imports + dark theme
```
