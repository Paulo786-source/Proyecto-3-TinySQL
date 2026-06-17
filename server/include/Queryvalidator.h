#pragma once

#include "queryprocessor.h"
#include "storeddatamanager.h"
#include "indexmanager.h"
#include "typesystem.h"
#include <string>
#include <set>

// valida semanticamente un ASTNode
// verifica: bd existe, tabla existe, columnas validas, tipos correctos
class QueryValidator {
public:

    QueryValidator(StoredDataManager& sdm, const IndexManager& indexMgr);

    // retorna string vacio si es valido, o mensaje de error
    std::string validate(const ASTNode& node, const std::string& dbContext) const;

private:

    StoredDataManager& sdm_;
    const IndexManager& indexMgr_;

    static const std::set<std::string> VALID_TYPES;
    static const std::set<std::string> CATALOG_TABLES;

    bool columnExists(const std::string& colName,
        const std::vector<ColumnDefinition>& schema) const;

    ColumnDefinition findColumn(const std::string& colName,
        const std::vector<ColumnDefinition>& schema) const;

    std::string requireDbContext(const std::string& db) const;
    std::string requireDbExists(const std::string& db) const;
    std::string requireTableExists(const std::string& db,
        const std::string& table) const;

    // validaciones por tipo
    std::string validateCreateDb(const ASTNode& node) const;
    std::string validateSetDb(const ASTNode& node) const;
    std::string validateCreateTable(const ASTNode& node, const std::string& db) const;
    std::string validateDropTable(const ASTNode& node, const std::string& db) const;
    std::string validateInsert(const ASTNode& node, const std::string& db) const;
    std::string validateSelect(const ASTNode& node, const std::string& db) const;
    std::string validateUpdate(const ASTNode& node, const std::string& db) const;
    std::string validateDelete(const ASTNode& node, const std::string& db) const;
    std::string validateCreateIndex(const ASTNode& node, const std::string& db) const;
};