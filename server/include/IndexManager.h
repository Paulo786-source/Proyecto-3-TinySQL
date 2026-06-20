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

// ─── B-TREE ───────────────────────────────────────────────────────────────────
// Implementacion de B-Tree de orden t (cada nodo tiene entre t-1 y 2t-1 claves)
// Cada clave mapea a un offset en disco donde vive el registro

template<typename K>
class BTree {
public:
    static constexpr int T = 3; // orden del arbol: minimo 2, tipicamente 3-5

    struct Node {
        std::vector<K>         keys;      // claves almacenadas en este nodo
        std::vector<long long> offsets;   // offset en disco para cada clave
        std::vector<std::unique_ptr<Node>> children; // hijos del nodo
        bool leaf;                        // true si es hoja (no tiene hijos)

        Node(bool isLeaf) : leaf(isLeaf) {}
    };

    BTree() : root_(std::make_unique<Node>(true)) {}

    // inserta una clave con su offset en el arbol
    void insert(const K& key, long long offset) {
        // si la raiz esta llena, hay que dividirla
        if (static_cast<int>(root_->keys.size()) == 2 * T - 1) {
            auto newRoot = std::make_unique<Node>(false);
            newRoot->children.push_back(std::move(root_));
            splitChild(*newRoot, 0);
            root_ = std::move(newRoot);
        }
        insertNonFull(*root_, key, offset);
    }

    // busca una clave y retorna su offset, o -1 si no existe
    long long search(const K& key) const {
        return searchNode(*root_, key);
    }

    // elimina una clave del arbol
    void remove(const K& key) {
        removeFromNode(*root_, key);
        // si la raiz quedo vacia y tiene un hijo, ese hijo es la nueva raiz
        if (root_->keys.empty() && !root_->children.empty()) {
            root_ = std::move(root_->children[0]);
        }
    }

private:
    std::unique_ptr<Node> root_;

    // busca recursivamente en un nodo
    long long searchNode(const Node& node, const K& key) const {
        int i = 0;
        // avanza mientras la clave sea mayor que keys[i]
        while (i < static_cast<int>(node.keys.size()) && key > node.keys[i])
            ++i;

        // encontro la clave exacta
        if (i < static_cast<int>(node.keys.size()) && key == node.keys[i])
            return node.offsets[i];

        // si es hoja y no encontro, no existe
        if (node.leaf) return -1;

        // busca en el hijo correspondiente
        return searchNode(*node.children[i], key);
    }

    // inserta en un nodo que garantizamos no esta lleno
    void insertNonFull(Node& node, const K& key, long long offset) {
        int i = static_cast<int>(node.keys.size()) - 1;

        if (node.leaf) {
            // inserta en la posicion correcta manteniendo orden
            node.keys.push_back(K{});
            node.offsets.push_back(0);
            while (i >= 0 && key < node.keys[i]) {
                node.keys[i + 1] = node.keys[i];
                node.offsets[i + 1] = node.offsets[i];
                --i;
            }
            node.keys[i + 1] = key;
            node.offsets[i + 1] = offset;
        }
        else {
            // encuentra el hijo donde debe ir la clave
            while (i >= 0 && key < node.keys[i]) --i;
            ++i;

            // si el hijo esta lleno, lo divide primero
            if (static_cast<int>(node.children[i]->keys.size()) == 2 * T - 1) {
                splitChild(node, i);
                // despues de dividir, determina en cual de los dos hijos insertar
                if (key > node.keys[i]) ++i;
            }
            insertNonFull(*node.children[i], key, offset);
        }
    }

