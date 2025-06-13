/*
* ================== DATABASE_MIGRATOR ==================
* јвтоматизированный инструмент дл€ работы с двум€ базами данных: исходной
* и создаваемой. ¬сю информацию берет из config-файла, написанного в
* формате json и содержащего:
*   1. ƒанные дл€ доступа к исходной PostgreSQL базе данных
*   2. ƒанные дл€ доступа к выходной PostgreSQL базе данных
*   3. »нформацию о таблицах и требовани€х к их обработке
*/

#pragma once

#ifndef DATABASE_MIGRATOR_H
#define DATABASE_MIGRATOR_H

#include <string>
#include <fstream>
#include <vector>
#include <map>
#include "external/json.hpp"
#include <libpq-fe.h>
#include "Logger.h"

using json = nlohmann::json;

typedef void (*ProgressCallback)(int);

struct DatabaseConfig {
    std::string host;
    int port;
    std::string dbname;
    std::string user;
    std::string password;

    DatabaseConfig& operator=(const json& j);
};

struct TableConfig {
    std::string source;
    std::string target;
    bool exclude;
    bool create_if_missing;
    std::map<std::string, std::map<std::string, std::string>> columns;

    TableConfig(const json& j);
    TableConfig& operator=(const json& j);
};

class DatabaseMigrator {
private:
    static ProgressCallback callback_;

    DatabaseConfig source_db;
    DatabaseConfig target_db;
    std::vector<TableConfig> tables;
    bool isConfigInitialized = false;

    void load_config();
    std::string create_connection_string(const DatabaseConfig& config);
    void migrate();
    void migrate_table(const TableConfig& table_config);
    std::string generate_ddl(const TableConfig& table_config);
    std::string convert_value(const std::string& value, const std::string& type);

public:
    std::string config_path;

    static void registerCallback(ProgressCallback cb);

    DatabaseMigrator(std::string config_path);
    bool execute_migration();
};

#endif // DATABASE_MIGRATOR_H