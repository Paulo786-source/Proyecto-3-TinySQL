-- =========================================================
-- test_indexes.sql  |  Indices BTREE y BST
-- =========================================================
-- Como ejecutar: copia UN bloque a la vez al textarea y ejecutalo.
-- Los comentarios (lineas con --) no los copies; son solo guia.
-- =========================================================


-- ---------------------------------------------------------
-- PARTE 1: Crear tabla Producto e insertar datos sin indice.
-- Resultado esperado: 3 filas visibles; aun no hay indice.
-- ---------------------------------------------------------

SET DATABASE TestDB;
CREATE TABLE Producto (ID INTEGER, Nombre VARCHAR(50), Precio DOUBLE);
INSERT INTO Producto VALUES(1, 'Laptop', 1500.0);
INSERT INTO Producto VALUES(2, 'Mouse', 25.0);
INSERT INTO Producto VALUES(3, 'Teclado', 75.0);
SELECT * FROM Producto


-- ---------------------------------------------------------
-- PARTE 2: Crear indice BTREE y verificar en el catalogo.
-- Resultado esperado: SystemIndexes muestra idx_producto_id
--                     sobre la columna ID de Producto.
-- ---------------------------------------------------------

SET DATABASE TestDB;
CREATE INDEX idx_producto_id ON Producto(ID) OF TYPE BTREE;
SELECT * FROM SystemIndexes


-- ---------------------------------------------------------
-- PARTE 3: SELECT usando el indice sobre registros existentes.
-- Resultado esperado: acceso directo por offset, no secuencial.
--   - Primera query: fila de Laptop (ID=1).
--   - Segunda query: fila de Teclado (ID=3).
-- ---------------------------------------------------------

SET DATABASE TestDB;
SELECT * FROM Producto WHERE ID = 1;
SELECT * FROM Producto WHERE ID = 3


-- ---------------------------------------------------------
-- PARTE 4: INSERT despues de crear el indice + SELECT.
-- Resultado esperado: los nuevos registros se agregan al
--   arbol BTREE automaticamente y son encontrados por indice.
-- ---------------------------------------------------------

SET DATABASE TestDB;
INSERT INTO Producto VALUES(4, 'Monitor', 300.0);
INSERT INTO Producto VALUES(5, 'Webcam', 50.0);
SELECT * FROM Producto WHERE ID = 4;
SELECT * FROM Producto WHERE ID = 5;
SELECT * FROM Producto


-- ---------------------------------------------------------
-- PARTE 5: DELETE de todos los registros y DROP TABLE.
-- Resultado esperado:
--   - Cada DELETE actualiza el arbol BTREE (remueve la key).
--   - SELECT final muestra tabla vacia.
--   - DROP TABLE ejecuta sin error.
-- ---------------------------------------------------------

SET DATABASE TestDB;
DELETE FROM Producto WHERE ID = 1;
DELETE FROM Producto WHERE ID = 2;
DELETE FROM Producto WHERE ID = 3;
DELETE FROM Producto WHERE ID = 4;
DELETE FROM Producto WHERE ID = 5;
SELECT * FROM Producto;
DROP TABLE Producto


-- ---------------------------------------------------------
-- PARTE 6: Crear tabla Categoria e insertar datos sin indice.
-- Resultado esperado: 3 filas visibles; aun no hay indice.
-- ---------------------------------------------------------

SET DATABASE TestDB;
CREATE TABLE Categoria (ID INTEGER, Descripcion VARCHAR(100));
INSERT INTO Categoria VALUES(1, 'Electronica');
INSERT INTO Categoria VALUES(2, 'Ropa');
INSERT INTO Categoria VALUES(3, 'Alimentos');
SELECT * FROM Categoria


-- ---------------------------------------------------------
-- PARTE 7: Crear indice BST y verificar en el catalogo.
-- Resultado esperado: SystemIndexes muestra idx_cat_id
--                     sobre la columna ID de Categoria.
-- ---------------------------------------------------------

SET DATABASE TestDB;
CREATE INDEX idx_cat_id ON Categoria(ID) OF TYPE BST;
SELECT * FROM SystemIndexes


-- ---------------------------------------------------------
-- PARTE 8: SELECT usando el indice BST.
-- Resultado esperado: fila de Ropa (ID=2) encontrada por BST.
-- ---------------------------------------------------------

SET DATABASE TestDB;
SELECT * FROM Categoria WHERE ID = 2


-- ---------------------------------------------------------
-- PARTE 9: DELETE de todos los registros y DROP TABLE.
-- Resultado esperado:
--   - Cada DELETE actualiza el arbol BST.
--   - SELECT final muestra tabla vacia.
--   - DROP TABLE ejecuta sin error.
-- ---------------------------------------------------------

SET DATABASE TestDB;
DELETE FROM Categoria WHERE ID = 1;
DELETE FROM Categoria WHERE ID = 2;
DELETE FROM Categoria WHERE ID = 3;
SELECT * FROM Categoria;
DROP TABLE Categoria