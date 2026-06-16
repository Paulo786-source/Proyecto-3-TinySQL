#include "StoredDataManager.h"
#include <filesystem>

// ruta base donde se guardan las bases de datos en disco
static const std::string BASE_PATH = "./data/";

// BASES DE DATOS

bool StoredDataManager::createDatabase(const std::string& dbName) {
    // crea la carpeta de la base de datos en disco
    std::string path = BASE_PATH + dbName;
    return std::filesystem::create_directories(path);
}

bool StoredDataManager::databaseExists(const std::string& dbName) {
    return false;
}

std::vector<std::string> StoredDataManager::listDatabases() {
    return {};
}

// TABLAS

bool StoredDataManager::createTableFile(const std::string& dbName, const std::string& tableName, const std::vector<ColumnDefinition>& columns) {
    return false;
}

bool StoredDataManager::dropTable(const std::string& dbName, const std::string& tableName) {
    return false;
}

bool StoredDataManager::tableExists(const std::string& dbName, const std::string& tableName) {
    return false;
}

bool StoredDataManager::isTableEmpty(const std::string& dbName, const std::string& tableName) {
    return true;
}

std::vector<ColumnDefinition> StoredDataManager::getTableSchema(const std::string& dbName, const std::string& tableName) {
    return {};
}

std::vector<std::string> StoredDataManager::listTables(const std::string& dbName) {
    return {};
}

// REGISTROS

long long StoredDataManager::appendRecord(const std::string& dbName, const std::string& tableName, const Row& values) {
    return -1;
}

Row StoredDataManager::readRecord(const std::string& dbName, const std::string& tableName, long long offset) {
    return {};
}

ResultSet StoredDataManager::readAllRecords(const std::string& dbName, const std::string& tableName) {
    return {};
}

bool StoredDataManager::updateRecord(const std::string& dbName, const std::string& tableName, long long offset, const Row& newValues) {
    return false;
}

bool StoredDataManager::deleteRecord(const std::string& dbName, const std::string& tableName, long long offset) {
    return false;
}

// ÍNDICES

bool StoredDataManager::addIndex(const IndexDefinition& index) {
    return false;
}

bool StoredDataManager::removeIndex(const std::string& dbName, const std::string& tableName) {
    return false;
}

std::vector<IndexDefinition> StoredDataManager::listIndexes(const std::string& dbName, const std::string& tableName) {
    return {};
}

std::vector<IndexDefinition> StoredDataManager::loadIndexesOnStartup() {
    return {};
}

// CATÁLOGO

ResultSet StoredDataManager::readCatalogTable(const std::string& catalogTableName) {
    return {};
}