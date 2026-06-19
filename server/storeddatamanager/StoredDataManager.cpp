#include "storeddatamanager.h"
#include "systemcatalog.h"
#include <filesystem>
#include <fstream>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <utility>

namespace fs = std::filesystem;

// rutas base (relativas al ejecutable)
static const std::string BASE_PATH = "./data/";
static const std::string CATALOG_PATH = "./data/catalog/";

// clave xor (igual que en systemcatalog)
static const uint8_t XOR_KEY[] = { 0xA3, 0x5C, 0xF7, 0x2B, 0x91, 0xDE, 0x44, 0x68 };
static constexpr int XOR_KEY_LEN = sizeof(XOR_KEY);

// constantes
static constexpr int COL_NAME_LEN_D = 64;
static constexpr int COL_TYPE_LEN_D = 16;
static constexpr uint32_t MAGIC_NUMBER = 0x54424C21u;  // "TBL!"

// estructuras del formato de archivo

#pragma pack(push, 1)

struct TableHeader {
    uint32_t magic;
    uint32_t numColumns;
};

struct ColumnHeader {
    char name[COL_NAME_LEN_D];
    char type[COL_TYPE_LEN_D];
    int32_t size;
    uint8_t nullable;
};

#pragma pack(pop)

// helpers internos

// xor in-place
static void xorBlock(char* data, int size) {
    for (int i = 0; i < size; ++i) {
        data[i] ^= static_cast<char>(XOR_KEY[i % XOR_KEY_LEN]);
    }
}

// llena un buffer de tamano fijo con ceros y copia src
static void fillField(char* dest, const std::string& src, int maxLen) {
    std::memset(dest, 0, maxLen);
    std::strncpy(dest, src.c_str(), maxLen - 1);
}

// convierte char[] a string
static std::string fromField(const char* src) {
    return std::string(src);
}

// calcula el tamano en bytes de una columna en disco
static int columnByteSize(const ColumnDefinition& col) {
    if (col.type == "INTEGER" || col.type == "DATETIME") {
        return 8;
    }
    if (col.type == "DOUBLE") {
        return 8;
    }
    if (col.type == "VARCHAR") {
        return col.size > 0 ? col.size : 1;
    }
    return 8;
}

// calcula el tamano total de un registro (alive + datos)
static int recordByteSize(const std::vector<ColumnDefinition>& cols) {
    int total = 1;  // byte para alive
    for (const auto& col : cols) {
        total += columnByteSize(col);
    }
    return total;
}

// construye el path de un archivo de tabla
static std::string tablePath(const std::string& dbName,
    const std::string& tableName)
{
    return BASE_PATH + dbName + "/" + tableName + ".tbl";
}

// instancia global del catalogo

static SystemCatalog& getCatalog() {
    static SystemCatalog catalog(CATALOG_PATH);
    return catalog;
}

// OPERACIONES DE BASE DE DATOS

bool StoredDataManager::createDatabase(const std::string& dbName) {
    // crea la carpeta en disco y la registra en el catalogo
    std::string path = BASE_PATH + dbName;
    bool folderCreated = fs::create_directories(path);

    if (folderCreated) {
        getCatalog().addDatabase(dbName);
    }

    return folderCreated;
}

bool StoredDataManager::databaseExists(const std::string& dbName) {
    // consulta al catalogo (fuente de verdad)
    return getCatalog().databaseExists(dbName);
}

std::vector<std::string> StoredDataManager::listDatabases() {
    return getCatalog().listDatabases();
}

// OPERACIONES DE TABLA

bool StoredDataManager::createTableFile(const std::string& dbName,
    const std::string& tableName,
    const std::vector<ColumnDefinition>& columns)
{
    std::string path = tablePath(dbName, tableName);

    // no sobreescribir si ya existe
    if (fs::exists(path)) {
        return false;
    }

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // escribe el header (sin encriptar)
    TableHeader header{};
    header.magic = MAGIC_NUMBER;
    header.numColumns = static_cast<uint32_t>(columns.size());
    file.write(reinterpret_cast<char*>(&header), sizeof(TableHeader));

    // escribe los column headers (sin encriptar)
    for (const auto& col : columns) {
        ColumnHeader ch{};
        fillField(ch.name, col.name, COL_NAME_LEN_D);
        fillField(ch.type, col.type, COL_TYPE_LEN_D);
        ch.size = static_cast<int32_t>(col.size);
        ch.nullable = col.nullable ? 1 : 0;
        file.write(reinterpret_cast<char*>(&ch), sizeof(ColumnHeader));
    }

    file.close();

    // registra en el catalogo
    getCatalog().addTable(dbName, tableName);
    getCatalog().addColumns(dbName, tableName, columns);

    return true;
}

bool StoredDataManager::dropTable(const std::string& dbName,
    const std::string& tableName)
{
    std::string path = tablePath(dbName, tableName);

    // elimina el archivo de disco
    if (!fs::remove(path)) {
        return false;
    }

    // limpia el catalogo (columnas, indices, tabla)
    getCatalog().removeColumns(dbName, tableName);
    getCatalog().removeIndex(dbName, tableName);
    getCatalog().removeTable(dbName, tableName);

    return true;
}

