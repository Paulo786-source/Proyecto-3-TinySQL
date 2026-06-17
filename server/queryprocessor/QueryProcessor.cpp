#include "queryprocessor.h"
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <functional>
#include <unordered_map>
#include <set>

// helpers de string

// convierte a mayusculas para comparar keywords sql
static std::string toUpper(const std::string& s) {
    std::string result = s;
    for (char& c : result) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return result;
}

// divide el sql en tokens, respetando strings entre comillas
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
        else if ((c == ' ' || c == '\t' || c == '\n' || c == '\r')
            && !inSingleQuote && !inDoubleQuote) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        }
        else if ((c == '(' || c == ')' || c == ',' || c == ';')
            && !inSingleQuote && !inDoubleQuote) {
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

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

// quita comillas simples o dobles que rodean un valor literal
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

// tipos de columna validos
static const std::set<std::string> VALID_TYPES = {
    "INTEGER", "DOUBLE", "VARCHAR", "DATETIME"
};

// CONSTRUCTOR

QueryProcessor::QueryProcessor(StoredDataManager& sdm) : sdm_(sdm) {}

// PARSE - convierte sql en astnode

ASTNode QueryProcessor::parse(const std::string& sql) {
    std::vector<std::string> tokens = tokenize(sql);

    if (tokens.empty()) {
        throw std::runtime_error("sentencia sql vacia");
    }

    std::string keyword = toUpper(tokens[0]);

    // CREATE

    if (keyword == "CREATE") {
        if (tokens.size() < 3) {
            throw std::runtime_error("sintaxis invalida: create requiere tipo y nombre");
        }
        std::string objectType = toUpper(tokens[1]);

        // CREATE DATABASE
        if (objectType == "DATABASE") {
            ASTNode node;
            node.type = "CREATE_DB";
            node.database = tokens[2];
            node.hasWhere = false;
            node.hasOrderBy = false;
            return node;
        }

        // CREATE TABLE
        if (objectType == "TABLE") {
            if (tokens.size() < 4) {
                throw std::runtime_error("create table requiere nombre de tabla");
            }

            ASTNode node;
            node.type = "CREATE_TABLE";
            node.table = tokens[2];
            node.hasWhere = false;
            node.hasOrderBy = false;

            // busca el token '(' de apertura
            size_t i = 3;
            if (toUpper(tokens[i]) == "AS") {
                ++i;
            }
            if (i >= tokens.size() || tokens[i] != "(") {
                throw std::runtime_error("create table: se esperaba '(' despues del nombre");
            }
            ++i;

            // parsea la lista de columnas
            while (i < tokens.size() && tokens[i] != ")") {
                if (tokens[i] == ",") {
                    ++i;
                    continue;
                }

                ColumnDefinition col;
                col.nullable = true;
                col.size = 0;

                col.name = tokens[i++];

                if (i >= tokens.size()) {
                    throw std::runtime_error("definicion de columna incompleta para: " + col.name);
                }

                col.type = toUpper(tokens[i++]);

                if (VALID_TYPES.find(col.type) == VALID_TYPES.end()) {
                    throw std::runtime_error("tipo de columna invalido: '" + col.type +
                        "'. tipos permitidos: INTEGER, DOUBLE, VARCHAR(n), DATETIME");
                }

                // varchar necesita tamano: varchar(n)
                if (col.type == "VARCHAR") {
                    if (i >= tokens.size() || tokens[i] != "(") {
                        throw std::runtime_error("varchar requiere tamanno: VARCHAR(n)");
                    }
                    ++i;

                    if (i >= tokens.size()) {
                        throw std::runtime_error("varchar requiere un tamanno numerico");
                    }

                    try {
                        col.size = std::stoi(tokens[i++]);
                    }
                    catch (...) {
                        throw std::runtime_error("varchar requiere un tamanno numerico valido");
                    }

                    if (col.size <= 0) {
                        throw std::runtime_error("el tamanno de varchar debe ser positivo");
                    }

                    if (i >= tokens.size() || tokens[i] != ")") {
                        throw std::runtime_error("se esperaba ')' despues del tamanno de varchar");
                    }
                    ++i;
                }

                node.columns.push_back(col);
            }

            if (node.columns.empty()) {
                throw std::runtime_error("create table: la tabla debe tener al menos una columna");
            }

            return node;
        }

        // CREATE INDEX - fase 3
        if (objectType == "INDEX") {
            throw std::runtime_error("create index se implementa en fase 3");
        }

        throw std::runtime_error("tipo de create no soportado: " + tokens[1]);
    }

    // SET DATABASE

    if (keyword == "SET") {
        if (tokens.size() < 3) {
            throw std::runtime_error("sintaxis invalida: set database requiere un nombre");
        }
        if (toUpper(tokens[1]) != "DATABASE") {
            throw std::runtime_error("sintaxis set invalida: se esperaba DATABASE");
        }
        ASTNode node;
        node.type = "SET_DB";
        node.database = tokens[2];
        node.hasWhere = false;
        node.hasOrderBy = false;
        return node;
    }

    // DROP TABLE

    if (keyword == "DROP") {
        if (tokens.size() < 3) {
            throw std::runtime_error("sintaxis invalida: drop table requiere nombre de tabla");
        }
        if (toUpper(tokens[1]) != "TABLE") {
            throw std::runtime_error("drop: solo se soporta drop table");
        }
        ASTNode node;
        node.type = "DROP_TABLE";
        node.table = tokens[2];
        node.hasWhere = false;
        node.hasOrderBy = false;
        return node;
    }

    // DML - fase 3

    if (keyword == "INSERT" || keyword == "SELECT" ||
        keyword == "UPDATE" || keyword == "DELETE") {
        throw std::runtime_error(keyword + " se implementa en fase 3");
    }

    throw std::runtime_error("sentencia no reconocida: " + tokens[0]);
}

// VALIDATE - validacion semantica

std::string QueryProcessor::validate(const ASTNode& node,
    const std::string& dbContext)
{
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

    if (node.type == "CREATE_TABLE") {
        if (dbContext.empty()) {
            return "no hay base de datos activa. use set database primero";
        }
        if (!sdm_.databaseExists(dbContext)) {
            return "la base de datos activa '" + dbContext + "' no existe";
        }
        if (node.table.empty()) {
            return "el nombre de la tabla no puede estar vacio";
        }
        if (sdm_.tableExists(dbContext, node.table)) {
            return "la tabla '" + node.table + "' ya existe en '" + dbContext + "'";
        }
        for (const auto& col : node.columns) {
            if (VALID_TYPES.find(col.type) == VALID_TYPES.end()) {
                return "tipo de columna invalido: '" + col.type + "'";
            }
            if (col.type == "VARCHAR" && col.size <= 0) {
                return "varchar requiere un tamanno positivo para la columna '" + col.name + "'";
            }
        }
        return "";
    }

    if (node.type == "DROP_TABLE") {
        if (dbContext.empty()) {
            return "no hay base de datos activa. use set database primero";
        }
        if (!sdm_.databaseExists(dbContext)) {
            return "la base de datos activa '" + dbContext + "' no existe";
        }
        if (node.table.empty()) {
            return "el nombre de la tabla no puede estar vacio";
        }
        if (!sdm_.tableExists(dbContext, node.table)) {
            return "la tabla '" + node.table + "' no existe en '" + dbContext + "'";
        }
        return "";
    }

    return "";
}

// EXECUTE - pipeline principal

QueryResult QueryProcessor::execute(const std::string& sql,
    const std::string& dbContext)
{
    QueryResult result;
    result.time_ms = 0;
    result.success = false;

    // parse
    ASTNode node;
    try {
        node = parse(sql);
    }
    catch (const std::exception& e) {
        result.error = std::string("error de sintaxis: ") + e.what();
        return result;
    }

    // validate
    std::string validationError = validate(node, dbContext);
    if (!validationError.empty()) {
        result.error = validationError;
        return result;
    }

    // dispatch usando command pattern
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
        result.error = "tipo de sentencia no implementado: " + node.type;
    }

    return result;
}

