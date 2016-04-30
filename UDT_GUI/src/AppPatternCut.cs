using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;


namespace Uber.DemoTools
{
    public enum PatternAction
    {
        Search,
        Cut,
        Count
    }

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
            var playerIndex = int.MinValue;
            if(_manualIndexPlayerSelectionRadioButton.IsChecked ?? false)
            {
                playerIndex = _playerIndexComboBox.SelectedIndex;
            }
            else if(_spectatedPlayerSelectionRadioButton.IsChecked ?? false)
            {
                playerIndex = (int)UDT_DLL.udtPlayerIndex.FirstPersonPlayer;
            }
            else if(_demoTakerPlayerSelectionRadioButton.IsChecked ?? false)
            {
                playerIndex = (int)UDT_DLL.udtPlayerIndex.DemoTaker;
            }

            config.PatternCutPlayerIndex = playerIndex;
            config.PatternCutPlayerName = _playerNameTextBox.Text.ToLower();
        }

        private App _app;
        private FilterGroupBox _patternsGroupBox;
        private RadioButton _demoTakerPlayerSelectionRadioButton;
        private RadioButton _spectatedPlayerSelectionRadioButton;
        private RadioButton _manualIndexPlayerSelectionRadioButton;
        private RadioButton _manualNamePlayerSelectionRadioButton;
        private ComboBox _playerIndexComboBox;
        private TextBox _playerNameTextBox;

        private FrameworkElement CreateTab()
        {
            _patternsGroupBox = new FilterGroupBox("Cut Patterns", UDT_DLL.udtStringArray.CutPatterns, 2);
            _patternsGroupBox.SetBitMask((uint)_app.Config.PatternsSelectionBitMask);

            var spectatedPlayerSelectionRadioButton = new RadioButton();
            _spectatedPlayerSelectionRadioButton = spectatedPlayerSelectionRadioButton;
            spectatedPlayerSelectionRadioButton.HorizontalAlignment = HorizontalAlignment.Left;
            spectatedPlayerSelectionRadioButton.VerticalAlignment = VerticalAlignment.Center;
            spectatedPlayerSelectionRadioButton.Margin = new Thickness(0, 0, 0, 5);
            spectatedPlayerSelectionRadioButton.GroupName = "PlayerSelection";
            spectatedPlayerSelectionRadioButton.Content = " First Person Player";
            spectatedPlayerSelectionRadioButton.ToolTip = "Whoever is being followed in first-person in the demo";
            spectatedPlayerSelectionRadioButton.IsChecked = true;

            var demoTakerPlayerSelectionRadioButton = new RadioButton();
            _demoTakerPlayerSelectionRadioButton = demoTakerPlayerSelectionRadioButton;
            demoTakerPlayerSelectionRadioButton.HorizontalAlignment = HorizontalAlignment.Left;
            demoTakerPlayerSelectionRadioButton.VerticalAlignment = VerticalAlignment.Center;
            demoTakerPlayerSelectionRadioButton.GroupName = "PlayerSelection";
            demoTakerPlayerSelectionRadioButton.Content = " Demo Taker";
            demoTakerPlayerSelectionRadioButton.ToolTip = "The player who recorded the demo";
            demoTakerPlayerSelectionRadioButton.IsChecked = false;

            var manualIndexPlayerSelectionRadioButton = new RadioButton();
            _manualIndexPlayerSelectionRadioButton = manualIndexPlayerSelectionRadioButton;
            manualIndexPlayerSelectionRadioButton.HorizontalAlignment = HorizontalAlignment.Left;
            manualIndexPlayerSelectionRadioButton.VerticalAlignment = VerticalAlignment.Center;
            manualIndexPlayerSelectionRadioButton.GroupName = "PlayerSelection";
            manualIndexPlayerSelectionRadioButton.Content = " This Player Index";
            manualIndexPlayerSelectionRadioButton.IsChecked = false;

            var manualNamePlayerSelectionRadioButton = new RadioButton();
            _manualNamePlayerSelectionRadioButton = manualNamePlayerSelectionRadioButton;
            manualNamePlayerSelectionRadioButton.HorizontalAlignment = HorizontalAlignment.Left;
            manualNamePlayerSelectionRadioButton.VerticalAlignment = VerticalAlignment.Center;
            manualNamePlayerSelectionRadioButton.GroupName = "PlayerSelection";
            manualNamePlayerSelectionRadioButton.Content = " This Player Name";
            manualNamePlayerSelectionRadioButton.IsChecked = false;

            var playerIndexComboBox = new ComboBox();
            _playerIndexComboBox = playerIndexComboBox;
            playerIndexComboBox.HorizontalAlignment = HorizontalAlignment.Left;
            playerIndexComboBox.VerticalAlignment = VerticalAlignment.Center;
            playerIndexComboBox.Margin = new Thickness(5, 0, 0, 0);
            playerIndexComboBox.Width = 40;
            for(var i = 0; i < 64; ++i)
            {
                playerIndexComboBox.Items.Add(i.ToString());
            }
            playerIndexComboBox.SelectedIndex = 0;

            var manualIndexPlayerSelectionRow = new StackPanel();
            manualIndexPlayerSelectionRow.Margin = new Thickness(0, 5, 0, 0);
            manualIndexPlayerSelectionRow.Orientation = Orientation.Horizontal;
            manualIndexPlayerSelectionRow.HorizontalAlignment = HorizontalAlignment.Stretch;
            manualIndexPlayerSelectionRow.VerticalAlignment = VerticalAlignment.Stretch;
            manualIndexPlayerSelectionRow.Children.Add(manualIndexPlayerSelectionRadioButton);
            manualIndexPlayerSelectionRow.Children.Add(playerIndexComboBox);

            var playerNameTextBox = new TextBox();
            _playerNameTextBox = playerNameTextBox;
            playerNameTextBox.HorizontalAlignment = HorizontalAlignment.Left;
            playerNameTextBox.VerticalAlignment = VerticalAlignment.Center;
            playerNameTextBox.Margin = new Thickness(5, 0, 0, 0);
            playerNameTextBox.Width = 100;

            var manualNamePlayerSelectionRow = new StackPanel();
            manualNamePlayerSelectionRow.Margin = new Thickness(0, 5, 0, 0);
            manualNamePlayerSelectionRow.Orientation = Orientation.Horizontal;
            manualNamePlayerSelectionRow.HorizontalAlignment = HorizontalAlignment.Stretch;
            manualNamePlayerSelectionRow.VerticalAlignment = VerticalAlignment.Stretch;
            manualNamePlayerSelectionRow.ToolTip = "Name comparisons are case insensitive and color codes are ignored";
            manualNamePlayerSelectionRow.Children.Add(manualNamePlayerSelectionRadioButton);
            manualNamePlayerSelectionRow.Children.Add(playerNameTextBox);

            var playerSelectionPanel = new StackPanel();
            playerSelectionPanel.Margin = new Thickness(10);
            playerSelectionPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            playerSelectionPanel.VerticalAlignment = VerticalAlignment.Stretch;
            playerSelectionPanel.Children.Add(spectatedPlayerSelectionRadioButton);
            playerSelectionPanel.Children.Add(demoTakerPlayerSelectionRadioButton);
            playerSelectionPanel.Children.Add(manualNamePlayerSelectionRow);
            playerSelectionPanel.Children.Add(manualIndexPlayerSelectionRow);

            var playerSelectionGroupBox = new GroupBox();
            playerSelectionGroupBox.Margin = new Thickness(5);
            playerSelectionGroupBox.Header = "Player Selection";
            playerSelectionGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            playerSelectionGroupBox.VerticalAlignment = VerticalAlignment.Top;
            playerSelectionGroupBox.Content = playerSelectionPanel;

            var actionsGroupBox = CreateActionsGroupBox(DoAction);
            
            var helpTextBlock = new TextBlock();
            helpTextBlock.Margin = new Thickness(5);
            helpTextBlock.TextWrapping = TextWrapping.WrapWithOverflow;
            helpTextBlock.Text =
                "UDT will create a cut section for each pattern match during parsing." + 
                "When parsing is done, overlapping cut sections get merged together and a new parsing pass is applied to do the actual cutting." +
                "\n\nNote: The \"Player Selection\" settings are respected by all filters except \"Chat\". " + 
                "In other words, it will use every chat message to look for matches." +
                "\n\nExample of cut section merging: suppose we have 2 matches, the first at 1:27 and the second at 1:30 with " +
                "start and end time offsets 10 and 8." +
                "UDT will create 2 cut sections: 1:17-1:35 and 1:20-1:38, which then get merged into 1: 1:17-1:38.";
            
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
            rootPanel.Children.Add(playerSelectionGroupBox);
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
            public List<DemoInfo> Demos;
            public List<string> FilePaths;
            public UDT_DLL.udtPatternInfo[] Patterns;
            public ArgumentResources Resources;
        }

        private static UInt32 GetBit(UDT_DLL.udtPatternType type)
        {
            return (UInt32)(1 << (int)type);
        }

        private static bool IsPatternActive(UInt32 patterns, UDT_DLL.udtPatternType type)
        {
            return (patterns & GetBit(type)) != 0;
        }

        public delegate void ActionDelegate(PatternAction action);

        public static GroupBox CreateActionsGroupBox(UDT_DLL.udtPatternType pattern)
        {
            return CreateActionsGroupBox((action) => CutByPatternComponent.DoAction(action, pattern));
        }

        private static GroupBox CreateActionsGroupBox(ActionDelegate action)
        {
            var cutButton = new Button();
            cutButton.HorizontalAlignment = HorizontalAlignment.Left;
            cutButton.VerticalAlignment = VerticalAlignment.Top;
            cutButton.Content = "Cut!";
            cutButton.Width = 75;
            cutButton.Height = 25;
            cutButton.Margin = new Thickness(5);
            cutButton.Click += (obj, args) => action(PatternAction.Cut);

            var searchButton = new Button();
            searchButton.HorizontalAlignment = HorizontalAlignment.Left;
            searchButton.VerticalAlignment = VerticalAlignment.Top;
            searchButton.Content = "Search!";
            searchButton.Width = 75;
            searchButton.Height = 25;
            searchButton.Margin = new Thickness(5);
            searchButton.Click += (obj, args) => action(PatternAction.Search);

            var actionsStackPanel = new StackPanel();
            actionsStackPanel.HorizontalAlignment = HorizontalAlignment.Left;
            actionsStackPanel.VerticalAlignment = VerticalAlignment.Top;
            actionsStackPanel.Margin = new Thickness(5);
            actionsStackPanel.Orientation = Orientation.Vertical;
            actionsStackPanel.Children.Add(cutButton);
            actionsStackPanel.Children.Add(searchButton);

            var actionsGroupBox = new GroupBox();
            actionsGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            actionsGroupBox.VerticalAlignment = VerticalAlignment.Top;
            actionsGroupBox.Margin = new Thickness(5);
            actionsGroupBox.Header = "Actions";
            actionsGroupBox.Content = actionsStackPanel;

            return actionsGroupBox;
        }

        private void DoAction(PatternAction action)
        {
            _app.SaveBothConfigs();
            DoAction(action, (uint)_app.Config.PatternsSelectionBitMask, false);
        }

        static private void DoAction(PatternAction action, UDT_DLL.udtPatternType pattern)
        {
            DoAction(action, GetBit(pattern), true);
        }

        static private void DoAction(PatternAction action, uint selectedPatterns, bool saveConfigs)
        {
            var app = App.Instance;

            var demos = app.SelectedWriteDemos;
            if(demos == null)
            {
                return;
            }

            if(selectedPatterns == 0)
            {
                app.LogError("You didn't check any pattern. Please check at least one to proceed.");
                return;
            }

            if(saveConfigs)
            {
                app.SaveBothConfigs();
            }
            var privateConfig = app.PrivateConfig;
            if(privateConfig.PatternCutPlayerIndex == int.MinValue && string.IsNullOrEmpty(privateConfig.PatternCutPlayerName))
            {
                app.LogError("The selected player name is empty. Please specify a player name or select a different matching method to proceed.");
                return;
            }

            var filePaths = new List<string>();
            foreach(var demo in demos)
            {
                filePaths.Add(demo.FilePath);
            }

            var config = app.Config;
            var patterns = new List<UDT_DLL.udtPatternInfo>();
            var resources = new ArgumentResources();

            if(IsPatternActive(selectedPatterns, UDT_DLL.udtPatternType.Chat))
            {
                if(config.ChatRules.Count == 0)
                {
                    app.LogError("[chat] No chat matching rule defined. Please add at least one to proceed.");
                    return;
                }

                var pattern = new UDT_DLL.udtPatternInfo();
                UDT_DLL.CreateChatPatternInfo(ref pattern, resources, config.ChatRules);
                patterns.Add(pattern);
            }

            if(IsPatternActive(selectedPatterns, UDT_DLL.udtPatternType.FragSequences))
            {
                if(privateConfig.FragCutAllowedMeansOfDeaths == 0)
                {
                    app.LogError("[frag sequence] You didn't check any Mean of Death. Please check at least one to proceed.");
                    return;
                }
                if(config.FragCutMinFragCount < 2)
                {
                    app.LogError("[frag sequence] 'Min. Frag Count' must be 2 or higher.");
                    return;
                }
                if(config.FragCutTimeBetweenFrags < 1)
                {
                    app.LogError("[frag sequence] 'Time Between Frags' must be strictly positive.");
                    return;
                }

                var pattern = new UDT_DLL.udtPatternInfo();
                var rules = UDT_DLL.CreateCutByFragArg(config, app.PrivateConfig);                
                UDT_DLL.CreateFragPatternInfo(ref pattern, resources, rules);
                patterns.Add(pattern);
            }

            if(IsPatternActive(selectedPatterns, UDT_DLL.udtPatternType.MidAirFrags))
            {
                if(!config.MidAirCutAllowRocket && !config.MidAirCutAllowBFG)
                {
                    app.LogError("[mid-air frags] You didn't check any weapon. Please check at least one to proceed.");
                    return;
                }

                var pattern = new UDT_DLL.udtPatternInfo();
                var rules = UDT_DLL.CreateCutByMidAirArg(config);
                UDT_DLL.CreateMidAirPatternInfo(ref pattern, resources, rules);
                patterns.Add(pattern);
            }

            if(IsPatternActive(selectedPatterns, UDT_DLL.udtPatternType.MultiFragRails))
            {
                var pattern = new UDT_DLL.udtPatternInfo();
                var rules = UDT_DLL.CreateCutByMultiRailArg(config);
                UDT_DLL.CreateMultiRailPatternInfo(ref pattern, resources, rules);
                patterns.Add(pattern);
            }

            if(IsPatternActive(selectedPatterns, UDT_DLL.udtPatternType.FlagCaptures))
            {
                if(!config.FlagCaptureAllowBaseToBase && !config.FlagCaptureAllowMissingToBase)
                {
                    app.LogError("[flag captures] You disabled both base and non-base pick-ups. Please enable at least one of them to proceed.");
                    return;
                }

                var pattern = new UDT_DLL.udtPatternInfo();
                var rules = UDT_DLL.CreateCutByFlagCaptureArg(config);
                UDT_DLL.CreateFlagCapturePatternInfo(ref pattern, resources, rules);
                patterns.Add(pattern);
            }

            if(IsPatternActive(selectedPatterns, UDT_DLL.udtPatternType.FlickRails))
            {
                if(config.FlickRailMinSpeed <= 0.0f && config.FlickRailMinAngleDelta <= 0.0f)
                {
                    app.LogError("[flick rails] Both thresholds are negative or zero, which will match all railgun frags.");
                    return;
                }

                var pattern = new UDT_DLL.udtPatternInfo();
                var rules = UDT_DLL.CreateCutByFlickRailArg(config);
                UDT_DLL.CreateFlickRailPatternInfo(ref pattern, resources, rules);
                patterns.Add(pattern);
            }

            if(IsPatternActive(selectedPatterns, UDT_DLL.udtPatternType.Matches))
            {
                var pattern = new UDT_DLL.udtPatternInfo();
                var rules = UDT_DLL.CreateCutByMatchArg(config);
                UDT_DLL.CreateMatchPatternInfo(ref pattern, resources, rules);
                patterns.Add(pattern);
            }

            var threadArg = new ThreadArg();
            threadArg.Demos = demos;
            threadArg.FilePaths = filePaths;
            threadArg.Patterns = patterns.ToArray();
            threadArg.Resources = resources;

            app.DisableUiNonThreadSafe();
            app.JoinJobThread();

            switch(action)
            {
                case PatternAction.Cut:
                    app.StartJobThread(DemoCutThread, threadArg);
                    break;

                case PatternAction.Search:
                    app.StartJobThread(DemoSearchThread, threadArg);
                    break;

                default:
                    break;
            }
        }
        
        static private void DemoCutThread(object arg)
        {
            var app = App.Instance;
            var threadArg = arg as ThreadArg;
            if(threadArg == null)
            {
                app.LogError("Invalid thread argument type");
                return;
            }

            if(threadArg.FilePaths == null || threadArg.Patterns == null || threadArg.Resources == null)
            {
                app.LogError("Invalid thread argument data");
                return;
            }

            app.InitParseArg();

            try
            {
                var resources = threadArg.Resources;
                var options = UDT_DLL.CreateCutByPatternOptions(app.Config, app.PrivateConfig);
                UDT_DLL.CutDemosByPattern(resources, ref app.ParseArg, threadArg.FilePaths, threadArg.Patterns, options);
            }
            catch(Exception exception)
            {
                app.LogError("Caught an exception while cutting demos: {0}", exception.Message);
            }
        }

        static private void DemoSearchThread(object arg)
        {
            var app = App.Instance;
            var threadArg = arg as ThreadArg;
            if(threadArg == null)
            {
                app.LogError("Invalid thread argument type");
                return;
            }

            if(threadArg.Demos == null || threadArg.FilePaths == null || threadArg.Patterns == null || threadArg.Resources == null)
            {
                app.LogError("Invalid thread argument data");
                return;
            }

            app.InitParseArg();

            try
            {
                var resources = threadArg.Resources;
                var options = UDT_DLL.CreateCutByPatternOptions(app.Config, app.PrivateConfig);
                var results = UDT_DLL.FindPatternsInDemos(resources, ref app.ParseArg, threadArg.FilePaths, threadArg.Patterns, options);
                app.UpdateSearchResults(results, threadArg.Demos);
            }
            catch(Exception exception)
            {
                app.LogError("Caught an exception while searching demos: {0}", exception.Message);
            }
        }
    }
}