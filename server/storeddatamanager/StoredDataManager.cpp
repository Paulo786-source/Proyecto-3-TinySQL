#include "storeddatamanager.h"
#include "systemcatalog.h"
#include <filesystem>
#include <fstream>
#include <cstring>
#include <stdexcept>
#include <algorithm>

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

// OPERACIONES DE REGISTROS - STUBS PARA FASE 3

long long StoredDataManager::appendRecord(const std::string& dbName,
    const std::string& tableName,
    const Row& values)
{
    // TODO fase 3: serializar, encriptar y escribir al final del archivo
    (void)dbName; (void)tableName; (void)values;
    return -1;
}

Row StoredDataManager::readRecord(const std::string& dbName,
    const std::string& tableName,
    long long offset)
{
    // TODO fase 3: leer, desencriptar y deserializar segun schema
    (void)dbName; (void)tableName; (void)offset;
    return {};
}

ResultSet StoredDataManager::readAllRecords(const std::string& dbName,
    const std::string& tableName)
{
    // TODO fase 3: recorrer todos los registros y retornar los activos
    (void)dbName; (void)tableName;
    return {};
}

bool StoredDataManager::updateRecord(const std::string& dbName,
    const std::string& tableName,
    long long offset,
    const Row& newValues)
{
    // TODO fase 3: sobrescribir el registro en la posicion offset
    (void)dbName; (void)tableName; (void)offset; (void)newValues;
    return false;
}

bool StoredDataManager::deleteRecord(const std::string& dbName,
    const std::string& tableName,
    long long offset)
{
    // TODO fase 3: poner alive = 0 en la posicion offset
    (void)dbName; (void)tableName; (void)offset;
    return false;
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