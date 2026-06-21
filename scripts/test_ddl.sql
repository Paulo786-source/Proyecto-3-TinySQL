-- Prueba creación y eliminación de tablas, y consulta del catálogo del sistema.

SET DATABASE TestDB;
CREATE TABLE Persona (ID INTEGER, Nombre VARCHAR(50), Salario DOUBLE, FechaNacimiento DATETIME);
SELECT * FROM SystemTables;
SELECT * FROM SystemColumns;
DROP TABLE Persona;
SELECT * FROM SystemTables