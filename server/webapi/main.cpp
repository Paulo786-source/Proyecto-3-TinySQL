#include "httplib.h"
#include "json.hpp"
#include "QueryProcessor.h"
#include "StoredDataManager.h"
#include <iostream>
#include <chrono>

using json = nlohmann::json;

// puerto donde escucha el servidor
static const int SERVER_PORT = 8081;

// serializa un QueryResult al json que espera el cliente
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

    // reconstruye en memoria todos los indices que quedaron
    // registrados en el catalogo de sesiones anteriores.
    // sin esta llamada, los indices se pierden al reiniciar
    // el servidor aunque el catalogo los tenga guardados.
    qp.loadIndexes();

    httplib::Server server;

    // POST /query — recibe { sql, db_context }, ejecuta y retorna QueryResult
    server.Post("/query", [&qp](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");

        // parsea el body json; false = no lanzar excepcion si esta malformado
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            QueryResult err;
            err.error = "request JSON invalido";
            err.success = false;
            err.time_ms = 0;
            res.set_content(buildJsonResponse(err), "application/json");
            return;
        }

        std::string sql = body.value("sql", "");
        std::string dbContext = body.value("db_context", "");

        // mide el tiempo real de procesamiento en el servidor
        auto start = std::chrono::steady_clock::now();
        QueryResult result = qp.execute(sql, dbContext);
        auto end = std::chrono::steady_clock::now();
        result.time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        res.set_content(buildJsonResponse(result), "application/json");
        });

    // preflight CORS para que React pueda conectarse
    server.Options("/query", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.set_content("", "text/plain");
        });

    std::cout << "TinySQLDb server corriendo en http://localhost:" << SERVER_PORT << std::endl;
    server.listen("localhost", SERVER_PORT);
    return 0;
}