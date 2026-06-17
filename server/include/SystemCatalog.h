#pragma once

#include <string>
#include <vector>
#include "storeddatamanager.h"

// catalogo del sistema: metadata en archivos binarios encriptados
// archivos: systemdatabases, systemtables, systemcolumns, systemindexes

class SystemCatalog {
public:

    // recibe la ruta base donde se guardan los archivos del catalogo
    explicit SystemCatalog(const std::string& catalogPath);

    // DATABASES

    bool addDatabase(const std::string& dbName);
    bool databaseExists(const std::string& dbName);
    std::vector<std::string> listDatabases();

    // TABLES

    bool addTable(const std::string& dbName, const std::string& tableName);
    bool removeTable(const std::string& dbName, const std::string& tableName); // soft-delete
    bool tableExists(const std::string& dbName, const std::string& tableName);
    std::vector<std::string> listTables(const std::string& dbName);

    // COLUMNS

    // guarda las columnas de una tabla
    bool addColumns(const std::string& dbName,
        const std::string& tableName,
        const std::vector<ColumnDefinition>& columns);

    bool removeColumns(const std::string& dbName, const std::string& tableName);
    // retorna las columnas de una tabla
    std::vector<ColumnDefinition> listColumns(const std::string& dbName,
        const std::string& tableName);

    // INDEXES

    bool addIndex(const IndexDefinition& index);

    bool removeIndex(const std::string& dbName, const std::string& tableName);
    // lista los indices de una tabla
    std::vector<IndexDefinition> listIndexes(const std::string& dbName,
        const std::string& tableName);
    // lista todos los indices (para reconstruir arboles al iniciar)
    std::vector<IndexDefinition> listAllIndexes();

    // select a tablas del sistema 

    // permite consultar systemdatabases, systemtables, etc con select
    ResultSet readAsTable(const std::string& catalogTableName);

    // xor encryption (publico para los helpers template en el .cpp)
    void xorEncrypt(char* data, int size);

private:
    std::string catalogPath_; // ruta donde estan los archivos
};