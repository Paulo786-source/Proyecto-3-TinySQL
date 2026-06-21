-- prueba de indices BST y BTREE

SET DATABASE TestDB;

-- crear tabla
CREATE TABLE Producto (
    ID INTEGER,
    Nombre VARCHAR(50),
    Precio DOUBLE
);

-- insertar datos antes de crear el indice
INSERT INTO Producto VALUES(1, 'Laptop', 1500.0);
INSERT INTO Producto VALUES(2, 'Mouse', 25.0);
INSERT INTO Producto VALUES(3, 'Teclado', 75.0);

-- crear indice BTREE sobre ID
-- debe cargar automaticamente los registros existentes
CREATE INDEX idx_producto_id ON Producto(ID) OF TYPE BTREE;

-- busqueda usando indice BTREE (O log n)
SELECT * FROM Producto WHERE ID = 1;
SELECT * FROM Producto WHERE ID = 3;

-- insertar mas registros (deben entrar al indice automaticamente)
INSERT INTO Producto VALUES(4, 'Monitor', 300.0);
INSERT INTO Producto VALUES(5, 'Webcam', 50.0);

-- verificar que los nuevos registros se encuentran con el indice
SELECT * FROM Producto WHERE ID = 4;
SELECT * FROM Producto WHERE ID = 5;

-- limpiar para probar BST
DELETE FROM Producto WHERE ID = 1;
DELETE FROM Producto WHERE ID = 2;
DELETE FROM Producto WHERE ID = 3;
DELETE FROM Producto WHERE ID = 4;
DELETE FROM Producto WHERE ID = 5;
DROP TABLE Producto;

-- crear tabla para BST
CREATE TABLE Categoria (
    ID INTEGER,
    Descripcion VARCHAR(100)
);

INSERT INTO Categoria VALUES(1, 'Electronica');
INSERT INTO Categoria VALUES(2, 'Ropa');
INSERT INTO Categoria VALUES(3, 'Alimentos');

-- crear indice BST sobre ID
CREATE INDEX idx_cat_id ON Categoria(ID) OF TYPE BST;

-- busqueda usando indice BST
SELECT * FROM Categoria WHERE ID = 2;

-- limpiar
DELETE FROM Categoria WHERE ID = 1;
DELETE FROM Categoria WHERE ID = 2;
DELETE FROM Categoria WHERE ID = 3;
DROP TABLE Categoria;