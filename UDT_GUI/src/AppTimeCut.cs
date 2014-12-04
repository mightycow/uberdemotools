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
        public List<ListView> ListViews { get { return null; } }
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

        public void SetStartAndEndTimes(int startTime, int endTime)
        {
            _startTimeEditBox.Text = App.FormatMinutesSeconds(startTime);
            _endTimeEditBox.Text = App.FormatMinutesSeconds(endTime);
        }

        private class CutByTimeInfo
        {
            public string FilePath = null;
            public int StartTime = -1;
            public int EndTime = -1;
        }

        private TextBox _startTimeEditBox = null;
        private TextBox _endTimeEditBox = null;

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

            var helpLabel = new Label();
            helpLabel.Margin = new Thickness(5);
            helpLabel.Content =
                "The times are absolute server times." +
                "\nThe format is either (seconds) or (minutes:seconds)";

            var helpGroupBox = new GroupBox();
            helpGroupBox.Margin = new Thickness(5);
            helpGroupBox.Header = "Help";
            helpGroupBox.Content = helpLabel;

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

            _app.DisableUiNonThreadSafe();
            _app.JoinJobThread();
            _app.SaveConfig();

            var startString = _startTimeEditBox.Text.Replace(":", "");
            var endString = _endTimeEditBox.Text.Replace(":", "");

            var info = new CutByTimeInfo();
            info.FilePath = demo.FilePath;
            info.StartTime = startTime;
            info.EndTime = endTime;

            _app.StartJobThread(DemoCutByTimeThread, info);
        }

        private void DemoCutByTimeThread(object arg)
        {
            try
            {
                DemoCutByTimeThreadImpl(arg);
            }
            catch(Exception exception)
            {
                EntryPoint.RaiseException(exception);
            }
        }

        private void DemoCutByTimeThreadImpl(object arg)
        {
            var info = (CutByTimeInfo)arg;
            var protocol = App.GetProtocolFromFilePath(info.FilePath);
            if(protocol == UDT_DLL.udtProtocol.Invalid)
            {
                _app.LogError("Unrecognized protocol for demo '{0}'", Path.GetFileName(info.FilePath));
                _app.EnableUiThreadSafe();
                return;
            }

            var outputFolder = _app.GetOutputFolder();
            var outputFolderPtr = Marshal.StringToHGlobalAnsi(outputFolder);

            var parseArg = _app.ParseArg;
            parseArg.CancelOperation = 0;
            parseArg.MessageCb = _app.DemoLoggingCallback;
            parseArg.OutputFolderPath = outputFolderPtr;
            parseArg.ProgressCb = _app.DemoProgressCallback;
            parseArg.ProgressContext = IntPtr.Zero;
            _app.ParseArg = parseArg;

            var startTimeDisplay = App.FormatMinutesSeconds(info.StartTime);
            var endTimeDisplay = App.FormatMinutesSeconds(info.EndTime);
            _app.LogInfo("Writing cut: {0}-{1}", startTimeDisplay, endTimeDisplay);
           
            try
            {
                UDT_DLL.CutByTime(_app.GetMainThreadContext(), ref _app.ParseArg, info.FilePath, info.StartTime, info.EndTime);
            }
            catch(Exception exception)
            {
                _app.LogError("Caught an exception while writing cut {0}-{1}: {2}", startTimeDisplay, endTimeDisplay, exception.Message);
            }

            Marshal.FreeHGlobal(outputFolderPtr);
            _app.EnableUiThreadSafe();
        }
    }
}