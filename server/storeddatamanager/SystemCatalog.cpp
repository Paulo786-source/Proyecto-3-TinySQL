#include "systemcatalog.h"
#include <fstream>
#include <cstring>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

// clave xor (8 bytes)
static const uint8_t XOR_KEY[] = { 0xA3, 0x5C, 0xF7, 0x2B, 0x91, 0xDE, 0x44, 0x68 };
static constexpr int XOR_KEY_LEN = sizeof(XOR_KEY);

// tamanos de campo fijos
static constexpr int DB_NAME_LEN = 64;
static constexpr int TABLE_NAME_LEN = 64;
static constexpr int COL_NAME_LEN = 64;
static constexpr int COL_TYPE_LEN = 16;
static constexpr int IDX_NAME_LEN = 64;
static constexpr int IDX_TYPE_LEN = 8;

// estructuras de registros en disco

struct DbRecord {
    char name[DB_NAME_LEN];
    uint8_t active;  // 1 activo, 0 eliminado
};

struct TableRecord {
    char dbName[DB_NAME_LEN];
    char tableName[TABLE_NAME_LEN];
    uint8_t active;
};

struct ColumnRecord {
    char dbName[DB_NAME_LEN];
    char tableName[TABLE_NAME_LEN];
    char colName[COL_NAME_LEN];
    char colType[COL_TYPE_LEN];
    int32_t colSize;
    uint8_t nullable;
    uint8_t active;
};

struct IndexRecord {
    char dbName[DB_NAME_LEN];
    char tableName[TABLE_NAME_LEN];
    char colName[COL_NAME_LEN];
    char indexName[IDX_NAME_LEN];
    char indexType[IDX_TYPE_LEN];
    uint8_t active;
};

// helpers internos

// llena un buffer de tamano fijo con ceros y copia src
static void fillField(char* dest, const std::string& src, int maxLen) {
    std::memset(dest, 0, maxLen);
    std::strncpy(dest, src.c_str(), maxLen - 1);
}

// convierte char[] a string
static std::string fromField(const char* src) {
    return std::string(src);
}

// CONSTRUCTOR

SystemCatalog::SystemCatalog(const std::string& catalogPath)
    : catalogPath_(catalogPath)
{
    // crea la carpeta si no existe
    fs::create_directories(catalogPath_);
}

// XOR ENCRYPTION

void SystemCatalog::xorEncrypt(char* data, int size) {
    for (int i = 0; i < size; ++i) {
        data[i] ^= static_cast<char>(XOR_KEY[i % XOR_KEY_LEN]);
    }
}

// HELPERS DE LECTURA/ESCRITURA GENERICA

// lee todos los registros de tipo T desde el archivo
template<typename T>
static std::vector<T> readAll(const std::string& path, SystemCatalog* cat) {
    std::vector<T> result;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return result;  // archivo no existe, catalogo vacio
    }

    T record;
    while (file.read(reinterpret_cast<char*>(&record), sizeof(T))) {
        cat->xorEncrypt(reinterpret_cast<char*>(&record), sizeof(T));
        result.push_back(record);
    }
    return result;
}

// escribe un vector de registros T en el archivo (sobrescribe completo)
template<typename T>
static void writeAll(const std::string& path, std::vector<T>& records,
    SystemCatalog* cat)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("no se pudo abrir para escritura: " + path);
    }

    for (T& rec : records) {
        T encrypted = rec;
        cat->xorEncrypt(reinterpret_cast<char*>(&encrypted), sizeof(T));
        file.write(reinterpret_cast<char*>(&encrypted), sizeof(T));
    }
}

// SYSTEMDATABASES

bool SystemCatalog::addDatabase(const std::string& dbName) {
    std::string path = catalogPath_ + "SystemDatabases";
    auto records = readAll<DbRecord>(path, this);

    // verifica que no exista ya
    for (auto& r : records) {
        if (r.active && fromField(r.name) == dbName) {
            return false;
        }
    }

    DbRecord rec{};
    fillField(rec.name, dbName, DB_NAME_LEN);
    rec.active = 1;
    records.push_back(rec);

    writeAll(path, records, this);
    return true;
}