// EJECUTORES - DDL

QueryResult QueryProcessor::executeCreateDatabase(const ASTNode& node) {
    QueryResult result;
    result.time_ms = 0;

    if (!sdm_.createDatabase(node.database)) {
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
    // la validacion ya confirmo que la bd existe
    result.success = true;
    return result;
}

// crea la tabla llamando al sdm
QueryResult QueryProcessor::executeCreateTable(const ASTNode& node,
    const std::string& dbContext)
{
    QueryResult result;
    result.time_ms = 0;

    if (!sdm_.createTableFile(dbContext, node.table, node.columns)) {
        result.error = "no se pudo crear la tabla '" + node.table + "'";
        result.success = false;
        return result;
    }

    result.success = true;
    return result;
}

// elimina la tabla solo si esta vacia
QueryResult QueryProcessor::executeDropTable(const ASTNode& node,
    const std::string& dbContext)
{
    QueryResult result;
    result.time_ms = 0;

    if (!sdm_.isTableEmpty(dbContext, node.table)) {
        result.error = "no se puede eliminar la tabla '" + node.table +
            "': contiene registros. elimine los datos primero";
        result.success = false;
        return result;
    }

    if (!sdm_.dropTable(dbContext, node.table)) {
        result.error = "no se pudo eliminar la tabla '" + node.table + "'";
        result.success = false;
        return result;
    }

    result.success = true;
    return result;
}

// EJECUTORES - DML (STUBS PARA FASE 3)

QueryResult QueryProcessor::executeInsert(const ASTNode& node,
    const std::string& dbContext)
{
    (void)node; (void)dbContext;
    QueryResult result;
    result.error = "insert se implementa en fase 3";
    result.success = false;
    return result;
}

QueryResult QueryProcessor::executeSelect(const ASTNode& node,
    const std::string& dbContext)
{
    (void)node; (void)dbContext;
    QueryResult result;
    result.error = "select se implementa en fase 3";
    result.success = false;
    return result;
}

QueryResult QueryProcessor::executeUpdate(const ASTNode& node,
    const std::string& dbContext)
{
    (void)node; (void)dbContext;
    QueryResult result;
    result.error = "update se implementa en fase 3";
    result.success = false;
    return result;
}

QueryResult QueryProcessor::executeDelete(const ASTNode& node,
    const std::string& dbContext)
{
    (void)node; (void)dbContext;
    QueryResult result;
    result.error = "delete se implementa en fase 3";
    result.success = false;
    return result;
}

QueryResult QueryProcessor::executeCreateIndex(const ASTNode& node,
    const std::string& dbContext)
{
    (void)node; (void)dbContext;
    QueryResult result;
    result.error = "create index se implementa en fase 3";
    result.success = false;
    return result;
}

void QueryProcessor::loadIndexes() {
    // fase 3
}