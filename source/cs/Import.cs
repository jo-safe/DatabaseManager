using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace DDL_MigratorGUI
{
    public static class DatabaseManager
    {
        public const string DatabaseManagerDLLPath = "database_manager.dll";

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void LogCallback([MarshalAs(UnmanagedType.LPStr)] string message);
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void ProgressCallback(int progress);

        [DllImport(DatabaseManagerDLLPath, CallingConvention = CallingConvention.Cdecl)]
        public static extern void RegisterLogCallback(LogCallback cb);

        [DllImport(DatabaseManagerDLLPath, CallingConvention = CallingConvention.Cdecl)]
        public static extern void RegisterMigrationProgressCallback(ProgressCallback cb);


        [DllImport(DatabaseManagerDLLPath, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr InitializeDatabaseMigrator(string config_path);

        [DllImport(DatabaseManagerDLLPath, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool ExecuteMigration(IntPtr migrator);


        [DllImport(DatabaseManagerDLLPath, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr ConnectDatabase(string connection_string);


        [DllImport(DatabaseManagerDLLPath, CallingConvention = CallingConvention.Cdecl)]
        public static extern void DisconnectDatabase(IntPtr operator_);


        [DllImport(DatabaseManagerDLLPath, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool ExecuteQuery(IntPtr operator_, string query);


        [DllImport(DatabaseManagerDLLPath, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool LoadAndExecute(IntPtr operator_, string script_path);


        [DllImport(DatabaseManagerDLLPath, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool BackupDatabase(IntPtr operator_, string config_path, string file_path);


        [DllImport(DatabaseManagerDLLPath, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool RestoreDatabase(IntPtr operator_, string config_path, string file_path);


        [DllImport(DatabaseManagerDLLPath, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ExitDatabaseOperator(IntPtr operator_);
    }
}