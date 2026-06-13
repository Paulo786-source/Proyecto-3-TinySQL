#pragma once

#include <string>
#include <vector>
#include "StoredDataManager.h"

// ─── Estructuras de datos ──────────────────────────────────────────────────────

// Representa una condición WHERE en una consulta SQL
// Ejemplo: WHERE ID = 2 → column="ID", op="=", value="2"
struct WhereClause {
    std::string column;     // Nombre de la columna a evaluar
    std::string op;         // Operador: "=", ">", "<", "LIKE", "NOT"
    std::string value;      // Valor con el que se compara
};

// Representa una asignación en una sentencia UPDATE
// Ejemplo: SET Nombre = "Felipe" → column="Nombre", value="Felipe"
struct SetClause {
    std::string column;     // Columna a actualizar
    std::string value;      // Nuevo valor
};

// Nodo del árbol sintáctico (AST) que representa cualquier sentencia SQL
// El Query Processor parsea el SQL y produce uno de estos nodos
struct ASTNode {
    std::string type;       // Tipo de sentencia: "CREATE_DB", "SET_DB", "CREATE_TABLE",
    // "DROP_TABLE", "INSERT", "SELECT", "UPDATE", "DELETE",
    // "CREATE_INDEX"

    std::string database;   // Base de datos involucrada (cuando aplica)
    std::string table;      // Tabla involucrada (cuando aplica)

    // Para CREATE TABLE
    std::vector<ColumnDefinition> columns;

    // Para INSERT
    std::vector<std::string> values;

    // Para SELECT y DELETE
    WhereClause where;
    bool hasWhere;          // Indica si la sentencia tiene cláusula WHERE

    // Para SELECT
    std::vector<std::string> selectColumns;  // Columnas a retornar (* = todas)
    std::string orderByColumn;               // Columna para ORDER BY
    std::string orderByDirection;            // "ASC" o "DESC"
    bool hasOrderBy;                         // Indica si tiene ORDER BY

    // Para UPDATE
    std::vector<SetClause> setClauses;

    // Para CREATE INDEX
    std::string indexName;      // Nombre del índice
    std::string indexColumn;    // Columna sobre la que se crea el índice
    std::string indexType;      // "BTREE" o "BST"
};

// Representa el resultado de ejecutar una sentencia SQL
struct QueryResult {
    std::vector<std::string> columns;   // Nombres de las columnas del resultado
    ResultSet rows;                     // Filas del resultado
    long long time_ms;                  // Tiempo de ejecución en milisegundos
    std::string error;                  // Mensaje de error (vacío si no hay error)
    bool success;                       // Indica si la sentencia se ejecutó con éxito
};

// ─── Interfaz QueryProcessor ──────────────────────────────────────────────────

// Esta capa recibe sentencias SQL en texto plano desde el Web API,
// las parsea, las valida y coordina su ejecución con el StoredDataManager.
// Es el cerebro del sistema: conoce SQL pero no sabe nada de archivos binarios.

class QueryProcessor {
public:

    // Constructor: recibe una referencia al SDM para poder llamar sus métodos
    QueryProcessor(StoredDataManager& sdm);

    // ── Método principal ─────────────────────────────────────────────────────

    // Recibe una sentencia SQL y el contexto de base de datos activa
    // Parsea, valida y ejecuta la sentencia
    // Retorna el resultado listo para que el Web API lo serialice a JSON
    QueryResult execute(const std::string& sql, const std::string& dbContext);

    // ── Parser ───────────────────────────────────────────────────────────────

    // Convierte una sentencia SQL en texto a un ASTNode
    // Lanza un error descriptivo si la sintaxis es incorrecta
    ASTNode parse(const std::string& sql);

    // ── Validador ────────────────────────────────────────────────────────────

    // Valida semánticamente el ASTNode:
    // - Que la base de datos exista
    // - Que la tabla exista
    // - Que las columnas sean válidas
    // - Que los tipos de datos sean correctos
    // Retorna un mensaje de error vacío si todo está bien
    std::string validate(const ASTNode& node, const std::string& dbContext);

    // ── Ejecutores por tipo de sentencia ─────────────────────────────────────

    // Crea una nueva base de datos
    QueryResult executeCreateDatabase(const ASTNode& node);

    // Valida que la base de datos exista y retorna confirmación
    QueryResult executeSetDatabase(const ASTNode& node);

    // Crea una nueva tabla con el esquema especificado
    QueryResult executeCreateTable(const ASTNode& node, const std::string& dbContext);

    // Elimina una tabla si está vacía
    QueryResult executeDropTable(const ASTNode& node, const std::string& dbContext);

    // Inserta una fila en la tabla y actualiza los índices asociados
    QueryResult executeInsert(const ASTNode& node, const std::string& dbContext);

    // Consulta filas usando índice si existe, o búsqueda secuencial si no
    // Aplica ORDER BY con Quicksort si se especifica
    QueryResult executeSelect(const ASTNode& node, const std::string& dbContext);

    // Actualiza filas que cumplan la condición WHERE y actualiza índices
    QueryResult executeUpdate(const ASTNode& node, const std::string& dbContext);

    // Elimina filas que cumplan la condición WHERE y actualiza índices
    QueryResult executeDelete(const ASTNode& node, const std::string& dbContext);

    // Crea un índice BST o BTREE sobre una columna de la tabla
    QueryResult executeCreateIndex(const ASTNode& node, const std::string& dbContext);

    // ── Recarga de índices al iniciar el servidor ────────────────────────────

    // Lee los índices registrados en el catálogo y reconstruye los árboles en memoria
    // Se llama una sola vez cuando el servidor arranca
    void loadIndexes();

private:
    // Referencia al StoredDataManager para acceder a los archivos
    StoredDataManager& sdm_;

    // Contexto de base de datos activa (se actualiza con SET DATABASE)
    std::string currentDatabase_;
};