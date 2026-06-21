-- Prueba INSERT, SELECT con filtros y ordenamiento, UPDATE y DELETE.

SET DATABASE TestDB;
CREATE TABLE Estudiante (ID INTEGER, Nombre VARCHAR(50), Nota DOUBLE);
INSERT INTO Estudiante VALUES(1, 'Carlos', 9.5);
INSERT INTO Estudiante VALUES(2, 'Maria', 8.7);
INSERT INTO Estudiante VALUES(3, 'Juan', 7.5);
INSERT INTO Estudiante VALUES(4, 'Ana', 10.0);
SELECT * FROM Estudiante;
SELECT * FROM Estudiante WHERE ID = 2;
SELECT * FROM Estudiante WHERE Nota > 8.0;
SELECT * FROM Estudiante ORDER BY Nota ASC;
SELECT * FROM Estudiante ORDER BY Nota DESC;
SELECT ID, Nombre FROM Estudiante;
UPDATE Estudiante SET Nota = 9.0 WHERE ID = 3;
SELECT * FROM Estudiante WHERE ID = 3;
DELETE FROM Estudiante WHERE ID = 4;
SELECT * FROM Estudiante;
DELETE FROM Estudiante WHERE Nota > 9.0;
SELECT * FROM Estudiante;
DELETE FROM Estudiante WHERE ID = 2;
DELETE FROM Estudiante WHERE ID = 3;
DROP TABLE Estudiante