using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Controls;


namespace Uber.DemoTools
{
    public interface AppComponent
    {
        void PopulateViews(DemoInfo demoInfo);
        void SaveToConfigObject(UdtConfig config);

        FrameworkElement RootControl { get; }
        List<ListView> ListViews { get; }
    }

    public class AppSettingsComponent : AppComponent
    {
        private App _app = null;
        private CheckBox _outputModeCheckBox = null;
        private TextBox _outputFolderTextBox = null;
        private FrameworkElement _outputFolderRow = null;
        private CheckBox _folderScanModeCheckBox = null;
        private CheckBox _skipFolderScanModeCheckBox = null;
        private FrameworkElement _skipRecursiveDialog = null;
        private TextBox _maxThreadCountTextBox = null;

        public FrameworkElement RootControl { get; private set; }
        public List<ListView> ListViews { get { return null; } }

        public AppSettingsComponent(App app)
        {
            _app = app;
            RootControl = CreateSettingsControl();
        }

        public void SaveToConfigObject(UdtConfig config)
        {
            config.OutputToInputFolder = _outputModeCheckBox.IsChecked ?? false;
            config.OutputFolder = _outputFolderTextBox.Text;
            GetMaxThreadCount(ref config.MaxThreadCount);
        }

        public void PopulateViews(DemoInfo demoInfo)
        {
            // Nothing to do.
        }

        private FrameworkElement CreateSettingsControl()
        {
            var outputModeCheckBox = new CheckBox();
            _outputModeCheckBox = outputModeCheckBox;
            outputModeCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            outputModeCheckBox.VerticalAlignment = VerticalAlignment.Center;
            outputModeCheckBox.IsChecked = _app.Config.OutputToInputFolder;
            outputModeCheckBox.Content = " Output cut demos to the input demos' folders?";
            outputModeCheckBox.Checked += (obj, args) => OnSameOutputChecked();
            outputModeCheckBox.Unchecked += (obj, args) => OnSameOutputUnchecked();

            var outputFolderTextBox = new TextBox();
            _outputFolderTextBox = outputFolderTextBox;
            outputFolderTextBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            outputFolderTextBox.VerticalAlignment = VerticalAlignment.Center;
            outputFolderTextBox.Text = _app.Config.OutputFolder;
            outputFolderTextBox.Width = 300;

            var outputFolderBrowseButton = new Button();
            outputFolderBrowseButton.HorizontalAlignment = HorizontalAlignment.Right;
            outputFolderBrowseButton.VerticalAlignment = VerticalAlignment.Center;
            outputFolderBrowseButton.Margin = new Thickness(5, 0, 0, 0);
            outputFolderBrowseButton.Content = "...";
            outputFolderBrowseButton.Width = 40;
            outputFolderBrowseButton.Height = 20;
            outputFolderBrowseButton.Click += (obj, arg) => OnOutputFolderBrowseClicked();

            var outputFolderOpenButton = new Button();
            outputFolderOpenButton.HorizontalAlignment = HorizontalAlignment.Right;
            outputFolderOpenButton.VerticalAlignment = VerticalAlignment.Center;
            outputFolderOpenButton.Margin = new Thickness(5, 0, 0, 0);
            outputFolderOpenButton.Content = "Open";
            outputFolderOpenButton.Width = 40;
            outputFolderOpenButton.Height = 20;
            outputFolderOpenButton.Click += (obj, arg) => OnOutputFolderOpenClicked();

            var outputFolderDockPanel = new DockPanel();
            outputFolderDockPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            outputFolderDockPanel.VerticalAlignment = VerticalAlignment.Center;
            outputFolderDockPanel.LastChildFill = true;
            outputFolderDockPanel.Children.Add(outputFolderOpenButton);
            outputFolderDockPanel.Children.Add(outputFolderBrowseButton);
            outputFolderDockPanel.Children.Add(outputFolderTextBox);
            DockPanel.SetDock(outputFolderOpenButton, Dock.Right);
            DockPanel.SetDock(outputFolderBrowseButton, Dock.Right);

            var skipChatOffsetsDialogCheckBox = new CheckBox();
            skipChatOffsetsDialogCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            skipChatOffsetsDialogCheckBox.VerticalAlignment = VerticalAlignment.Center;
            skipChatOffsetsDialogCheckBox.IsChecked = _app.Config.SkipChatOffsetsDialog;
            skipChatOffsetsDialogCheckBox.Content = " Skip the 'Chat Offsets' dialog";
            skipChatOffsetsDialogCheckBox.Checked += (obj, args) => OnSkipChatOffsetsChecked();
            skipChatOffsetsDialogCheckBox.Unchecked += (obj, args) => OnSkipChatOffsetsUnchecked();

            var skipFolderScanModeCheckBox = new CheckBox();
            _skipFolderScanModeCheckBox = skipFolderScanModeCheckBox;
            skipFolderScanModeCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            skipFolderScanModeCheckBox.VerticalAlignment = VerticalAlignment.Center;
            skipFolderScanModeCheckBox.IsChecked = _app.Config.SkipScanFoldersRecursivelyDialog;
            skipFolderScanModeCheckBox.Content = " Skip the dialog asking if folder scanning should be recursive";
            skipFolderScanModeCheckBox.Checked += (obj, args) => OnSkipFolderScanRecursiveChecked();
            skipFolderScanModeCheckBox.Unchecked += (obj, args) => OnSkipFolderScanRecursiveUnchecked();

            var folderScanModeCheckBox = new CheckBox();
            _folderScanModeCheckBox = folderScanModeCheckBox;
            folderScanModeCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            folderScanModeCheckBox.VerticalAlignment = VerticalAlignment.Center;
            folderScanModeCheckBox.IsChecked = _app.Config.SkipScanFoldersRecursivelyDialog;
            folderScanModeCheckBox.Content = " Scan subfolders recursively?";
            folderScanModeCheckBox.Checked += (obj, args) => OnFolderScanRecursiveChecked();
            folderScanModeCheckBox.Unchecked += (obj, args) => OnFolderScanRecursiveUnchecked();

            var maxThreadCountTextBox = new TextBox();
            _maxThreadCountTextBox = maxThreadCountTextBox;
            maxThreadCountTextBox.ToolTip = "The maximum number of threads that you allow UDT to use during batch process operations";
            maxThreadCountTextBox.HorizontalAlignment = HorizontalAlignment.Left;
            maxThreadCountTextBox.VerticalAlignment = VerticalAlignment.Center;
            maxThreadCountTextBox.Text = _app.Config.MaxThreadCount.ToString();
            maxThreadCountTextBox.Width = 25;

            const int OutputFolderIndex = 1;
            const int SkipRecursiveDialogIndex = 4;
            var panelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            panelList.Add(App.CreateTuple("Output Mode", outputModeCheckBox));
            panelList.Add(App.CreateTuple("=>  Output Folder", outputFolderDockPanel));
            panelList.Add(App.CreateTuple("Chat History", skipChatOffsetsDialogCheckBox));
            panelList.Add(App.CreateTuple("Recursive Scan", skipFolderScanModeCheckBox));
            panelList.Add(App.CreateTuple("=> Recursive", folderScanModeCheckBox));
            panelList.Add(App.CreateTuple("Max Thread Count", maxThreadCountTextBox));

            var settingsPanel = WpfHelper.CreateDualColumnPanel(panelList, 120, 2);
            settingsPanel.HorizontalAlignment = HorizontalAlignment.Left;
            settingsPanel.VerticalAlignment = VerticalAlignment.Top;
            settingsPanel.Margin = new Thickness(0);

            var settingStackPanel = settingsPanel as StackPanel;
            _outputFolderRow = settingStackPanel.Children[OutputFolderIndex] as FrameworkElement;
            _outputFolderRow.Visibility = _app.Config.OutputToInputFolder ? Visibility.Collapsed : Visibility.Visible;
            _skipRecursiveDialog = settingStackPanel.Children[SkipRecursiveDialogIndex] as FrameworkElement;
            _skipRecursiveDialog.Visibility = _app.Config.SkipScanFoldersRecursivelyDialog ? Visibility.Visible : Visibility.Collapsed;

            var settingsGroupBox = new GroupBox();
            settingsGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            settingsGroupBox.VerticalAlignment = VerticalAlignment.Top;
            settingsGroupBox.Margin = new Thickness(5);
            settingsGroupBox.Header = "Settings";
            settingsGroupBox.Content = settingsPanel;

            return settingsGroupBox;
        }

