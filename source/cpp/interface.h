/*
* =========== INTERFACE ===========
* Описание API (для использовании в виде DLL)
*/

#include "logger.h"
#include "database_migrator.h"
#include "database_operator.h"
#include <windows.h>

#ifdef DLL_EXPORTS
#define DLL_API extern "C" __declspec(dllexport)
#else
#define DLL_API extern "C" __declspec(dllimport)
#endif

// Обеспечение хост-приложению доступа к логам
DLL_API void RegisterLogCallback(LogCallback);
// Обеспечение хост-приложению доступа к прогрессу миграции
DLL_API void RegisterMigrationProgressCallback(ProgressCallback);

// Создание обьекта DatabaseMigrator
DLL_API DatabaseMigrator* InitializeDatabaseMigrator(const char*);
// Запуск миграции на основе загруженного config-файла
DLL_API bool ExecuteMigration(DatabaseMigrator*);

// Подключение к БД в ручном режиме
DLL_API DatabaseOperator* ConnectDatabase(const char*);
// Отключение от базы данных
DLL_API void DisconnectDatabase(DatabaseOperator*);
// Выполнение строкового запроса 
DLL_API bool ExecuteQuery(DatabaseOperator*, const char*);
// Загрузка файла скрипта и его выполнение на подключенной БД
DLL_API bool LoadAndExecute(DatabaseOperator*, const char*);
// Сохранение резервной копии БД
DLL_API bool BackupDatabase(DatabaseOperator*, const char*, const char*);
// Загрузка резервной копии БД
DLL_API bool RestoreDatabase(DatabaseOperator*, const char*, const char*);
// Отключение от БД и удаление экземпляра DatabaseOperator
DLL_API void ExitDatabaseOperator(DatabaseOperator*);