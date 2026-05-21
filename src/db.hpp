#pragma once

#include <libpq-fe.h>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

struct PGconnDeleter {
    void operator()(PGconn *conn) const {
        if (conn)
            PQfinish(conn);
    }
};

struct PGresultDeleter {
    void operator()(PGresult *res) const {
        if (res)
            PQclear(res);
    }
};

class Database {
    std::unordered_map<std::string, std::string> cache;
    const std::string conninfo = "postgresql://postgres:pass@localhost:5432/db-1";
    std::unique_ptr<PGconn, PGconnDeleter> conn;
    const std::string sql_init = R"(
        CREATE TABLE IF NOT EXISTS cache (
            key TEXT PRIMARY KEY,
            result TEXT NOT NULL,
            err_code TEXT NOT NULL DEFAULT ''
        );
    )";

  public:
    Database() : conn(PQconnectdb(conninfo.c_str())) {
        if (PQstatus(conn.get()) != CONNECTION_OK)
            throw std::runtime_error(get_error());
        query(sql_init);
        load_cache();
    }

    std::optional<std::string> read(const std::string &key) {
        auto it = cache.find(key);
        if (it != cache.end())
            return it->second;
        return std::nullopt;
    }

    void write(const std::string &key, const std::string &result, const std::string &err_code) {
        cache[key] = result;
        const std::string sql = R"(
            INSERT INTO cache (key, result, err_code)
            VALUES ($1, $2, $3)
            ON CONFLICT (key) DO UPDATE SET result = EXCLUDED.result, err_code = EXCLUDED.err_code;
        )";
        query(sql, {key, result, err_code});
    }

  private:
    std::string get_error() const { return std::string(PQerrorMessage(conn.get())); }

    void load_cache() {
        const std::string sql = "SELECT key, result FROM cache;";
        std::unique_ptr<PGresult, PGresultDeleter> res(
            PQexec(conn.get(), sql.c_str()));
        if (PQresultStatus(res.get()) == PGRES_TUPLES_OK) {
            int n_rows = PQntuples(res.get());
            for (int i = 0; i < n_rows; ++i) {
                std::string k = PQgetvalue(res.get(), i, 0);
                std::string v = PQgetvalue(res.get(), i, 1);
                cache[k] = v;
            }
        }
    }

    std::optional<std::vector<std::string>> query(const std::string &sql,
                                                   const std::vector<std::string> &params = {}) {
        std::vector<const char *> param_values;
        param_values.reserve(params.size());
        for (const auto &p : params)
            param_values.push_back(p.c_str());

        std::unique_ptr<PGresult, PGresultDeleter> res(PQexecParams(
            conn.get(), sql.c_str(), static_cast<int>(params.size()), nullptr,
            param_values.empty() ? nullptr : param_values.data(), nullptr, nullptr, 0));

        ExecStatusType status = PQresultStatus(res.get());

        if (status == PGRES_COMMAND_OK)
            return std::nullopt;

        if (status == PGRES_TUPLES_OK) {
            std::vector<std::string> row;
            if (PQntuples(res.get()) == 0)
                return row;
            int n = PQnfields(res.get());
            row.reserve(n);
            for (int j = 0; j < n; ++j)
                row.push_back(PQgetvalue(res.get(), 0, j));
            return row;
        }

        throw std::runtime_error("SQL error: " + get_error());
    }
};