        private void OnSameOutputChecked()
        {
            _outputFolderRow.Visibility = Visibility.Collapsed;
            _app.Config.OutputToInputFolder = true;
        }

        private void OnSameOutputUnchecked()
        {
            _outputFolderRow.Visibility = Visibility.Visible;
            _app.Config.OutputToInputFolder = false;
        }

        private void OnOutputFolderBrowseClicked()
        {
            using(var openFolderDialog = new System.Windows.Forms.FolderBrowserDialog())
            {
                openFolderDialog.Description = "Browse for a folder the processed demos will get written to";
                openFolderDialog.ShowNewFolderButton = true;
                openFolderDialog.RootFolder = Environment.SpecialFolder.Desktop;
                if(openFolderDialog.ShowDialog() != System.Windows.Forms.DialogResult.OK)
                {
                    return;
                }

                _outputFolderTextBox.Text = openFolderDialog.SelectedPath;
            }
        }

        private void OnOutputFolderOpenClicked()
        {
            var folderPath = _outputFolderTextBox.Text;
            if(!Directory.Exists(folderPath))
            {
                return;
            }

            try
            {
                Process.Start(folderPath);
            }
            catch(Exception exception)
            {
                _app.LogError("Failed to open the output folder: " + exception.Message);
            }
        }

        private void OnSkipChatOffsetsChecked()
        {
            _app.Config.SkipChatOffsetsDialog = true;
        }

        private void OnSkipChatOffsetsUnchecked()
        {
            _app.Config.SkipChatOffsetsDialog = false;
        }

        private void OnSkipFolderScanRecursiveChecked()
        {
            _skipRecursiveDialog.Visibility = Visibility.Visible;
            _app.Config.SkipScanFoldersRecursivelyDialog = true;
        }

        private void OnSkipFolderScanRecursiveUnchecked()
        {
            _skipRecursiveDialog.Visibility = Visibility.Collapsed;
            _app.Config.SkipScanFoldersRecursivelyDialog = false;
        }

        private void OnFolderScanRecursiveChecked()
        {
            _app.Config.ScanFoldersRecursively = true;
        }

        private void OnFolderScanRecursiveUnchecked()
        {
            _app.Config.ScanFoldersRecursively = false;
        }

        private void GetMaxThreadCount(ref int maxThreadCount)
        {
            int value = 0;
            if(!int.TryParse(_maxThreadCountTextBox.Text, out value))
            {
                return;
            }

            if(value > 0)
            {
                maxThreadCount = value;
            }
        }
    }
}