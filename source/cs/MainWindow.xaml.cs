using Microsoft.Win32;
using System;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;

namespace DDL_MigratorGUI
{
    public partial class MainWindow : Window
    {
        App _app;
        public MainWindow(App app)
        {
            InitializeComponent();
            _app = app;
            SetWidgetsStatus(false);
        }

        public void SetWidgetsStatus(bool status)
        {
            BT_ExecuteQuery.IsEnabled = status;
            BT_ExecuteScript.IsEnabled = status;
            BT_Backup.IsEnabled = status;
            BT_Restore.IsEnabled = status;
        }

        public void LogMessage(string message)
        {
            Dispatcher.Invoke(() =>
            {
                TB_Log.AppendText(message);
            });
        }

        public void UpdateProgress(int progress) 
        { 
            Dispatcher.Invoke(() => 
            {
                PB_Migration.Value = progress;
            });
        }

        private void BT_ChooseConfig_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new OpenFileDialog
            {
                Title = "Выбор файла конфигурации",
                Filter = "Файлы конфигурации (*.json)|*.json",
                CheckFileExists = true,
                CheckPathExists = true,
                Multiselect = false
            };

            bool? result = dialog.ShowDialog();

            if (result == true)
            {
                TB_ConfigPath.Text = dialog.FileName;
                _app._DatabaseMigrator = DatabaseManager.InitializeDatabaseMigrator(dialog.FileName);
            }
        }

        private async void BT_ExecuteMigration_Click(object sender, RoutedEventArgs e)
        {
            BT_ExecuteMigration.IsEnabled = false;
            L_MigrationStatus.Content = "Статус: миграция запущена...";

            try
            {
                bool res = false;
                await Task.Run(() =>
                {
                    res = DatabaseManager.ExecuteMigration(_app._DatabaseMigrator);
                });
                if (res)
                    L_MigrationStatus.Content = "Статус: миграция выполнена успешно";
                else
                    L_MigrationStatus.Content = "Статус: ошибка выполнения миграции";
            }
            catch (Exception ex)
            {
                L_MigrationStatus.Content = $"Статус: ошибка ({ex.Message})";
            }
            finally
            {
                BT_ExecuteMigration.IsEnabled = true;
            }
        }

        private async void BT_Connect_Click(object sender, RoutedEventArgs e)
        {
            if (((Button)sender).Content.ToString() == "Подключиться")
            {
                BT_Connect.IsEnabled = false;
                string connectionString = $"dbName={TB_DbName.Text} host={TB_Host} port={TB_Port} user={TB_User} password={TB_Password}";
                await Task.Run(() =>
                {
                    _app._DatabaseOperator = DatabaseManager.ConnectDatabase(connectionString);
                });
                if (_app._DatabaseOperator == IntPtr.Zero)
                    L_DbStatus.Content = "Статус: ошибка подключения";
                else
                {
                    L_DbStatus.Content = "Статус: подключение установлено";
                    ((Button)sender).Content = "Отключиться";
                }

                BT_Connect.IsEnabled = true;
                SetWidgetsStatus(_app._DatabaseOperator != IntPtr.Zero);
                ((Button)sender).Content = "Отключиться";
            }
            else
            {
                await Task.Run(() =>
                {
                    DatabaseManager.DisconnectDatabase(_app._DatabaseOperator);
                });
                ((Button)sender).Content = "Подключиться";
            }
        }

        private async void BT_ExecuteQuery_Click(object sender, RoutedEventArgs e)
        {
            SetWidgetsStatus(false);
            L_DbStatus.Content = "Статус: выполнение запроса...";
            string query = TB_Query.Text;
            bool res = false;
            await Task.Run(() =>
            {
                res = DatabaseManager.ExecuteQuery(_app._DatabaseOperator, query);
            });
            if (res)
                L_DbStatus.Content = "Статус: запрос выпонен успешно";
            else
                L_DbStatus.Content = "Статус: ошибка выполнения запроса";
            SetWidgetsStatus(true);
        }

