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

            window.WindowStyle = WindowStyle.ToolWindow;
            window.AllowsTransparency = false;
            window.Background = new SolidColorBrush(System.Windows.SystemColors.ControlColor);
            window.ShowInTaskbar = false;
            window.Width = 240;
            window.Height = 180;
            window.Left = parent.Left + (parent.Width - window.Width) / 2;
            window.Top = parent.Top + (parent.Height - window.Height) / 2;
            window.Icon = UDT.Properties.Resources.UDTIcon.ToImageSource();
            window.Title = "Cut Offsets";
            window.Content = rootPanel;
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

        public CutByTimeComponent(App app)
        {
            _app = app;
            RootControl = CreateCutByTimeTab();
        }

        public void PopulateViews(DemoInfo demoInfo)
        {
            // Nothing to do.
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

        private TextBox _startTimeEditBox = null;
        private TextBox _endTimeEditBox = null;
        private TextBox _gameStateIndexEditBox = null;

        private FrameworkElement CreateCutByTimeTab()
        {
            var startTimeEditBox = new TextBox();
            _startTimeEditBox = startTimeEditBox;
            startTimeEditBox.Width = 50;
            startTimeEditBox.Text = "00:00";
            startTimeEditBox.ToolTip = "seconds OR minutes:seconds";

            var endTimeEditBox = new TextBox();
            _endTimeEditBox = endTimeEditBox;
            endTimeEditBox.Width = 50;
            endTimeEditBox.Text = "00:20";
            endTimeEditBox.ToolTip = "seconds OR minutes:seconds";

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

            var cutOptionsGroupBox = new GroupBox();
            cutOptionsGroupBox.Header = "Cut Configuration";
            cutOptionsGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            cutOptionsGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            cutOptionsGroupBox.Margin = new Thickness(5);
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

            var helpTextBlock = new TextBlock();
            helpTextBlock.Margin = new Thickness(5);
            helpTextBlock.TextWrapping = TextWrapping.WrapWithOverflow;
            helpTextBlock.Text =
                "The times are server times, not match times." +
                "\nTime format is either (seconds) or (minutes:seconds)." +
                "\n\nThe GameState index is the 0-based index of the last GameState message that comes before the content you want to cut." +
                "\n\nTo see the range of usable time values for a specific GameState index, check out the \"Server Time Range\" row(s) in the \"Info\" tab.";

            var helpGroupBox = new GroupBox();
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

            int gameStateIndex = -1;
            if(!int.TryParse(_gameStateIndexEditBox.Text, out gameStateIndex) || gameStateIndex < 0 || gameStateIndex >= demo.GameStateFileOffsets.Count)
            {
                if(demo.GameStateFileOffsets.Count > 1)
                {
                    _app.LogError("Invalid GameState index. Valid range for this demo: {0}-{1}", 0, demo.GameStateFileOffsets.Count - 1);
                    return;
                }
                else
                {
                    _gameStateIndexEditBox.Text = "0";
                    gameStateIndex = 0;
                    _app.LogWarning("Invalid GameState index. The only valid value for this demo is 0. UDT set it right for you.");
                }
            }

            _app.DisableUiNonThreadSafe();
            _app.JoinJobThread();
            _app.SaveConfig();

            var info = new CutByTimeInfo();
            info.GameStateIndex = gameStateIndex;
            info.FileOffset = demo.GameStateFileOffsets[gameStateIndex];
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

            var outputFolder = _app.GetOutputFolder();
            var outputFolderPtr = Marshal.StringToHGlobalAnsi(outputFolder);
            _app.InitParseArg();
            _app.ParseArg.OutputFolderPath = outputFolderPtr;
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

            Marshal.FreeHGlobal(outputFolderPtr);
        }
    }
}