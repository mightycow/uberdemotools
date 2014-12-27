using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;


namespace Uber.DemoTools
{
    public class CutByPatternComponent : AppComponent
    {
        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return null; } }
        public List<DemoInfoListView> InfoListViews { get { return null; } }
        public ComponentType Type { get { return ComponentType.CutByTime; } }
        public bool MultiDemoMode { get { return true; } }

        public CutByPatternComponent(App app)
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
            config.PatternsSelectionBitMask = (int)_patternsGroupBox.GetBitMask();
        }

        public void SaveToConfigObject(UdtPrivateConfig config)
        {
            // Nothing to do.
        }

        private App _app;
        private FilterGroupBox _patternsGroupBox;

        private FrameworkElement CreateTab()
        {
            _patternsGroupBox = new FilterGroupBox("Cut Patterns", UDT_DLL.udtStringArray.CutPatterns, 2);
            _patternsGroupBox.SetBitMask((uint)_app.Config.PatternsSelectionBitMask);

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
                "UDT will create a cut section for each pattern match during parsing." +
                "\nWhen parsing is done, overlapping cut sections get merged together and a new parsing pass is applied to do the actual cutting." +
                "\n\nExample: suppose we have 2 matches, the first at 1:27 and the second at 1:30 with start and end time offsets 10 and 8." +
                "\nUDT will create 2 cut sections: 1:17-1:35 and 1:20-1:38, which then get merged into 1: 1:17-1:38.";
            
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
            rootPanel.Children.Add(_patternsGroupBox.RootElement);
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
            public UDT_DLL.udtPatternInfo[] Patterns = null;
            public UDT_DLL.ArgumentResources Resources = null;
        }

        private static UInt32 GetBit(UDT_DLL.udtPatternType type)
        {
            return (UInt32)(1 << (int)type);
        }

        private static bool IsPatternActive(UInt32 patterns, UDT_DLL.udtPatternType type)
        {
            return (patterns & GetBit(type)) != 0;
        }

        private void OnCutClicked()
        {
            var demos = _app.SelectedDemos;
            if(demos == null)
            {
                _app.LogError("No demo was selected. Please select at least one to proceed.");
                return;
            }

            _app.SaveConfig();
            var config = _app.Config;
            if(config.PatternsSelectionBitMask == 0)
            {
                _app.LogError("You didn't check any pattern. Please check at least one to proceed.");
                return;
            }

            _app.DisableUiNonThreadSafe();
            _app.JoinJobThread();

            var filePaths = new List<string>();
            foreach(var demo in demos)
            {
                filePaths.Add(demo.FilePath);
            }

            var selectedPatterns = (UInt32)config.PatternsSelectionBitMask;
            var patterns = new List<UDT_DLL.udtPatternInfo>();
            var resources = new UDT_DLL.ArgumentResources();

            if(IsPatternActive(selectedPatterns, UDT_DLL.udtPatternType.GlobalChat))
            {
                var pattern = new UDT_DLL.udtPatternInfo();
                UDT_DLL.CreateChatPatternInfo(ref pattern, resources, config.ChatRules);
                patterns.Add(pattern);
            }

            if(IsPatternActive(selectedPatterns, UDT_DLL.udtPatternType.FragSequences))
            {
                var pattern = new UDT_DLL.udtPatternInfo();
                var rules = UDT_DLL.CreateCutByFragArg(config, _app.PrivateConfig);                
                UDT_DLL.CreateFragPatternInfo(ref pattern, resources, rules);
                patterns.Add(pattern);
            }

            if(IsPatternActive(selectedPatterns, UDT_DLL.udtPatternType.MidAirFrags))
            {
                var pattern = new UDT_DLL.udtPatternInfo();
                var rules = UDT_DLL.CreateCutByMidAirArg(config);
                UDT_DLL.CreateMidAirPatternInfo(ref pattern, resources, rules);
                patterns.Add(pattern);
            }

            if(IsPatternActive(selectedPatterns, UDT_DLL.udtPatternType.MultiRailFrags))
            {
                var pattern = new UDT_DLL.udtPatternInfo();
                var rules = UDT_DLL.CreateCutByMultiRailArg(config);
                UDT_DLL.CreateMultiRailPatternInfo(ref pattern, resources, rules);
                patterns.Add(pattern);
            }

            var threadArg = new ThreadArg();
            threadArg.FilePaths = filePaths;
            threadArg.Patterns = patterns.ToArray();
            threadArg.Resources = resources;

            _app.StartJobThread(DemoCutThread, threadArg);
        }

        private void DemoCutThread(object arg)
        {
            var threadArg = arg as ThreadArg;
            if(threadArg == null)
            {
                _app.LogError("Invalid thread argument type");
                return;
            }

            if(threadArg.FilePaths == null || threadArg.Patterns == null || threadArg.Resources == null)
            {
                _app.LogError("Invalid thread argument data");
                return;
            }

            var outputFolder = _app.GetOutputFolder();
            var outputFolderPtr = Marshal.StringToHGlobalAnsi(outputFolder);
            _app.InitParseArg();
            _app.ParseArg.OutputFolderPath = outputFolderPtr;

            var resources = threadArg.Resources;
            resources.GlobalAllocationHandles.Add(outputFolderPtr);

            try
            {
                var config = _app.Config;
                UDT_DLL.CutDemosByPattern(
                    resources, ref _app.ParseArg, threadArg.FilePaths, threadArg.Patterns, 
                    config.CutStartOffset, config.CutEndOffset, config.MaxThreadCount);
            }
            catch(Exception exception)
            {
                _app.LogError("Caught an exception while cutting demos: {0}", exception.Message);
            }
        }
    }
}