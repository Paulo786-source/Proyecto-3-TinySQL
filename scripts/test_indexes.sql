-- Prueba creación de índices BTREE y BST, búsqueda con índice y sin índice.

SET DATABASE TestDB;
CREATE TABLE Producto (ID INTEGER, Nombre VARCHAR(50), Precio DOUBLE);
INSERT INTO Producto VALUES(1, 'Laptop', 1500.0);
INSERT INTO Producto VALUES(2, 'Mouse', 25.0);
INSERT INTO Producto VALUES(3, 'Teclado', 75.0);
CREATE INDEX idx_producto_id ON Producto(ID) OF TYPE BTREE;
SELECT * FROM Producto WHERE ID = 1;
SELECT * FROM Producto WHERE ID = 3;
INSERT INTO Producto VALUES(4, 'Monitor', 300.0);
INSERT INTO Producto VALUES(5, 'Webcam', 50.0);
SELECT * FROM Producto WHERE ID = 4;
SELECT * FROM Producto WHERE ID = 5;
DELETE FROM Producto WHERE ID = 1;
DELETE FROM Producto WHERE ID = 2;
DELETE FROM Producto WHERE ID = 3;
DELETE FROM Producto WHERE ID = 4;
DELETE FROM Producto WHERE ID = 5;
DROP TABLE Producto;
CREATE TABLE Categoria (ID INTEGER, Descripcion VARCHAR(100));
INSERT INTO Categoria VALUES(1, 'Electronica');
INSERT INTO Categoria VALUES(2, 'Ropa');
INSERT INTO Categoria VALUES(3, 'Alimentos');
CREATE INDEX idx_cat_id ON Categoria(ID) OF TYPE BST;
SELECT * FROM Categoria WHERE ID = 2;
DELETE FROM Categoria WHERE ID = 1;
DELETE FROM Categoria WHERE ID = 2;
DELETE FROM Categoria WHERE ID = 3;
DROP TABLE Categoria