// DatabaseMigrator.cpp
#include "database_migrator.h"
#include <sstream>
#include <iomanip>
#include "external/base64.hpp"

ProgressCallback DatabaseMigrator::callback_ = nullptr;

DatabaseConfig& DatabaseConfig::operator=(const json& j) {
    try {
        // Проверка на наличие необходимых полей
        if (!j.contains("host") || !j.contains("port") ||
            !j.contains("dbname") || !j.contains("user") ||
            !j.contains("password")) {
            throw std::invalid_argument("Missing required database configuration fields (json id=302)");
        }

        host = j["host"].get<std::string>();
        port = j["port"].get<int>();
        dbname = j["dbname"].get<std::string>();
        user = j["user"].get<std::string>();
        password = j["password"].get<std::string>();

        return *this;
    }
    catch (const std::invalid_argument& e) {
        Logger::log(Logger::ERROR, "DatabaseConfig",
            "Error parsing database configuration: " + std::string(e.what()));
        throw;
    }
}

TableConfig::TableConfig(const json& j) {
    *this = j;
}


TableConfig& TableConfig::operator=(const json& j) {
    try {
        // Проверка на наличие обязательных полей
        if (!j.contains("source"))
            throw std::invalid_argument("Missing required field 'source' in table configuration (json id=302)");

        source = j["source"].get<std::string>();
        target = j.value("target", "");
        exclude = j.value("exclude", false);
        create_if_missing = j.value("create_if_missing", false);

        columns.clear();

        // Обработка заголовков столбцов
        if (j.contains("columns")) {
            const json& columns_json = j["columns"];
            if (!columns_json.is_object()) {
                throw std::domain_error("Table columns must be an object (json id=302)");
            }

            for (const auto& [column_name, column_config] : columns_json.items()) {
                std::map<std::string, std::string> column;
                if (column_config.contains("target_name"))
                    column["target_name"] = column_config["target_name"].get<std::string>();
                if (column_config.contains("type"))
                    column["type"] = column_config["type"].get<std::string>();
                if (column_config.contains("exclude"))
                    column["exclude"] = column_config["exclude"].get<std::string>();

                columns[column_name] = column;
            }
        }

        return *this;
    }
    catch (const std::invalid_argument& e) {
        Logger::log(Logger::ERROR, "TableConfig",
            "Error parsing table configuration: " + std::string(e.what()));
        throw;
    }
    catch (const std::domain_error& e) {
        Logger::log(Logger::ERROR, "TableConfig",
            "Error in table configuration: " + std::string(e.what()));
        throw;
    }
}

DatabaseMigrator::DatabaseMigrator(std::string config_path)
    : config_path(config_path) {
    load_config();
}

void DatabaseMigrator::registerCallback(ProgressCallback cb) {
    callback_ = cb;
}

void DatabaseMigrator::load_config() {
    try {
        std::ifstream config_file(config_path);
        json config;
        config_file >> config;

        source_db = config["source_database"];
        target_db = config["target_database"];

        if (config.contains("tables")) {
            const json& tables_json = config["tables"];

            if (!tables_json.is_array()) {
                throw std::invalid_argument("Tables must be an array");
            }

            tables.clear();
            for (const json& table_config_json : tables_json) { 
                TableConfig table_config = table_config_json;
                tables.push_back(table_config);
            }
        }
        else {
            tables.clear();
        }

        Logger::log(Logger::INFO, "DatabaseMigrator", "Configuration loaded successfully");
        isConfigInitialized = true;
    }
    catch (const std::invalid_argument& e) {
        Logger::log(Logger::ERROR, "DatabaseMigrator",
            "Error loading configuration: " + std::string(e.what()));
        throw;
    }
}

std::string DatabaseMigrator::create_connection_string(const DatabaseConfig& config) {
    std::stringstream ss;
    ss << "host=" << config.host
        << " port=" << config.port
        << " dbname=" << config.dbname
        << " user=" << config.user
        << " password=" << config.password;
    return ss.str();
}

bool DatabaseMigrator::execute_migration() {
    Logger::log(Logger::INFO, "DatabaseMigrator", "Starting database migration");

    try {
        migrate();
        Logger::log(Logger::INFO, "DatabaseMigrator", "Migration completed successfully");
        return true;
    }
    catch (const std::exception& e) {
        Logger::log(Logger::ERROR, "DatabaseMigrator",
            "Migration failed: " + std::string(e.what()));
        throw;
        return false;
    }
}

void DatabaseMigrator::migrate() {
    int total_rows = 0;
    int current_rows = 0;

    if (!isConfigInitialized)
        throw std::runtime_error("Configuration file isn't set");

    if (callback_ != nullptr) {
        for (const auto& table : tables) {
            if (table.exclude) continue;

            PGconn* conn = PQconnectdb(create_connection_string(source_db).c_str());
            PGresult* res = PQexec(conn,
                ("SELECT COUNT(*) FROM " + table.source).c_str());

            if (PQresultStatus(res) == PGRES_TUPLES_OK) {
                total_rows += std::stoi(PQgetvalue(res, 0, 0));
            }
            PQclear(res);
            PQfinish(conn);
        }
    }
    
    for (const auto& table : tables) {
        if (table.exclude) {
            Logger::log(Logger::INFO, "DatabaseMigrator",
                "Skipping excluded table: " + table.source);
            continue;
        }

        migrate_table(table);

        if (callback_ != nullptr) {
            PGconn* conn = PQconnectdb(create_connection_string(source_db).c_str());
            PGresult* res = PQexec(conn,
                ("SELECT COUNT(*) FROM " + table.source).c_str());

            if (PQresultStatus(res) == PGRES_TUPLES_OK) {
                current_rows += std::stoi(PQgetvalue(res, 0, 0));
            }
            PQclear(res);
            PQfinish(conn);

            int progress = (current_rows * 100) / total_rows;
            callback_(progress);
        }
    }
}

