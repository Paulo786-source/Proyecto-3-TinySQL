#include "httplib.h"
#include "json.hpp"
#include "QueryProcessor.h"
#include "StoredDataManager.h"
#include <iostream>
#include <string>

using json = nlohmann::json;

// serializa un QueryResult al JSON que espera el cliente
static std::string buildJsonResponse(const QueryResult& result) {
    json j;
    j["columns"] = result.columns;
    j["rows"] = result.rows;
    j["time_ms"] = result.time_ms;
    j["error"] = result.error;
    return j.dump();
}

int main() {
    StoredDataManager sdm;
    QueryProcessor qp(sdm);

    httplib::Server server;

    // POST /query — recibe { sql, db_context }, ejecuta y retorna QueryResult
    server.Post("/query", [&qp](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");

        // parsea el body JSON; false = no lanzar excepción si está malformado
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            QueryResult err;
            err.error = "request JSON inválido";
            err.success = false;
            err.time_ms = 0;
            res.set_content(buildJsonResponse(err), "application/json");
            return;
        }

        std::string sql = body.value("sql", "");
        std::string dbContext = body.value("db_context", "");

        QueryResult result = qp.execute(sql, dbContext);
        res.set_content(buildJsonResponse(result), "application/json");
        });

    // preflight CORS para que React pueda conectarse
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