    // divide el hijo i-esimo de parent (que debe estar lleno)
    void splitChild(Node& parent, int i) {
        Node& fullChild = *parent.children[i];
        auto newChild = std::make_unique<Node>(fullChild.leaf);

        // la clave del medio sube al padre
        K   midKey = fullChild.keys[T - 1];
        long long midOff = fullChild.offsets[T - 1];

        // las claves de la derecha van al nuevo hijo
        for (int j = T; j < 2 * T - 1; ++j) {
            newChild->keys.push_back(fullChild.keys[j]);
            newChild->offsets.push_back(fullChild.offsets[j]);
        }

        // los hijos de la derecha van al nuevo hijo (si no es hoja)
        if (!fullChild.leaf) {
            for (int j = T; j < 2 * T; ++j)
                newChild->children.push_back(std::move(fullChild.children[j]));
            fullChild.children.resize(T);
        }

        // recorta el hijo original
        fullChild.keys.resize(T - 1);
        fullChild.offsets.resize(T - 1);

        // inserta la clave del medio en el padre
        parent.keys.insert(parent.keys.begin() + i, midKey);
        parent.offsets.insert(parent.offsets.begin() + i, midOff);
        parent.children.insert(parent.children.begin() + i + 1,
            std::move(newChild));
    }

    // elimina una clave del nodo o de sus descendientes
    void removeFromNode(Node& node, const K& key) {
        int i = 0;
        while (i < static_cast<int>(node.keys.size()) && key > node.keys[i])
            ++i;

        if (i < static_cast<int>(node.keys.size()) && key == node.keys[i]) {
            // caso 1: la clave esta en este nodo
            if (node.leaf) {
                // caso 1a: es hoja, simplemente elimina
                node.keys.erase(node.keys.begin() + i);
                node.offsets.erase(node.offsets.begin() + i);
            }
            else {
                // caso 1b: nodo interno
                if (static_cast<int>(node.children[i]->keys.size()) >= T) {
                    // reemplaza con el predecesor (mayor clave del hijo izquierdo)
                    auto [predKey, predOff] = getPredecessor(*node.children[i]);
                    node.keys[i] = predKey;
                    node.offsets[i] = predOff;
                    removeFromNode(*node.children[i], predKey);
                }
                else if (static_cast<int>(node.children[i + 1]->keys.size()) >= T) {
                    // reemplaza con el sucesor (menor clave del hijo derecho)
                    auto [succKey, succOff] = getSuccessor(*node.children[i + 1]);
                    node.keys[i] = succKey;
                    node.offsets[i] = succOff;
                    removeFromNode(*node.children[i + 1], succKey);
                }
                else {
                    // fusiona los dos hijos
                    mergeChildren(node, i);
                    removeFromNode(*node.children[i], key);
                }
            }
        }
        else if (!node.leaf) {
            // caso 2: la clave no esta en este nodo, busca en el hijo
            bool lastChild = (i == static_cast<int>(node.children.size()) - 1);

            // si el hijo tiene menos de t claves, lo rellena primero
            if (static_cast<int>(node.children[i]->keys.size()) < T) {
                fillChild(node, i);
                // fillChild puede haber cambiado los indices
                if (lastChild && i > static_cast<int>(node.keys.size()))
                    --i;
            }
            removeFromNode(*node.children[i], key);
        }
    }

    // retorna la clave y offset del predecesor (la clave mas grande del subarbol)
    std::pair<K, long long> getPredecessor(const Node& node) const {
        const Node* cur = &node;
        while (!cur->leaf)
            cur = cur->children.back().get();
        return { cur->keys.back(), cur->offsets.back() };
    }

    // retorna la clave y offset del sucesor (la clave mas pequena del subarbol)
    std::pair<K, long long> getSuccessor(const Node& node) const {
        const Node* cur = &node;
        while (!cur->leaf)
            cur = cur->children.front().get();
        return { cur->keys.front(), cur->offsets.front() };
    }

    // fusiona el hijo i+1 en el hijo i, bajando keys[i] del padre
    void mergeChildren(Node& parent, int i) {
        Node& left = *parent.children[i];
        Node& right = *parent.children[i + 1];

        // baja la clave del padre al hijo izquierdo
        left.keys.push_back(parent.keys[i]);
        left.offsets.push_back(parent.offsets[i]);

        // mueve todas las claves e hijos del hijo derecho al izquierdo
        for (auto& k : right.keys)    left.keys.push_back(k);
        for (auto& o : right.offsets) left.offsets.push_back(o);
        for (auto& c : right.children)
            left.children.push_back(std::move(c));

        // elimina la clave del padre y el hijo derecho
        parent.keys.erase(parent.keys.begin() + i);
        parent.offsets.erase(parent.offsets.begin() + i);
        parent.children.erase(parent.children.begin() + i + 1);
    }

