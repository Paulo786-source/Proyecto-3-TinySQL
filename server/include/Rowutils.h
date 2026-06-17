#pragma once

#include "storeddatamanager.h"
#include "queryprocessor.h"
#include "typesystem.h"
#include <string>
#include <vector>
#include <algorithm>

namespace RowUtils {

    // helper interno
    inline std::string toLower(const std::string& s) {
        std::string r = s;
        for (char& c : r)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return r;
    }

    // evalua si una fila cumple la condicion WHERE
    // soporta =, >, <, >=, <=, LIKE, NOT
    // LIKE usa * como comodin

    inline bool matchesWhere(const Row& row,
        const WhereClause& where,
        const std::vector<ColumnDefinition>& schema)
    {
        // busca la columna por nombre
        int colIdx = -1;
        for (int i = 0; i < static_cast<int>(schema.size()); ++i) {
            if (toLower(schema[i].name) == toLower(where.column)) {
                colIdx = i; break;
            }
        }
        if (colIdx < 0 || colIdx >= static_cast<int>(row.size())) return false;

        const std::string& cell = row[colIdx];
        const ColumnDefinition& col = schema[colIdx];
        const std::string& filter = where.value;

        // LIKE: *texto* busca que contenga "texto"
        if (where.op == "LIKE") {
            std::string pat = filter;
            while (!pat.empty() && pat.front() == '*') pat.erase(0, 1);
            while (!pat.empty() && pat.back() == '*') pat.pop_back();
            return toLower(cell).find(toLower(pat)) != std::string::npos;
        }

        // NOT: distinto de
        if (where.op == "NOT") {
            return toLower(cell) != toLower(filter);
        }

        // comparaciones numericas
        if (col.type == "INTEGER" || col.type == "DATETIME") {
            long long cv, fv;
            try {
                cv = std::stoll(cell);
                fv = (col.type == "DATETIME")
                    ? TypeSystem::parseDatetime(filter)
                    : std::stoll(filter);
            }
            catch (...) { return false; }

            if (where.op == "=")  return cv == fv;
            if (where.op == ">")  return cv > fv;
            if (where.op == "<")  return cv < fv;
            if (where.op == ">=") return cv >= fv;
            if (where.op == "<=") return cv <= fv;
        }
        else if (col.type == "DOUBLE") {
            double cv, fv;
            try { cv = std::stod(cell); fv = std::stod(filter); }
            catch (...) { return false; }

            if (where.op == "=")  return cv == fv;
            if (where.op == ">")  return cv > fv;
            if (where.op == "<")  return cv < fv;
            if (where.op == ">=") return cv >= fv;
            if (where.op == "<=") return cv <= fv;
        }
        else {
            // VARCHAR: comparacion de strings
            if (where.op == "=")  return cell == filter;
            if (where.op == ">")  return cell > filter;
            if (where.op == "<")  return cell < filter;
            if (where.op == ">=") return cell >= filter;
            if (where.op == "<=") return cell <= filter;
        }
        return false;
    }

    // quicksort

    inline bool rowLess(const Row& a, const Row& b,
        int colIdx, const std::string& colType, bool asc)
    {
        if (colIdx < 0 || colIdx >= static_cast<int>(a.size())) return false;
        const std::string& va = a[colIdx];
        const std::string& vb = b[colIdx];

        bool less;
        if (colType == "INTEGER" || colType == "DATETIME") {
            try { less = std::stoll(va) < std::stoll(vb); }
            catch (...) { less = va < vb; }
        }
        else if (colType == "DOUBLE") {
            try { less = std::stod(va) < std::stod(vb); }
            catch (...) { less = va < vb; }
        }
        else {
            less = va < vb;
        }
        return asc ? less : !less;
    }

    // particion de lomuto
    inline int partition(std::vector<Row>& rows, int lo, int hi,
        int colIdx, const std::string& colType, bool asc)
    {
        const Row& pivot = rows[hi];
        int i = lo - 1;
        for (int j = lo; j < hi; ++j) {
            if (rowLess(rows[j], pivot, colIdx, colType, asc)) {
                std::swap(rows[++i], rows[j]);
            }
        }
        std::swap(rows[i + 1], rows[hi]);
        return i + 1;
    }

    inline void quicksortImpl(std::vector<Row>& rows, int lo, int hi,
        int colIdx, const std::string& colType, bool asc)
    {
        if (lo < hi) {
            int p = partition(rows, lo, hi, colIdx, colType, asc);
            quicksortImpl(rows, lo, p - 1, colIdx, colType, asc);
            quicksortImpl(rows, p + 1, hi, colIdx, colType, asc);
        }
    }

    // ordena las filas por la columna indicada
    inline void sortRows(std::vector<Row>& rows,
        const std::string& colName,
        const std::string& direction,
        const std::vector<ColumnDefinition>& schema)
    {
        if (rows.size() < 2) return;

        int colIdx = -1;
        std::string typ = "VARCHAR";
        for (int i = 0; i < static_cast<int>(schema.size()); ++i) {
            if (toLower(schema[i].name) == toLower(colName)) {
                colIdx = i;
                typ = schema[i].type;
                break;
            }
        }
        if (colIdx < 0) return;

        bool asc = (direction != "DESC");
        quicksortImpl(rows, 0, static_cast<int>(rows.size()) - 1, colIdx, typ, asc);
    }

}