#pragma once

#include "queryprocessor.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <set>
#include <cctype>

// convierte SQL en texto a un ASTNode
class SQLParser {
public:

    // convierte 'sql' a ASTNode, lanza excepcion si hay error
    ASTNode parse(const std::string& sql);

private:

    // divide el SQL en tokens, respeta comillas
    std::vector<std::string> tokenize(const std::string& sql);

    // quita comillas de un literal
    std::string stripQuotes(const std::string& token);

    std::string toUpper(const std::string& s);

    // parsers por tipo de sentencia
    ASTNode parseCreate(const std::vector<std::string>& tok);
    ASTNode parseCreateTable(const std::vector<std::string>& tok);
    ASTNode parseCreateIndex(const std::vector<std::string>& tok);
    ASTNode parseInsert(const std::vector<std::string>& tok);
    ASTNode parseSelect(const std::vector<std::string>& tok);
    ASTNode parseUpdate(const std::vector<std::string>& tok);
    ASTNode parseDelete(const std::vector<std::string>& tok);

    // parsea una clausula WHERE
    WhereClause parseWhere(const std::vector<std::string>& tok, size_t& i);

    static const std::set<std::string> VALID_TYPES;
};