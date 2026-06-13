#pragma once

#include <string>
#include <vector>
#include "StoredDataManager.h"

// ─── Interfaz SystemCatalog ───────────────────────────────────────────────────

// El System Catalog es una carpeta especial en disco que contiene archivos
// binarios con metadata sobre todas las bases de datos del sistema.
// Es compartido entre todas las bases de datos.
//
// Archivos que lo componen:
// - SystemDatabases  → lista de bases de datos existentes
// - SystemTables     → lista de tablas por base de datos
// - SystemColumns    → columnas de cada tabla
// - SystemIndexes    → índices asociados a cada tabla
//
// Esta clase es usada internamente por StoredDataManager.
// El Query Processor no la llama directamente — accede a ella a través del SDM.

class SystemCatalog {
public:

    // Constructor: recibe la ruta base donde vive el catálogo en disco
    // Ejemplo: "C:/TinySQLDb/catalog/"
    SystemCatalog(const std::string& catalogPath);

    // ── SystemDatabases ───────────────────────────────────────────────────────

    // Agrega una nueva base de datos al archivo SystemDatabases
    bool addDatabase(const std::string& dbName);

    // Retorna true si la base de datos existe en SystemDatabases
    bool databaseExists(const std::string& dbName);

    // Retorna la lista de todas las bases de datos registradas
    std::vector<std::string> listDatabases();

    // ── SystemTables ──────────────────────────────────────────────────────────

    // Registra una nueva tabla en el archivo SystemTables
    bool addTable(const std::string& dbName, const std::string& tableName);

    // Elimina una tabla del archivo SystemTables
    bool removeTable(const std::string& dbName, const std::string& tableName);

    // Retorna true si la tabla existe en la base de datos dada
    bool tableExists(const std::string& dbName, const std::string& tableName);

    // Retorna la lista de tablas registradas en una base de datos
    std::vector<std::string> listTables(const std::string& dbName);

    // ── SystemColumns ─────────────────────────────────────────────────────────

    // Registra las columnas de una tabla en el archivo SystemColumns
    bool addColumns(const std::string& dbName,
        const std::string& tableName,
        const std::vector<ColumnDefinition>& columns);

    // Elimina las columnas de una tabla del archivo SystemColumns
    bool removeColumns(const std::string& dbName, const std::string& tableName);

    // Retorna las definiciones de columnas de una tabla
    std::vector<ColumnDefinition> listColumns(const std::string& dbName,
        const std::string& tableName);

    // ── SystemIndexes ─────────────────────────────────────────────────────────

    // Registra un nuevo índice en el archivo SystemIndexes
    bool addIndex(const IndexDefinition& index);

    // Elimina un índice del archivo SystemIndexes
    bool removeIndex(const std::string& dbName, const std::string& tableName);

    // Retorna todos los índices registrados para una tabla
    std::vector<IndexDefinition> listIndexes(const std::string& dbName,
        const std::string& tableName);

    // Retorna todos los índices de todas las tablas
    // Se usa al iniciar el servidor para reconstruir los árboles en memoria
    std::vector<IndexDefinition> listAllIndexes();

    // ── Lecturas genéricas para SELECT en tablas del sistema ──────────────────

    // Retorna todas las filas de un archivo del catálogo como ResultSet genérico
    // Nombres válidos: "SystemDatabases", "SystemTables", "SystemColumns", "SystemIndexes"
    // Permite que el Query Processor trate estas tablas como cualquier otra en un SELECT
    ResultSet readAsTable(const std::string& catalogTableName);

private:
    // Ruta base donde se encuentran los archivos del catálogo
    std::string catalogPath_;

    // ── Métodos privados de encriptación ──────────────────────────────────────

    // Encripta o desencripta un bloque de bytes usando XOR con una clave fija
    // XOR es simétrico: aplicarlo dos veces devuelve el dato original
    void xorEncrypt(char* data, int size);
};