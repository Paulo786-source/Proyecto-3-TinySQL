#pragma once

#include <string>
#include <vector>

// estructuras de datos

// definicion de una columna en el esquema de una tabla
struct ColumnDefinition {
    std::string name;
    std::string type;     // integer, double, varchar, datetime
    int size;             // solo para varchar(n), 0 para los demas
    bool nullable;
};

// una fila como lista de strings
using Row = std::vector<std::string>;

// conjunto de resultados (varias filas)
using ResultSet = std::vector<Row>;

// definicion de un indice
struct IndexDefinition {
    std::string indexName;
    std::string tableName;
    std::string columnName;
    std::string type;      // btree o bst
    std::string database;
};

// INTERFAZ STOREDDATAMANAGER
// esta capa maneja todo el i/o de archivos binarios.
// no sabe nada de sql - solo lee y escribe datos.

class StoredDataManager {
public:

    // OPERACIONES DE BASE DE DATOS

    // crea la carpeta de la base de datos y la registra en systemdatabases
    bool createDatabase(const std::string& dbName);

    // retorna true si la base de datos existe
    bool databaseExists(const std::string& dbName);

    // retorna la lista de todas las bases de datos
    std::vector<std::string> listDatabases();

    // OPERACIONES DE TABLA

    // crea el archivo binario de la tabla con su esquema
    // tambien la registra en el catalogo del sistema
    bool createTableFile(const std::string& dbName,
        const std::string& tableName,
        const std::vector<ColumnDefinition>& columns);

    // elimina el archivo de la tabla y la remueve del catalogo
    // solo funciona si la tabla esta vacia
    bool dropTable(const std::string& dbName, const std::string& tableName);

    // retorna true si la tabla existe
    bool tableExists(const std::string& dbName, const std::string& tableName);

    // retorna true si la tabla no tiene registros activos
    bool isTableEmpty(const std::string& dbName, const std::string& tableName);

    // retorna el esquema de una tabla
    std::vector<ColumnDefinition> getTableSchema(const std::string& dbName,
        const std::string& tableName);

    // retorna la lista de tablas en una base de datos
    std::vector<std::string> listTables(const std::string& dbName);

    // OPERACIONES DE REGISTROS

    // agrega un registro al final del archivo
    // retorna el offset en bytes donde se escribio
    long long appendRecord(const std::string& dbName,
        const std::string& tableName,
        const Row& values);

    // lee el registro en la posicion offset
    Row readRecord(const std::string& dbName,
        const std::string& tableName,
        long long offset);

    // lee todos los registros activos (no eliminados)
    ResultSet readAllRecords(const std::string& dbName,
        const std::string& tableName);

    // lee todos los registros activos con sus offsets en disco
    // usado por UPDATE y DELETE sin indice para localizar registros
    std::vector<std::pair<long long, Row>> readAllRecordsWithOffsets(
        const std::string& dbName,
        const std::string& tableName);

    // sobrescribe el registro en la posicion offset
    bool updateRecord(const std::string& dbName,
        const std::string& tableName,
        long long offset,
        const Row& newValues);

    // marca el registro como eliminado (soft-delete)
    bool deleteRecord(const std::string& dbName,
        const std::string& tableName,
        long long offset);

    // OPERACIONES DE INDICES

    // registra un indice en systemindexes
    bool addIndex(const IndexDefinition& index);

    // remueve un indice de systemindexes
    bool removeIndex(const std::string& dbName,
        const std::string& tableName);

    // retorna los indices de una tabla
    std::vector<IndexDefinition> listIndexes(const std::string& dbName,
        const std::string& tableName);

    // carga todos los indices al iniciar el servidor
    std::vector<IndexDefinition> loadIndexesOnStartup();

    // LECTURA DEL CATALOGO (para select en tablas del sistema)

    // retorna el contenido de una tabla del catalogo como resultset
    // nombres validos: systemdatabases, systemtables, systemcolumns, systemindexes
    ResultSet readCatalogTable(const std::string& catalogTableName);
};