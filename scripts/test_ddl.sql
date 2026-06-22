-- =========================================================
-- test_ddl.sql  |  DDL: CREATE TABLE, DROP TABLE, catalogo
-- =========================================================
-- Como ejecutar: copia UN bloque a la vez al textarea y ejecutalo.
-- Los comentarios (lineas con --) no los copies; son solo guia.
-- =========================================================


-- ---------------------------------------------------------
-- PARTE 1: Crear la tabla y verificar que el catalogo
--           la registro correctamente.
-- Resultado esperado: SystemTables muestra "Persona",
--                     SystemColumns muestra sus 4 columnas.
-- ---------------------------------------------------------

SET DATABASE TestDB;
CREATE TABLE Persona (ID INTEGER, Nombre VARCHAR(50), Salario DOUBLE, FechaNacimiento DATETIME);
SELECT * FROM SystemTables;
SELECT * FROM SystemColumns


-- ---------------------------------------------------------
-- PARTE 2: Eliminar la tabla y verificar que el catalogo
--           la removio correctamente.
-- Resultado esperado: SystemTables ya no muestra "Persona".
-- ---------------------------------------------------------

SET DATABASE TestDB;
DROP TABLE Persona;
SELECT * FROM SystemTables