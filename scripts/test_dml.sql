-- =========================================================
-- test_dml.sql  |  DML: INSERT, SELECT, UPDATE, DELETE
-- =========================================================
-- Como ejecutar: copia UN bloque a la vez al textarea y ejecutalo.
-- Los comentarios (lineas con --) no los copies; son solo guia.
-- Prerequisito: TestDB debe existir (correr 00_setup si no existe).
-- =========================================================


-- ---------------------------------------------------------
-- PARTE 1: Crear la tabla e insertar registros.
-- Resultado esperado: tabla con 4 filas visibles.
-- ---------------------------------------------------------

SET DATABASE TestDB;
CREATE TABLE Estudiante (ID INTEGER, Nombre VARCHAR(50), Nota DOUBLE);
INSERT INTO Estudiante VALUES(1, 'Carlos', 9.5);
INSERT INTO Estudiante VALUES(2, 'Maria', 8.7);
INSERT INTO Estudiante VALUES(3, 'Juan', 7.5);
INSERT INTO Estudiante VALUES(4, 'Ana', 10.0);
SELECT * FROM Estudiante


-- ---------------------------------------------------------
-- PARTE 2: SELECT con filtros WHERE y columnas especificas.
-- Resultado esperado:
--   - Primera query: solo la fila de Maria (ID=2).
--   - Segunda query: Carlos, Maria y Ana (Nota > 8.0).
--   - Tercera query: solo columnas ID y Nombre, todos.
-- ---------------------------------------------------------

SET DATABASE TestDB;
SELECT * FROM Estudiante WHERE ID = 2;
SELECT * FROM Estudiante WHERE Nota > 8.0;
SELECT ID, Nombre FROM Estudiante


-- ---------------------------------------------------------
-- PARTE 3: SELECT con ORDER BY (Quicksort).
-- Resultado esperado:
--   - Primera query: Juan, Maria, Carlos, Ana (ASC por Nota).
--   - Segunda query: Ana, Carlos, Maria, Juan (DESC por Nota).
-- ---------------------------------------------------------

SET DATABASE TestDB;
SELECT * FROM Estudiante ORDER BY Nota ASC;
SELECT * FROM Estudiante ORDER BY Nota DESC


-- ---------------------------------------------------------
-- PARTE 4: UPDATE y verificacion del cambio.
-- Resultado esperado:
--   - Fila de Juan (ID=3) ahora tiene Nota = 9.0.
--   - SELECT * muestra la tabla completa actualizada.
-- ---------------------------------------------------------

SET DATABASE TestDB;
UPDATE Estudiante SET Nota = 9.0 WHERE ID = 3;
SELECT * FROM Estudiante WHERE ID = 3;
SELECT * FROM Estudiante


-- ---------------------------------------------------------
-- PARTE 5: DELETE por ID puntual y verificacion.
-- Resultado esperado: Ana (ID=4) desaparece; quedan 3 filas.
-- ---------------------------------------------------------

SET DATABASE TestDB;
DELETE FROM Estudiante WHERE ID = 4;
SELECT * FROM Estudiante


-- ---------------------------------------------------------
-- PARTE 6: DELETE con condicion sobre columna no clave.
-- Resultado esperado: se eliminan todos con Nota > 9.0.
--   Carlos tenia 9.5 -> eliminado. Quedan Maria y Juan.
-- ---------------------------------------------------------

SET DATABASE TestDB;
DELETE FROM Estudiante WHERE Nota > 9.0;
SELECT * FROM Estudiante


-- ---------------------------------------------------------
-- PARTE 7: Limpieza final y DROP TABLE.
-- Resultado esperado:
--   - SELECT muestra tabla vacia antes del DROP.
--   - DROP TABLE ejecuta sin error (tabla vacia).
-- ---------------------------------------------------------

SET DATABASE TestDB;
DELETE FROM Estudiante WHERE ID = 2;
DELETE FROM Estudiante WHERE ID = 3;
SELECT * FROM Estudiante;
DROP TABLE Estudiante