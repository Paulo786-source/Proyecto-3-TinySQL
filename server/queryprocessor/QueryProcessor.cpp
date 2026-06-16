#include "QueryProcessor.h"
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <functional>
#include <unordered_map>

// constructor
QueryProcessor::QueryProcessor(StoredDataManager& sdm) : sdm_(sdm) {}

// convierte un string a mayúsculas para parsing case-insensitive
static std::string toUpper(const std::string& s) {
    std::string result = s;
    for (char& c : result) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return result;
}

// divide el sql en tokens respetando strings entre comillas simples y dobles
static std::vector<std::string> tokenize(const std::string& sql) {
    std::vector<std::string> tokens;
    std::string current;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;

    for (size_t i = 0; i < sql.size(); i++) {
        char c = sql[i];

        if (c == '\'' && !inDoubleQuote) {
            inSingleQuote = !inSingleQuote;
            current += c;
        }
        else if (c == '"' && !inSingleQuote) {
            inDoubleQuote = !inDoubleQuote;
            current += c;
        }
        else if ((c == ' ' || c == '\t' || c == '\n') && !inSingleQuote && !inDoubleQuote) {
            // espacio fuera de comillas termina el token actual
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        }
        else if ((c == '(' || c == ')' || c == ',' || c == ';') && !inSingleQuote && !inDoubleQuote) {
            // puntuación fuera de comillas termina el token y se agrega sola
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

    // agrega el último token si quedó algo pendiente
    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

// elimina las comillas simples o dobles que rodean un valor si las tiene
static std::string stripQuotes(const std::string& token) {
    if (token.size() >= 2) {
        char first = token.front();
        char last = token.back();
        if ((first == '\'' && last == '\'') || (first == '"' && last == '"')) {
            return token.substr(1, token.size() - 2);
        }
    }
    return token;
}

ASTNode QueryProcessor::parse(const std::string& sql) {
    std::vector<std::string> tokens = tokenize(sql);

    if (tokens.empty()) {
        throw std::runtime_error("sentencia SQL vacía");
    }

    std::string keyword = toUpper(tokens[0]);

    if (keyword == "CREATE") {
        if (tokens.size() < 3) {
            throw std::runtime_error("sintaxis inválida: CREATE requiere tipo y nombre");
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
        throw std::runtime_error("tipo de CREATE no soportado aún: " + tokens[1]);
    }

    if (keyword == "SET") {
        if (tokens.size() < 3) {
            throw std::runtime_error("sintaxis inválida: SET DATABASE requiere un nombre");
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
        throw std::runtime_error("sintaxis SET inválida: se esperaba DATABASE");
    }

    // otras sentencias se implementan en fases posteriores
    throw std::runtime_error("sentencia no reconocida: " + tokens[0]);
}

std::string QueryProcessor::validate(const ASTNode& node, const std::string& dbContext) {
    if (node.type == "CREATE_DB") {
        if (node.database.empty()) {
            return "el nombre de la base de datos no puede estar vacío";
        }
        if (sdm_.databaseExists(node.database)) {
            return "la base de datos '" + node.database + "' ya existe";
        }
        return "";
    }

    if (node.type == "SET_DB") {
        if (node.database.empty()) {
            return "el nombre de la base de datos no puede estar vacío";
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
    result.time_ms = 0;
    result.success = false;

    // paso 1: parseo
    ASTNode node;
    try {
        node = parse(sql);
    }
    catch (const std::exception& e) {
        result.error = std::string("error de sintaxis: ") + e.what();
        return result;
    }

    // paso 2: validación semántica
    std::string validationError = validate(node, dbContext);
    if (!validationError.empty()) {
        result.error = validationError;
        return result;
    }

    // paso 3: dispatch al ejecutor correspondiente
    using Handler = std::function<QueryResult()>;
    std::unordered_map<std::string, Handler> dispatch = {
        { "CREATE_DB",    [&] { return executeCreateDatabase(node); } },
        { "SET_DB",       [&] { return executeSetDatabase(node); } },
        { "CREATE_TABLE", [&] { return executeCreateTable(node, dbContext); } },
        { "DROP_TABLE",   [&] { return executeDropTable(node, dbContext); } },
        { "INSERT",       [&] { return executeInsert(node, dbContext); } },
        { "SELECT",       [&] { return executeSelect(node, dbContext); } },
        { "UPDATE",       [&] { return executeUpdate(node, dbContext); } },
        { "DELETE",       [&] { return executeDelete(node, dbContext); } },
        { "CREATE_INDEX", [&] { return executeCreateIndex(node, dbContext); } },
    };

    auto it = dispatch.find(node.type);
    if (it != dispatch.end()) {
        result = it->second();
    }
    else {
        result.error = "tipo de sentencia no implementado aún: " + node.type;
    }

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
    // la validación ya confirmó que la base de datos existe
    result.success = true;
    return result;
}

// stubs: se implementan en fases posteriores
QueryResult QueryProcessor::executeCreateTable(const ASTNode& node, const std::string& dbContext) {
    (void)node;
    (void)dbContext;
    QueryResult result;
    result.error = "CREATE TABLE se implementa en fase 2";
    result.success = false;
    return result;
}

QueryResult QueryProcessor::executeDropTable(const ASTNode& node, const std::string& dbContext) {
    (void)node;
    (void)dbContext;
    QueryResult result;
    result.error = "DROP TABLE se implementa en fase 2";
    result.success = false;
    return result;
}

QueryResult QueryProcessor::executeInsert(const ASTNode& node, const std::string& dbContext) {
    (void)node;
    (void)dbContext;
    QueryResult result;
    result.error = "INSERT se implementa en fase 3";
    result.success = false;
    return result;
}

QueryResult QueryProcessor::executeSelect(const ASTNode& node, const std::string& dbContext) {
    (void)node;
    (void)dbContext;
    QueryResult result;
    result.error = "SELECT se implementa en fase 3";
    result.success = false;
    return result;
}

QueryResult QueryProcessor::executeUpdate(const ASTNode& node, const std::string& dbContext) {
    (void)node;
    (void)dbContext;
    QueryResult result;
    result.error = "UPDATE se implementa en fase 3";
    result.success = false;
    return result;
}

QueryResult QueryProcessor::executeDelete(const ASTNode& node, const std::string& dbContext) {
    (void)node;
    (void)dbContext;
    QueryResult result;
    result.error = "DELETE se implementa en fase 3";
    result.success = false;
    return result;
}

QueryResult QueryProcessor::executeCreateIndex(const ASTNode& node, const std::string& dbContext) {
    (void)node;
    (void)dbContext;
    QueryResult result;
    result.error = "CREATE INDEX se implementa en fase 3";
    result.success = false;
    return result;
}

void QueryProcessor::loadIndexes() {
    // se implementa en fase 3 cuando existan índices que recargar
}