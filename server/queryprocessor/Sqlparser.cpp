#include "sqlparser.h"
#include <algorithm>

// tipos de columna validos
const std::set<std::string> SQLParser::VALID_TYPES = {
    "INTEGER", "DOUBLE", "VARCHAR", "DATETIME"
};

// helpers de string

std::string SQLParser::toUpper(const std::string& s) {
    std::string r = s;
    for (char& c : r)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return r;
}

std::string SQLParser::stripQuotes(const std::string& t) {
    if (t.size() >= 2) {
        char f = t.front(), b = t.back();
        if ((f == '\'' && b == '\'') || (f == '"' && b == '"'))
            return t.substr(1, t.size() - 2);
    }
    return t;
}

// tokenizador
// respeta strings entre comillas, separa operadores y parentesis
std::vector<std::string> SQLParser::tokenize(const std::string& sql) {
    std::vector<std::string> tokens;
    std::string current;
    bool inSingle = false, inDouble = false;

    for (size_t i = 0; i < sql.size(); ++i) {
        char c = sql[i];

        if (c == '\'' && !inDouble) {
            inSingle = !inSingle;
            current += c;
        }
        else if (c == '"' && !inSingle) {
            inDouble = !inDouble;
            current += c;
        }
        else if ((c == ' ' || c == '\t' || c == '\n' || c == '\r')
            && !inSingle && !inDouble)
        {
            if (!current.empty()) { tokens.push_back(current); current.clear(); }
        }
        else if ((c == '(' || c == ')' || c == ',' || c == ';')
            && !inSingle && !inDouble)
        {
            if (!current.empty()) { tokens.push_back(current); current.clear(); }
            tokens.push_back(std::string(1, c));
        }
        else if ((c == '=' || c == '<' || c == '>')
            && !inSingle && !inDouble)
        {
            if (!current.empty()) { tokens.push_back(current); current.clear(); }
            std::string op(1, c);
            if (i + 1 < sql.size() && sql[i + 1] == '=') { op += '='; ++i; }
            tokens.push_back(op);
        }
        else {
            current += c;
        }
    }
    if (!current.empty()) tokens.push_back(current);
    return tokens;
}

// parse: punto de entrada
// reconoce la primera keyword y delega al parser correspondiente
ASTNode SQLParser::parse(const std::string& sql) {
    auto tok = tokenize(sql);
    if (tok.empty()) throw std::runtime_error("sentencia SQL vacia");

    std::string kw = toUpper(tok[0]);

    if (kw == "CREATE") return parseCreate(tok);
    if (kw == "SET") {
        if (tok.size() < 3 || toUpper(tok[1]) != "DATABASE")
            throw std::runtime_error("sintaxis: SET DATABASE <nombre>");
        ASTNode n; n.type = "SET_DB"; n.database = tok[2]; return n;
    }
    if (kw == "DROP") {
        if (tok.size() < 3 || toUpper(tok[1]) != "TABLE")
            throw std::runtime_error("sintaxis: DROP TABLE <nombre>");
        ASTNode n; n.type = "DROP_TABLE"; n.table = tok[2]; return n;
    }
    if (kw == "INSERT") return parseInsert(tok);
    if (kw == "SELECT") return parseSelect(tok);
    if (kw == "UPDATE") return parseUpdate(tok);
    if (kw == "DELETE") return parseDelete(tok);

    throw std::runtime_error("sentencia no reconocida: " + tok[0]);
}

// CREATE DATABASE, TABLE o INDEX
ASTNode SQLParser::parseCreate(const std::vector<std::string>& tok) {
    if (tok.size() < 3)
        throw std::runtime_error("CREATE requiere tipo y nombre");
    std::string obj = toUpper(tok[1]);

    if (obj == "DATABASE") {
        ASTNode n; n.type = "CREATE_DB"; n.database = tok[2]; return n;
    }
    if (obj == "TABLE")  return parseCreateTable(tok);
    if (obj == "INDEX")  return parseCreateIndex(tok);

    throw std::runtime_error("tipo de CREATE no soportado: " + tok[1]);
}

