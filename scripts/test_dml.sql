-- prueba de operaciones DML

SET DATABASE TestDB;

-- crear tabla de prueba
CREATE TABLE Estudiante (
    ID INTEGER,
    Nombre VARCHAR(50),
    Nota DOUBLE
);

-- insertar registros
INSERT INTO Estudiante VALUES(1, 'Carlos', 9.5);
INSERT INTO Estudiante VALUES(2, 'Maria', 8.7);
INSERT INTO Estudiante VALUES(3, 'Juan', 7.5);
INSERT INTO Estudiante VALUES(4, 'Ana', 10.0);

-- seleccionar todos los registros
SELECT * FROM Estudiante;

-- seleccionar con WHERE
SELECT * FROM Estudiante WHERE ID = 2;
SELECT * FROM Estudiante WHERE Nota > 8.0;

-- seleccionar con ORDER BY
SELECT * FROM Estudiante ORDER BY Nota ASC;
SELECT * FROM Estudiante ORDER BY Nota DESC;

-- seleccionar columnas especificas
SELECT ID, Nombre FROM Estudiante;

-- actualizar un registro
UPDATE Estudiante SET Nota = 9.0 WHERE ID = 3;
SELECT * FROM Estudiante WHERE ID = 3;

-- actualizar multiples registros
UPDATE Estudiante SET Nota = 0.0 WHERE ID = 4;
SELECT * FROM Estudiante;

-- eliminar un registro
DELETE FROM Estudiante WHERE ID = 4;
SELECT * FROM Estudiante;

-- eliminar multiples registros
DELETE FROM Estudiante WHERE Nota > 9.0;
SELECT * FROM Estudiante;

-- limpiar
DELETE FROM Estudiante WHERE ID = 1;
DELETE FROM Estudiante WHERE ID = 2;
DELETE FROM Estudiante WHERE ID = 3;
DROP TABLE Estudiante;