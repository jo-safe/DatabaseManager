#include "database_operator.h"
#include <fstream>
#include <sstream>
#include <iostream>

DatabaseOperator::DatabaseOperator()
    : conn_(nullptr), connected_(false) {
    Logger::log(Logger::INFO, "DatabaseOperator", "DatabaseOperator initialized");
}

DatabaseOperator::~DatabaseOperator() {
    if (connected_) {
        disconnect();
    }
}

void DatabaseOperator::handle_error(const std::string& operation) {
    if (conn_ && PQstatus(conn_) != CONNECTION_OK) {
        std::string error_msg = PQerrorMessage(conn_);
        Logger::log(Logger::ERROR, "DatabaseOperator",
            "Error during " + operation + ": " + error_msg);
    }
}

void DatabaseOperator::execute_query(const std::string& query) {
    PGresult* res = PQexec(conn_, query.c_str());
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        handle_error("query execution");
    }
    PQclear(res);
}

bool DatabaseOperator::connect(const std::string& connStr) {
    if (connected_) {
        disconnect();
    }

    conn_str_ = connStr;
    conn_ = PQconnectdb(conn_str_.c_str());

    if (PQstatus(conn_) != CONNECTION_OK) {
        handle_error("connection");
        return false;
    }

    connected_ = true;
    Logger::log(Logger::INFO, "DatabaseOperator", "Connected to database");
    return true;
}

void DatabaseOperator::disconnect() {
    try {
        if (connected_ && conn_) {
            PQfinish(conn_);
            conn_ = nullptr;
            connected_ = false;
            Logger::log(Logger::INFO, "DatabaseOperator", "Disconnected from database");
        }
    }
    catch (std::exception) { }
}

bool DatabaseOperator::exec(const std::string& query) {
    if (!connected_) {
        Logger::log(Logger::ERROR, "DatabaseOperator", "Not connected to database");
        return false;
    }

    execute_query(query);
    return true;
}

std::string DatabaseOperator::load(const std::string& filename) {
    if (!connected_) {
        Logger::log(Logger::ERROR, "DatabaseOperator", "Not connected to database");
        return "";
    }

    std::ifstream file(filename);
    if (!file.is_open()) {
        Logger::log(Logger::ERROR, "DatabaseOperator",
            "Failed to open file: " + filename);
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

bool DatabaseOperator::backup(const std::string& json_config, const std::string& out_file) {
    if (!connected_) {
        Logger::log(Logger::ERROR, "DatabaseOperator", "Not connected to database");
        return false;
    }

    std::vector<std::string> tables_to_backup;

    // Если указан config-файл, экспорт происходит согласно инструкции 
    if (!json_config.empty()) {
        std::ifstream config_file(json_config);
        if (!config_file.is_open()) {
            Logger::log(Logger::ERROR, "DatabaseOperator",
                "Failed to open backup config file: " + json_config);
            return false;
        }

        nlohmann::json config;
        try {
            config_file >> config;
            for (const auto& table : config["tables"]) {
                tables_to_backup.push_back(table);
            }
        }
        catch (const std::exception& e) {
            Logger::log(Logger::ERROR, "DatabaseOperator",
                "Invalid backup config format: " + std::string(e.what()));
            return false;
        }
    }
    else {
        // Иначе - обрабатываются все таблицы
        execute_query("SELECT tablename FROM pg_tables WHERE schemaname = 'public'");
        PGresult* res = PQexec(conn_, "SELECT tablename FROM pg_tables WHERE schemaname = 'public'");
        if (PQresultStatus(res) == PGRES_TUPLES_OK) {
            int rows = PQntuples(res);
            for (int i = 0; i < rows; ++i) {
                tables_to_backup.push_back(std::string(PQgetvalue(res, i, 0)));
            }
        }
        PQclear(res);
    }

    std::string backup_dir = out_file.substr(0, out_file.find_last_of("/"));
    if (!backup_dir.empty()) {
        std::string mkdir_cmd = "mkdir -p " + backup_dir;
        system(mkdir_cmd.c_str());
    }

    std::ofstream backup_file(out_file, std::ios::binary);
    if (!backup_file.is_open()) {
        Logger::log(Logger::ERROR, "DatabaseOperator",
            "Failed to open backup file: " + out_file);
        return false;
    }

    // Выполняется копирование для каждой таблицы в бинарный вид
    for (const auto& table : tables_to_backup) {
        std::string query = "COPY " + table + " TO STDOUT BINARY";
        PGresult* res = PQexec(conn_, query.c_str());

        if (PQresultStatus(res) == PGRES_COPY_OUT) {
            char* buffer = new char[8192];
            int len;
            while ((len = PQgetCopyData(conn_, &buffer, 8192)) > 0) {
                backup_file.write(buffer, len);
            }
            delete[] buffer;
            PQputCopyEnd(conn_, NULL);
            PQclear(res);
        }
        else {
            Logger::log(Logger::ERROR, "DatabaseOperator",
                "Failed to backup table: " + table);
            PQclear(res);
            backup_file.close();
            return false;
        }
    }

    backup_file.close();
    Logger::log(Logger::INFO, "DatabaseOperator",
        "Backup completed successfully to: " + out_file);
    return true;
}

bool DatabaseOperator::restore(const std::string& in_file, const std::string& json_config) {
    if (!connected_) {
        Logger::log(Logger::ERROR, "DatabaseOperator", "Not connected to database");
        return false;
    }

    std::vector<std::string> tables_to_restore;
    std::ifstream backup_file(in_file, std::ios::binary);

    // При указаном config-файле загружаются указанные таблицы
    if (!json_config.empty()) {
        std::ifstream config_file(json_config);
        if (!config_file.is_open()) {
            Logger::log(Logger::ERROR, "DatabaseOperator",
                "Failed to open restore config file: " + json_config);
            return false;
        }

        json config;
        try {
            config_file >> config;
            for (const auto& table : config["tables"]) {
                tables_to_restore.push_back(table);
            }
        }
        catch (const std::exception& e) {
            Logger::log(Logger::ERROR, "DatabaseOperator",
                "Invalid restore config format: " + std::string(e.what()));
            return false;
        }
    }
    else {
        // Иначе - все имеющиеся
        if (!backup_file.is_open()) {
            Logger::log(Logger::ERROR, "DatabaseOperator",
                "Failed to open backup file: " + in_file);
            return false;
        }
    }

    // Восстановление каждой таблицы
    for (const auto& table : tables_to_restore) {
        std::string query = "COPY " + table + " FROM STDIN BINARY";
        PGresult* res = PQexec(conn_, query.c_str());

        if (PQresultStatus(res) == PGRES_COPY_IN) {
            char* buffer = new char[8192];
            int len;
            while ((len = (int)backup_file.readsome(buffer, 8192)) > 0) {
                PQputCopyData(conn_, buffer, len);
            }
            delete[] buffer;
            PQputCopyEnd(conn_, NULL);
        }
        else {
            Logger::log(Logger::ERROR, "DatabaseOperator",
                "Failed to restore table: " + table);
            PQclear(res);
            backup_file.close();
            return false;
        }
    }

    backup_file.close();
    Logger::log(Logger::INFO, "DatabaseOperator",
        "Restore completed successfully from: " + in_file);
    return true;
}

void DatabaseOperator::exit() {
    disconnect();
    Logger::log(Logger::INFO, "DatabaseOperator", "DatabaseOperator exited");
}