#include "StoredDataManager.h"
#include <filesystem>

// ruta base donde se guardan las bases de datos en disco
static const std::string BASE_PATH = "./data/";

// -- bases de datos -----------------------------------------------------------

bool StoredDataManager::createDatabase(const std::string& dbName) {
    // crea la carpeta de la base de datos en disco
    std::string path = BASE_PATH + dbName;
    return std::filesystem::create_directories(path);
}

bool StoredDataManager::databaseExists(const std::string& dbName) {
    // verifica si la carpeta de la base de datos existe en disco
    std::string path = BASE_PATH + dbName;
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

std::vector<std::string> StoredDataManager::listDatabases() {
    // se implementa con el system catalog en fase 1 por el companero
    return {};
}

// -- tablas -------------------------------------------------------------------

bool StoredDataManager::createTableFile(const std::string& dbName, const std::string& tableName, const std::vector<ColumnDefinition>& columns) {
    (void)dbName;
    (void)tableName;
    (void)columns;
    return false;
}

bool StoredDataManager::dropTable(const std::string& dbName, const std::string& tableName) {
    (void)dbName;
    (void)tableName;
    return false;
}

bool StoredDataManager::tableExists(const std::string& dbName, const std::string& tableName) {
    (void)dbName;
    (void)tableName;
    return false;
}

bool StoredDataManager::isTableEmpty(const std::string& dbName, const std::string& tableName) {
    (void)dbName;
    (void)tableName;
    return true;
}

std::vector<ColumnDefinition> StoredDataManager::getTableSchema(const std::string& dbName, const std::string& tableName) {
    (void)dbName;
    (void)tableName;
    return {};
}

std::vector<std::string> StoredDataManager::listTables(const std::string& dbName) {
    (void)dbName;
    return {};
}

// -- registros ----------------------------------------------------------------

long long StoredDataManager::appendRecord(const std::string& dbName, const std::string& tableName, const Row& values) {
    (void)dbName;
    (void)tableName;
    (void)values;
    return -1;
}

Row StoredDataManager::readRecord(const std::string& dbName, const std::string& tableName, long long offset) {
    (void)dbName;
    (void)tableName;
    (void)offset;
    return {};
}

ResultSet StoredDataManager::readAllRecords(const std::string& dbName, const std::string& tableName) {
    (void)dbName;
    (void)tableName;
    return {};
}

bool StoredDataManager::updateRecord(const std::string& dbName, const std::string& tableName, long long offset, const Row& newValues) {
    (void)dbName;
    (void)tableName;
    (void)offset;
    (void)newValues;
    return false;
}

bool StoredDataManager::deleteRecord(const std::string& dbName, const std::string& tableName, long long offset) {
    (void)dbName;
    (void)tableName;
    (void)offset;
    return false;
}

// -- indices ------------------------------------------------------------------

bool StoredDataManager::addIndex(const IndexDefinition& index) {
    (void)index;
    return false;
}

bool StoredDataManager::removeIndex(const std::string& dbName, const std::string& tableName) {
    (void)dbName;
    (void)tableName;
    return false;
}

std::vector<IndexDefinition> StoredDataManager::listIndexes(const std::string& dbName, const std::string& tableName) {
    (void)dbName;
    (void)tableName;
    return {};
}

std::vector<IndexDefinition> StoredDataManager::loadIndexesOnStartup() {
    return {};
}

// -- catalogo -----------------------------------------------------------------

ResultSet StoredDataManager::readCatalogTable(const std::string& catalogTableName) {
    (void)catalogTableName;
    return {};
}