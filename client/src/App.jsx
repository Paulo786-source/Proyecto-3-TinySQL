import { useState } from 'react'
import axios from 'axios'
import './App.css'

const API_URL = 'http://localhost:8081/query'

function App() {
    const [sql, setSql] = useState('')
    const [dbContext, setDbContext] = useState('')
    const [result, setResult] = useState(null)
    const [loading, setLoading] = useState(false)

    const execute = async () => {
        if (!sql.trim()) return
        setLoading(true)
        setResult(null)
        try {
            const response = await axios.post(API_URL, { sql, db_context: dbContext })
            setResult(response.data)
            // actualiza el contexto si fue un SET DATABASE exitoso
            if (response.data.success && sql.trim().toUpperCase().startsWith('SET DATABASE')) {
                const parts = sql.trim().split(/\s+/)
                if (parts.length >= 3) setDbContext(parts[2])
            }
        } catch (err) {
            setResult({ error: 'no se pudo conectar con el servidor', columns: [], rows: [], time_ms: 0 })
        }
        setLoading(false)
    }

    return (
        <div className="app">
            {/* contexto de base de datos activa */}
            <div className="db-context">
                {dbContext
                    ? <span>Base de datos activa: <strong>{dbContext}</strong></span>
                    : <span className="no-db">Sin base de datos seleccionada</span>
                }
            </div>

            {/* editor SQL */}
            <div className="editor-section">
                <textarea
                    className="sql-editor"
                    value={sql}
                    onChange={e => setSql(e.target.value)}
                    placeholder="Escribe tu sentencia SQL aqui..."
                    rows={6}
                    spellCheck={false}
                />
                <button className="run-btn" onClick={execute} disabled={loading}>
                    {loading ? 'Ejecutando...' : 'Ejecutar'}
                </button>
            </div>

            {/* resultados */}
            {result && (
                <div className="result-section">
                    {result.error
                        ? <p className="error">{result.error}</p>
                        : (
                            <>
                                {result.columns.length > 0
                                    ? (
                                        <table className="result-table">
                                            <thead>
                                                <tr>
                                                    {result.columns.map((col, i) => <th key={i}>{col}</th>)}
                                                </tr>
                                            </thead>
                                            <tbody>
                                                {result.rows.map((row, i) => (
                                                    <tr key={i}>
                                                        {row.map((cell, j) => <td key={j}>{cell}</td>)}
                                                    </tr>
                                                ))}
                                            </tbody>
                                        </table>
                                    )
                                    : <p className="ok">Sentencia ejecutada correctamente</p>
                                }
                                <p className="time">Tiempo de ejecucion: {result.time_ms} ms</p>
                            </>
                        )
                    }
                </div>
            )}
        </div>
    )
}

export default App