bool SystemCatalog::databaseExists(const std::string& dbName) {
    std::string path = catalogPath_ + "SystemDatabases";
    auto records = readAll<DbRecord>(path, this);
    for (auto& r : records) {
        if (r.active && fromField(r.name) == dbName) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> SystemCatalog::listDatabases() {
    std::string path = catalogPath_ + "SystemDatabases";
    auto records = readAll<DbRecord>(path, this);
    std::vector<std::string> result;
    for (auto& r : records) {
        if (r.active) {
            result.push_back(fromField(r.name));
        }
    }
    return result;
}

// SYSTEMTABLES

bool SystemCatalog::addTable(const std::string& dbName,
    const std::string& tableName)
{
    std::string path = catalogPath_ + "SystemTables";
    auto records = readAll<TableRecord>(path, this);

    // verifica que no exista ya
    for (auto& r : records) {
        if (r.active && fromField(r.dbName) == dbName
            && fromField(r.tableName) == tableName) {
            return false;
        }
    }

    TableRecord rec{};
    fillField(rec.dbName, dbName, DB_NAME_LEN);
    fillField(rec.tableName, tableName, TABLE_NAME_LEN);
    rec.active = 1;
    records.push_back(rec);

    writeAll(path, records, this);
    return true;
}

bool SystemCatalog::removeTable(const std::string& dbName,
    const std::string& tableName)
{
    std::string path = catalogPath_ + "SystemTables";
    auto records = readAll<TableRecord>(path, this);
    bool found = false;
    for (auto& r : records) {
        if (r.active && fromField(r.dbName) == dbName
            && fromField(r.tableName) == tableName) {
            r.active = 0;  // soft-delete
            found = true;
        }
    }
    if (found) {
        writeAll(path, records, this);
    }
    return found;
}

bool SystemCatalog::tableExists(const std::string& dbName,
    const std::string& tableName)
{
    std::string path = catalogPath_ + "SystemTables";
    auto records = readAll<TableRecord>(path, this);
    for (auto& r : records) {
        if (r.active && fromField(r.dbName) == dbName
            && fromField(r.tableName) == tableName) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> SystemCatalog::listTables(const std::string& dbName) {
    std::string path = catalogPath_ + "SystemTables";
    auto records = readAll<TableRecord>(path, this);
    std::vector<std::string> result;
    for (auto& r : records) {
        if (r.active && fromField(r.dbName) == dbName) {
            result.push_back(fromField(r.tableName));
        }
    }
    return result;
}

// SYSTEMCOLUMNS

bool SystemCatalog::addColumns(const std::string& dbName,
    const std::string& tableName,
    const std::vector<ColumnDefinition>& columns)
{
    std::string path = catalogPath_ + "SystemColumns";
    auto records = readAll<ColumnRecord>(path, this);

    // agrega una fila por cada columna
    for (const auto& col : columns) {
        ColumnRecord rec{};
        fillField(rec.dbName, dbName, DB_NAME_LEN);
        fillField(rec.tableName, tableName, TABLE_NAME_LEN);
        fillField(rec.colName, col.name, COL_NAME_LEN);
        fillField(rec.colType, col.type, COL_TYPE_LEN);
        rec.colSize = static_cast<int32_t>(col.size);
        rec.nullable = col.nullable ? 1 : 0;
        rec.active = 1;
        records.push_back(rec);
    }

    writeAll(path, records, this);
    return true;
}

bool SystemCatalog::removeColumns(const std::string& dbName,
    const std::string& tableName)
{
    std::string path = catalogPath_ + "SystemColumns";
    auto records = readAll<ColumnRecord>(path, this);
    bool found = false;
    for (auto& r : records) {
        if (r.active && fromField(r.dbName) == dbName
            && fromField(r.tableName) == tableName) {
            r.active = 0;  // soft-delete
            found = true;
        }
    }
    if (found) {
        writeAll(path, records, this);
    }
    return found;
}

std::vector<ColumnDefinition> SystemCatalog::listColumns(
    const std::string& dbName, const std::string& tableName)
{
    std::string path = catalogPath_ + "SystemColumns";
    auto records = readAll<ColumnRecord>(path, this);
    std::vector<ColumnDefinition> result;
    for (auto& r : records) {
        if (r.active && fromField(r.dbName) == dbName
            && fromField(r.tableName) == tableName) {
            ColumnDefinition col;
            col.name = fromField(r.colName);
            col.type = fromField(r.colType);
            col.size = static_cast<int>(r.colSize);
            col.nullable = (r.nullable == 1);
            result.push_back(col);
        }
    }
    return result;
}

// SYSTEMINDEXES

bool SystemCatalog::addIndex(const IndexDefinition& index) {
    std::string path = catalogPath_ + "SystemIndexes";
    auto records = readAll<IndexRecord>(path, this);

    IndexRecord rec{};
    fillField(rec.dbName, index.database, DB_NAME_LEN);
    fillField(rec.tableName, index.tableName, TABLE_NAME_LEN);
    fillField(rec.colName, index.columnName, COL_NAME_LEN);
    fillField(rec.indexName, index.indexName, IDX_NAME_LEN);
    fillField(rec.indexType, index.type, IDX_TYPE_LEN);
    rec.active = 1;
    records.push_back(rec);

    writeAll(path, records, this);
    return true;
}

bool SystemCatalog::removeIndex(const std::string& dbName,
    const std::string& tableName)
{
    std::string path = catalogPath_ + "SystemIndexes";
    auto records = readAll<IndexRecord>(path, this);
    bool found = false;
    for (auto& r : records) {
        if (r.active && fromField(r.dbName) == dbName
            && fromField(r.tableName) == tableName) {
            r.active = 0;  // soft-delete
            found = true;
        }
    }
    if (found) {
        writeAll(path, records, this);
    }
    return found;
}

std::vector<IndexDefinition> SystemCatalog::listIndexes(
    const std::string& dbName, const std::string& tableName)
{
    std::string path = catalogPath_ + "SystemIndexes";
    auto records = readAll<IndexRecord>(path, this);
    std::vector<IndexDefinition> result;
    for (auto& r : records) {
        if (r.active && fromField(r.dbName) == dbName
            && fromField(r.tableName) == tableName) {
            IndexDefinition idx;
            idx.database = fromField(r.dbName);
            idx.tableName = fromField(r.tableName);
            idx.columnName = fromField(r.colName);
            idx.indexName = fromField(r.indexName);
            idx.type = fromField(r.indexType);
            result.push_back(idx);
        }
    }
    return result;
}

std::vector<IndexDefinition> SystemCatalog::listAllIndexes() {
    std::string path = catalogPath_ + "SystemIndexes";
    auto records = readAll<IndexRecord>(path, this);
    std::vector<IndexDefinition> result;
    for (auto& r : records) {
        if (r.active) {
            IndexDefinition idx;
            idx.database = fromField(r.dbName);
            idx.tableName = fromField(r.tableName);
            idx.columnName = fromField(r.colName);
            idx.indexName = fromField(r.indexName);
            idx.type = fromField(r.indexType);
            result.push_back(idx);
        }
    }
    return result;
}

// READASTABLE - PARA SELECT SOBRE TABLAS DEL CATALOGO

// permite hacer select a las tablas del catalogo (systemdatabases, systemtables, etc)

ResultSet SystemCatalog::readAsTable(const std::string& catalogTableName) {
    ResultSet result;

    if (catalogTableName == "SystemDatabases") {
        // cada fila tiene una columna: el nombre de la base de datos
        for (const auto& name : listDatabases()) {
            result.push_back({ name });
        }
    }
    else if (catalogTableName == "SystemTables") {
        std::string path = catalogPath_ + "SystemTables";
        auto records = readAll<TableRecord>(path, this);
        // cada fila tiene dos columnas: database y table
        for (auto& r : records) {
            if (r.active) {
                result.push_back({
                    fromField(r.dbName),
                    fromField(r.tableName)
                    });
            }
        }
    }
    else if (catalogTableName == "SystemColumns") {
        std::string path = catalogPath_ + "SystemColumns";
        auto records = readAll<ColumnRecord>(path, this);
        // cada fila tiene: db, table, column, type, size, nullable
        for (auto& r : records) {
            if (r.active) {
                result.push_back({
                    fromField(r.dbName),
                    fromField(r.tableName),
                    fromField(r.colName),
                    fromField(r.colType),
                    std::to_string(r.colSize),
                    (r.nullable ? "YES" : "NO")
                    });
            }
        }
    }
    else if (catalogTableName == "SystemIndexes") {
        std::string path = catalogPath_ + "SystemIndexes";
        auto records = readAll<IndexRecord>(path, this);
        // cada fila tiene: db, table, column, index_name, index_type
        for (auto& r : records) {
            if (r.active) {
                result.push_back({
                    fromField(r.dbName),
                    fromField(r.tableName),
                    fromField(r.colName),
                    fromField(r.indexName),
                    fromField(r.indexType)
                    });
            }
        }
    }

    return result;
}