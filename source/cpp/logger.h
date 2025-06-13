/*
* ================== LOGGER ==================
* Инструмент логирования, реализующий следующий функционал:
*     1. Запись логов в файл в json-формате (структура LogEntry)
*     2. Логирование в stdout (опционально)
*     3. Передача логов хост-приложению в виде const char*
*         (при вызове хост-приложением registerCallback)
*/

#pragma once

#ifndef LOGGER_H
#define LOGGER_H

//#define CONSOLE_LOGGING

#include <string>
#include <iostream>
#include <mutex>
#include <ctime>
#include <chrono>

typedef void (*LogCallback)(const char* message);

class Logger {
public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static void registerCallback(LogCallback cb);

    enum LogLevel {
        DEBUG = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3
    };

    struct LogEntry {
        std::string timestamp;
        std::string level;
        std::string module;
        std::string message;
    };

    static void log(LogLevel level, const std::string& module, const std::string& message);

private:
    static LogCallback callback_;

    static std::string getCurrentTimestamp();
    static std::string logLevelToString(LogLevel level);
    static std::string createLogJson(const LogEntry& entry);
    static void saveToFile(const std::string& jsonString);
    static std::mutex mtx_;
};

class LoggerConsole {
public:
    LoggerConsole(const LoggerConsole&) = delete;
    LoggerConsole& operator=(const LoggerConsole&) = delete;

    static void log(const std::string& message);
    static void logError(const std::string& message);

    static bool initialize();
};

#endif // LOGGER_H