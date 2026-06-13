# TinySQLDb

Motor de bases de datos relacional sencillo implementado en C++ con un cliente web en React.

Desarrollado para CE2103 - Algoritmos y Estructuras de Datos II | I Semestre 2026
Instituto Tecnológico de Costa Rica

---

## Autores

- Paulo Centeno Flores
- Dilana Gamboa Gonzales

---

## Estructura del proyecto

TinySQLDb/
├── server/                  # Servidor en C++
│   ├── webapi/              # Capa HTTP (cpp-httplib)
│   ├── queryprocessor/      # Procesamiento y validación de SQL
│   ├── storeddatamanager/   # Manejo de archivos binarios y encriptación
│   ├── include/             # Headers compartidos
│   └── CMakeLists.txt
├── client/                  # Cliente web en React (Vite)
├── docs/                    # Documentación técnica
└── scripts/                 # Scripts de prueba

---

## Requisitos

### Servidor (C++)
- CMake 3.20 o superior
- Visual Studio 2022 (Windows)
- Sin dependencias adicionales — cpp-httplib está incluido en /server/include

### Cliente (React)
- Node.js LTS — descargar desde https://nodejs.org

---

## Cómo correr el proyecto

### 1. Clonar el repositorio

git clone https://github.com/TU_USUARIO/TinySQLDb.git
cd TinySQLDb

### 2. Compilar y correr el servidor

cd server
mkdir build
cd build
cmake ..
cmake --build .
.\Debug\webapi.exe

El servidor corre en http://localhost:8080

### 3. Instalar y correr el cliente

cd client
npm install
npm run dev

El cliente corre en http://localhost:5173

---

## Cómo usar el sistema

1. Abrir http://localhost:5173 en el navegador
2. Escribir una consulta SQL en el editor
3. Hacer click en Ejecutar para enviarla al servidor
4. Los resultados se muestran como tabla debajo del editor

---

## Sentencias SQL soportadas

| Sentencia | Descripción |
|---|---|
| CREATE DATABASE nombre | Crea una nueva base de datos |
| SET DATABASE nombre | Establece el contexto de base de datos activa |
| CREATE TABLE nombre (...) | Crea una nueva tabla |
| DROP TABLE nombre | Elimina una tabla vacía |
| INSERT INTO nombre VALUES(...) | Inserta una fila |
| SELECT * FROM nombre [WHERE ...] [ORDER BY ...] | Consulta filas |
| UPDATE nombre SET ... [WHERE ...] | Actualiza filas |
| DELETE FROM nombre [WHERE ...] | Elimina filas |
| CREATE INDEX nombre ON tabla(col) OF TYPE BTREE o BST | Crea un índice |

---

## Arquitectura

[Cliente React]
      │  HTTP POST /query (JSON)
      ▼
[Web API — C++]
      │  Sentencia SQL
      ▼
[Query Processor — C++]
      │  Consulta validada
      ▼
[Stored Data Manager — C++]
      │  Lectura/escritura de archivos binarios
      ▼
[Archivos binarios encriptados en disco]
