using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Controls;


namespace Uber.DemoTools
{
    public enum ComponentType
    {
        Settings,
        ChatEvents,
        FragEvents,
        Patterns,
        CutByTime,
        CutByPattern,
        ChatFilters,
        MidAirFilters,
        MultiRailFilters,
        Count
    }

    public interface AppComponent
    {
        void PopulateViews(DemoInfo demoInfo);
        void SaveToConfigObject(UdtConfig config);
        void SaveToConfigObject(UdtPrivateConfig config);

        FrameworkElement RootControl { get; }
        List<DemoInfoListView> AllListViews { get; }
        List<DemoInfoListView> InfoListViews { get; }
        ComponentType Type { get; }
        bool MultiDemoMode { get; }
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
        private TextBox _inputFolderTextBox = null;
        private FrameworkElement _inputFolderRow = null;
        private CheckBox _useInputFolderForBrowsingCheckBox = null;
        private CheckBox _useInputFolderOnStartUpCheckBox = null;
        private CheckBox _analyzeOnLoadCheckBox = null;
        private TextBox _startTimeOffsetEditBox = null;
        private TextBox _endTimeOffsetEditBox = null;
        private CheckBox _printAllocStatsCheckBox = null;
        private CheckBox _printExecutionTimeCheckBox = null;

        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return null; } }
        public List<DemoInfoListView> InfoListViews { get { return null; } }
        public ComponentType Type { get { return ComponentType.Settings; } }
        public bool MultiDemoMode { get { return true; } }

        public AppSettingsComponent(App app)
        {
            _app = app;
            RootControl = CreateSettingsControl();
        }

        public void SaveToConfigObject(UdtConfig config)
        {
            int time = 0;
            if(App.GetOffsetSeconds(_startTimeOffsetEditBox.Text, out time))
            {
                config.CutStartOffset = time;
            }
            if(App.GetOffsetSeconds(_endTimeOffsetEditBox.Text, out time))
            {
                config.CutEndOffset = time;
            }

            config.OutputToInputFolder = _outputModeCheckBox.IsChecked ?? false;
            config.OutputFolder = _outputFolderTextBox.Text;
            config.InputFolder = _inputFolderTextBox.Text;
            config.UseInputFolderAsDefaultBrowsingLocation = _useInputFolderForBrowsingCheckBox.IsChecked ?? false;
            config.OpenDemosFromInputFolderOnStartUp = _useInputFolderOnStartUpCheckBox.IsChecked ?? false;
            config.AnalyzeOnLoad = _analyzeOnLoadCheckBox.IsChecked ?? false;
            config.PrintAllocationStats = _printAllocStatsCheckBox.IsChecked ?? false;
            config.PrintExecutionTime = _printExecutionTimeCheckBox.IsChecked ?? false;
            GetMaxThreadCount(ref config.MaxThreadCount);
        }

        public void SaveToConfigObject(UdtPrivateConfig config)
        {
            // Nothing to do.
        }

        public void PopulateViews(DemoInfo demoInfo)
        {
            // Nothing to do.
        }

        private FrameworkElement CreateSettingsControl()
        {
            var startTimeOffsetEditBox = new TextBox();
            _startTimeOffsetEditBox = startTimeOffsetEditBox;
            startTimeOffsetEditBox.Width = 40;
            startTimeOffsetEditBox.Text = _app.Config.CutStartOffset.ToString();
            startTimeOffsetEditBox.ToolTip = "How many seconds before the (first) pattern matching event do we start the cut?";

            var endTimeOffsetEditBox = new TextBox();
            _endTimeOffsetEditBox = endTimeOffsetEditBox;
            endTimeOffsetEditBox.Width = 40;
            endTimeOffsetEditBox.Text = _app.Config.CutEndOffset.ToString();
            endTimeOffsetEditBox.ToolTip = "How many seconds after the (last) pattern matching event event do we end the cut?";

            var outputModeCheckBox = new CheckBox();
            _outputModeCheckBox = outputModeCheckBox;
            outputModeCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            outputModeCheckBox.VerticalAlignment = VerticalAlignment.Center;
            outputModeCheckBox.IsChecked = _app.Config.OutputToInputFolder;
            outputModeCheckBox.Content = " Output cut demos to the input demos' folders?";
            outputModeCheckBox.Checked += (obj, args) => OnSameOutputChecked();
            outputModeCheckBox.Unchecked += (obj, args) => OnSameOutputUnchecked();

            var outputFolderRow = CreateFolderRow(
                ref _outputFolderTextBox, 
                _app.Config.OutputFolder, 
                "Browse for the folder the processed demos will get written to");

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
            folderScanModeCheckBox.IsChecked = _app.Config.ScanFoldersRecursively;
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

            var useInputFolderForBrowsingCheckBox = new CheckBox();
            _useInputFolderForBrowsingCheckBox = useInputFolderForBrowsingCheckBox;
            useInputFolderForBrowsingCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            useInputFolderForBrowsingCheckBox.VerticalAlignment = VerticalAlignment.Center;
            useInputFolderForBrowsingCheckBox.IsChecked = _app.Config.UseInputFolderAsDefaultBrowsingLocation;
            useInputFolderForBrowsingCheckBox.Content = " Use input folder as default browsing location?";
            useInputFolderForBrowsingCheckBox.Checked += (obj, args) => OnInputFolderForBrowsingChecked();
            useInputFolderForBrowsingCheckBox.Unchecked += (obj, args) => OnInputFolderForBrowsingUnchecked();

            var useInputFolderOnStartUpCheckBox = new CheckBox();
            _useInputFolderOnStartUpCheckBox = useInputFolderOnStartUpCheckBox;
            useInputFolderOnStartUpCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            useInputFolderOnStartUpCheckBox.VerticalAlignment = VerticalAlignment.Center;
            useInputFolderOnStartUpCheckBox.IsChecked = _app.Config.OpenDemosFromInputFolderOnStartUp;
            useInputFolderOnStartUpCheckBox.Content = " Open demos from input folder on application start-up?";
            useInputFolderOnStartUpCheckBox.Checked += (obj, args) => OnUseInputFolderOnStartUpChecked();
            useInputFolderOnStartUpCheckBox.Unchecked += (obj, args) => OnUseInputFolderOnStartUpUnchecked();

            var inputFolderRow = CreateFolderRow(
                ref _inputFolderTextBox,
                _app.Config.InputFolder,
                "Browse for the folder demos will be read or searched from");

            var analyzeOnLoadCheckBox = new CheckBox();
            _analyzeOnLoadCheckBox = analyzeOnLoadCheckBox;
            analyzeOnLoadCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            analyzeOnLoadCheckBox.VerticalAlignment = VerticalAlignment.Center;
            analyzeOnLoadCheckBox.IsChecked = _app.Config.AnalyzeOnLoad;
            analyzeOnLoadCheckBox.Content = " Analyze demos when loading them into the list?";
            analyzeOnLoadCheckBox.Checked += (obj, args) => OnAnalyzeOnLoadChecked();
            analyzeOnLoadCheckBox.Unchecked += (obj, args) => OnAnalyzeOnLoadUnchecked();
            analyzeOnLoadCheckBox.ToolTip = " You can always launch the analysis pass from the \"Manage\" tab.";

            var printAllocStatsCheckBox = new CheckBox();
            _printAllocStatsCheckBox = printAllocStatsCheckBox;
            printAllocStatsCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            printAllocStatsCheckBox.VerticalAlignment = VerticalAlignment.Center;
            printAllocStatsCheckBox.IsChecked = _app.Config.PrintAllocationStats;
            printAllocStatsCheckBox.Content = " Print memory allocations stats when job processing is finished?";
            printAllocStatsCheckBox.Checked += (obj, args) => OnPrintAllocStatsChecked();
            printAllocStatsCheckBox.Unchecked += (obj, args) => OnPrintAllocStatsUnchecked();

            var printExecutionTimeCheckBox = new CheckBox();
            _printExecutionTimeCheckBox = printExecutionTimeCheckBox;
            printExecutionTimeCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            printExecutionTimeCheckBox.VerticalAlignment = VerticalAlignment.Center;
            printExecutionTimeCheckBox.IsChecked = _app.Config.PrintExecutionTime;
            printExecutionTimeCheckBox.Content = " Print execution time when job processing is finished?";
            printExecutionTimeCheckBox.Checked += (obj, args) => OnPrintExecutionTimeChecked();
            printExecutionTimeCheckBox.Unchecked += (obj, args) => OnPrintExecutionTimeUnchecked();

            const int OutputFolderIndex = 1;
            const int SkipRecursiveDialogIndex = 4;
            const int InputFolderIndex = 8;
            var panelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            panelList.Add(App.CreateTuple("Output Mode", outputModeCheckBox));
            panelList.Add(App.CreateTuple("=>  Output Folder", outputFolderRow));
            panelList.Add(App.CreateTuple("Chat History", skipChatOffsetsDialogCheckBox));
            panelList.Add(App.CreateTuple("Recursive Scan", skipFolderScanModeCheckBox));
            panelList.Add(App.CreateTuple("=> Recursive", folderScanModeCheckBox));
            panelList.Add(App.CreateTuple("Max Thread Count", maxThreadCountTextBox));
            panelList.Add(App.CreateTuple("Browsing Location", useInputFolderForBrowsingCheckBox));
            panelList.Add(App.CreateTuple("Open on Start-up", useInputFolderOnStartUpCheckBox));
            panelList.Add(App.CreateTuple("=> Input Folder", inputFolderRow));
            panelList.Add(App.CreateTuple("Analyze on Load", analyzeOnLoadCheckBox));
            panelList.Add(App.CreateTuple("Start Time Offset [s]", startTimeOffsetEditBox));
            panelList.Add(App.CreateTuple("End Time Offset [s]", endTimeOffsetEditBox));
            panelList.Add(App.CreateTuple("Print Alloc Stats", printAllocStatsCheckBox));
            panelList.Add(App.CreateTuple("Print Execution Time", printExecutionTimeCheckBox));

            var settingsPanel = WpfHelper.CreateDualColumnPanel(panelList, 135, 2);
            settingsPanel.HorizontalAlignment = HorizontalAlignment.Left;
            settingsPanel.VerticalAlignment = VerticalAlignment.Top;
            settingsPanel.Margin = new Thickness(0);

            var settingStackPanel = settingsPanel as StackPanel;
            _outputFolderRow = settingStackPanel.Children[OutputFolderIndex] as FrameworkElement;
            SetActive(_outputFolderRow, !_app.Config.OutputToInputFolder);
            _skipRecursiveDialog = settingStackPanel.Children[SkipRecursiveDialogIndex] as FrameworkElement;
            SetActive(_skipRecursiveDialog, _app.Config.SkipScanFoldersRecursivelyDialog);
            _inputFolderRow = settingStackPanel.Children[InputFolderIndex] as FrameworkElement;
            UpdateInputFolderActive();

            var settingsGroupBox = new GroupBox();
            settingsGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            settingsGroupBox.VerticalAlignment = VerticalAlignment.Top;
            settingsGroupBox.Margin = new Thickness(5);
            settingsGroupBox.Header = "Settings";
            settingsGroupBox.Content = settingsPanel;

            return settingsGroupBox;
        }

        private FrameworkElement CreateFolderRow(ref TextBox textBox, string defaultValue, string browseDesc)
        {
            var folderTextBox = new TextBox();
            textBox = folderTextBox;
            folderTextBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            folderTextBox.VerticalAlignment = VerticalAlignment.Center;
            folderTextBox.Text = defaultValue;
            folderTextBox.Width = 300;

            var folderBrowseButton = new Button();
            folderBrowseButton.HorizontalAlignment = HorizontalAlignment.Right;
            folderBrowseButton.VerticalAlignment = VerticalAlignment.Center;
            folderBrowseButton.Margin = new Thickness(5, 0, 0, 0);
            folderBrowseButton.Content = "...";
            folderBrowseButton.Width = 40;
            folderBrowseButton.Height = 20;
            folderBrowseButton.Click += (obj, arg) => BrowseForFolder(folderTextBox, browseDesc);

            var folderOpenButton = new Button();
            folderOpenButton.HorizontalAlignment = HorizontalAlignment.Right;
            folderOpenButton.VerticalAlignment = VerticalAlignment.Center;
            folderOpenButton.Margin = new Thickness(5, 0, 0, 0);
            folderOpenButton.Content = "Open";
            folderOpenButton.Width = 40;
            folderOpenButton.Height = 20;
            folderOpenButton.Click += (obj, arg) => OpenFolder(folderTextBox.Text);

            var folderDockPanel = new DockPanel();
            folderDockPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            folderDockPanel.VerticalAlignment = VerticalAlignment.Center;
            folderDockPanel.LastChildFill = true;
            folderDockPanel.Children.Add(folderOpenButton);
            folderDockPanel.Children.Add(folderBrowseButton);
            folderDockPanel.Children.Add(folderTextBox);
            DockPanel.SetDock(folderOpenButton, Dock.Right);
            DockPanel.SetDock(folderBrowseButton, Dock.Right);

            return folderDockPanel;
        }

        private void OnSameOutputChecked()
        {
            SetActive(_outputFolderRow, false);
            _app.Config.OutputToInputFolder = true;
        }

        private void OnSameOutputUnchecked()
        {
            SetActive(_outputFolderRow, true);
            _app.Config.OutputToInputFolder = false;
        }

        private void BrowseForFolder(TextBox folderTextBox, string desc)
        {
            using(var openFolderDialog = new System.Windows.Forms.FolderBrowserDialog())
            {
                openFolderDialog.Description = desc;
                openFolderDialog.ShowNewFolderButton = true;
                if(!string.IsNullOrWhiteSpace(folderTextBox.Text) && Directory.Exists(folderTextBox.Text))
                {
                    openFolderDialog.SelectedPath = folderTextBox.Text;
                }
                if(openFolderDialog.ShowDialog() != System.Windows.Forms.DialogResult.OK)
                {
                    return;
                }

                folderTextBox.Text = openFolderDialog.SelectedPath;
            }
        }

        private void OpenFolder(string folderPath)
        {
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
                _app.LogError("Failed to open the folder: " + exception.Message);
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
            SetActive(_skipRecursiveDialog, true);
            _app.Config.SkipScanFoldersRecursivelyDialog = true;
        }

        private void OnSkipFolderScanRecursiveUnchecked()
        {
            SetActive(_skipRecursiveDialog, false);
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

        private void UpdateInputFolderActive()
        {
            SetActive(_inputFolderRow, _app.Config.UseInputFolderAsDefaultBrowsingLocation || _app.Config.OpenDemosFromInputFolderOnStartUp);
        }

        private void OnInputFolderForBrowsingChecked()
        {
            _app.Config.UseInputFolderAsDefaultBrowsingLocation = true;
            UpdateInputFolderActive();
        }

        private void OnInputFolderForBrowsingUnchecked()
        {
            _app.Config.UseInputFolderAsDefaultBrowsingLocation = false;
            UpdateInputFolderActive();
        }

        private void OnUseInputFolderOnStartUpChecked()
        {
            _app.Config.OpenDemosFromInputFolderOnStartUp = true;
            UpdateInputFolderActive();
        }

        private void OnUseInputFolderOnStartUpUnchecked()
        {
            _app.Config.OpenDemosFromInputFolderOnStartUp = false;
            UpdateInputFolderActive();
        }

        private void OnAnalyzeOnLoadChecked()
        {
            _app.Config.AnalyzeOnLoad = true;
        }

        private void OnAnalyzeOnLoadUnchecked()
        {
            _app.Config.AnalyzeOnLoad = false;
        }

        private void OnPrintAllocStatsChecked()
        {
            _app.Config.PrintAllocationStats = true;
        }

        private void OnPrintAllocStatsUnchecked()
        {
            _app.Config.PrintAllocationStats = false;
        }

        private void OnPrintExecutionTimeChecked()
        {
            _app.Config.PrintExecutionTime = true;
        }

        private void OnPrintExecutionTimeUnchecked()
        {
            _app.Config.PrintExecutionTime = false;
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

        private void SetActive(FrameworkElement element, bool active)
        {
            element.Opacity = active ? 1.0 : 0.5;
        }
    }
}