using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Controls;


namespace Uber.DemoTools
{
    // @TODO: Move this...
    public enum CSharpPerfStats
    {
        Duration,
        FileCount,
        Count
    }

    public static class CSharpPerfStatsConstants
    {
        public static readonly string[] Strings = new string[(int)CSharpPerfStats.Count]
        {
            "duration with C# overhead",
            "file count"
        };
    }

    // @TODO: Move this...
    public static class BitManip
    {
        public static bool IsBitSet(uint mask, int bitIndex)
        {
            return (mask & ((uint)1 << bitIndex)) != 0;
        }

        public static void SetBit(ref uint mask, int bitIndex)
        {
            mask |= (uint)1 << bitIndex;
        }

        public static void ClearBit(ref uint mask, int bitIndex)
        {
            mask &= ~((uint)1 << bitIndex);
        }

        public static int PopCnt(uint mask)
        {
            var count = 0;
            for(var i = 0; i < 32; ++i)
            {
                if(IsBitSet(mask, i))
                {
                    ++count;
                }
            }

            return count;
        }
    }

    public enum ComponentType
    {
        Settings,
        ChatEvents,
        FragEvents,
        Stats,
        Commands,
        Captures,
        Scores,
        Patterns,
        CutByTime,
        CutByPattern,
        ChatFilters,
        MidAirFilters,
        MultiRailFilters,
        FlagCaptureFilters,
        FlickRailFilters,
        MatchFilters,
        Modifiers,
        TimeShiftModifier,
        SearchResults,
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
        private CheckBox _mergeCutSectionsCheckBox = null;
        private CheckBox _colorLogMessagesCheckBox = null;
        private CheckBox _runUpdaterAtStartUpCheckBox = null;
        private readonly List<CheckBox> _jsonEnabledPlugInsCheckBoxes = new List<CheckBox>();
        private readonly List<CheckBox> _enabledPerfStatsCheckBoxes = new List<CheckBox>();
        private readonly List<CheckBox> _enabledCSharpPerfStatsCheckBoxes = new List<CheckBox>();

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
            config.MergeCutSectionsFromDifferentPatterns = _mergeCutSectionsCheckBox.IsChecked ?? false;
            config.ColorLogWarningsAndErrors = _colorLogMessagesCheckBox.IsChecked ?? false;
            GetMaxThreadCount(ref config.MaxThreadCount);
            config.JSONPlugInsEnabled = CreateBitMask(_jsonEnabledPlugInsCheckBoxes);
            config.PerfStatsEnabled = CreateBitMask(_enabledPerfStatsCheckBoxes);
            config.CSharpPerfStatsEnabled = CreateBitMask(_enabledCSharpPerfStatsCheckBoxes);
        }

        public void SaveToConfigObject(UdtPrivateConfig config)
        {
            // Nothing to do.
        }

        public void PopulateViews(DemoInfo demoInfo)
        {
            // Nothing to do.
        }

        public void SetInputFolderPath(string path)
        {
            _inputFolderTextBox.Text = path;
        }