        private async void BT_ExecuteScript_Click(object sender, RoutedEventArgs e)
        {
            SetWidgetsStatus(false);
            L_DbStatus.Content = "Статус: выполнение скрипта...";
            var dialog = new OpenFileDialog
            {
                Title = "Выбор файла SQL-скрипта",
                Filter = "SQL-скрипт (*.sql, *.psql, *.pgsql)|*.sql;*.psql;*.pgsql",
                CheckFileExists = true,
                CheckPathExists = true,
                Multiselect = false
            };

            bool? result = dialog.ShowDialog();
            if (result == true)
            {
                await Task.Run(() =>
                {
                    result = DatabaseManager.LoadAndExecute(_app._DatabaseOperator, dialog.FileName);
                });
                if (result == true)
                    L_DbStatus.Content = "Статус: скрипт выполнент успешно";
                else
                    L_DbStatus.Content = "Статус: ошибка выполнения скрипта";
            }
            SetWidgetsStatus(true);
        }

        private async void BT_Backup_Click(object sender, RoutedEventArgs e)
        {
            SetWidgetsStatus(false);
            L_DbStatus.Content = "Статус: создание резервной копии...";
            string config_path, out_path;
            
            var dialog = new OpenFileDialog
            {
                Title = "Выбор файла конфигунации",
                Filter = "Файлы конфигурации (*.json)|*.json",
                CheckFileExists = true,
                CheckPathExists = true,
                Multiselect = false
            };

            bool? res = dialog.ShowDialog();
            if (res == true)
                config_path = dialog.FileName;
            else return;

            dialog = new OpenFileDialog
            {
                Title = "Выбор файла сохранения резервной копии",
                Filter = "Все файлы (*.*)|*.*",
                CheckFileExists = true,
                CheckPathExists = true,
                Multiselect = false
            };

            if (res == true && dialog.ShowDialog() == true)
            {
                out_path = dialog.FileName;

                try
                {
                    await Task.Run(() =>
                    {
                        res = DatabaseManager.BackupDatabase(_app._DatabaseOperator, config_path, out_path);
                    });
                    if (res == true)
                        L_DbStatus.Content = "Статус: резервная копия успешно создана";
                    else
                        L_DbStatus.Content = "Статус: ошибка резервного копирования";
                }
                catch (Exception ex)
                {
                    L_DbStatus.Content = $"Статус: ошибка резервного копирования ({ex.Message})";
                }
                finally
                {
                    SetWidgetsStatus(true);
                }
            }
        }

        private async void BT_Restore_Click(object sender, RoutedEventArgs e)
        {
            SetWidgetsStatus(false);
            L_DbStatus.Content = "Статус: восстановление резервной копии...";
            string config_path, in_path;

            var dialog = new OpenFileDialog
            {
                Title = "Выбор файла конфигунации",
                Filter = "Файлы конфигурации (*.json)|*.json",
                CheckFileExists = true,
                CheckPathExists = true,
                Multiselect = false
            };

            bool? res = dialog.ShowDialog();
            if (res == true)
                config_path = dialog.FileName;
            else return;

            dialog = new OpenFileDialog
            {
                Title = "Выбор файла восстановления резервной копии",
                Filter = "Все файлы (*.*)|*.*",
                CheckFileExists = true,
                CheckPathExists = true,
                Multiselect = false
            };

            if (res == true && dialog.ShowDialog() == true)
            {
                in_path = dialog.FileName;

                try
                {
                    await Task.Run(() =>
                    {
                        res = DatabaseManager.RestoreDatabase(_app._DatabaseOperator, config_path, in_path);
                    });
                    if (res == true)
                        L_DbStatus.Content = "Статус: резервная копия успешно восстановлена";
                    else
                        L_DbStatus.Content = "Статус: ошибка восстановления резервной копии";
                }
                catch (Exception ex)
                {
                    L_DbStatus.Content = $"Статус: ошибка восстановления резервной копии ({ex.Message})";
                }
                finally
                {
                    SetWidgetsStatus(true);
                }
            }
        }
    }
}