#include "interface.h"

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // Инициализация DLL
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        // Очистка
        break;
    }
    return TRUE;
}

DLL_API void RegisterLogCallback(LogCallback callback) {
    Logger::registerCallback(callback);
}

DLL_API void RegisterMigrationProgressCallback(ProgressCallback callback) {
    DatabaseMigrator::registerCallback(callback);
}

DLL_API DatabaseMigrator* InitializeDatabaseMigrator(const char* config_path) {
    return new DatabaseMigrator(config_path);
}

DLL_API bool ExecuteMigration(DatabaseMigrator* migrator) {
    return migrator->execute_migration();
}

DLL_API DatabaseOperator* ConnectDatabase(const char* connection_string) {
    DatabaseOperator* operator_ = new DatabaseOperator();
    if (operator_->connect(connection_string))
        return operator_;
    else
        return nullptr;
}

DLL_API void DisconnectDatabase(DatabaseOperator* operator_) {
    operator_->disconnect();
}

DLL_API bool ExecuteQuery(DatabaseOperator* operator_, const char* query) {
    return operator_->exec(query);
}

DLL_API bool LoadAndExecute(DatabaseOperator* operator_, const char* file_name) {
    std::string script = operator_->load(file_name);
    return operator_->exec(script);
}

DLL_API bool BackupDatabase(DatabaseOperator* operator_, const char* json_config = "", const char* file_path = "") {
    return operator_->backup(json_config, file_path);
}

DLL_API bool RestoreDatabase(DatabaseOperator* operator_, const char* json_config = "", const char* file_path = "") {
    return operator_->restore(json_config, file_path);
}

DLL_API void ExitDatabaseOperator(DatabaseOperator* operator_) {
    operator_->exit();
    delete operator_;
}