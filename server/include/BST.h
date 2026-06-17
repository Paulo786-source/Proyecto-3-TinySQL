#pragma once

#include <memory>
#include <stdexcept>
#include <string>

// arbol binario de busqueda generico en memoria
// mapea claves a offsets de disco para encontrar registros rapido
// usa template para soportar integer, double, varchar y datetime
// no permite claves duplicadas

template<typename K>
class BST {
public:

    // inserta una clave con su offset
    void insert(const K& key, long long offset);

    // busca una clave, retorna -1 si no existe
    long long search(const K& key) const;

    // elimina una clave del arbol
    void remove(const K& key);

    bool empty() const { return root_ == nullptr; }

private:

    struct Node {
        K         key;
        long long offset;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;

        Node(const K& k, long long off)
            : key(k), offset(off), left(nullptr), right(nullptr) {
        }
    };

    std::unique_ptr<Node> root_;

    void insertRec(std::unique_ptr<Node>& node, const K& key, long long offset);
    long long searchRec(const Node* node, const K& key) const;
    std::unique_ptr<Node> removeRec(std::unique_ptr<Node> node, const K& key);
    Node* findMin(Node* node) const;
};

// IMPLEMENTACION

template<typename K>
void BST<K>::insert(const K& key, long long offset) {
    insertRec(root_, key, offset);
}

template<typename K>
void BST<K>::insertRec(std::unique_ptr<Node>& node, const K& key, long long offset) {
    if (node == nullptr) {
        node = std::make_unique<Node>(key, offset);
        return;
    }

    if (key < node->key) {
        insertRec(node->left, key, offset);
    }
    else if (key > node->key) {
        insertRec(node->right, key, offset);
    }
    else {
        // no se permiten duplicados en columnas indexadas
        throw std::runtime_error(
            "valor duplicado en columna indexada: no se permiten duplicados"
        );
    }
}

template<typename K>
long long BST<K>::search(const K& key) const {
    return searchRec(root_.get(), key);
}

template<typename K>
long long BST<K>::searchRec(const Node* node, const K& key) const {
    if (node == nullptr) {
        return -1;
    }

    if (key == node->key) {
        return node->offset;
    }
    else if (key < node->key) {
        return searchRec(node->left.get(), key);
    }
    else {
        return searchRec(node->right.get(), key);
    }
}

template<typename K>
void BST<K>::remove(const K& key) {
    root_ = removeRec(std::move(root_), key);
}

template<typename K>
std::unique_ptr<typename BST<K>::Node>
BST<K>::removeRec(std::unique_ptr<Node> node, const K& key) {
    if (node == nullptr) {
        return nullptr;
    }

    if (key < node->key) {
        node->left = removeRec(std::move(node->left), key);
    }
    else if (key > node->key) {
        node->right = removeRec(std::move(node->right), key);
    }
    else {
        // nodo encontrado

        // sin hijo izquierdo: sube el derecho
        if (node->left == nullptr) {
            return std::move(node->right);
        }

        // sin hijo derecho: sube el izquierdo
        if (node->right == nullptr) {
            return std::move(node->left);
        }

        // dos hijos: reemplazar con el sucesor inorder
        Node* successor = findMin(node->right.get());
        node->key = successor->key;
        node->offset = successor->offset;
        node->right = removeRec(std::move(node->right), successor->key);
    }

    return node;
}

template<typename K>
typename BST<K>::Node* BST<K>::findMin(Node* node) const {
    while (node->left != nullptr) {
        node = node->left.get();
    }
    return node;
}