#pragma once

#include <libpq-fe.h>
#include <memory>
#include <optional>
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
    Database();

    std::optional<std::string> read(const std::string &key);

    void write(const std::string &key, const std::string &result, const std::string &err_code);

  private:
    std::string get_error() const;

    void load_cache();

    std::optional<std::vector<std::string>> query(const std::string &sql,
                                                   const std::vector<std::string> &params = {});
};
