using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;


namespace Uber.DemoTools
{
    public class TimeOffsetsDialog
    {
        private int _startOffset = -1;
        private int _endOffset = -1;
        private bool _valid = false;

        public int StartOffset
        {
            get { return _startOffset; }
        }

        public int EndOffset
        {
            get { return _endOffset; }
        }

        public bool Valid
        {
            get { return _valid; }
        }

        public TimeOffsetsDialog(Window parent, int startOffset, int endOffset)
        {
            var startTimeEditBox = new TextBox();
            startTimeEditBox.Width = 50;
            startTimeEditBox.Text = "10";
            startTimeEditBox.ToolTip = "seconds OR minutes:seconds";

            var endTimeEditBox = new TextBox();
            endTimeEditBox.Width = 50;
            endTimeEditBox.Text = "10";
            endTimeEditBox.ToolTip = "seconds OR minutes:seconds";

            var panelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            panelList.Add(App.CreateTuple("Start Time", startTimeEditBox));
            panelList.Add(App.CreateTuple("End Time", endTimeEditBox));
            var optionsPanel = WpfHelper.CreateDualColumnPanel(panelList, 100, 5);
            optionsPanel.HorizontalAlignment = HorizontalAlignment.Center;
            optionsPanel.VerticalAlignment = VerticalAlignment.Center;

            var cutOptionsGroupBox = new GroupBox();
            cutOptionsGroupBox.Header = "Cut Configuration";
            cutOptionsGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            cutOptionsGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            cutOptionsGroupBox.Margin = new Thickness(5);
            cutOptionsGroupBox.Content = optionsPanel;

            var okButton = new Button();
            okButton.Content = "OK";
            okButton.Width = 75;
            okButton.Height = 25;
            okButton.Margin = new Thickness(5);
            okButton.HorizontalAlignment = HorizontalAlignment.Right;

            var cancelButton = new Button();
            cancelButton.Content = "Cancel";
            cancelButton.Width = 75;
            cancelButton.Height = 25;
            cancelButton.Margin = new Thickness(5);
            cancelButton.HorizontalAlignment = HorizontalAlignment.Right;

            var rootPanel = new DockPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Center;
            rootPanel.VerticalAlignment = VerticalAlignment.Center;
            rootPanel.Children.Add(cutOptionsGroupBox);
            rootPanel.Children.Add(cancelButton);
            rootPanel.Children.Add(okButton);

            DockPanel.SetDock(cutOptionsGroupBox, Dock.Top);
            DockPanel.SetDock(cancelButton, Dock.Right);
            DockPanel.SetDock(okButton, Dock.Right);

            var window = new Window();
            okButton.Click += (obj, args) => { window.DialogResult = true; window.Close(); };
            cancelButton.Click += (obj, args) => { window.DialogResult = false; window.Close(); };

            window.Owner = parent;
            window.WindowStyle = WindowStyle.ToolWindow;
            window.AllowsTransparency = false;
            window.Background = new SolidColorBrush(System.Windows.SystemColors.ControlColor);
            window.ShowInTaskbar = false;
            window.SizeToContent = SizeToContent.WidthAndHeight;
            window.ResizeMode = ResizeMode.NoResize;
            window.Icon = UDT.Properties.Resources.UDTIcon.ToImageSource();
            window.Title = "Cut Offsets";
            window.Content = rootPanel;
            window.Loaded += (obj, args) =>
            {
                window.Left = parent.Left + (parent.Width - window.Width) / 2;
                window.Top = parent.Top + (parent.Height - window.Height) / 2;
            };
            window.ShowDialog();

            _valid = window.DialogResult ?? false;
            if(!_valid)
            {
                return;
            }

            if(!int.TryParse(startTimeEditBox.Text, out _startOffset))
            {
                _valid = false;
                return;
            }

            if(!int.TryParse(endTimeEditBox.Text, out _endOffset))
            {
                _valid = false;
                return;
            }

            if(_startOffset <= 0 || _endOffset <= 0)
            {
                _valid = false;
            }
        }
    }

