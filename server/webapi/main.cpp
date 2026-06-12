#include "httplib.h"
#include <iostream>
#include <string>

int main() {
    httplib::Server server;

    // POST /query — endpoint principal del sistema
    server.Post("/query", [](const httplib::Request& req, httplib::Response& res) {
        // Por ahora devuelve JSON hardcodeado
        // En fases posteriores aqui se llamara al Query Processor
        std::string response = R"({
            "columns": ["id", "name"],
            "rows": [["1", "test"]],
            "time_ms": 0,
            "error": ""
        })";

        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(response, "application/json");
        });

    // Maneja preflight CORS para que React pueda conectarse
    server.Options("/query", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.set_content("", "text/plain");
        });

    std::cout << "TinySQLDb server running on http://localhost:8080" << std::endl;
    server.listen("localhost", 8080);

    return 0;
}