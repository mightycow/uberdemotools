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
    public partial class App
    {
        private class CutByTimeInfo
        {
            public string InFilePath = "<invalid>";
            public string OutFilePath = "<invalid>";
            public int StartTime = -1;
            public int EndTime = -1;
            public List<Demo.GameStateInfo> GameStates = null;
        }

        private class TimeOffsetsDialog
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
                panelList.Add(CreateTuple("Start Time", startTimeEditBox));
                panelList.Add(CreateTuple("End Time", endTimeEditBox));
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
            panelList.Add(CreateTuple("Start Time", startTimeEditBox));
            panelList.Add(CreateTuple("End Time", endTimeEditBox));
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
            if(_demoListView.SelectedIndex == -1)
            {
                LogError("No demo was selected. Please select one to proceed.");
                return;
            }

            int startTime = -1;
            if(!GetTimeSeconds(_startTimeEditBox.Text, out startTime))
            {
                LogError("Invalid start time. Format must be (seconds) or (minutes:seconds)");
                return;
            }

            int endTime = -1;
            if(!GetTimeSeconds(_endTimeEditBox.Text, out endTime))
            {
                LogError("Invalid end time. Format must be (seconds) or (minutes:seconds)");
                return;
            }

            DisableUiNonThreadSafe();

            if(_jobThread != null)
            {
                _jobThread.Join();
            }

            var startString = _startTimeEditBox.Text.Replace(":", "");
            var endString = _endTimeEditBox.Text.Replace(":", "");

            var info = new CutByTimeInfo();
            info.InFilePath = _demos[_demoListView.SelectedIndex].FilePath;
            info.OutFilePath = GenerateOutputFilePath(info.InFilePath, startString, endString);
            info.StartTime = startTime;
            info.EndTime = endTime;
            info.GameStates = _demos[_demoListView.SelectedIndex].DemoGameStates;

            _jobThread = new Thread(DemoCutByTimeThread);
            _jobThread.Start(info);
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
            var timer = new Stopwatch();
            timer.Start();

            Demo.ProgressCallback progressCb = (progressPc) =>
            {
                if(timer.ElapsedMilliseconds < 50)
                {
                    return _cancelJobValue;
                }

                timer.Stop();
                timer.Reset();
                timer.Start();

                SetProgressThreadSafe(100.0 * (double)progressPc);

                return _cancelJobValue;
            };

            var info = (CutByTimeInfo)arg;
            var protocol = GetProtocolFromFilePath(info.InFilePath);
            if(protocol == DemoProtocol.Invalid)
            {
                LogError("Unrecognized protocol for demo '{0}'", Path.GetFileName(info.InFilePath));
                EnableUiThreadSafe();
                return;
            }

            var startTimeDisplay = FormatMinutesSeconds(info.StartTime);
            var endTimeDisplay = FormatMinutesSeconds(info.EndTime);
            LogInfo("Writing cut: {0}-{1}", startTimeDisplay, endTimeDisplay);

            var cut = new DemoCut();
            cut.FilePath = info.OutFilePath;
            cut.StartTimeMs = info.StartTime * 1000;
            cut.EndTimeMs = info.EndTime * 1000;

            var cuts = new List<DemoCut>();
            cuts.Add(cut);

            int totalParseTime = 0;
            List<int> gsParseTimes = new List<int>();
            List<List<DemoCut>> gsCutsList = new List<List<DemoCut>>();
            Demo.CreateCutList(cuts, info.GameStates, ref gsCutsList, ref gsParseTimes, ref totalParseTime);
            
            try
            {
                var gsIdx = gsCutsList.FindIndex(cutList => cutList.Count == 1);
                if(gsIdx >= 0)
                {
                    var gameState = info.GameStates[gsIdx];
                    Demo.Cut(protocol, info.InFilePath, progressCb, DemoLoggingCallback, cuts, gameState);
                }
                else
                {
                    LogError("Cut list creation must have failed... couldn't find the cut in the list :-(");
                }
            }
            catch(SEHException exception) 
            {
                LogError("Caught an exception while writing cut {0}-{1}: {2}", startTimeDisplay, endTimeDisplay, exception.Message);
            }

            timer.Stop();

            EnableUiThreadSafe();
        }

        private string FormatMinutesSeconds(int totalSeconds)
        {
            var minutes = totalSeconds / 60;
            var seconds = totalSeconds % 60;

            return minutes.ToString() + ":" + seconds.ToString("00");
        }

        private bool ParseMinutesSeconds(string time, out int totalSeconds)
        {
            totalSeconds = -1;

            int colonIdx = time.IndexOf(':');
            if(colonIdx < 0)
            {
                return false;
            }

            int minutes = -1;
            if(!int.TryParse(time.Substring(0, colonIdx), out minutes))
            {
                return false;
            }

            int seconds = -1;
            if(!int.TryParse(time.Substring(colonIdx + 1), out seconds))
            {
                return false;
            }

            totalSeconds = 60 * minutes + seconds;

            return true;
        }
    }
}