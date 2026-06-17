#pragma once

#include "storeddatamanager.h"
#include "bst.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <stdexcept>

// interfaz de indice
// todos los indices usan esta interfaz, el QueryProcessor no sabe si es bst o btree
struct IndexHandle {
    virtual ~IndexHandle() = default;

    virtual void insert(const std::string& keyStr, long long offset) = 0;
    virtual long long search(const std::string& keyStr) const = 0;
    virtual void remove(const std::string& keyStr) = 0;
};

// bst handle para integer y datetime
struct BSTHandleInt : public IndexHandle {
    BST<long long> tree;

    void insert(const std::string& keyStr, long long offset) override {
        tree.insert(std::stoll(keyStr), offset);
    }
    long long search(const std::string& keyStr) const override {
        return tree.search(std::stoll(keyStr));
    }
    void remove(const std::string& keyStr) override {
        tree.remove(std::stoll(keyStr));
    }
};

// bst handle para double
struct BSTHandleDouble : public IndexHandle {
    BST<double> tree;

    void insert(const std::string& keyStr, long long offset) override {
        tree.insert(std::stod(keyStr), offset);
    }
    long long search(const std::string& keyStr) const override {
        return tree.search(std::stod(keyStr));
    }
    void remove(const std::string& keyStr) override {
        tree.remove(std::stod(keyStr));
    }
};

// bst handle para varchar
struct BSTHandleStr : public IndexHandle {
    BST<std::string> tree;

    void insert(const std::string& keyStr, long long offset) override {
        tree.insert(keyStr, offset);
    }
    long long search(const std::string& keyStr) const override {
        return tree.search(keyStr);
    }
    void remove(const std::string& keyStr) override {
        tree.remove(keyStr);
    }
};

// btree handles - persona b los implementa
// por ahora son stubs para que compile
struct BTreeHandleInt : public IndexHandle {
    void insert(const std::string&, long long) override {
        throw std::runtime_error("btree no implementado");
    }
    long long search(const std::string&) const override { return -1; }
    void remove(const std::string&) override {}
};

struct BTreeHandleDouble : public IndexHandle {
    void insert(const std::string&, long long) override {
        throw std::runtime_error("btree no implementado");
    }
    long long search(const std::string&) const override { return -1; }
    void remove(const std::string&) override {}
};

struct BTreeHandleStr : public IndexHandle {
    void insert(const std::string&, long long) override {
        throw std::runtime_error("btree no implementado");
    }
    long long search(const std::string&) const override { return -1; }
    void remove(const std::string&) override {}
};

// fabrica de indexhandle
// crea el handle segun el tipo de indice y el tipo de columna
inline std::unique_ptr<IndexHandle> makeIndexHandle(
    const std::string& indexType,
    const std::string& columnType)
{
    bool isBTree = (indexType == "BTREE");

    // datetime se maneja como long long (timestamp)
    if (columnType == "INTEGER" || columnType == "DATETIME") {
        return isBTree
            ? std::unique_ptr<IndexHandle>(std::make_unique<BTreeHandleInt>())
            : std::unique_ptr<IndexHandle>(std::make_unique<BSTHandleInt>());
    }
    if (columnType == "DOUBLE") {
        return isBTree
            ? std::unique_ptr<IndexHandle>(std::make_unique<BTreeHandleDouble>())
            : std::unique_ptr<IndexHandle>(std::make_unique<BSTHandleDouble>());
    }
    // varchar y cualquier otro tipo
    return isBTree
        ? std::unique_ptr<IndexHandle>(std::make_unique<BTreeHandleStr>())
        : std::unique_ptr<IndexHandle>(std::make_unique<BSTHandleStr>());
}

// indexmanager
// mantiene todos los indices en memoria
class IndexManager {
public:

    // clave compuesta "db/tabla/columna" para el mapa
    static std::string makeKey(const std::string& db,
        const std::string& table,
        const std::string& column)
    {
        return db + "/" + table + "/" + column;
    }

    // agrega un indice al mapa
    void add(const std::string& db, const std::string& table,
        const std::string& column, std::unique_ptr<IndexHandle> handle)
    {
        indexes_[makeKey(db, table, column)] = std::move(handle);
    }

    // obtiene un indice, retorna nullptr si no existe
    IndexHandle* get(const std::string& db, const std::string& table,
        const std::string& column) const
    {
        auto it = indexes_.find(makeKey(db, table, column));
        return (it != indexes_.end()) ? it->second.get() : nullptr;
    }

    // elimina un indice del mapa
    void remove(const std::string& db, const std::string& table,
        const std::string& column)
    {
        indexes_.erase(makeKey(db, table, column));
    }

    // verifica si una tabla tiene algun indice
    bool hasAnyIndex(const std::string& db, const std::string& table) const {
        std::string prefix = db + "/" + table + "/";
        for (const auto& pair : indexes_) {
            if (pair.first.substr(0, prefix.size()) == prefix) {
                return true;
            }
        }
        return false;
    }

    // retorna la columna indexada de una tabla (solo un indice por tabla)
    std::string getIndexedColumn(const std::string& db,
        const std::string& table) const
    {
        std::string prefix = db + "/" + table + "/";
        for (const auto& pair : indexes_) {
            if (pair.first.substr(0, prefix.size()) == prefix) {
                return pair.first.substr(prefix.size());
            }
        }
        return "";
    }

private:
    // mapa de clave compuesta al handle del arbol
    std::unordered_map<std::string, std::unique_ptr<IndexHandle>> indexes_;
};