    public class CutByTimeComponent : AppComponent
    {
        private App _app;

        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return null; } }
        public List<DemoInfoListView> InfoListViews { get { return null; } }
        public ComponentType Type { get { return ComponentType.CutByTime; } }
        public bool MultiDemoMode { get { return false; } }

        public CutByTimeComponent(App app)
        {
            _app = app;
            RootControl = CreateCutByTimeTab();
        }

        public void PopulateViews(DemoInfo demoInfo)
        {
            var hideGameStateEditBox = demoInfo.Analyzed && demoInfo.GameStateFileOffsets.Count == 1;
            _gameStateIndexRow.Visibility = hideGameStateEditBox ? Visibility.Collapsed : Visibility.Visible;

            if(demoInfo.MatchTimes.Count == 0)
            {
                _calcGroupBox.Visibility = Visibility.Collapsed;
                return;
            }

            _calcGroupBox.Visibility = Visibility.Visible;
            _calcOvertimeRow.Visibility = demoInfo.MatchTimes[0].HadOvertime ? Visibility.Visible : Visibility.Collapsed;

            var matchCount = demoInfo.MatchTimes.Count;
            if(matchCount > 1)
            {
                _calcMatchSelectionRow.Visibility = Visibility.Visible;
                _calcMatchSelectionComboBox.Items.Clear();
                for(var i = 0; i < matchCount; ++i)
                {
                    var name = string.Format("Match #{0} (GS {1}, {2} - {3})", 
                        (i + 1).ToString(), 
                        demoInfo.MatchTimes[i].GameStateIndex,
                        App.FormatMinutesSeconds(demoInfo.MatchTimes[i].StartTimeMs / 1000),
                        App.FormatMinutesSeconds(demoInfo.MatchTimes[i].EndTimeMs / 1000));
                    _calcMatchSelectionComboBox.Items.Add(name);
                }
                _calcMatchSelectionComboBox.SelectedIndex = 0;
            }
            else
            {
                _calcMatchSelectionRow.Visibility = Visibility.Collapsed;
            }

            UpdateCalcServerTime();
        }

        public void SaveToConfigObject(UdtConfig config)
        {
            // Nothing to do.
        }

        public void SaveToConfigObject(UdtPrivateConfig config)
        {
            // Nothing to do.
        }

        public void SetCutInfo(int gsIndex, int startTime, int endTime)
        {
            _gameStateIndexEditBox.Text = gsIndex.ToString();
            _startTimeEditBox.Text = App.FormatMinutesSeconds(startTime);
            _endTimeEditBox.Text = App.FormatMinutesSeconds(endTime);
        }

        private class CutByTimeInfo
        {
            public string FilePath = null;
            public UInt32 FileOffset = 0;
            public Int32 GameStateIndex = 0;
            public int StartTime = -1;
            public int EndTime = -1;
        }

        private TextBox _startTimeEditBox;
        private TextBox _endTimeEditBox;
        private TextBox _gameStateIndexEditBox;
        private FrameworkElement _gameStateIndexRow;
        private ComboBox _calcMatchSelectionComboBox;
        private TextBox _calcTimeEditBox;
        private CheckBox _calcClockDirCheckBox;
        private CheckBox _calcOvertimeCheckBox;
        private TextBox _calcOutputTimeEditBox;
        private TextBlock _calcErrorTextBlock;
        private GroupBox _calcGroupBox;
        private FrameworkElement _calcMatchSelectionRow;
        private FrameworkElement _calcOvertimeRow;
        private FrameworkElement _calcErrorRow;

        private FrameworkElement CreateCutByTimeTab()
        {
            var startTimeEditBox = new TextBox();
            _startTimeEditBox = startTimeEditBox;
            startTimeEditBox.Width = 50;
            startTimeEditBox.Text = "00:00";
            startTimeEditBox.ToolTip = "Format: seconds OR minutes:seconds";

            var endTimeEditBox = new TextBox();
            _endTimeEditBox = endTimeEditBox;
            endTimeEditBox.Width = 50;
            endTimeEditBox.Text = "00:20";
            endTimeEditBox.ToolTip = "Format: seconds OR minutes:seconds";

            var gameStateIndexEditBox = new TextBox();
            _gameStateIndexEditBox = gameStateIndexEditBox;
            gameStateIndexEditBox.Width = 50;
            gameStateIndexEditBox.Text = "0";
            gameStateIndexEditBox.ToolTip = "The 0-based index of the last GameState message that comes before the content you want to cut";

            var panelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            panelList.Add(App.CreateTuple("Start Time", startTimeEditBox));
            panelList.Add(App.CreateTuple("End Time", endTimeEditBox));
            panelList.Add(App.CreateTuple("GameState Index", gameStateIndexEditBox));
            var optionsPanel = WpfHelper.CreateDualColumnPanel(panelList, 100, 5);
            optionsPanel.HorizontalAlignment = HorizontalAlignment.Center;
            optionsPanel.VerticalAlignment = VerticalAlignment.Center;
            _gameStateIndexRow = optionsPanel.Children[2] as FrameworkElement;

            var cutOptionsGroupBox = new GroupBox();
            cutOptionsGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            cutOptionsGroupBox.VerticalAlignment = VerticalAlignment.Top;
            cutOptionsGroupBox.Margin = new Thickness(5);
            cutOptionsGroupBox.Header = "Cut Configuration";
            cutOptionsGroupBox.Content = optionsPanel;

            var cutButton = new Button();
            cutButton.HorizontalAlignment = HorizontalAlignment.Left;
            cutButton.VerticalAlignment = VerticalAlignment.Center;
            cutButton.Content = "Cut!";
            cutButton.Width = 75;
            cutButton.Height = 25;
            cutButton.Margin = new Thickness(5);
            cutButton.Click += (obj, args) => OnCutByTimeClicked();

            var actionsGroupBox = new GroupBox();
            actionsGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            actionsGroupBox.VerticalAlignment = VerticalAlignment.Top;
            actionsGroupBox.Margin = new Thickness(5);
            actionsGroupBox.Header = "Actions";
            actionsGroupBox.Content = cutButton;

            var calcMatchSelectionComboBox = new ComboBox();
            _calcMatchSelectionComboBox = calcMatchSelectionComboBox;
            calcMatchSelectionComboBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            calcMatchSelectionComboBox.VerticalAlignment = VerticalAlignment.Center;
            calcMatchSelectionComboBox.Items.Add("Match #1");
            calcMatchSelectionComboBox.SelectedIndex = 0;
            calcMatchSelectionComboBox.SelectionChanged += (obj, args) => UpdateCalcServerTime();

            var calcTimeEditBox = new TextBox();
            _calcTimeEditBox = calcTimeEditBox;
            calcTimeEditBox.VerticalAlignment = VerticalAlignment.Center;
            calcTimeEditBox.Width = 50;
            calcTimeEditBox.Text = "00:00";
            calcTimeEditBox.ToolTip = "Format: seconds OR minutes:seconds";
            calcTimeEditBox.TextChanged += (obj, args) => UpdateCalcServerTime();

            var calcClockDirCheckBox = new CheckBox();
            _calcClockDirCheckBox = calcClockDirCheckBox;
            calcClockDirCheckBox.VerticalAlignment = VerticalAlignment.Center;
            calcClockDirCheckBox.Content = " Is the timestamp for the game clock going up?";
            calcClockDirCheckBox.IsChecked = true;
            calcClockDirCheckBox.Checked += (obj, args) => UpdateCalcServerTime();
            calcClockDirCheckBox.Unchecked += (obj, args) => UpdateCalcServerTime();

            var calcOvertimeCheckBox = new CheckBox();
            _calcOvertimeCheckBox = calcOvertimeCheckBox;
            calcOvertimeCheckBox.VerticalAlignment = VerticalAlignment.Center;
            calcOvertimeCheckBox.Content = " Was the game in overtime at that timestamp?";
            calcOvertimeCheckBox.IsChecked = false;
            calcOvertimeCheckBox.Checked += (obj, args) => UpdateCalcServerTime();
            calcOvertimeCheckBox.Unchecked += (obj, args) => UpdateCalcServerTime();

            var calcOutputTimeEditBox = new TextBox();
            _calcOutputTimeEditBox = calcOutputTimeEditBox;
            calcOutputTimeEditBox.VerticalAlignment = VerticalAlignment.Center;
            calcOutputTimeEditBox.Width = 50;
            calcOutputTimeEditBox.Text = "?";
            calcOutputTimeEditBox.IsReadOnly = true;
            calcOutputTimeEditBox.TextChanged += (obj, args) => UpdateCalcServerTime();

            var calcFillValuesButton = new Button();
            calcFillValuesButton.VerticalAlignment = VerticalAlignment.Center;
            calcFillValuesButton.Width = 75;
            calcFillValuesButton.Height = 25;
            calcFillValuesButton.Content = "Fill Values";
            calcFillValuesButton.ToolTip = "Fill in the fields of the Cut Configuration group box above using your settings?";
            calcFillValuesButton.Click += (obj, args) => FillCutByTimeFields();

            var calcErrorTextBlock = new TextBlock();
            _calcErrorTextBlock = calcErrorTextBlock;
            calcErrorTextBlock.VerticalAlignment = VerticalAlignment.Center;
            calcErrorTextBlock.Text = "none";

            var calcList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            calcList.Add(App.CreateTuple("", "Round-based game types are not supported."));
            calcList.Add(App.CreateTuple("", "Overtimes with the clock going down are not supported."));
            calcList.Add(App.CreateTuple("Match", calcMatchSelectionComboBox));
            calcList.Add(App.CreateTuple("Match Time", calcTimeEditBox));
            calcList.Add(App.CreateTuple("Clock Goes Up?", calcClockDirCheckBox));
            calcList.Add(App.CreateTuple("In Overtime?", calcOvertimeCheckBox));
            calcList.Add(App.CreateTuple("Server Time", calcOutputTimeEditBox));
            calcList.Add(App.CreateTuple("Error", calcErrorTextBlock));
            calcList.Add(App.CreateTuple("", calcFillValuesButton));
            var calcPanel = WpfHelper.CreateDualColumnPanel(calcList, 100, 3);
            calcPanel.HorizontalAlignment = HorizontalAlignment.Center;
            calcPanel.VerticalAlignment = VerticalAlignment.Center;
            _calcMatchSelectionRow = calcPanel.Children[2] as FrameworkElement;
            _calcOvertimeRow = calcPanel.Children[5] as FrameworkElement;
            _calcErrorRow = calcPanel.Children[7] as FrameworkElement;

            var calcGroupBox = new GroupBox();
            _calcGroupBox = calcGroupBox;
            calcGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            calcGroupBox.VerticalAlignment = VerticalAlignment.Top;
            calcGroupBox.Margin = new Thickness(5);
            calcGroupBox.Header = "Match Time to Server Time Converter";
            calcGroupBox.Content = calcPanel;

            var helpTextBlock = new TextBlock();
            helpTextBlock.Margin = new Thickness(5);
            helpTextBlock.TextWrapping = TextWrapping.WrapWithOverflow;
            helpTextBlock.Text =
                "The times UDT uses are server times, not match times." +
                "\nThe time format is either (seconds) or (minutes:seconds)." +
                "\n\nThe GameState index is the 0-based index of the last GameState message that comes before the content you want to cut." +
                "\n\nTo see the range of usable time values for a specific GameState index, check out the \"Server Time Range\" row(s) in the \"General\" tab under \"Info\".";

            var helpGroupBox = new GroupBox();
            helpGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            helpGroupBox.VerticalAlignment = VerticalAlignment.Top;
            helpGroupBox.Margin = new Thickness(5);
            helpGroupBox.Header = "Help";
            helpGroupBox.Content = helpTextBlock;

            var rootPanel = new WrapPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.Margin = new Thickness(5);
            rootPanel.Orientation = Orientation.Horizontal;
            rootPanel.Children.Add(cutOptionsGroupBox);
            rootPanel.Children.Add(actionsGroupBox);
            rootPanel.Children.Add(calcGroupBox);
            rootPanel.Children.Add(helpGroupBox);

            var scrollViewer = new ScrollViewer();
            scrollViewer.HorizontalAlignment = HorizontalAlignment.Stretch;
            scrollViewer.VerticalAlignment = VerticalAlignment.Stretch;
            scrollViewer.Margin = new Thickness(5);
            scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled;
            scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
            scrollViewer.Content = rootPanel;

            return scrollViewer;
        }

        private void OnCutByTimeClicked()
        {
            var demo = _app.SelectedDemo;
            if(demo == null)
            {
                _app.LogError("No demo was selected. Please select one to proceed.");
                return;
            }

            if(!App.IsValidWriteProtocol(demo.ProtocolNumber))
            {
                _app.LogError("Can't write demos of that protocol");
                return;
            }

            int startTime = -1;
            if(!App.GetTimeSeconds(_startTimeEditBox.Text, out startTime))
            {
                _app.LogError("Invalid start time. Format must be (seconds) or (minutes:seconds)");
                return;
            }

            int endTime = -1;
            if(!App.GetTimeSeconds(_endTimeEditBox.Text, out endTime))
            {
                _app.LogError("Invalid end time. Format must be (seconds) or (minutes:seconds)");
                return;
            }

            if(startTime >= endTime)
            {
                _app.LogError("Invalid times. Start time must be strictly inferior to end time.");
                return;
            }

            var gameStateCount = demo.GameStateFileOffsets.Count;
            int gameStateIndex = -1;
            if(!int.TryParse(_gameStateIndexEditBox.Text, out gameStateIndex))
            {
                gameStateIndex = -1;
            }

            uint fileOffset = 0;
            if(!demo.Analyzed || gameStateCount == 0)
            {
                if(gameStateIndex != 0)
                {
                    _app.LogError("You selected a non-0 GameState index but UDT doesn't know the file offset of it, if it even exists. Please analyze the demo first to proceed.");
                    return;
                }
            }
            else if(gameStateCount > 0)
            {
                if(gameStateCount > 1 && gameStateIndex >= gameStateCount)
                {
                    _app.LogError("Invalid GameState index. Valid range for this demo: {0}-{1}", 0, demo.GameStateFileOffsets.Count - 1);
                    return;
                }

                if(gameStateCount == 1 && gameStateIndex != 0)
                {
                    _gameStateIndexEditBox.Text = "0";
                    gameStateIndex = 0;
                    _app.LogWarning("Invalid GameState index. The only valid value for this demo is 0. UDT set it right for you.");
                }

                fileOffset = demo.GameStateFileOffsets[gameStateIndex];
            }

            if(demo.Analyzed && gameStateIndex >= 0 && gameStateIndex < demo.GameStateSnapshotTimesMs.Count)
            {
                var times = demo.GameStateSnapshotTimesMs[gameStateIndex];
                if(startTime > times.Item2 / 1000)
                {
                    _app.LogError("Invalid start time: it comes after the last snapshot of that GameState.");
                    return;
                }
                if(endTime < times.Item1 / 1000)
                {
                    _app.LogError("Invalid end time: it comes before the first snapshot of that GameState.");
                    return;
                }
            }

            _app.DisableUiNonThreadSafe();
            _app.JoinJobThread();
            _app.SaveConfig();

            var info = new CutByTimeInfo();
            info.GameStateIndex = gameStateIndex;
            info.FileOffset = fileOffset;
            info.FilePath = demo.FilePath;
            info.StartTime = startTime;
            info.EndTime = endTime;

            _app.StartJobThread(DemoCutByTimeThread, info);
        }

        private void DemoCutByTimeThread(object arg)
        {
            var info = (CutByTimeInfo)arg;
            if(info == null)
            {
                _app.LogError("Invalid thread argument type");
                return;
            }

            var protocol = App.GetProtocolFromFilePath(info.FilePath);
            if(protocol == UDT_DLL.udtProtocol.Invalid)
            {
                _app.LogError("Unrecognized protocol for demo '{0}'", Path.GetFileName(info.FilePath));
                return;
            }

            _app.InitParseArg();
            _app.ParseArg.FileOffset = info.FileOffset;
            _app.ParseArg.GameStateIndex = info.GameStateIndex;

            try
            {
                UDT_DLL.CutDemoByTime(_app.GetMainThreadContext(), ref _app.ParseArg, info.FilePath, info.StartTime, info.EndTime);
            }
            catch(Exception exception)
            {
                var startTimeDisplay = App.FormatMinutesSeconds(info.StartTime);
                var endTimeDisplay = App.FormatMinutesSeconds(info.EndTime);
                _app.LogError("Caught an exception while writing cut {0}-{1}: {2}", startTimeDisplay, endTimeDisplay, exception.Message);
            }
        }

        private void UpdateCalcServerTime()
        {
            var demo = _app.SelectedDemo;
            if(demo == null)
            {
                SetUnknownServerTime("no demo selected");
                return;
            }

            var matchIndex = _calcMatchSelectionComboBox.SelectedIndex;
            if(matchIndex < 0 || matchIndex >= demo.MatchTimes.Count)
            {
                SetUnknownServerTime("invalid match index");
                return;
            }

            var match = demo.MatchTimes[matchIndex];
            if(match.RoundBasedMode)
            {
                SetUnknownServerTime("match has a round-based game type");
                return;
            }

            int matchTimeMs = 0;
            if(!App.GetTimeSeconds(_calcTimeEditBox.Text, out matchTimeMs))
            {
                SetUnknownServerTime("invalid match time format");
                return;
            }
            matchTimeMs *= 1000;

            var clockGoingUp = _calcClockDirCheckBox.IsChecked ?? false;
            if(_calcOvertimeCheckBox.IsChecked ?? false)
            {
                // @NOTE: We don't have support for overtimes with the clock going down 
                // like CPMA does in duels by default.
                // For that we would need the length of a timed overtime and the overtime's index...
                if(match.TimeLimit == 0 || !clockGoingUp)
                {
                    SetUnknownServerTime("found no time limit for this match");
                    return;
                }

                matchTimeMs += match.TimeLimit * 60000;
            }

            var serverTimeMs = 0;
            if(clockGoingUp)
            {
                serverTimeMs = match.StartTimeMs + matchTimeMs;
            }
            else
            {
                if(match.TimeLimit == 0)
                {
                    SetUnknownServerTime("found no time limit for this match");
                    return;
                }

                var durationMs = match.TimeLimit * 60000;
                serverTimeMs = match.StartTimeMs + durationMs - matchTimeMs;
            }

            foreach(var timeOut in match.TimeOuts)
            {
                if(timeOut.StartTimeMs >= serverTimeMs)
                {
                    break;
                }

                serverTimeMs += timeOut.EndTimeMs - timeOut.StartTimeMs;
            }

            // In some cases, a match time exactly at the first or last second
            // would be considered out of range.
            // We extend the allowed range by 1 second on each side
            // to avoid the inconvenience of having to change the value in the GUI. 
            if(serverTimeMs < match.StartTimeMs - 1000 || 
                serverTimeMs > match.EndTimeMs + 1000)
            {
                SetUnknownServerTime("match time out of range");
                return;
            }

            SetValidServerTime(serverTimeMs);
        }

        private void SetUnknownServerTime(string error)
        {
            _calcOutputTimeEditBox.Text = "?";
            _calcErrorRow.Visibility = Visibility.Visible;
            _calcErrorTextBlock.Text = error;
        }

        private void SetValidServerTime(int serverTimeMs)
        {
            _calcOutputTimeEditBox.Text = App.FormatMinutesSeconds(serverTimeMs / 1000);
            _calcErrorRow.Visibility = Visibility.Collapsed;
            _calcErrorTextBlock.Text = "none";
        }

        private void FillCutByTimeFields()
        {
            var app = App.Instance;
            var demo = app.SelectedDemo;
            if(demo == null)
            {
                return;
            }

            var matchIndex = _calcMatchSelectionComboBox.SelectedIndex;
            var gsIndex = 0;
            if(matchIndex != -1 && 
                matchIndex < demo.MatchTimes.Count)
            {
                gsIndex = demo.MatchTimes[matchIndex].GameStateIndex;
            }

            var cutTime = 0;
            if(!App.ParseMinutesSeconds(_calcOutputTimeEditBox.Text, out cutTime))
            {
                return;
            }

            app.SetCutByTimeFields(gsIndex, cutTime, cutTime);
        }
    }
}