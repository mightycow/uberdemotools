using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Controls;


namespace Uber.DemoTools
{
    public partial class App
    {
        private CheckBox _outputModeCheckBox = null;
        private TextBox _outputFolderTextBox = null;
        private FrameworkElement _outputFolderRow = null;
        private CheckBox _folderScanModeCheckBox = null;
        private CheckBox _skipFolderScanModeCheckBox = null;
        private FrameworkElement _skipRecursiveDialog = null;

        private FrameworkElement CreateSettingsTab()
        {
            var outputModeCheckBox = new CheckBox();
            _outputModeCheckBox = outputModeCheckBox;
            outputModeCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            outputModeCheckBox.VerticalAlignment = VerticalAlignment.Center;
            outputModeCheckBox.IsChecked = _config.OutputToInputFolder;
            outputModeCheckBox.Content = " Output cut demos to the input demos' folders?";
            outputModeCheckBox.Checked += (obj, args) => OnSameOutputChecked();
            outputModeCheckBox.Unchecked += (obj, args) => OnSameOutputUnchecked();

            var outputFolderTextBox = new TextBox();
            _outputFolderTextBox = outputFolderTextBox;
            outputFolderTextBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            outputFolderTextBox.VerticalAlignment = VerticalAlignment.Center;
            outputFolderTextBox.Text = _config.OutputFolder;
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
            skipChatOffsetsDialogCheckBox.IsChecked = _config.SkipChatOffsetsDialog;
            skipChatOffsetsDialogCheckBox.Content = " Skip the 'Chat Offsets' dialog";
            skipChatOffsetsDialogCheckBox.Checked += (obj, args) => OnSkipChatOffsetsChecked();
            skipChatOffsetsDialogCheckBox.Unchecked += (obj, args) => OnSkipChatOffsetsUnchecked();

            var skipFolderScanModeCheckBox = new CheckBox();
            _skipFolderScanModeCheckBox = skipFolderScanModeCheckBox;
            skipFolderScanModeCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            skipFolderScanModeCheckBox.VerticalAlignment = VerticalAlignment.Center;
            skipFolderScanModeCheckBox.IsChecked = _config.SkipScanFoldersRecursivelyDialog;
            skipFolderScanModeCheckBox.Content = " Skip the dialog asking if folder scanning should be recursive";
            skipFolderScanModeCheckBox.Checked += (obj, args) => OnSkipFolderScanRecursiveChecked();
            skipFolderScanModeCheckBox.Unchecked += (obj, args) => OnSkipFolderScanRecursiveUnchecked();

            var folderScanModeCheckBox = new CheckBox();
            _folderScanModeCheckBox = folderScanModeCheckBox;
            folderScanModeCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            folderScanModeCheckBox.VerticalAlignment = VerticalAlignment.Center;
            folderScanModeCheckBox.IsChecked = _config.SkipScanFoldersRecursivelyDialog;
            folderScanModeCheckBox.Content = " Scan subfolders recursively?";
            folderScanModeCheckBox.Checked += (obj, args) => OnFolderScanRecursiveChecked();
            folderScanModeCheckBox.Unchecked += (obj, args) => OnFolderScanRecursiveUnchecked();

            const int OutputFolderIndex = 1;
            const int SkipRecursiveDialogIndex = 4;
            var panelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            panelList.Add(CreateTuple("Output Mode", outputModeCheckBox));
            panelList.Add(CreateTuple("=>  Output Folder", outputFolderDockPanel));
            panelList.Add(CreateTuple("Chat History", skipChatOffsetsDialogCheckBox));
            panelList.Add(CreateTuple("Recursive Scan", skipFolderScanModeCheckBox));
            panelList.Add(CreateTuple("=> Recursive", folderScanModeCheckBox));

            var settingsPanel = WpfHelper.CreateDualColumnPanel(panelList, 120, 2);
            settingsPanel.HorizontalAlignment = HorizontalAlignment.Left;
            settingsPanel.VerticalAlignment = VerticalAlignment.Top;
            settingsPanel.Margin = new Thickness(0);

            var settingStackPanel = settingsPanel as StackPanel;
            _outputFolderRow = settingStackPanel.Children[OutputFolderIndex] as FrameworkElement;
            _outputFolderRow.Visibility = _config.OutputToInputFolder ? Visibility.Collapsed : Visibility.Visible;
            _skipRecursiveDialog = settingStackPanel.Children[SkipRecursiveDialogIndex] as FrameworkElement;
            _skipRecursiveDialog.Visibility = _config.SkipScanFoldersRecursivelyDialog ? Visibility.Visible : Visibility.Collapsed;

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
            _config.OutputToInputFolder = true;
        }

        private void OnSameOutputUnchecked()
        {
            _outputFolderRow.Visibility = Visibility.Visible;
            _config.OutputToInputFolder = false;
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
                LogError("Failed to open the output folder: " + exception.Message);
            }
        }

        private void OnSkipChatOffsetsChecked()
        {
            _config.SkipChatOffsetsDialog = true;
        }

        private void OnSkipChatOffsetsUnchecked()
        {
            _config.SkipChatOffsetsDialog = false;
        }

        private void OnSkipFolderScanRecursiveChecked()
        {
            _skipRecursiveDialog.Visibility = Visibility.Visible;
            _config.SkipScanFoldersRecursivelyDialog = true;
        }

        private void OnSkipFolderScanRecursiveUnchecked()
        {
            _skipRecursiveDialog.Visibility = Visibility.Collapsed;
            _config.SkipScanFoldersRecursivelyDialog = false;
        }

        private void OnFolderScanRecursiveChecked()
        {
            _config.ScanFoldersRecursively = true;
        }

        private void OnFolderScanRecursiveUnchecked()
        {
            _config.ScanFoldersRecursively = false;
        }
    }
}