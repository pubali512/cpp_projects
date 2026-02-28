# React for front-end setup 

- Prerequisites:
  - Node.js installed (https://nodejs.org/en/ - LTS)
  - VS code extensions: React, Vite, JavaScript syntax highlighting, Prettier, Tailwind CSS IntelliSense

- Initialization Project structure
    - Front end folder (npm init vite . -- --template react) [Vite installation]

- Project structure:
    - WebHybridChatbotProject/
     │
     ├── frontend/
     │   ├──src/
     │   │   └── App.jsx (App shell: Full Chatbot code)
     │   │   └── main.jsx (entry point)
     │   │   └── index.css (global styles using Tailwind css)
     │   ├── index.html (Vite template)
     │   ├── package.json (dependencies, scripts)
     │   ├── vite.config.js (Vite configuration: The tailwindcss() plugin and the proxy set to 18080)
     │   ├── package-lock.json (dependency tree)
     │   ├── tailwind.config.js (tailwind configuration)
     │   └── postcss.config.js (postcss configuration)
 


## Setup commands and steps

- Remove App.css from src folder as Tailwind css is used globally

# Crow for back-end setup
- Prerequisites:
    - Crow (C++ web framework library): Set up official Crow from GitHub (git clone https://github.com/CrowCpp/Crow.git crow)
    - CMake (Build system generator): Install via terminal (For MacOS: brew install cmake)

- Project structure:
    - WebHybridChatbotProject/
     │
     ├── backend/
     │   ├── main.cpp
     │   ├── HybridChatbot.h
     │   ├── HybridChatbot.cpp
     │   ├── dataset.json
     │   └── rules.json
     │
     ├── CMakeLists.txt
     │
     ├── external/
     │   └── crow/
     └── build/



## Setup commands and steps