bool StoredDataManager::tableExists(const std::string& dbName,
    const std::string& tableName)
{
    return getCatalog().tableExists(dbName, tableName);
}

bool StoredDataManager::isTableEmpty(const std::string& dbName,
    const std::string& tableName)
{
    auto cols = getTableSchema(dbName, tableName);
    if (cols.empty()) {
        return true;
    }

    std::string path = tablePath(dbName, tableName);
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return true;
    }

    // salta el header
    int headerSize = sizeof(TableHeader) + static_cast<int>(cols.size()) * sizeof(ColumnHeader);
    file.seekg(headerSize, std::ios::beg);

    // recorre los registros buscando uno activo
    int recSize = recordByteSize(cols);
    std::vector<char> buf(recSize);

    while (file.read(buf.data(), recSize)) {
        std::vector<char> dec = buf;
        xorBlock(dec.data(), recSize);
        uint8_t alive = static_cast<uint8_t>(dec[0]);
        if (alive == 1) {
            return false;  // encontrado un registro activo
        }
    }

    return true;
}

std::vector<ColumnDefinition> StoredDataManager::getTableSchema(
    const std::string& dbName, const std::string& tableName)
{
    // lee del catalogo, no del archivo de tabla
    return getCatalog().listColumns(dbName, tableName);
}

std::vector<std::string> StoredDataManager::listTables(const std::string& dbName) {
    return getCatalog().listTables(dbName);
}

// OPERACIONES DE REGISTROS

long long StoredDataManager::appendRecord(const std::string& dbName,
    const std::string& tableName,
    const Row& values)
{
    auto cols = getTableSchema(dbName, tableName);
    if (cols.empty()) return -1;

    std::string path = tablePath(dbName, tableName);
    std::ofstream file(path, std::ios::binary | std::ios::app);
    if (!file.is_open()) return -1;

    // guarda el offset antes de escribir (posicion actual = fin del archivo)
    long long offset = static_cast<long long>(file.tellp());

    // construye el buffer: [alive(1 byte)] [campo1] [campo2] ... [campoN]
    int recSize = recordByteSize(cols);
    std::vector<char> buf(recSize, 0);

    // byte 0: alive = 1 (registro activo)
    buf[0] = 1;

    // serializa cada valor segun su tipo
    int pos = 1;
    for (size_t i = 0; i < cols.size(); ++i) {
        int fieldSize = columnByteSize(cols[i]);
        const std::string& val = (i < values.size()) ? values[i] : "";

        if (cols[i].type == "INTEGER" || cols[i].type == "DATETIME") {
            long long v = 0;
            try { v = std::stoll(val); }
            catch (...) { v = 0; }
            std::memcpy(&buf[pos], &v, sizeof(long long));
        }
        else if (cols[i].type == "DOUBLE") {
            double v = 0.0;
            try { v = std::stod(val); }
            catch (...) { v = 0.0; }
            std::memcpy(&buf[pos], &v, sizeof(double));
        }
        else {
            // VARCHAR: copia los caracteres, rellena con ceros hasta fieldSize
            std::memset(&buf[pos], 0, fieldSize);
            std::strncpy(&buf[pos], val.c_str(), fieldSize - 1);
        }
        pos += fieldSize;
    }

    // encripta el buffer completo con XOR antes de escribir en disco
    xorBlock(buf.data(), recSize);

    file.write(buf.data(), recSize);
    file.close();

    return offset;
}

Row StoredDataManager::readRecord(const std::string& dbName,
    const std::string& tableName,
    long long offset)
{
    auto cols = getTableSchema(dbName, tableName);
    if (cols.empty()) return {};

    std::string path = tablePath(dbName, tableName);
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return {};

    // salta directamente al offset donde esta el registro
    file.seekg(offset, std::ios::beg);

    int recSize = recordByteSize(cols);
    std::vector<char> buf(recSize);
    if (!file.read(buf.data(), recSize)) return {};

    // desencripta
    xorBlock(buf.data(), recSize);

    // verifica que el registro este activo
    uint8_t alive = static_cast<uint8_t>(buf[0]);
    if (alive != 1) return {};

    // deserializa cada campo
    Row row;
    int pos = 1;
    for (const auto& col : cols) {
        int fieldSize = columnByteSize(col);

        if (col.type == "INTEGER" || col.type == "DATETIME") {
            long long v = 0;
            std::memcpy(&v, &buf[pos], sizeof(long long));
            row.push_back(std::to_string(v));
        }
        else if (col.type == "DOUBLE") {
            double v = 0.0;
            std::memcpy(&v, &buf[pos], sizeof(double));
            row.push_back(std::to_string(v));
        }
        else {
            std::string val(&buf[pos], strnlen(&buf[pos], fieldSize));
            row.push_back(val);
        }
        pos += fieldSize;
    }

    return row;
}

