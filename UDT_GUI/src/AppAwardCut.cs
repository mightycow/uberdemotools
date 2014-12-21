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
    public class CutByAwardComponent : AppComponent
    {
        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return null; } }
        public List<DemoInfoListView> InfoListViews { get { return null; } }
        public ComponentType Type { get { return ComponentType.CutByAward; } }

        public CutByAwardComponent(App app)
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
            int intValue = 0;

            if(App.GetOffsetSeconds(_startTimeOffsetEditBox.Text, out intValue))
            {
                _app.Config.AwardCutStartOffset = intValue;
            }

            if(App.GetOffsetSeconds(_endTimeOffsetEditBox.Text, out intValue))
            {
                _app.Config.AwardCutEndOffset = intValue;
            }
        }

        private App _app;
        private TextBox _startTimeOffsetEditBox = null;
        private TextBox _endTimeOffsetEditBox = null;
        private FilterGroupBox _awardFilters = null;

        private FrameworkElement CreateTab()
        {
            _awardFilters = new FilterGroupBox("Award Filters", UDT_DLL.udtStringArray.Awards, 2);

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

            var startTimeOffsetEditBox = new TextBox();
            _startTimeOffsetEditBox = startTimeOffsetEditBox;
            startTimeOffsetEditBox.Width = 40;
            startTimeOffsetEditBox.Text = _app.Config.AwardCutStartOffset.ToString();
            startTimeOffsetEditBox.ToolTip = "How many seconds before the first award in the sequence do we start the cut?";

            var endTimeOffsetEditBox = new TextBox();
            _endTimeOffsetEditBox = endTimeOffsetEditBox;
            endTimeOffsetEditBox.Width = 40;
            endTimeOffsetEditBox.Text = _app.Config.AwardCutEndOffset.ToString();
            endTimeOffsetEditBox.ToolTip = "How many seconds after the last award in the sequence do we end the cut?";

            var timeOffsetPanelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            timeOffsetPanelList.Add(App.CreateTuple("Start Time Offset", startTimeOffsetEditBox));
            timeOffsetPanelList.Add(App.CreateTuple("End Time Offset", endTimeOffsetEditBox));
            var timeOffsetPanel = WpfHelper.CreateDualColumnPanel(timeOffsetPanelList, 100, 5);
            timeOffsetPanel.HorizontalAlignment = HorizontalAlignment.Center;
            timeOffsetPanel.VerticalAlignment = VerticalAlignment.Center;
            
            var timeOffsetsGroupBox = new GroupBox();
            timeOffsetsGroupBox.Header = "Time Offsets";
            timeOffsetsGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            timeOffsetsGroupBox.VerticalAlignment = VerticalAlignment.Top;
            timeOffsetsGroupBox.Margin = new Thickness(5);
            timeOffsetsGroupBox.Content = timeOffsetPanel;
            
            var helpTextBlock = new TextBlock();
            helpTextBlock.Margin = new Thickness(5);
            helpTextBlock.TextWrapping = TextWrapping.WrapWithOverflow;
            helpTextBlock.Text = 
                "The mid-air reward medal will only be detected by UDT in Quake Live's .dm_90 demo files." + 
                "\nThe mid-air medal is awarded when you hit an enemy mid-air with a projectile (airgrenades, airrockets).";
            
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
            rootPanel.Children.Add(_awardFilters.RootElement);
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

        private class ThreadArg
        {
            public List<string> FilePaths = null;
            public UInt32 AllowedAwards = 0;
        }

        private void OnCutClicked()
        {
            var demos = _app.SelectedDemos;
            if(demos == null)
            {
                _app.LogError("No demo was selected. Please select one to proceed.");
                return;
            }

            UInt32 allowedAwards = _awardFilters.GetBitMask();
            if(allowedAwards == 0)
            {
                _app.LogError("You didn't check any Award. Please check at least one to proceed.");
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

            var threadArg = new ThreadArg();
            threadArg.FilePaths = filePaths;
            threadArg.AllowedAwards = allowedAwards;

            _app.StartJobThread(DemoCuThread, threadArg);
        }

        private void DemoCuThread(object arg)
        {
            try
            {
                DemoCutThreadImpl(arg);
            }
            catch(Exception exception)
            {
                EntryPoint.RaiseException(exception);
            }
        }

        private void DemoCutThreadImpl(object arg)
        {
            var threadArg = arg as ThreadArg;
            if(threadArg == null)
            {
                _app.LogError("Invalid thread argument type");
                _app.EnableUiThreadSafe();
                return;
            }

            var filePaths = threadArg.FilePaths;
            if(filePaths == null)
            {
                _app.LogError("Invalid thread argument data");
                _app.EnableUiThreadSafe();
                return;
            }

            var outputFolder = _app.GetOutputFolder();
            var outputFolderPtr = Marshal.StringToHGlobalAnsi(outputFolder);
            _app.InitParseArg();
            _app.ParseArg.OutputFolderPath = outputFolderPtr;

            var config = _app.Config;
            try
            {
                var rules = new UDT_DLL.udtCutByAwardArg();
                rules.StartOffsetSec = (UInt32)config.AwardCutStartOffset;
                rules.EndOffsetSec = (UInt32)config.AwardCutEndOffset;
                rules.AllowedAwards = threadArg.AllowedAwards;
                UDT_DLL.CutDemosByAward(ref _app.ParseArg, filePaths, rules, config.MaxThreadCount);
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