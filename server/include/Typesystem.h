#pragma once

#include "storeddatamanager.h"
#include <string>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace TypeSystem {

    // parsea "YYYY-MM-DD HH:MM:SS" a timestamp unix
    inline long long parseDatetime(const std::string& s) {
        std::tm tm{};
        std::istringstream ss(s);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        if (ss.fail()) {
            throw std::runtime_error(
                "formato de DATETIME invalido: '" + s +
                "'. Se esperaba: YYYY-MM-DD HH:MM:SS"
            );
        }
        tm.tm_isdst = -1;
        time_t t = std::mktime(&tm);
        if (t == static_cast<time_t>(-1)) {
            throw std::runtime_error("fecha DATETIME fuera de rango: '" + s + "'");
        }
        return static_cast<long long>(t);
    }

    // convierte timestamp unix a "YYYY-MM-DD HH:MM:SS"
    inline std::string formatDatetime(long long ts) {
        time_t t = static_cast<time_t>(ts);
        std::tm* tm = std::localtime(&t);
        if (!tm) return "0000-00-00 00:00:00";
        char buf[20];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
        return std::string(buf);
    }

    // valida que el valor sea del tipo correcto
    inline std::string validateValue(const std::string& value,
        const ColumnDefinition& col)
    {
        if (col.type == "INTEGER") {
            try { std::stoll(value); }
            catch (...) {
                return "el valor '" + value + "' no es un INTEGER valido"
                    " para la columna '" + col.name + "'";
            }
        }
        else if (col.type == "DOUBLE") {
            try { std::stod(value); }
            catch (...) {
                return "el valor '" + value + "' no es un DOUBLE valido"
                    " para la columna '" + col.name + "'";
            }
        }
        else if (col.type == "VARCHAR") {
            if (static_cast<int>(value.size()) > col.size) {
                return "el valor '" + value + "' excede VARCHAR(" +
                    std::to_string(col.size) + ") para '" + col.name + "'";
            }
        }
        else if (col.type == "DATETIME") {
            try { parseDatetime(value); }
            catch (const std::exception& e) { return e.what(); }
        }
        return "";
    }

    // convierte para guardar en disco (datetime a timestamp)
    inline std::string normalizeValue(const std::string& value,
        const ColumnDefinition& col)
    {
        if (col.type == "DATETIME") {
            return std::to_string(parseDatetime(value));
        }
        return value;
    }

    // convierte para mostrar en pantalla (timestamp a datetime legible)
    inline std::string displayValue(const std::string& stored,
        const ColumnDefinition& col)
    {
        if (col.type == "DATETIME") {
            try { return formatDatetime(std::stoll(stored)); }
            catch (...) { return stored; }
        }
        return stored;
    }

} 