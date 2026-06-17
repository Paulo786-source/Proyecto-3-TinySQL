#pragma once

#include <string>
#include <vector>
#include "storeddatamanager.h"

// estructuras de datos

// condicion WHERE para consultas sql
// ejemplo: WHERE id = 2 → column="id", op="=", value="2"
struct WhereClause {
    std::string column;
    std::string op;        // =, >, <, LIKE, NOT
    std::string value;
};

// asignacion para UPDATE
// ejemplo: SET nombre = "felipe" → column="nombre", value="felipe"
struct SetClause {
    std::string column;
    std::string value;
};

// nodo del arbol sintactico (AST)
// el query processor parsea el sql y produce uno de estos nodos
struct ASTNode {
    std::string type;      // CREATE_DB, SET_DB, CREATE_TABLE, DROP_TABLE,
    // INSERT, SELECT, UPDATE, DELETE, CREATE_INDEX

    std::string database;
    std::string table;

    // para CREATE TABLE
    std::vector<ColumnDefinition> columns;

    // para INSERT
    std::vector<std::string> values;

    // para SELECT y DELETE
    WhereClause where;
    bool hasWhere;

    // para SELECT
    std::vector<std::string> selectColumns;  // * para todas
    std::string orderByColumn;
    std::string orderByDirection;            // ASC o DESC
    bool hasOrderBy;

    // para UPDATE
    std::vector<SetClause> setClauses;

    // para CREATE INDEX
    std::string indexName;
    std::string indexColumn;
    std::string indexType;   // BTREE o BST
};

// resultado de ejecutar una sentencia sql
struct QueryResult {
    std::vector<std::string> columns;
    ResultSet rows;
    long long time_ms;
    std::string error;       // vacio si no hay error
    bool success;
};

// INTERFAZ QUERYPROCESSOR
// recibe sql en texto desde el web api, lo parsea, valida y ejecuta.
// es el cerebro del sistema: entiende sql pero no sabe de archivos binarios.

class QueryProcessor {
public:

    // constructor: recibe referencia al sdm
    QueryProcessor(StoredDataManager& sdm);

    // METODO PRINCIPAL

    // recibe sql y el contexto de base de datos activa
    // parsea, valida y ejecuta la sentencia
    // retorna el resultado listo para serializar a json
    QueryResult execute(const std::string& sql, const std::string& dbContext);

    // PARSER

    // convierte sql en texto a un ASTNode
    // lanza excepcion si la sintaxis es incorrecta
    ASTNode parse(const std::string& sql);

    // VALIDADOR

    // valida semanticamente el ASTNode:
    // - que la base de datos exista
    // - que la tabla exista
    // - que las columnas sean validas
    // - que los tipos de datos sean correctos
    // retorna string vacio si todo esta bien
    std::string validate(const ASTNode& node, const std::string& dbContext);

    // EJECUTORES POR TIPO DE SENTENCIA

    QueryResult executeCreateDatabase(const ASTNode& node);
    QueryResult executeSetDatabase(const ASTNode& node);
    QueryResult executeCreateTable(const ASTNode& node, const std::string& dbContext);
    QueryResult executeDropTable(const ASTNode& node, const std::string& dbContext);
    QueryResult executeInsert(const ASTNode& node, const std::string& dbContext);
    QueryResult executeSelect(const ASTNode& node, const std::string& dbContext);
    QueryResult executeUpdate(const ASTNode& node, const std::string& dbContext);
    QueryResult executeDelete(const ASTNode& node, const std::string& dbContext);
    QueryResult executeCreateIndex(const ASTNode& node, const std::string& dbContext);

    // RECARGA DE INDICES AL INICIAR

    // lee los indices del catalogo y reconstruye los arboles en memoria
    // se llama una vez al arrancar el servidor
    void loadIndexes();

private:
    StoredDataManager& sdm_;
    std::string currentDatabase_;
};