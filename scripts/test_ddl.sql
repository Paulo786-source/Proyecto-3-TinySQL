-- prueba de operaciones DDL

-- crear base de datos
CREATE DATABASE TestDB;

-- seleccionar base de datos
SET DATABASE TestDB;

-- crear tabla con todos los tipos de datos soportados
CREATE TABLE Persona (
    ID INTEGER,
    Nombre VARCHAR(50),
    Salario DOUBLE,
    FechaNacimiento DATETIME
);

-- verificar que la tabla existe en el catalogo
SELECT * FROM SystemTables;

-- verificar que las columnas existen en el catalogo
SELECT * FROM SystemColumns;

-- eliminar la tabla (debe estar vacia)
DROP TABLE Persona;

-- verificar que ya no existe
SELECT * FROM SystemTables;