        public void SetOutputFolderPath(string path)
        {
            _outputFolderTextBox.Text = path;
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
            skipChatOffsetsDialogCheckBox.Content = " Skip the 'Cut Offsets' dialog?";
            skipChatOffsetsDialogCheckBox.Checked += (obj, args) => OnSkipChatOffsetsChecked();
            skipChatOffsetsDialogCheckBox.Unchecked += (obj, args) => OnSkipChatOffsetsUnchecked();

            var skipFolderScanModeCheckBox = new CheckBox();
            _skipFolderScanModeCheckBox = skipFolderScanModeCheckBox;
            skipFolderScanModeCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            skipFolderScanModeCheckBox.VerticalAlignment = VerticalAlignment.Center;
            skipFolderScanModeCheckBox.IsChecked = _app.Config.SkipScanFoldersRecursivelyDialog;
            skipFolderScanModeCheckBox.Content = " Skip the dialog asking if folder scanning should be recursive?";
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
            maxThreadCountTextBox.ToolTip = 
                "The maximum number of threads that you allow UDT to use during batch process operations\n" + 
                "Only set this to a number greater than 1 if you read your demo files from an SSD\n" + 
                "Otherwise, performance will decrease";
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
            analyzeOnLoadCheckBox.Checked += (obj, args) => _app.Config.AnalyzeOnLoad = true;
            analyzeOnLoadCheckBox.Unchecked += (obj, args) => _app.Config.AnalyzeOnLoad = false;
            analyzeOnLoadCheckBox.ToolTip = " You can always launch the analysis pass from the \"Manage\" tab.";

            var mergeCutSectionsCheckBox = new CheckBox();
            _mergeCutSectionsCheckBox = mergeCutSectionsCheckBox;
            mergeCutSectionsCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            mergeCutSectionsCheckBox.VerticalAlignment = VerticalAlignment.Center;
            mergeCutSectionsCheckBox.IsChecked = _app.Config.MergeCutSectionsFromDifferentPatterns;
            mergeCutSectionsCheckBox.Content = " Merge overlapping cut sections matched by different pattern types?";
            mergeCutSectionsCheckBox.Checked += (obj, args) => _app.Config.MergeCutSectionsFromDifferentPatterns = true;
            mergeCutSectionsCheckBox.Unchecked += (obj, args) => _app.Config.MergeCutSectionsFromDifferentPatterns = false;

            var colorLogMessagesCheckBox = new CheckBox();
            _colorLogMessagesCheckBox = colorLogMessagesCheckBox;
            colorLogMessagesCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            colorLogMessagesCheckBox.VerticalAlignment = VerticalAlignment.Center;
            colorLogMessagesCheckBox.IsChecked = _app.Config.ColorLogWarningsAndErrors;
            colorLogMessagesCheckBox.Content = " Color the log window's warning and error messages?";
            colorLogMessagesCheckBox.Checked += (obj, args) => _app.Config.ColorLogWarningsAndErrors = true;
            colorLogMessagesCheckBox.Unchecked += (obj, args) => _app.Config.ColorLogWarningsAndErrors = false;
            colorLogMessagesCheckBox.ToolTip = "The option is disabled by default because it might not integrate well with your current theme.";

            var runUpdaterAtStartUpCheckBox = new CheckBox();
            _runUpdaterAtStartUpCheckBox = runUpdaterAtStartUpCheckBox;
            runUpdaterAtStartUpCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            runUpdaterAtStartUpCheckBox.VerticalAlignment = VerticalAlignment.Center;
            runUpdaterAtStartUpCheckBox.IsChecked = _app.Config.RunUpdaterAtStartUp;
            runUpdaterAtStartUpCheckBox.Content = " Run the updater at application start-up time?";
            runUpdaterAtStartUpCheckBox.Checked += (obj, args) => _app.Config.RunUpdaterAtStartUp = true;
            runUpdaterAtStartUpCheckBox.Unchecked += (obj, args) => _app.Config.RunUpdaterAtStartUp = false;

            const int OutputFolderIndex = 1;
            const int SkipRecursiveDialogIndex = 4;
            const int InputFolderIndex = 8;
            var panelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            panelList.Add(App.CreateTuple("Output Mode", outputModeCheckBox));
            panelList.Add(App.CreateTuple("=>  Output Folder", outputFolderRow));
            panelList.Add(App.CreateTuple("Chat/Death History", skipChatOffsetsDialogCheckBox));
            panelList.Add(App.CreateTuple("Recursive Scan", skipFolderScanModeCheckBox));
            panelList.Add(App.CreateTuple("=> Recursive", folderScanModeCheckBox));
            panelList.Add(App.CreateTuple("Max Thread Count", maxThreadCountTextBox));
            panelList.Add(App.CreateTuple("Browsing Location", useInputFolderForBrowsingCheckBox));
            panelList.Add(App.CreateTuple("Open on Start-up", useInputFolderOnStartUpCheckBox));
            panelList.Add(App.CreateTuple("=> Input Folder", inputFolderRow));
            panelList.Add(App.CreateTuple("Analyze on Load", analyzeOnLoadCheckBox));
            panelList.Add(App.CreateTuple("Start Time Offset [s]", startTimeOffsetEditBox));
            panelList.Add(App.CreateTuple("End Time Offset [s]", endTimeOffsetEditBox));
            panelList.Add(App.CreateTuple("Merge Cut Sections", mergeCutSectionsCheckBox));
            panelList.Add(App.CreateTuple("Color Log Messages", colorLogMessagesCheckBox));
            panelList.Add(App.CreateTuple("Start-up Updates", runUpdaterAtStartUpCheckBox));

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

            var plugInNames = UDT_DLL.GetStringArray(UDT_DLL.udtStringArray.PlugInNames);
            var jsonPlugInsStackPanel = new StackPanel();
            jsonPlugInsStackPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            jsonPlugInsStackPanel.VerticalAlignment = VerticalAlignment.Stretch;
            jsonPlugInsStackPanel.Margin = new Thickness(5);
            jsonPlugInsStackPanel.Children.Add(new TextBlock { Text = "Select which analyzers are enabled" });
            for(int i = 0; i < (int)UDT_DLL.udtParserPlugIn.Count; ++i)
            {
                var checkBox = new CheckBox();
                checkBox.Margin = new Thickness(5, 5, 0, 0);
                checkBox.Content = " " + plugInNames[i].Capitalize();
                checkBox.IsChecked = BitManip.IsBitSet(_app.Config.JSONPlugInsEnabled, i);
                var iCopy = i; // Make sure we capture a local copy in the lambda.
                checkBox.Checked += (obj, args) => BitManip.SetBit(ref _app.Config.JSONPlugInsEnabled, iCopy);
                checkBox.Unchecked += (obj, args) => BitManip.ClearBit(ref _app.Config.JSONPlugInsEnabled, iCopy);

                _jsonEnabledPlugInsCheckBoxes.Add(checkBox);
                jsonPlugInsStackPanel.Children.Add(checkBox);
            }

            var jsonPlugInsGroupBox = new GroupBox();
            jsonPlugInsGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            jsonPlugInsGroupBox.VerticalAlignment = VerticalAlignment.Top;
            jsonPlugInsGroupBox.Margin = new Thickness(5);
            jsonPlugInsGroupBox.Header = "JSON Export";
            jsonPlugInsGroupBox.Content = jsonPlugInsStackPanel;

            var perfStatsNames = UDT_DLL.GetStringArray(UDT_DLL.udtStringArray.PerfStatsNames);
            var perfStatsStackPanel = new StackPanel();
            perfStatsStackPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            perfStatsStackPanel.VerticalAlignment = VerticalAlignment.Stretch;
            perfStatsStackPanel.Margin = new Thickness(5);
            perfStatsStackPanel.Children.Add(new TextBlock { Text = "Select which job stats are printed in the log window" });
            for(int i = 0; i < (int)CSharpPerfStats.Count; ++i)
            {
                var checkBox = new CheckBox();
                checkBox.Margin = new Thickness(5, 5, 0, 0);
                checkBox.Content = " " + CSharpPerfStatsConstants.Strings[i].Capitalize();
                checkBox.IsChecked = BitManip.IsBitSet(_app.Config.CSharpPerfStatsEnabled, i);
                var iCopy = i; // Make sure we capture a local copy in the lambda.
                checkBox.Checked += (obj, args) => BitManip.SetBit(ref _app.Config.CSharpPerfStatsEnabled, iCopy);
                checkBox.Unchecked += (obj, args) => BitManip.ClearBit(ref _app.Config.CSharpPerfStatsEnabled, iCopy);

                _enabledCSharpPerfStatsCheckBoxes.Add(checkBox);
                perfStatsStackPanel.Children.Add(checkBox);
            }
            for(int i = 0; i < (int)UDT_DLL.StatsConstants.PerfFieldCount; ++i)
            {
                var checkBox = new CheckBox();
                checkBox.Margin = new Thickness(5, 5, 0, 0);
                checkBox.Content = " " + perfStatsNames[i].Capitalize();
                checkBox.IsChecked = BitManip.IsBitSet(_app.Config.PerfStatsEnabled, i);
                var iCopy = i; // Make sure we capture a local copy in the lambda.
                checkBox.Checked += (obj, args) => BitManip.SetBit(ref _app.Config.PerfStatsEnabled, iCopy);
                checkBox.Unchecked += (obj, args) => BitManip.ClearBit(ref _app.Config.PerfStatsEnabled, iCopy);

                _enabledPerfStatsCheckBoxes.Add(checkBox);
                perfStatsStackPanel.Children.Add(checkBox);
            }

            var perfStatsGroupBox = new GroupBox();
            perfStatsGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            perfStatsGroupBox.VerticalAlignment = VerticalAlignment.Top;
            perfStatsGroupBox.Margin = new Thickness(5);
            perfStatsGroupBox.Header = "Performance Logging";
            perfStatsGroupBox.Content = perfStatsStackPanel;

            var rootPanel = new WrapPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.Children.Add(settingsGroupBox);
            rootPanel.Children.Add(perfStatsGroupBox);
            rootPanel.Children.Add(jsonPlugInsGroupBox);

            var scrollViewer = new ScrollViewer();
            scrollViewer.HorizontalAlignment = HorizontalAlignment.Stretch;
            scrollViewer.VerticalAlignment = VerticalAlignment.Stretch;
            scrollViewer.Margin = new Thickness(5);
            scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled;
            scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
            scrollViewer.Content = rootPanel;

            return scrollViewer; 
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

        private static uint CreateBitMask(List<CheckBox> checkBoxes)
        {
            uint mask = 0;
            for(var i = 0; i < checkBoxes.Count; ++i)
            {
                if(checkBoxes[i].IsChecked ?? false)
                {
                    BitManip.SetBit(ref mask, i);
                }
            }

            return mask;
        }
    }
}