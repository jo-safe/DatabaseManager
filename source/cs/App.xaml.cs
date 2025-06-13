using System;
using System.Windows;

namespace DDL_MigratorGUI
{
    public partial class App : Application
    {
        private MainWindow _mainWindow = null!;
        private DatabaseManager.LogCallback _logCallback = null!;
        private DatabaseManager.ProgressCallback _progressCallback = null!;

        public IntPtr _DatabaseMigrator;
        public IntPtr _DatabaseOperator;

        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);
            _mainWindow = new MainWindow(this);
            _mainWindow.Show();

            _logCallback = new DatabaseManager.LogCallback(_mainWindow.LogMessage);
            DatabaseManager.RegisterLogCallback(_logCallback);

            _progressCallback = new DatabaseManager.ProgressCallback(_mainWindow.UpdateProgress);
            DatabaseManager.RegisterMigrationProgressCallback(_progressCallback);
        }

        protected override void OnExit(ExitEventArgs e)
        {
            base.OnExit(e);
            try
            {
                DatabaseManager.ExitDatabaseOperator(_DatabaseOperator);
            }
            catch (Exception) { }
        }
    }
}
