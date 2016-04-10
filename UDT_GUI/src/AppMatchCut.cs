using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;


namespace Uber.DemoTools
{
    public class MatchFilterComponent : AppComponent
    {
        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return null; } }
        public List<DemoInfoListView> InfoListViews { get { return null; } }
        public ComponentType Type { get { return ComponentType.MatchFilters; } }
        public bool MultiDemoMode { get { return true; } }

        public MatchFilterComponent(App app)
        {
            _app = app;
            RootControl = CreateTab();
        }

        public void PopulateViews(DemoInfo demoInfo)
        {
            // Nothing to do.
        }

        public void SaveToConfigObject(UdtConfig config)
        {
            var seconds = 0;
            if(App.GetTimeSeconds(_startTimeOffsetTextBox.Text, out seconds))
            {
                config.MatchCutStartTimeOffsetMs = seconds * 1000;
            }
            if(App.GetTimeSeconds(_endTimeOffsetTextBox.Text, out seconds))
            {
                config.MatchCutEndTimeOffsetMs = seconds * 1000;
            }
        }

        public void SaveToConfigObject(UdtPrivateConfig config)
        {
            // Nothing to do.
        }

        private App _app;
        private TextBox _startTimeOffsetTextBox;
        private TextBox _endTimeOffsetTextBox;

        private FrameworkElement CreateTab()
        {
            var startTimeOffsetTextBox = new TextBox();
            _startTimeOffsetTextBox = startTimeOffsetTextBox;
            startTimeOffsetTextBox.Width = 50;
            startTimeOffsetTextBox.Text = App.FormatMinutesSeconds(_app.Config.MatchCutStartTimeOffsetMs / 1000);
            startTimeOffsetTextBox.ToolTip = "seconds OR minutes:seconds";

            var endTimeOffsetTextBox = new TextBox();
            _endTimeOffsetTextBox = endTimeOffsetTextBox;
            endTimeOffsetTextBox.Width = 50;
            endTimeOffsetTextBox.Text = App.FormatMinutesSeconds(_app.Config.MatchCutEndTimeOffsetMs / 1000);
            endTimeOffsetTextBox.ToolTip = "seconds OR minutes:seconds";

            var rulesPanelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            rulesPanelList.Add(App.CreateTuple("Start Time Offset", startTimeOffsetTextBox));
            rulesPanelList.Add(App.CreateTuple("End Time Offset", endTimeOffsetTextBox));

            var rulesPanel = WpfHelper.CreateDualColumnPanel(rulesPanelList, 120, 5);
            rulesPanel.HorizontalAlignment = HorizontalAlignment.Center;
            rulesPanel.VerticalAlignment = VerticalAlignment.Center;

            var rulesGroupBox = new GroupBox();
            rulesGroupBox.Header = "Match Cut Rules";
            rulesGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            rulesGroupBox.VerticalAlignment = VerticalAlignment.Top;
            rulesGroupBox.Margin = new Thickness(5);
            rulesGroupBox.Content = rulesPanel;

            var cutButton = new Button();
            cutButton.HorizontalAlignment = HorizontalAlignment.Left;
            cutButton.VerticalAlignment = VerticalAlignment.Top;
            cutButton.Content = "Cut!";
            cutButton.Width = 75;
            cutButton.Height = 25;
            cutButton.Margin = new Thickness(5);
            cutButton.Click += (obj, args) => OnCutClicked();

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
                "This cut filter is there to help create a new demo for every match in the demo.\n\n" +
                "The 'Start Time Offset' field is only applied if no pre-match count-down is found.\n" +
                "The 'End Time Offset' field is only applied if no post-match intermission is found.";

            var helpGroupBox = new GroupBox();
            helpGroupBox.Margin = new Thickness(5);
            helpGroupBox.Header = "Help";
            helpGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            helpGroupBox.VerticalAlignment = VerticalAlignment.Top;
            helpGroupBox.Content = helpTextBlock;

            var rootPanel = new WrapPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.Margin = new Thickness(5);
            rootPanel.Orientation = Orientation.Horizontal;
            rootPanel.Children.Add(rulesGroupBox);
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

        private void OnCutClicked()
        {
            var demos = _app.SelectedWriteDemos;
            if(demos == null)
            {
                return;
            }

            _app.SaveBothConfigs();
            _app.DisableUiNonThreadSafe();
            _app.JoinJobThread();

            var filePaths = new List<string>();
            foreach(var demo in demos)
            {
                filePaths.Add(demo.FilePath);
            }

            _app.StartJobThread(DemoCutThread, filePaths);
        }

        private void DemoCutThread(object arg)
        {
            var filePaths = arg as List<string>;
            if(filePaths == null)
            {
                _app.LogError("Invalid thread argument");
                return;
            }

            _app.InitParseArg();

            try
            {
                var config = _app.Config;
                var rules = UDT_DLL.CreateCutByMatchArg(config);
                UDT_DLL.CutDemosByMatch(ref _app.ParseArg, filePaths, rules, UDT_DLL.CreateCutByPatternOptions(config, _app.PrivateConfig));
            }
            catch(Exception exception)
            {
                _app.LogError("Caught an exception while cutting demos: {0}", exception.Message);
            }
        }
    }
}