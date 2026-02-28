#include "crow.h"
#include "crow/middlewares/cors.h"
#include "HybridChatbot.h"

int main() {

    crow::App<crow::CORSHandler> app;   // Use CORS (official Crow)

    // Enable CORS
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .origin("*")
        .methods("POST"_method, "GET"_method)
        .headers("Content-Type");

    HybridChatbot chatbot;

    // Health route
    CROW_ROUTE(app, "/")([]() {
        return "Hybrid Chatbot Server Running 🚀";
    });

    // Chat API
    CROW_ROUTE(app, "/chat").methods("POST"_method)
    ([&chatbot](const crow::request& req) {

        auto body = crow::json::load(req.body);

        if (!body || !body.has("message"))
            return crow::response(400, "Invalid request");

        std::string userMessage = body["message"].s();
        std::string reply = chatbot.getResponse(userMessage);

        crow::json::wvalue response;
        response["reply"] = reply;

        return crow::response(response);
    });

    // Reload Route for live updates
    CROW_ROUTE(app, "/reload")
    ([&chatbot]() {
        chatbot.reloadData();
        crow::response res(200, "Success! Chatbot is now using updated rules and dataset."); 
        res.set_header("Cache-Control", "no-cache, no-store, must-revalidate");        
        res.set_header("Pragma", "no-cache");        
        res.set_header("Expires", "0");       
        return res;
    });

    // Finally, start the server
    app.port(18080).multithreaded().run();

    return 0;
}