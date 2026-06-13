#pragma once

#include <string>
#include <vector>

// ─── Estructuras de datos ──────────────────────────────────────────────────────

// Representa la definición de una columna en el esquema de una tabla
struct ColumnDefinition {
    std::string name;       // Nombre de la columna (ej. "ID", "Nombre")
    std::string type;       // Tipo de dato: "INTEGER", "DOUBLE", "VARCHAR", "DATETIME"
    int size;               // Solo relevante para VARCHAR(n), de lo contrario 0
    bool nullable;          // Indica si la columna acepta valores NULL
};

// Representa una fila de datos como una lista de valores en string
// Cada valor corresponde a una columna en el mismo orden que el esquema
using Row = std::vector<std::string>;

// Representa un conjunto de resultados (múltiples filas)
using ResultSet = std::vector<Row>;

// Representa la definición de un índice en el catálogo del sistema
struct IndexDefinition {
    std::string indexName;   // Nombre del índice (ej. "Estudiante_Id")
    std::string tableName;   // Tabla a la que pertenece el índice
    std::string columnName;  // Columna sobre la que se construye el índice
    std::string type;        // "BTREE" o "BST"
    std::string database;    // Base de datos a la que pertenece la tabla
};

// ─── Interfaz StoredDataManager ───────────────────────────────────────────────

// Esta capa es responsable de todo el I/O de archivos binarios.
// No conoce nada sobre sintaxis SQL — solo lee y escribe datos.
// Todos los archivos se almacenan en formato binario y encriptados con XOR.

class StoredDataManager {
public:

    // ── Operaciones de base de datos ─────────────────────────────────────────

    // Crea una nueva carpeta en disco para la base de datos y la registra
    // en el archivo de catálogo SystemDatabases
    bool createDatabase(const std::string& dbName);

    // Retorna true si existe una base de datos con el nombre dado
    bool databaseExists(const std::string& dbName);

    // Retorna la lista de nombres de todas las bases de datos existentes
    std::vector<std::string> listDatabases();

    // ── Operaciones de tabla ─────────────────────────────────────────────────

    // Crea un archivo binario para la tabla con un encabezado que contiene el esquema
    // También registra la tabla y sus columnas en el catálogo del sistema
    bool createTableFile(const std::string& dbName,
        const std::string& tableName,
        const std::vector<ColumnDefinition>& columns);

    // Elimina el archivo de la tabla del disco y lo remueve del catálogo del sistema
    // Solo tiene éxito si la tabla está vacía
    bool dropTable(const std::string& dbName, const std::string& tableName);

    // Retorna true si la tabla existe en la base de datos dada
    bool tableExists(const std::string& dbName, const std::string& tableName);

    // Retorna true si la tabla no tiene registros activos
    bool isTableEmpty(const std::string& dbName, const std::string& tableName);

    // Retorna las definiciones de columnas para una tabla dada
    std::vector<ColumnDefinition> getTableSchema(const std::string& dbName,
        const std::string& tableName);

    // Retorna la lista de nombres de todas las tablas en una base de datos
    std::vector<std::string> listTables(const std::string& dbName);

    // ── Operaciones de registros ─────────────────────────────────────────────

    // Agrega un nuevo registro al final del archivo de la tabla
    // Retorna el offset en bytes donde se escribió el registro
    // Este offset es usado por el índice para localizar el registro rápidamente
    long long appendRecord(const std::string& dbName,
        const std::string& tableName,
        const Row& values);

    // Lee el registro en el offset de bytes dado en el archivo de la tabla
    Row readRecord(const std::string& dbName,
        const std::string& tableName,
        long long offset);

    // Lee todos los registros activos (no eliminados) del archivo de la tabla
    ResultSet readAllRecords(const std::string& dbName,
        const std::string& tableName);

    // Sobreescribe el registro en el offset dado con nuevos valores
    bool updateRecord(const std::string& dbName,
        const std::string& tableName,
        long long offset,
        const Row& newValues);

    // Marca el registro en el offset dado como eliminado usando un flag de 1 byte
    // El registro no se elimina físicamente — simplemente se ignora en lecturas futuras
    bool deleteRecord(const std::string& dbName,
        const std::string& tableName,
        long long offset);

    // ── Operaciones de catálogo de índices ───────────────────────────────────

    // Registra un nuevo índice en el archivo de catálogo SystemIndexes
    bool addIndex(const IndexDefinition& index);

    // Remueve un índice del archivo de catálogo SystemIndexes
    bool removeIndex(const std::string& dbName,
        const std::string& tableName);

    // Retorna todos los índices registrados para una tabla dada
    std::vector<IndexDefinition> listIndexes(const std::string& dbName,
        const std::string& tableName);

    // Lee todas las definiciones de índices del catálogo al iniciar el servidor
    // El Query Processor usa esta lista para reconstruir los árboles en memoria
    std::vector<IndexDefinition> loadIndexesOnStartup();

    // ── Lecturas del catálogo del sistema (para SELECT en tablas del sistema) ─

    // Retorna todas las filas de una tabla del catálogo del sistema por nombre
    // Nombres válidos: "SystemDatabases", "SystemTables", "SystemColumns", "SystemIndexes"
    ResultSet readCatalogTable(const std::string& catalogTableName);
};