cmake_minimum_required(VERSION 3.16)
project(WebHybridChatbot)

set(CMAKE_CXX_STANDARD 17)

# 1. Bring in the Crow project
add_subdirectory(external/crow)

# 2. Define your app
add_executable(chatbot
    backend/main.cpp
    backend/HybridChatbot.cpp
)

# 3. FIX: Explicitly tell your app where Crow's headers are
target_include_directories(chatbot PRIVATE external/crow/include)

# 4. Link the libraries
target_link_libraries(chatbot PRIVATE Crow::Crow)

# 5. Sync JSON data to build folder after every build
add_custom_command(
    TARGET chatbot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/backend/rules.json"
        "${CMAKE_CURRENT_BINARY_DIR}/rules.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/backend/dataset.json"
        "${CMAKE_CURRENT_BINARY_DIR}/dataset.json"
    COMMENT "Syncing JSON data from backend/ to build folder..."
)