// CREATE TABLE <nombre> (col type, ...)
ASTNode SQLParser::parseCreateTable(const std::vector<std::string>& tok) {
    if (tok.size() < 4)
        throw std::runtime_error("CREATE TABLE requiere nombre de tabla");

    ASTNode n; n.type = "CREATE_TABLE"; n.table = tok[2];
    size_t i = 3;

    if (toUpper(tok[i]) == "AS") ++i;

    if (i >= tok.size() || tok[i] != "(")
        throw std::runtime_error("CREATE TABLE: se esperaba '('");
    ++i;

    // parsea cada columna hasta encontrar ")"
    while (i < tok.size() && tok[i] != ")") {
        if (tok[i] == ",") { ++i; continue; }

        ColumnDefinition col; col.nullable = true; col.size = 0;
        col.name = tok[i++];

        if (i >= tok.size())
            throw std::runtime_error("definicion incompleta para columna: " + col.name);

        col.type = toUpper(tok[i++]);
        if (VALID_TYPES.find(col.type) == VALID_TYPES.end())
            throw std::runtime_error("tipo de columna invalido: '" + col.type + "'");

        // VARCHAR necesita tamano: VARCHAR(n)
        if (col.type == "VARCHAR") {
            if (i >= tok.size() || tok[i] != "(")
                throw std::runtime_error("VARCHAR requiere tamanno: VARCHAR(n)");
            ++i;
            try { col.size = std::stoi(tok[i++]); }
            catch (...) { throw std::runtime_error("VARCHAR requiere tamanno numerico"); }
            if (col.size <= 0)
                throw std::runtime_error("el tamanno de VARCHAR debe ser positivo");
            if (i >= tok.size() || tok[i] != ")")
                throw std::runtime_error("se esperaba ')' despues del tamanno de VARCHAR");
            ++i;
        }
        n.columns.push_back(col);
    }

    if (n.columns.empty())
        throw std::runtime_error("CREATE TABLE: debe tener al menos una columna");
    return n;
}

// CREATE INDEX <nombre> ON <tabla>(<col>) OF TYPE <BTREE|BST>
ASTNode SQLParser::parseCreateIndex(const std::vector<std::string>& tok) {
    if (tok.size() < 11)
        throw std::runtime_error(
            "sintaxis: CREATE INDEX <nombre> ON <tabla>(<columna>) OF TYPE <BTREE|BST>");

    ASTNode n; n.type = "CREATE_INDEX";
    n.indexName = tok[2];

    if (toUpper(tok[3]) != "ON")
        throw std::runtime_error("CREATE INDEX: se esperaba ON");
    n.table = tok[4];

    if (tok[5] != "(")
        throw std::runtime_error("CREATE INDEX: se esperaba '('");
    n.indexColumn = tok[6];
    if (tok[7] != ")")
        throw std::runtime_error("CREATE INDEX: se esperaba ')'");
    if (toUpper(tok[8]) != "OF" || toUpper(tok[9]) != "TYPE")
        throw std::runtime_error("CREATE INDEX: se esperaba OF TYPE");

    n.indexType = toUpper(tok[10]);
    if (n.indexType != "BTREE" && n.indexType != "BST")
        throw std::runtime_error(
            "tipo de indice invalido: '" + n.indexType + "'. Use BTREE o BST");
    return n;
}

// INSERT INTO <tabla> VALUES(v1, v2, ...)
ASTNode SQLParser::parseInsert(const std::vector<std::string>& tok) {
    if (tok.size() < 6 || toUpper(tok[1]) != "INTO")
        throw std::runtime_error("sintaxis: INSERT INTO <tabla> VALUES(<valores>)");

    ASTNode n; n.type = "INSERT"; n.table = tok[2];

    if (toUpper(tok[3]) != "VALUES" || tok[4] != "(")
        throw std::runtime_error("INSERT: se esperaba VALUES(");

    size_t i = 5;
    while (i < tok.size() && tok[i] != ")") {
        if (tok[i] != ",") n.values.push_back(stripQuotes(tok[i]));
        ++i;
    }
    return n;
}

