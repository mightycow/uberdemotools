using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Media;


namespace Uber.DemoTools
{
    public class CutByFragComponent : AppComponent
    {
        public FrameworkElement RootControl { get; private set; }
        public List<ListView> ListViews { get { return null; } }
        public ComponentType Type { get { return ComponentType.CutByChat; } }

        public CutByFragComponent(App app)
        {
            _app = app;
            RootControl = CreateCutByFragTab();
        }

        public void PopulateViews(DemoInfo demoInfo)
        { 
            // Nothing to do.
        }

        public void SaveToConfigObject(UdtConfig config)
        {
            int time = 0;
            if(App.GetOffsetSeconds(_startTimeOffsetEditBox.Text, out time))
            {
                _app.Config.FragCutStartOffset = time;
            }
            if(App.GetOffsetSeconds(_endTimeOffsetEditBox.Text, out time))
            {
                _app.Config.FragCutEndOffset = time;
            }
        }

        private App _app;
        private TextBox _startTimeOffsetEditBox = null;
        private TextBox _endTimeOffsetEditBox = null;

        private FrameworkElement CreateCutByFragTab()
        {
            var rulesGroupBox = new GroupBox();
            rulesGroupBox.Header = "Chat Rules";
            rulesGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            rulesGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            rulesGroupBox.Margin = new Thickness(5);
            rulesGroupBox.Content = "Hello";

            var cutButton = new Button();
            cutButton.HorizontalAlignment = HorizontalAlignment.Left;
            cutButton.VerticalAlignment = VerticalAlignment.Top;
            cutButton.Content = "Cut!";
            cutButton.Width = 75;
            cutButton.Height = 25;
            cutButton.Margin = new Thickness(5);
            cutButton.Click += (obj, args) => OnCutByFragClicked();

            var actionsGroupBox = new GroupBox();
            actionsGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            actionsGroupBox.VerticalAlignment = VerticalAlignment.Top;
            actionsGroupBox.Margin = new Thickness(5);
            actionsGroupBox.Header = "Actions";
            actionsGroupBox.Content = cutButton;

            var startTimeOffsetEditBox = new TextBox();
            _startTimeOffsetEditBox = startTimeOffsetEditBox;
            startTimeOffsetEditBox.Width = 40;
            startTimeOffsetEditBox.Text = _app.Config.FragCutStartOffset.ToString();
            startTimeOffsetEditBox.ToolTip = "How many seconds before the first frag in the sequence do we start the cut?";

            var endTimeOffsetEditBox = new TextBox();
            _endTimeOffsetEditBox = endTimeOffsetEditBox;
            endTimeOffsetEditBox.Width = 40;
            endTimeOffsetEditBox.Text = _app.Config.FragCutEndOffset.ToString();
            endTimeOffsetEditBox.ToolTip = "How many seconds after the last frag in the sequence do we end the cut?";

            var panelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            panelList.Add(App.CreateTuple("Start Time Offset", startTimeOffsetEditBox));
            panelList.Add(App.CreateTuple("End Time Offset", endTimeOffsetEditBox));
            var optionsPanel = WpfHelper.CreateDualColumnPanel(panelList, 100, 5);
            optionsPanel.HorizontalAlignment = HorizontalAlignment.Center;
            optionsPanel.VerticalAlignment = VerticalAlignment.Center;
            
            var timeOffsetsGroupBox = new GroupBox();
            timeOffsetsGroupBox.Header = "Time Offsets";
            timeOffsetsGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            timeOffsetsGroupBox.VerticalAlignment = VerticalAlignment.Top;
            timeOffsetsGroupBox.Margin = new Thickness(5);
            timeOffsetsGroupBox.Content = optionsPanel;

            var helpTextBlock = new TextBlock();
            helpTextBlock.Margin = new Thickness(5);
            helpTextBlock.TextWrapping = TextWrapping.WrapWithOverflow;
            helpTextBlock.Text = "@TODO:";

            var helpGroupBox = new GroupBox();
            helpGroupBox.Margin = new Thickness(5);
            helpGroupBox.Header = "Help";
            helpGroupBox.Content = helpTextBlock;

            var rootPanel = new WrapPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.Margin = new Thickness(5);
            rootPanel.Orientation = Orientation.Horizontal;
            rootPanel.Children.Add(rulesGroupBox);
            rootPanel.Children.Add(timeOffsetsGroupBox);
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

        private void OnCutByFragClicked()
        {
            var demos = _app.SelectedDemos;
            if(demos == null)
            {
                _app.LogError("No demo was selected. Please select one to proceed.");
                return;
            }

            _app.SaveConfig();
            _app.DisableUiNonThreadSafe();
            _app.JoinJobThread();

            var filePaths = new List<string>();
            foreach(var demo in demos)
            {
                filePaths.Add(demo.FilePath);
            }

            _app.StartJobThread(DemoCutByFragThread, filePaths);
        }

        private void DemoCutByFragThread(object arg)
        {
            try
            {
                DemoCutByFragThreadImpl(arg);
            }
            catch(Exception exception)
            {
                EntryPoint.RaiseException(exception);
            }
        }

        private void DemoCutByFragThreadImpl(object arg)
        {
            var filePaths = arg as List<string>;
            if(filePaths == null)
            {
                return;
            }

            var outputFolder = _app.GetOutputFolder();
            var outputFolderPtr = Marshal.StringToHGlobalAnsi(outputFolder);
            _app.InitParseArg();
            _app.ParseArg.OutputFolderPath = outputFolderPtr;

            try
            {
                var config = _app.Config;
                var rules = new UDT_DLL.udtCutByFragArg();
                rules.MinFragCount = 3; // @TODO:
                rules.TimeBetweenFragsSec = 10; // @TODO:
                rules.TimeMode = 0; // @TODO:
                rules.StartOffsetSec = (UInt32)config.ChatCutStartOffset;
                rules.EndOffsetSec = (UInt32)config.ChatCutEndOffset;
                UDT_DLL.CutDemosByFrag(ref _app.ParseArg, filePaths, rules, config.MaxThreadCount);
            }
            catch(Exception exception)
            {
                _app.LogError("Caught an exception while cutting demos: {0}", exception.Message);
            }

            Marshal.FreeHGlobal(outputFolderPtr);
            _app.EnableUiThreadSafe();
        }
    }
}