void DatabaseMigrator::migrate_table(const TableConfig& table_config) {
    PGconn* source_conn = nullptr;
    PGconn* target_conn = nullptr;

    try {
        source_conn = PQconnectdb(create_connection_string(source_db).c_str());
        target_conn = PQconnectdb(create_connection_string(target_db).c_str());

        if (PQstatus(source_conn) != CONNECTION_OK) {
            throw std::runtime_error("Failed to connect to source database");
        }
        if (PQstatus(target_conn) != CONNECTION_OK) {
            throw std::runtime_error("Failed to connect to target database");
        }

        // Создание таблицы (если требуется)
        if (table_config.create_if_missing) {
            std::string ddl = generate_ddl(table_config);
            Logger::log(Logger::INFO, "DatabaseMigrator",
                "Creating table: " + (table_config.target.empty() ? table_config.source : table_config.target));

            PGresult* res = PQexec(target_conn, ddl.c_str());
            if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                throw std::runtime_error("Failed to create table");
            }
            PQclear(res);
        }

        std::string query = "SELECT * FROM " + table_config.source;
        PGresult* res = PQexec(source_conn, query.c_str());

        if (PQresultStatus(res) == PGRES_TUPLES_OK) {
            int rows = PQntuples(res);
            int cols = PQnfields(res);

            for (int row = 0; row < rows; row++) {
                std::stringstream insert_stmt;
                insert_stmt << "INSERT INTO "
                    << (table_config.target.empty() ? table_config.source : table_config.target)
                    << " VALUES (";

                for (int col = 0; col < cols; col++) {
                    if (col > 0) insert_stmt << ",";

                    const char* value = PQgetvalue(res, row, col);
                    if (value && *value) {
                        std::string converted = convert_value(value,
                            table_config.columns.at(PQfname(res, col)).at("type"));
                        insert_stmt << "'" << converted << "'";
                    }
                    else {
                        insert_stmt << "NULL";
                    }
                }

                insert_stmt << ")";

                PGresult* insert_res = PQexec(target_conn, insert_stmt.str().c_str());
                if (PQresultStatus(insert_res) != PGRES_COMMAND_OK) {
                    throw std::runtime_error("Failed to insert data");
                }
                PQclear(insert_res);
            }
        }

        PQclear(res);
    }
    catch (const std::exception& e) {
        Logger::log(Logger::ERROR, "DatabaseMigrator",
            "Error migrating table " + table_config.source + ": " + std::string(e.what()));
        throw;
    }

    if (source_conn) PQfinish(source_conn);
    if (target_conn) PQfinish(target_conn);
}

std::string DatabaseMigrator::generate_ddl(const TableConfig& table_config) {
    std::stringstream ddl;
    ddl << "CREATE TABLE IF NOT EXISTS "
        << (table_config.target.empty() ? table_config.source : table_config.target)
        << " (\n";

    std::vector<std::string> columns;

    for (const auto& col : table_config.columns) {
        if (col.second.find("exclude") != col.second.end() &&
            col.second.at("exclude") == "true") {
            continue;
        }

        std::string column_def = col.first;
        if (col.second.find("target_name") != col.second.end()) {
            column_def = col.second.at("target_name");
        }

        column_def += " " + (col.second.find("type") != col.second.end() ?
            col.second.at("type") : "TEXT");
        columns.push_back(column_def);
    }

    for (size_t i = 0; i < columns.size(); i++) {
        ddl << "    " << columns[i];
        if (i < columns.size() - 1) ddl << ",";
        ddl << "\n";
    }

    ddl << ");";
    return ddl.str();
}

std::string DatabaseMigrator::convert_value(const std::string& value, const std::string& type) {
    if (type == "BASE64") {
        return Base64::encode(value);
    }
    else if (type == "VARCHAR") {
        size_t open_paren = value.find_first_of('(');
        if (open_paren != std::string::npos) {
            size_t close_paren = value.find_first_of(')', open_paren);
            if (close_paren != std::string::npos) {
                int length = std::stoi(value.substr(open_paren + 1, close_paren - open_paren - 1));
                return value.substr(0, open_paren);
            }
        }
        return value;
    }
    else if (type == "BIGINT") {
        try {
            std::string num_str = value;
            if (num_str[0] == '\\' && num_str[num_str.length() - 1] == '\\') {
                num_str = num_str.substr(1, num_str.length() - 2);
            }
            return num_str;
        }
        catch (...) {
            return value;
        }
    }

    return value;
}