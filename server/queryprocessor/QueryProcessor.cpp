#include "queryprocessor.h"
#include "sqlparser.h"
#include "queryvalidator.h"
#include "typesystem.h"
#include "rowutils.h"
#include <functional>
#include <unordered_map>
#include <set>

// helper local

static std::string toLowerQP(const std::string& s) {
    std::string r = s;
    for (char& c : r)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

// =============================================================================
// CONSTRUCTOR
// =============================================================================

QueryProcessor::QueryProcessor(StoredDataManager& sdm) : sdm_(sdm) {}

// =============================================================================
// EXECUTE - PIPELINE PRINCIPAL
// =============================================================================

QueryResult QueryProcessor::execute(const std::string& sql,
    const std::string& db)
{
    QueryResult result;

    // paso 1: parseo
    ASTNode node;
    try {
        SQLParser parser;
        node = parser.parse(sql);
    }
    catch (const std::exception& e) {
        result.error = std::string("error de sintaxis: ") + e.what();
        return result;
    }

    // paso 2: validacion semantica
    QueryValidator validator(sdm_, indexMgr_);
    std::string valErr = validator.validate(node, db);
    if (!valErr.empty()) {
        result.error = valErr;
        return result;
    }

    // paso 3: despacho (command pattern)
    using Handler = std::function<QueryResult()>;
    std::unordered_map<std::string, Handler> dispatch = {
        { "CREATE_DB",    [&] { return executeCreateDatabase(node); } },
        { "SET_DB",       [&] { return executeSetDatabase(node); } },
        { "CREATE_TABLE", [&] { return executeCreateTable(node, db); } },
        { "DROP_TABLE",   [&] { return executeDropTable(node, db); } },
        { "INSERT",       [&] { return executeInsert(node, db); } },
        { "SELECT",       [&] { return executeSelect(node, db); } },
        { "UPDATE",       [&] { return executeUpdate(node, db); } },
        { "DELETE",       [&] { return executeDelete(node, db); } },
        { "CREATE_INDEX", [&] { return executeCreateIndex(node, db); } },
    };

    auto it = dispatch.find(node.type);
    if (it != dispatch.end()) return it->second();

    result.error = "tipo de sentencia no implementado: " + node.type;
    return result;
}

// metodos publicos que delegan al parser/validator (para tests)

ASTNode QueryProcessor::parse(const std::string& sql) {
    SQLParser p; return p.parse(sql);
}

std::string QueryProcessor::validate(const ASTNode& node, const std::string& db) {
    QueryValidator v(sdm_, indexMgr_); return v.validate(node, db);
}

// =============================================================================
// EJECUTORES DDL
// =============================================================================

QueryResult QueryProcessor::executeCreateDatabase(const ASTNode& node) {
    QueryResult r;
    if (!sdm_.createDatabase(node.database)) {
        r.error = "no se pudo crear la base de datos '" + node.database + "'";
        return r;
    }
    r.success = true;
    return r;
}

QueryResult QueryProcessor::executeSetDatabase(const ASTNode&) {
    QueryResult r; r.success = true; return r;
}

QueryResult QueryProcessor::executeCreateTable(const ASTNode& node,
    const std::string& db)
{
    QueryResult r;
    if (!sdm_.createTableFile(db, node.table, node.columns)) {
        r.error = "no se pudo crear la tabla '" + node.table + "'";
        return r;
    }
    r.success = true;
    return r;
}

QueryResult QueryProcessor::executeDropTable(const ASTNode& node,
    const std::string& db)
{
    QueryResult r;
    if (!sdm_.isTableEmpty(db, node.table)) {
        r.error = "no se puede eliminar '" + node.table +
            "': contiene registros. elimine los datos primero";
        return r;
    }
    // limpia el indice en memoria si existia
    std::string idxCol = indexMgr_.getIndexedColumn(db, node.table);
    if (!idxCol.empty()) indexMgr_.remove(db, node.table, idxCol);

    if (!sdm_.dropTable(db, node.table)) {
        r.error = "no se pudo eliminar la tabla '" + node.table + "'";
        return r;
    }
    r.success = true;
    return r;
}

// =============================================================================
// EXECUTE INSERT
// =============================================================================

QueryResult QueryProcessor::executeInsert(const ASTNode& node,
    const std::string& db)
{
    QueryResult r;
    auto schema = sdm_.getTableSchema(db, node.table);

    // normaliza datetime a timestamp antes de escribir en disco
    Row normalized;
    normalized.reserve(schema.size());
    for (size_t i = 0; i < schema.size(); ++i)
        normalized.push_back(TypeSystem::normalizeValue(node.values[i], schema[i]));

    // verifica duplicado en indice antes de escribir (no deja registros huerfanos)
    std::string idxCol = indexMgr_.getIndexedColumn(db, node.table);
    IndexHandle* idx = nullptr;
    int idxColPos = -1;
    if (!idxCol.empty()) {
        idx = indexMgr_.get(db, node.table, idxCol);
        for (int i = 0; i < static_cast<int>(schema.size()); ++i) {
            if (toLowerQP(schema[i].name) == toLowerQP(idxCol)) {
                idxColPos = i; break;
            }
        }
        if (idx && idxColPos >= 0 && idx->search(normalized[idxColPos]) >= 0) {
            r.error = "valor duplicado en columna indexada '" + idxCol +
                "': " + node.values[idxColPos] + " ya existe";
            return r;
        }
    }

    // escribe en disco
    long long offset = sdm_.appendRecord(db, node.table, normalized);
    if (offset < 0) {
        r.error = "no se pudo insertar el registro en '" + node.table + "'";
        return r;
    }

    // actualiza el indice con el offset del nuevo registro
    if (idx && idxColPos >= 0) {
        try { idx->insert(normalized[idxColPos], offset); }
        catch (const std::exception& e) { r.error = e.what(); return r; }
    }

    r.success = true;
    return r;
}

// =============================================================================
// EXECUTE SELECT
// =============================================================================

QueryResult QueryProcessor::executeSelect(const ASTNode& node,
    const std::string& db)
{
    QueryResult r;

    // select sobre tablas del system catalog
    static const std::unordered_map<std::string, std::vector<std::string>> CAT_COLS = {
        { "SystemDatabases", { "name" } },
        { "SystemTables",    { "database", "table" } },
        { "SystemColumns",   { "database", "table", "column", "type", "size", "nullable" } },
        { "SystemIndexes",   { "database", "table", "column", "indexName", "indexType" } },
    };
    auto catIt = CAT_COLS.find(node.table);
    if (catIt != CAT_COLS.end()) {
        r.columns = catIt->second;
        r.rows = sdm_.readCatalogTable(node.table);
        r.success = true;
        return r;
    }

    auto schema = sdm_.getTableSchema(db, node.table);
    ResultSet filtered;

    // estrategia de busqueda: indice vs secuencial
    if (node.hasWhere) {
        std::string idxCol = indexMgr_.getIndexedColumn(db, node.table);
        IndexHandle* idx = nullptr;

        // si la columna del WHERE esta indexada y el operador es "=", usa indice
        if (!idxCol.empty()
            && toLowerQP(idxCol) == toLowerQP(node.where.column)
            && node.where.op == "=")
        {
            idx = indexMgr_.get(db, node.table, idxCol);
        }

        if (idx) {
            // busqueda por indice: O(log n)
            long long offset = idx->search(node.where.value);
            if (offset >= 0) {
                Row row = sdm_.readRecord(db, node.table, offset);
                if (!row.empty()) filtered.push_back(row);
            }
        }
        else {
            // busqueda secuencial: O(n)
            for (auto& row : sdm_.readAllRecords(db, node.table))
                if (RowUtils::matchesWhere(row, node.where, schema))
                    filtered.push_back(row);
        }
    }
    else {
        // sin WHERE: todos los registros
        filtered = sdm_.readAllRecords(db, node.table);
    }

    // ORDER BY con quicksort propio
    if (node.hasOrderBy)
        RowUtils::sortRows(filtered, node.orderByColumn,
            node.orderByDirection, schema);

    // proyeccion de columnas
    bool selectAll = node.selectColumns.empty()
        || (node.selectColumns.size() == 1
            && node.selectColumns[0] == "*");

    if (selectAll) {
        // SELECT *: todas las columnas
        for (const auto& col : schema) r.columns.push_back(col.name);
        for (auto& row : filtered) {
            Row displayed;
            for (size_t i = 0; i < schema.size() && i < row.size(); ++i)
                displayed.push_back(TypeSystem::displayValue(row[i], schema[i]));
            r.rows.push_back(displayed);
        }
    }
    else {
        // columnas especificas
        r.columns = node.selectColumns;
        for (auto& row : filtered) {
            Row projected;
            for (const auto& colName : node.selectColumns) {
                bool pushed = false;
                for (size_t i = 0; i < schema.size() && i < row.size(); ++i) {
                    if (toLowerQP(schema[i].name) == toLowerQP(colName)) {
                        projected.push_back(TypeSystem::displayValue(row[i], schema[i]));
                        pushed = true; break;
                    }
                }
                if (!pushed) projected.push_back("");
            }
            r.rows.push_back(projected);
        }
    }

    r.success = true;
    return r;
}

// =============================================================================
// EXECUTE UPDATE
// =============================================================================

QueryResult QueryProcessor::executeUpdate(const ASTNode& node,
    const std::string& db)
{
    QueryResult r;
    auto schema = sdm_.getTableSchema(db, node.table);

    // busca si la tabla tiene un indice
    std::string idxCol = indexMgr_.getIndexedColumn(db, node.table);
    IndexHandle* idx = nullptr;
    int idxColPos = -1;
    if (!idxCol.empty()) {
        idx = indexMgr_.get(db, node.table, idxCol);
        for (int i = 0; i < static_cast<int>(schema.size()); ++i)
            if (toLowerQP(schema[i].name) == toLowerQP(idxCol)) { idxColPos = i; break; }
    }

    // localiza las filas a actualizar
    std::vector<std::pair<long long, Row>> targets;
    if (node.hasWhere && idx
        && toLowerQP(idxCol) == toLowerQP(node.where.column)
        && node.where.op == "=")
    {
        // usa el indice: O(log n)
        long long off = idx->search(node.where.value);
        if (off >= 0) {
            Row row = sdm_.readRecord(db, node.table, off);
            if (!row.empty()) targets.push_back({ off, row });
        }
    }
    else {
        // sin indice: requiere readAllRecordsWithOffsets() de persona b
        r.error = "UPDATE sin indice en la columna WHERE requiere "
            "readAllRecordsWithOffsets() de Persona B";
        return r;
    }

    // aplica los cambios a cada fila encontrada
    int updated = 0;
    for (auto& [off, row] : targets) {
        Row newRow = row;
        for (const auto& sc : node.setClauses) {
            for (size_t i = 0; i < schema.size(); ++i) {
                if (toLowerQP(schema[i].name) == toLowerQP(sc.column)) {
                    std::string norm = TypeSystem::normalizeValue(sc.value, schema[i]);

                    // si actualiza la columna indexada, verifica duplicado
                    if (idx && static_cast<int>(i) == idxColPos) {
                        long long existing = idx->search(norm);
                        if (existing >= 0 && existing != off) {
                            r.error = "valor duplicado en columna indexada '" + idxCol +
                                "': " + sc.value + " ya existe";
                            return r;
                        }
                        // actualiza el indice: remueve la clave vieja, inserta la nueva
                        idx->remove(row[i]);
                        idx->insert(norm, off);
                    }

                    newRow[i] = norm;
                    break;
                }
            }
        }
        // escribe el registro actualizado en disco
        if (!sdm_.updateRecord(db, node.table, off, newRow)) {
            r.error = "no se pudo actualizar un registro en '" + node.table + "'";
            return r;
        }
        ++updated;
    }

    r.success = true;
    r.columns = { "rows_updated" };
    r.rows = { { std::to_string(updated) } };
    return r;
}

// =============================================================================
// EXECUTE DELETE
// =============================================================================

QueryResult QueryProcessor::executeDelete(const ASTNode& node,
    const std::string& db)
{
    QueryResult r;
    auto schema = sdm_.getTableSchema(db, node.table);

    // busca si la tabla tiene un indice
    std::string idxCol = indexMgr_.getIndexedColumn(db, node.table);
    IndexHandle* idx = nullptr;
    int idxColPos = -1;
    if (!idxCol.empty()) {
        idx = indexMgr_.get(db, node.table, idxCol);
        for (int i = 0; i < static_cast<int>(schema.size()); ++i)
            if (toLowerQP(schema[i].name) == toLowerQP(idxCol)) { idxColPos = i; break; }
    }

    // localiza las filas a eliminar
    std::vector<std::pair<long long, Row>> targets;
    if (node.hasWhere && idx
        && toLowerQP(idxCol) == toLowerQP(node.where.column)
        && node.where.op == "=")
    {
        // usa el indice: O(log n)
        long long off = idx->search(node.where.value);
        if (off >= 0) {
            Row row = sdm_.readRecord(db, node.table, off);
            if (!row.empty()) targets.push_back({ off, row });
        }
    }
    else {
        // sin indice: requiere readAllRecordsWithOffsets() de persona b
        r.error = "DELETE sin indice en la columna WHERE requiere "
            "readAllRecordsWithOffsets() de Persona B";
        return r;
    }

    // elimina cada fila encontrada
    int deleted = 0;
    for (auto& [off, row] : targets) {
        // remueve del indice si existe
        if (idx && idxColPos >= 0) idx->remove(row[idxColPos]);
        // marca como eliminado en disco
        if (!sdm_.deleteRecord(db, node.table, off)) {
            r.error = "no se pudo eliminar un registro en '" + node.table + "'";
            return r;
        }
        ++deleted;
    }

    r.success = true;
    r.columns = { "rows_deleted" };
    r.rows = { { std::to_string(deleted) } };
    return r;
}

// =============================================================================
// EXECUTE CREATE INDEX
// =============================================================================

QueryResult QueryProcessor::executeCreateIndex(const ASTNode& node,
    const std::string& db)
{
    QueryResult r;
    auto schema = sdm_.getTableSchema(db, node.table);

    // encuentra el tipo de la columna indexada
    std::string colType = "VARCHAR";
    for (const auto& col : schema)
        if (toLowerQP(col.name) == toLowerQP(node.indexColumn)) {
            colType = col.type; break;
        }

    // crea el arbol correcto (BST o BTree) via la fabrica de IndexManager
    auto handle = makeIndexHandle(node.indexType, colType);

    // registra el indice en el catalogo
    IndexDefinition def;
    def.indexName = node.indexName;
    def.tableName = node.table;
    def.columnName = node.indexColumn;
    def.type = node.indexType;
    def.database = db;
    if (!sdm_.addIndex(def)) {
        r.error = "no se pudo registrar el indice en el catalogo";
        return r;
    }

    // agrega el indice al IndexManager en memoria
    indexMgr_.add(db, node.table, node.indexColumn, std::move(handle));

    r.success = true;
    return r;
}

// =============================================================================
// LOAD INDEXES
// =============================================================================

void QueryProcessor::loadIndexes() {
    // lee todos los indices del catalogo y reconstruye los arboles en memoria
    for (const auto& def : sdm_.loadIndexesOnStartup()) {
        auto schema = sdm_.getTableSchema(def.database, def.tableName);
        std::string colType = "VARCHAR";
        for (const auto& col : schema)
            if (toLowerQP(col.name) == toLowerQP(def.columnName)) {
                colType = col.type; break;
            }
        indexMgr_.add(def.database, def.tableName, def.columnName,
            makeIndexHandle(def.type, colType));
    }
}