ResultSet StoredDataManager::readAllRecords(const std::string& dbName,
    const std::string& tableName)
{
    // reutiliza readAllRecordsWithOffsets y descarta los offsets
    auto withOffsets = readAllRecordsWithOffsets(dbName, tableName);
    ResultSet result;
    result.reserve(withOffsets.size());
    for (auto& [offset, row] : withOffsets) {
        result.push_back(std::move(row));
    }
    return result;
}

std::vector<std::pair<long long, Row>> StoredDataManager::readAllRecordsWithOffsets(
    const std::string& dbName,
    const std::string& tableName)
{
    std::vector<std::pair<long long, Row>> result;

    auto cols = getTableSchema(dbName, tableName);
    if (cols.empty()) return result;

    std::string path = tablePath(dbName, tableName);
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return result;

    // salta el header del archivo
    int headerSize = sizeof(TableHeader)
        + static_cast<int>(cols.size()) * sizeof(ColumnHeader);
    file.seekg(headerSize, std::ios::beg);

    int recSize = recordByteSize(cols);
    std::vector<char> buf(recSize);

    while (true) {
        // guarda el offset antes de leer
        long long offset = static_cast<long long>(file.tellg());
        if (!file.read(buf.data(), recSize)) break;

        std::vector<char> dec = buf;
        xorBlock(dec.data(), recSize);

        // solo incluye registros activos
        uint8_t alive = static_cast<uint8_t>(dec[0]);
        if (alive != 1) continue;

        // deserializa la fila
        Row row;
        int pos = 1;
        for (const auto& col : cols) {
            int fieldSize = columnByteSize(col);

            if (col.type == "INTEGER" || col.type == "DATETIME") {
                long long v = 0;
                std::memcpy(&v, &dec[pos], sizeof(long long));
                row.push_back(std::to_string(v));
            }
            else if (col.type == "DOUBLE") {
                double v = 0.0;
                std::memcpy(&v, &dec[pos], sizeof(double));
                row.push_back(std::to_string(v));
            }
            else {
                std::string val(&dec[pos], strnlen(&dec[pos], fieldSize));
                row.push_back(val);
            }
            pos += fieldSize;
        }

        result.push_back({ offset, std::move(row) });
    }

    return result;
}

bool StoredDataManager::updateRecord(const std::string& dbName,
    const std::string& tableName,
    long long offset,
    const Row& newValues)
{
    auto cols = getTableSchema(dbName, tableName);
    if (cols.empty()) return false;

    std::string path = tablePath(dbName, tableName);
    std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) return false;

    // salta directamente al offset del registro
    file.seekp(offset, std::ios::beg);

    int recSize = recordByteSize(cols);
    std::vector<char> buf(recSize, 0);

    // el registro sigue activo
    buf[0] = 1;

    int pos = 1;
    for (size_t i = 0; i < cols.size(); ++i) {
        int fieldSize = columnByteSize(cols[i]);
        const std::string& val = (i < newValues.size()) ? newValues[i] : "";

        if (cols[i].type == "INTEGER" || cols[i].type == "DATETIME") {
            long long v = 0;
            try { v = std::stoll(val); }
            catch (...) { v = 0; }
            std::memcpy(&buf[pos], &v, sizeof(long long));
        }
        else if (cols[i].type == "DOUBLE") {
            double v = 0.0;
            try { v = std::stod(val); }
            catch (...) { v = 0.0; }
            std::memcpy(&buf[pos], &v, sizeof(double));
        }
        else {
            std::memset(&buf[pos], 0, fieldSize);
            std::strncpy(&buf[pos], val.c_str(), fieldSize - 1);
        }
        pos += fieldSize;
    }

    // encripta y sobrescribe en la misma posicion
    xorBlock(buf.data(), recSize);
    file.write(buf.data(), recSize);

    return file.good();
}

bool StoredDataManager::deleteRecord(const std::string& dbName,
    const std::string& tableName,
    long long offset)
{
    std::string path = tablePath(dbName, tableName);
    std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) return false;

    // salta al offset del registro
    file.seekp(offset, std::ios::beg);

    // cambia solo el byte alive de 1 a 0
    // como esta encriptado con XOR, el valor a escribir es 0 XOR clave[0]
    char aliveByte = static_cast<char>(0) ^ static_cast<char>(XOR_KEY[0]);
    file.write(&aliveByte, 1);

    return file.good();
}

// OPERACIONES DE INDICES - DELEGAN AL CATALOGO

bool StoredDataManager::addIndex(const IndexDefinition& index) {
    return getCatalog().addIndex(index);
}

bool StoredDataManager::removeIndex(const std::string& dbName,
    const std::string& tableName)
{
    return getCatalog().removeIndex(dbName, tableName);
}

std::vector<IndexDefinition> StoredDataManager::listIndexes(
    const std::string& dbName, const std::string& tableName)
{
    return getCatalog().listIndexes(dbName, tableName);
}

std::vector<IndexDefinition> StoredDataManager::loadIndexesOnStartup() {
    return getCatalog().listAllIndexes();
}

ResultSet StoredDataManager::readCatalogTable(const std::string& catalogTableName) {
    return getCatalog().readAsTable(catalogTableName);
}