    // garantiza que el hijo i tenga al menos t claves
    void fillChild(Node& parent, int i) {
        if (i > 0 &&
            static_cast<int>(parent.children[i - 1]->keys.size()) >= T) {
            // roba una clave del hermano izquierdo
            borrowFromLeft(parent, i);
        }
        else if (i < static_cast<int>(parent.children.size()) - 1 &&
            static_cast<int>(parent.children[i + 1]->keys.size()) >= T) {
            // roba una clave del hermano derecho
            borrowFromRight(parent, i);
        }
        else {
            // fusiona con un hermano
            if (i < static_cast<int>(parent.children.size()) - 1)
                mergeChildren(parent, i);
            else
                mergeChildren(parent, i - 1);
        }
    }

    // roba la clave mas grande del hermano izquierdo
    void borrowFromLeft(Node& parent, int i) {
        Node& child = *parent.children[i];
        Node& left = *parent.children[i - 1];

        // baja la clave del padre al inicio del hijo
        child.keys.insert(child.keys.begin(), parent.keys[i - 1]);
        child.offsets.insert(child.offsets.begin(), parent.offsets[i - 1]);

        // si no es hoja, mueve el ultimo hijo del hermano izquierdo
        if (!child.leaf)
            child.children.insert(child.children.begin(),
                std::move(left.children.back()));

        // sube la ultima clave del hermano izquierdo al padre
        parent.keys[i - 1] = left.keys.back();
        parent.offsets[i - 1] = left.offsets.back();
        left.keys.pop_back();
        left.offsets.pop_back();
        if (!left.leaf) left.children.pop_back();
    }

    // roba la clave mas pequena del hermano derecho
    void borrowFromRight(Node& parent, int i) {
        Node& child = *parent.children[i];
        Node& right = *parent.children[i + 1];

        // baja la clave del padre al final del hijo
        child.keys.push_back(parent.keys[i]);
        child.offsets.push_back(parent.offsets[i]);

        // si no es hoja, mueve el primer hijo del hermano derecho
        if (!child.leaf)
            child.children.push_back(std::move(right.children.front()));

        // sube la primera clave del hermano derecho al padre
        parent.keys[i] = right.keys.front();
        parent.offsets[i] = right.offsets.front();
        right.keys.erase(right.keys.begin());
        right.offsets.erase(right.offsets.begin());
        if (!right.leaf)
            right.children.erase(right.children.begin());
    }
};

// ─── BTREE HANDLES (implementaciones del Strategy pattern) ────────────────────

class BTreeHandleInt : public IndexHandle {
    BTree<long long> tree_;
public:
    void insert(const std::string& key, long long offset) override {
        long long k = 0;
        try { k = std::stoll(key); }
        catch (...) {}
        tree_.insert(k, offset);
    }
    long long search(const std::string& key) const override {
        long long k = 0;
        try { k = std::stoll(key); }
        catch (...) {}
        return tree_.search(k);
    }
    void remove(const std::string& key) override {
        long long k = 0;
        try { k = std::stoll(key); }
        catch (...) {}
        tree_.remove(k);
    }
};

class BTreeHandleDouble : public IndexHandle {
    BTree<double> tree_;
public:
    void insert(const std::string& key, long long offset) override {
        double k = 0.0;
        try { k = std::stod(key); }
        catch (...) {}
        tree_.insert(k, offset);
    }
    long long search(const std::string& key) const override {
        double k = 0.0;
        try { k = std::stod(key); }
        catch (...) {}
        return tree_.search(k);
    }
    void remove(const std::string& key) override {
        double k = 0.0;
        try { k = std::stod(key); }
        catch (...) {}
        tree_.remove(k);
    }
};

class BTreeHandleStr : public IndexHandle {
    BTree<std::string> tree_;
public:
    void insert(const std::string& key, long long offset) override {
        tree_.insert(key, offset);
    }
    long long search(const std::string& key) const override {
        return tree_.search(key);
    }
    void remove(const std::string& key) override {
        tree_.remove(key);
    }
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