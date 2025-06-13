/*
* ============== DATABASE_OPERATOR ==============
* Реализация ручного доступа к работе с PostgreSQL базой данных.
* Пользователю доступны подключение к базе данных, 
* выполнение SQL-запросов, загрузка SQL-скрипта для выполнения
* на подключенной базе данных. Основные используемые функции:
*   - backup - резервное копирование БД
*   - restore - восстановление резервной копии в открытую БД
*/

#pragma once

#ifndef DATABASE_OPERATOR_H
#define DATABASE_OPERATOR_H

#include <string>
#include <libpq-fe.h>
#include "external/json.hpp"
#include "Logger.h"

using json = nlohmann::json;

class DatabaseOperator {
private:
    PGconn* conn_;
    std::string conn_str_;
    bool connected_;

    void handle_error(const std::string& operation);
    void execute_query(const std::string& query);

public:
    DatabaseOperator();
    ~DatabaseOperator();

    bool connect(const std::string& connStr);
    void disconnect();
    bool exec(const std::string& query);
    std::string load(const std::string& filename);
    bool backup(const std::string& jsonConfig = "", const std::string& outFile = "");
    bool restore(const std::string& jsonConfig = "", const std::string& inFile = "");
    void exit();
};

#endif // DATABASE_OPERATOR_H