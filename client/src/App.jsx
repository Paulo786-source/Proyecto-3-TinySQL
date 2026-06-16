import { useState } from 'react'
import axios from 'axios'
import './App.css'

// puerto del servidor — cambiar aquí si se modifica en el backend
const SERVER_URL = 'http://localhost:8081/query'

function App() {
    // sql que escribe el usuario en el textarea
    const [sqlInput, setSqlInput] = useState('')

    // base de datos activa; se actualiza cuando SET DATABASE tiene éxito
    const [dbContext, setDbContext] = useState('')

    // resultados de la última sentencia ejecutada
    const [columns, setColumns] = useState([])
    const [rows, setRows] = useState([])
    const [timeMs, setTimeMs] = useState(null)
    const [errorMsg, setErrorMsg] = useState('')

    // indica si hay una petición en vuelo
    const [loading, setLoading] = useState(false)

    // historial de resultados cuando se ejecutan múltiples sentencias
    const [history, setHistory] = useState([])

    // ejecuta una sola sentencia SQL contra el servidor
    const executeSingle = async (sql, context) => {
        const response = await axios.post(SERVER_URL, {
            sql: sql.trim(),
            db_context: context
        })
        return response.data
    }

    // separa el input en sentencias individuales por punto y coma
    // y las ejecuta una por una según el protocolo del enunciado
    const handleExecute = async () => {
        const sentences = sqlInput
            .split(';')
            .map(s => s.trim())
            .filter(s => s.length > 0)

        if (sentences.length === 0) {
            return
        }

        setLoading(true)
        setErrorMsg('')
        setColumns([])
        setRows([])
        setTimeMs(null)
        setHistory([])

        let currentContext = dbContext
        const newHistory = []

        try {
            for (const sentence of sentences) {
                const result = await executeSingle(sentence, currentContext)

                // si fue un SET DATABASE exitoso, actualiza el contexto activo
                const upper = sentence.toUpperCase().trim()
                if (upper.startsWith('SET DATABASE') && !result.error) {
                    const parts = sentence.trim().split(/\s+/)
                    if (parts.length >= 3) {
                        currentContext = parts[2]
                        setDbContext(parts[2])
                    }
                }

                newHistory.push({
                    sql: sentence,
                    columns: result.columns || [],
                    rows: result.rows || [],
                    time_ms: result.time_ms,
                    error: result.error || ''
                })

                // si una sentencia falla, detiene la ejecución del script
                if (result.error) {
                    break
                }
            }

            setHistory(newHistory)

            // muestra el resultado de la última sentencia ejecutada
            const last = newHistory[newHistory.length - 1]
            if (last) {
                setColumns(last.columns)
                setRows(last.rows)
                setTimeMs(last.time_ms)
                setErrorMsg(last.error)
            }
        }
        catch (err) {
            setErrorMsg('error de red: ' + (err.message || 'no se pudo conectar al servidor'))
        }
        finally {
            setLoading(false)
        }
    }

    // ctrl+enter ejecuta sin necesidad del botón
    const handleKeyDown = (e) => {
        if (e.ctrlKey && e.key === 'Enter') {
            handleExecute()
        }
    }

    return (
        <div className="app">
            <header className="app-header">
                <h1>TinySQLDb</h1>
                <div className="db-context">
                    {dbContext
                        ? <span>Base de datos activa: <strong>{dbContext}</strong></span>
                        : <span className="no-db">Sin base de datos seleccionada</span>
                    }
                </div>
            </header>

            <main className="app-main">
                <div className="editor-section">
                    <textarea
                        className="sql-editor"
                        value={sqlInput}
                        onChange={e => setSqlInput(e.target.value)}
                        onKeyDown={handleKeyDown}
                        placeholder="Escribe tu SQL aquí... (Ctrl+Enter para ejecutar)"
                        spellCheck={false}
                    />
                    <button
                        className="execute-btn"
                        onClick={handleExecute}
                        disabled={loading}
                    >
                        {loading ? 'Ejecutando...' : 'Ejecutar'}
                    </button>
                </div>

                {/* historial de sentencias ejecutadas en el script */}
                {history.length > 1 && (
                    <div className="history-section">
                        <h3>Historial de sentencias</h3>
                        {history.map((entry, i) => (
                            <div key={i} className={`history-entry ${entry.error ? 'history-error' : 'history-ok'}`}>
                                <code>{entry.sql}</code>
                                <span>{entry.error ? entry.error : `OK — ${entry.time_ms} ms`}</span>
                            </div>
                        ))}
                    </div>
                )}

                <div className="results-section">
                    {errorMsg && (
                        <div className="error-msg">
                            {errorMsg}
                        </div>
                    )}

                    {timeMs !== null && !errorMsg && (
                        <div className="time-info">
                            Tiempo de ejecucion: {timeMs} ms
                        </div>
                    )}

                    {columns.length > 0 ? (
                        <table className="results-table">
                            <thead>
                                <tr>
                                    {columns.map((col, i) => (
                                        <th key={i}>{col}</th>
                                    ))}
                                </tr>
                            </thead>
                            <tbody>
                                {rows.map((row, i) => (
                                    <tr key={i}>
                                        {row.map((cell, j) => (
                                            <td key={j}>{cell}</td>
                                        ))}
                                    </tr>
                                ))}
                            </tbody>
                        </table>
                    ) : (
                        timeMs !== null && !errorMsg && (
                            <p className="no-rows">Sentencia ejecutada correctamente (sin filas de resultado)</p>
                        )
                    )}
                </div>
            </main>
        </div>
    )
}

export default App