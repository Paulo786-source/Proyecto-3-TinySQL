#include "queryvalidator.h"
#include <algorithm>
#include <cctype>

// conjuntos estaticos

const std::set<std::string> QueryValidator::VALID_TYPES = {
    "INTEGER", "DOUBLE", "VARCHAR", "DATETIME"
};

const std::set<std::string> QueryValidator::CATALOG_TABLES = {
    "SystemDatabases", "SystemTables", "SystemColumns", "SystemIndexes"
};

// CONSTRUCTOR

QueryValidator::QueryValidator(StoredDataManager& sdm, const IndexManager& indexMgr)
    : sdm_(sdm), indexMgr_(indexMgr) {
}

// HELPERS PRIVADOS

static std::string toLowerV(const std::string& s) {
    std::string r = s;
    for (char& c : r)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

// verifica si una columna existe en el schema
bool QueryValidator::columnExists(const std::string& colName,
    const std::vector<ColumnDefinition>& schema) const
{
    for (const auto& col : schema)
        if (toLowerV(col.name) == toLowerV(colName)) return true;
    return false;
}

// busca una columna en el schema, lanza excepcion si no existe
ColumnDefinition QueryValidator::findColumn(
    const std::string& colName,
    const std::vector<ColumnDefinition>& schema) const
{
    for (const auto& col : schema)
        if (toLowerV(col.name) == toLowerV(colName)) return col;
    throw std::runtime_error("columna no encontrada: " + colName);
}

// validaciones de contexto

std::string QueryValidator::requireDbContext(const std::string& db) const {
    if (db.empty())
        return "no hay base de datos activa. Use SET DATABASE primero";
    return "";
}

std::string QueryValidator::requireDbExists(const std::string& db) const {
    if (!sdm_.databaseExists(db))
        return "la base de datos '" + db + "' no existe";
    return "";
}

std::string QueryValidator::requireTableExists(const std::string& db,
    const std::string& table) const
{
    if (!sdm_.tableExists(db, table))
        return "la tabla '" + table + "' no existe en '" + db + "'";
    return "";
}

// VALIDATE: punto de entrada

std::string QueryValidator::validate(const ASTNode& node,
    const std::string& db) const
{
    if (node.type == "CREATE_DB")    return validateCreateDb(node);
    if (node.type == "SET_DB")       return validateSetDb(node);
    if (node.type == "CREATE_TABLE") return validateCreateTable(node, db);
    if (node.type == "DROP_TABLE")   return validateDropTable(node, db);
    if (node.type == "INSERT")       return validateInsert(node, db);
    if (node.type == "SELECT")       return validateSelect(node, db);
    if (node.type == "UPDATE")       return validateUpdate(node, db);
    if (node.type == "DELETE")       return validateDelete(node, db);
    if (node.type == "CREATE_INDEX") return validateCreateIndex(node, db);
    return "";
}

// VALIDACIONES POR TIPO

std::string QueryValidator::validateCreateDb(const ASTNode& node) const {
    if (node.database.empty())
        return "el nombre de la base de datos no puede estar vacio";
    if (sdm_.databaseExists(node.database))
        return "la base de datos '" + node.database + "' ya existe";
    return "";
}

std::string QueryValidator::validateSetDb(const ASTNode& node) const {
    if (node.database.empty())
        return "el nombre de la base de datos no puede estar vacio";
    if (!sdm_.databaseExists(node.database))
        return "la base de datos '" + node.database + "' no existe";
    return "";
}

std::string QueryValidator::validateCreateTable(const ASTNode& node,
    const std::string& db) const
{
    if (auto e = requireDbContext(db);  !e.empty()) return e;
    if (auto e = requireDbExists(db);   !e.empty()) return e;
    if (node.table.empty())
        return "el nombre de la tabla no puede estar vacio";
    if (sdm_.tableExists(db, node.table))
        return "la tabla '" + node.table + "' ya existe en '" + db + "'";
    for (const auto& col : node.columns) {
        if (VALID_TYPES.find(col.type) == VALID_TYPES.end())
            return "tipo de columna invalido: '" + col.type + "'";
        if (col.type == "VARCHAR" && col.size <= 0)
            return "VARCHAR requiere tamanno positivo para '" + col.name + "'";
    }
    return "";
}

std::string QueryValidator::validateDropTable(const ASTNode& node,
    const std::string& db) const
{
    if (auto e = requireDbContext(db); !e.empty()) return e;
    if (auto e = requireDbExists(db);  !e.empty()) return e;
    return requireTableExists(db, node.table);
}

std::string QueryValidator::validateInsert(const ASTNode& node,
    const std::string& db) const
{
    if (auto e = requireDbContext(db); !e.empty()) return e;
    if (auto e = requireDbExists(db);  !e.empty()) return e;
    if (auto e = requireTableExists(db, node.table); !e.empty()) return e;

    auto schema = sdm_.getTableSchema(db, node.table);
    if (node.values.size() != schema.size())
        return "INSERT: se esperaban " + std::to_string(schema.size()) +
        " valores, se recibieron " + std::to_string(node.values.size());

    for (size_t i = 0; i < schema.size(); ++i) {
        auto err = TypeSystem::validateValue(node.values[i], schema[i]);
        if (!err.empty()) return err;
    }
    return "";
}

std::string QueryValidator::validateSelect(const ASTNode& node,
    const std::string& db) const
{
    // SELECT sobre tablas del catalogo no requiere BD activa
    if (CATALOG_TABLES.count(node.table)) return "";

    if (auto e = requireDbContext(db); !e.empty()) return e;
    if (auto e = requireDbExists(db);  !e.empty()) return e;
    if (auto e = requireTableExists(db, node.table); !e.empty()) return e;

    auto schema = sdm_.getTableSchema(db, node.table);

    for (const auto& col : node.selectColumns) {
        if (col == "*") continue;
        if (!columnExists(col, schema))
            return "columna '" + col + "' no existe en '" + node.table + "'";
    }
    if (node.hasWhere && !columnExists(node.where.column, schema))
        return "columna WHERE '" + node.where.column + "' no existe en '" + node.table + "'";
    if (node.hasOrderBy && !columnExists(node.orderByColumn, schema))
        return "columna ORDER BY '" + node.orderByColumn + "' no existe en '" + node.table + "'";
    return "";
}

std::string QueryValidator::validateUpdate(const ASTNode& node,
    const std::string& db) const
{
    if (auto e = requireDbContext(db); !e.empty()) return e;
    if (auto e = requireDbExists(db);  !e.empty()) return e;
    if (auto e = requireTableExists(db, node.table); !e.empty()) return e;

    auto schema = sdm_.getTableSchema(db, node.table);

    for (const auto& sc : node.setClauses) {
        if (!columnExists(sc.column, schema))
            return "columna '" + sc.column + "' no existe en '" + node.table + "'";
        auto col = findColumn(sc.column, schema);
        auto err = TypeSystem::validateValue(sc.value, col);
        if (!err.empty()) return err;
    }
    if (node.hasWhere && !columnExists(node.where.column, schema))
        return "columna WHERE '" + node.where.column + "' no existe en '" + node.table + "'";
    return "";
}

std::string QueryValidator::validateDelete(const ASTNode& node,
    const std::string& db) const
{
    if (auto e = requireDbContext(db); !e.empty()) return e;
    if (auto e = requireDbExists(db);  !e.empty()) return e;
    if (auto e = requireTableExists(db, node.table); !e.empty()) return e;

    if (node.hasWhere) {
        auto schema = sdm_.getTableSchema(db, node.table);
        if (!columnExists(node.where.column, schema))
            return "columna WHERE '" + node.where.column + "' no existe en '" + node.table + "'";
    }
    return "";
}

std::string QueryValidator::validateCreateIndex(const ASTNode& node,
    const std::string& db) const
{
    if (auto e = requireDbContext(db); !e.empty()) return e;
    if (auto e = requireDbExists(db);  !e.empty()) return e;
    if (auto e = requireTableExists(db, node.table); !e.empty()) return e;

    // solo un indice por tabla
    if (indexMgr_.hasAnyIndex(db, node.table))
        return "la tabla '" + node.table +
        "' ya tiene un indice. Solo se permite uno por tabla";

    auto schema = sdm_.getTableSchema(db, node.table);
    if (!columnExists(node.indexColumn, schema))
        return "columna '" + node.indexColumn + "' no existe en '" + node.table + "'";

    // verifica que no haya valores duplicados en la columna indexada
    // antes de construir el arbol. el enunciado lo exige explicitamente.
    int colPos = -1;
    for (int i = 0; i < static_cast<int>(schema.size()); ++i) {
        if (toLowerV(schema[i].name) == toLowerV(node.indexColumn)) {
            colPos = i; break;
        }
    }

    if (colPos >= 0) {
        std::set<std::string> seen;
        for (auto& [off, row] : sdm_.readAllRecordsWithOffsets(db, node.table)) {
            if (colPos < static_cast<int>(row.size())) {
                const std::string& val = row[colPos];
                if (seen.count(val)) {
                    return "no se puede crear el indice: la columna '" +
                        node.indexColumn + "' tiene valores duplicados ('" +
                        val + "' aparece mas de una vez)";
                }
                seen.insert(val);
            }
        }
    }

    return "";
}