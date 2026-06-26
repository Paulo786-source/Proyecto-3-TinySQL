# Protocolo de Comunicación — TinySQLDb

Este documento define el contrato de comunicación entre el cliente React y el servidor C++.
No cambia después de la Fase 0.

---

## Generalidades

- Toda la comunicación es HTTP sobre localhost
- El cliente corre en http://localhost:5173
- El servidor corre en http://localhost:8080
- Todos los mensajes son en formato JSON
- El único endpoint es POST /query
- El Web API es el único componente que serializa y deserializa JSON

---

## Request

El cliente envía un POST a http://localhost:8080/query con el siguiente body:

{
    "sql": "SELECT * FROM Estudiante WHERE ID = 2",
    "db_context": "Universidad"
}

Campos:
- sql        → sentencia SQL a ejecutar (string, requerido)
- db_context → nombre de la base de datos activa (string, vacío si no se ha ejecutado SET DATABASE)

---

## Response

El servidor responde siempre con HTTP 200 y el siguiente body:

{
    "columns": ["ID", "Nombre", "PrimerApellido"],
    "rows": [
        ["1", "Isaac", "Ramirez"],
        ["2", "Juan", "Ramirez"]
    ],
    "time_ms": 12,
    "error": ""
}

Campos:
- columns  → lista de nombres de columnas del resultado (array de strings)
- rows     → lista de filas, cada fila es una lista de valores en string (array de arrays)
- time_ms  → tiempo que tardó el servidor en procesar la sentencia, en milisegundos (número)
- error    → mensaje de error descriptivo, vacío si la sentencia fue exitosa (string)

---

## Casos especiales

### Sentencia exitosa sin filas (INSERT, UPDATE, DELETE, CREATE, DROP, SET DATABASE)

{
    "columns": [],
    "rows": [],
    "time_ms": 3,
    "error": ""
}

### Sentencia con error

{
    "columns": [],
    "rows": [],
    "time_ms": 0,
    "error": "La base de datos 'Universidad' no existe"
}

### Múltiples sentencias en un script
El cliente separa las sentencias por punto y coma y las envía de una en una.
Cada sentencia genera su propio request y su propio response.

---

## Reglas importantes

1. El servidor siempre responde HTTP 200, incluso cuando hay un error SQL.
   Los errores se comunican mediante el campo "error", no mediante códigos HTTP.

2. Todos los valores en "rows" son strings, sin importar el tipo de dato original.
   El cliente no necesita conocer los tipos — solo los muestra en pantalla.

3. El campo "time_ms" mide únicamente el tiempo de procesamiento interno del servidor.
   No incluye el tiempo de red ni la serialización JSON.
   Se mide con std::chrono justo antes y después de llamar al Query Processor.

4. El campo "db_context" lo mantiene el cliente en su estado interno.
   Se actualiza cuando el usuario ejecuta SET DATABASE y se envía en cada request.

---

## Ejemplo completo

Request:
POST http://localhost:8080/query
Content-Type: application/json

{
    "sql": "CREATE DATABASE Universidad",
    "db_context": ""
}

Response:
{
    "columns": [],
    "rows": [],
    "time_ms": 5,
    "error": ""
}

Request:
POST http://localhost:8080/query
Content-Type: application/json

{
    "sql": "SET DATABASE Universidad",
    "db_context": ""
}

Response:
{
    "columns": [],
    "rows": [],
    "time_ms": 2,
    "error": ""
}