// SELECT * | cols FROM <tabla> [WHERE ...] [ORDER BY ...]
ASTNode SQLParser::parseSelect(const std::vector<std::string>& tok) {
    ASTNode n; n.type = "SELECT";
    size_t i = 1;

    // parsea las columnas hasta FROM
    while (i < tok.size() && toUpper(tok[i]) != "FROM") {
        if (tok[i] != ",") n.selectColumns.push_back(tok[i]);
        ++i;
    }
    if (i >= tok.size())
        throw std::runtime_error("SELECT: falta clausula FROM");
    ++i;

    if (i >= tok.size())
        throw std::runtime_error("SELECT: falta nombre de tabla");
    n.table = tok[i++];

    // WHERE opcional
    if (i < tok.size() && toUpper(tok[i]) == "WHERE") {
        ++i;
        n.where = parseWhere(tok, i);
        n.hasWhere = true;
    }

    // ORDER BY opcional
    if (i + 1 < tok.size()
        && toUpper(tok[i]) == "ORDER"
        && toUpper(tok[i + 1]) == "BY")
    {
        i += 2;
        if (i >= tok.size())
            throw std::runtime_error("SELECT: falta columna despues de ORDER BY");
        n.orderByColumn = tok[i++];
        n.orderByDirection = "ASC";
        if (i < tok.size()) {
            std::string dir = toUpper(tok[i]);
            if (dir == "ASC" || dir == "DESC") { n.orderByDirection = dir; ++i; }
        }
        n.hasOrderBy = true;
    }
    return n;
}

// UPDATE <tabla> SET col = val, ... [WHERE ...]
ASTNode SQLParser::parseUpdate(const std::vector<std::string>& tok) {
    if (tok.size() < 5)
        throw std::runtime_error("sintaxis: UPDATE <tabla> SET <col> = <val> [WHERE ...]");

    ASTNode n; n.type = "UPDATE"; n.table = tok[1];
    size_t i = 2;

    if (toUpper(tok[i]) != "SET")
        throw std::runtime_error("UPDATE: se esperaba SET");
    ++i;

    // parsea los pares col = val hasta WHERE o fin
    while (i < tok.size() && toUpper(tok[i]) != "WHERE") {
        if (tok[i] == ",") { ++i; continue; }
        SetClause sc;
        sc.column = tok[i++];
        if (i >= tok.size() || tok[i] != "=")
            throw std::runtime_error("UPDATE SET: se esperaba '=' despues de '" + sc.column + "'");
        ++i;
        if (i >= tok.size())
            throw std::runtime_error("UPDATE SET: falta valor para '" + sc.column + "'");
        sc.value = stripQuotes(tok[i++]);
        n.setClauses.push_back(sc);
    }
    if (n.setClauses.empty())
        throw std::runtime_error("UPDATE: debe haber al menos una asignacion SET");

    // WHERE opcional
    if (i < tok.size() && toUpper(tok[i]) == "WHERE") {
        ++i;
        n.where = parseWhere(tok, i);
        n.hasWhere = true;
    }
    return n;
}

// DELETE FROM <tabla> [WHERE ...]
ASTNode SQLParser::parseDelete(const std::vector<std::string>& tok) {
    if (tok.size() < 3 || toUpper(tok[1]) != "FROM")
        throw std::runtime_error("sintaxis: DELETE FROM <tabla> [WHERE ...]");

    ASTNode n; n.type = "DELETE"; n.table = tok[2];
    size_t i = 3;

    // WHERE opcional
    if (i < tok.size() && toUpper(tok[i]) == "WHERE") {
        ++i;
        n.where = parseWhere(tok, i);
        n.hasWhere = true;
    }
    return n;
}

// parsea una clausula WHERE: columna operador valor
WhereClause SQLParser::parseWhere(const std::vector<std::string>& tok, size_t& i) {
    if (i + 2 >= tok.size())
        throw std::runtime_error("clausula WHERE incompleta");
    WhereClause w;
    w.column = tok[i++];
    w.op = tok[i++];
    w.value = stripQuotes(tok[i++]);
    return w;
}