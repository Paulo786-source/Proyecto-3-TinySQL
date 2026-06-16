#include "QueryProcessor.h"
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <chrono>

QueryProcessor::QueryProcessor(StoredDataManager& sdm) : sdm_(sdm) {}

// convierte un string a mayusculas para parsing case-insensitive
static std::string toUpper(const std::string& s) {
    std::string result = s;
    for (char& c : result) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return result;
}

// divide el SQL en tokens respetando strings entre comillas simples
static std::vector<std::string> tokenize(const std::string& sql) {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < sql.size(); i++) {
        char c = sql[i];

        if (c == '\'' && !inQuotes) {
            // inicio de string entre comillas
            inQuotes = true;
            current += c;
        }
        else if (c == '\'' && inQuotes) {
            // fin de string entre comillas
            inQuotes = false;
            current += c;
        }
        else if ((c == ' ' || c == '\t' || c == '\n') && !inQuotes) {
            // espacio fuera de comillas: termina el token actual
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        }
        else if ((c == '(' || c == ')' || c == ',' || c == ';') && !inQuotes) {
            // puntuacion fuera de comillas: termina el token actual y agrega la puntuacion
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            tokens.push_back(std::string(1, c));
        }
        else {
            current += c;
        }
    }

    // agrega el ultimo token si quedo algo pendiente
    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

// elimina las comillas simples que rodean un valor si las tiene
static std::string stripQuotes(const std::string& token) {
    if (token.size() >= 2 && token.front() == '\'' && token.back() == '\'') {
        return token.substr(1, token.size() - 2);
    }
    return token;
}

ASTNode QueryProcessor::parse(const std::string& sql) {
    std::vector<std::string> tokens = tokenize(sql);

    if (tokens.empty()) {
        throw std::runtime_error("sentencia SQL vacia");
    }

    std::string keyword = toUpper(tokens[0]);

    if (keyword == "CREATE") {
        if (tokens.size() < 3) {
            throw std::runtime_error("sintaxis invalida: CREATE requiere tipo y nombre");
        }
        std::string objectType = toUpper(tokens[1]);

        if (objectType == "DATABASE") {
            ASTNode node;
            node.type = "CREATE_DB";
            node.database = tokens[2];
            node.hasWhere = false;
            node.hasOrderBy = false;
            return node;
        }
        // otros tipos de CREATE se implementan en fases posteriores
        throw std::runtime_error("tipo de CREATE no soportado: " + tokens[1]);
    }

    if (keyword == "SET") {
        if (tokens.size() < 3) {
            throw std::runtime_error("sintaxis invalida: SET DATABASE requiere un nombre");
        }
        std::string objectType = toUpper(tokens[1]);

        if (objectType == "DATABASE") {
            ASTNode node;
            node.type = "SET_DB";
            node.database = tokens[2];
            node.hasWhere = false;
            node.hasOrderBy = false;
            return node;
        }
        throw std::runtime_error("sintaxis SET invalida: se esperaba DATABASE");
    }

    // otros tipos de sentencia se implementan en fases posteriores
    throw std::runtime_error("sentencia no reconocida: " + tokens[0]);
}

std::string QueryProcessor::validate(const ASTNode& node, const std::string& dbContext) {
    if (node.type == "CREATE_DB") {
        if (node.database.empty()) {
            return "el nombre de la base de datos no puede estar vacio";
        }
        if (sdm_.databaseExists(node.database)) {
            return "la base de datos '" + node.database + "' ya existe";
        }
        return "";
    }

    if (node.type == "SET_DB") {
        if (node.database.empty()) {
            return "el nombre de la base de datos no puede estar vacio";
        }
        if (!sdm_.databaseExists(node.database)) {
            return "la base de datos '" + node.database + "' no existe";
        }
        return "";
    }

    return "";
}

QueryResult QueryProcessor::execute(const std::string& sql, const std::string& dbContext) {
    QueryResult result;
    result.success = false;

    auto start = std::chrono::steady_clock::now();

    // paso 1: parseo
    ASTNode node;
    try {
        node = parse(sql);
    }
    catch (const std::exception& e) {
        result.error = std::string("error de sintaxis: ") + e.what();
        result.time_ms = 0;
        return result;
    }

    // paso 2: validacion semantica
    std::string validationError = validate(node, dbContext);
    if (!validationError.empty()) {
        result.error = validationError;
        result.time_ms = 0;
        return result;
    }

    // paso 3: despacho al ejecutor correspondiente
    if (node.type == "CREATE_DB") {
        result = executeCreateDatabase(node);
    }
    else if (node.type == "SET_DB") {
        result = executeSetDatabase(node);
    }
    else {
        result.error = "tipo de sentencia no implementado aun: " + node.type;
    }

    auto end = std::chrono::steady_clock::now();
    result.time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    return result;
}

QueryResult QueryProcessor::executeCreateDatabase(const ASTNode& node) {
    QueryResult result;
    result.time_ms = 0;

    bool ok = sdm_.createDatabase(node.database);
    if (!ok) {
        result.error = "no se pudo crear la base de datos '" + node.database + "'";
        result.success = false;
        return result;
    }

    result.success = true;
    return result;
}

QueryResult QueryProcessor::executeSetDatabase(const ASTNode& node) {
    QueryResult result;
    result.time_ms = 0;
    // la validacion ya confirmo que la base de datos existe
    result.success = true;
    return result;
}

// stubs: se implementan en fases posteriores
QueryResult QueryProcessor::executeCreateTable(const ASTNode& node, const std::string& dbContext) {
    QueryResult result;
    result.error = "CREATE TABLE no implementado aun";
    result.success = false;
    return result;
}

QueryResult QueryProcessor::executeDropTable(const ASTNode& node, const std::string& dbContext) {
    QueryResult result;
    result.error = "DROP TABLE no implementado aun";
    result.success = false;
    return result;
}

QueryResult QueryProcessor::executeInsert(const ASTNode& node, const std::string& dbContext) {
    QueryResult result;
    result.error = "INSERT no implementado aun";
    result.success = false;
    return result;
}

QueryResult QueryProcessor::executeSelect(const ASTNode& node, const std::string& dbContext) {
    QueryResult result;
    result.error = "SELECT no implementado aun";
    result.success = false;
    return result;
}

QueryResult QueryProcessor::executeUpdate(const ASTNode& node, const std::string& dbContext) {
    QueryResult result;
    result.error = "UPDATE no implementado aun";
    result.success = false;
    return result;
}

QueryResult QueryProcessor::executeDelete(const ASTNode& node, const std::string& dbContext) {
    QueryResult result;
    result.error = "DELETE no implementado aun";
    result.success = false;
    return result;
}

QueryResult QueryProcessor::executeCreateIndex(const ASTNode& node, const std::string& dbContext) {
    QueryResult result;
    result.error = "CREATE INDEX no implementado aun";
    result.success = false;
    return result;
}

void QueryProcessor::loadIndexes() {
    // se implementa en fase 3 cuando existan indices que recargar
}