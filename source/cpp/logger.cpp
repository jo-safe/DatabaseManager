#include "Logger.h"
#include <sstream>
#include <iomanip>
#include <fstream>

std::mutex Logger::mtx_;
LogCallback Logger::callback_ = nullptr;

void Logger::registerCallback(LogCallback cb) {
    callback_ = cb;
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%dT%H:%M:%S") << "."
        << std::setfill('0') << std::setw(3) << now_ms.count();

    return ss.str();
}

std::string Logger::logLevelToString(LogLevel level) {
    switch (level) {
    case DEBUG: return "debug";
    case INFO: return "info";
    case WARN: return "warn";
    case ERROR: return "error";
    default: return "unknown";
    }
}

std::string Logger::createLogJson(const LogEntry& entry) {
    std::stringstream json;
    json << "{"
        << "\"timestamp\":\"" << entry.timestamp << "\","
        << "\"level\":\"" << entry.level << "\","
        << "\"module\":\"" << entry.module << "\","
        << "\"message\":\"" << entry.message << "\""
        << "}\n";
    return json.str();
}

void Logger::saveToFile(const std::string& jsonString) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::ofstream file("log.json");
    if (file.is_open()) {
        file << jsonString;
        file.close();
    }
}

void Logger::log(LogLevel level, const std::string& module, const std::string& message) {
    LogEntry entry = {
        getCurrentTimestamp(),
        logLevelToString(level),
        module,
        message
    };

    std::string jsonString = createLogJson(entry);

#ifdef CONSOLE_LOGGING
    if (level == ERROR || level == WARN) {
        LoggerConsole::logError(jsonString);
    }
    else {
        LoggerConsole::log(jsonString);
    }
#endif

    saveToFile(jsonString);  // Сохраняем в файл

    if (callback_ != nullptr)
        callback_(message.c_str());
}

void LoggerConsole::log(const std::string& message) {
    std::cout << message << std::endl;
}

void LoggerConsole::logError(const std::string& message) {
    log(message + " | ERROR");
}