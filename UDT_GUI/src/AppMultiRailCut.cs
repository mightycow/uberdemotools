using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;


namespace Uber.DemoTools
{
    public class MultiRailFiltersComponent : AppComponent
    {
        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return null; } }
        public List<DemoInfoListView> InfoListViews { get { return null; } }
        public ComponentType Type { get { return ComponentType.MultiRailFilters; } }
        public bool MultiDemoMode { get { return true; } }

        public MultiRailFiltersComponent(App app)
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
            int value;
            if(int.TryParse(_minFragCountComboBox.Text, out value))
            {
                config.MultiRailCutMinFragCount = value;
            }
        }

        public void SaveToConfigObject(UdtPrivateConfig config)
        {
            // Nothing to do.
        }

        private App _app;
        private ComboBox _minFragCountComboBox;

        private FrameworkElement CreateTab()
        {
            var minCount = _app.Config.MultiRailCutMinFragCount;
            var minCountIndex = 0;
            if(minCount >= 2 && minCount <= 8)
            {
                minCountIndex = minCount - 2;
            }

            var minFragCountComboBox = new ComboBox();
            _minFragCountComboBox = minFragCountComboBox;
            minFragCountComboBox.MinWidth = 40;
            for(var i = 2; i <= 8; ++i)
            {
                minFragCountComboBox.Items.Add(i.ToString());
            }
            minFragCountComboBox.SelectedIndex = minCountIndex;

            var rulesPanelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            rulesPanelList.Add(App.CreateTuple("Min. Frag Count", minFragCountComboBox));

            var rulesPanel = WpfHelper.CreateDualColumnPanel(rulesPanelList, 120, 5);
            rulesPanel.HorizontalAlignment = HorizontalAlignment.Center;
            rulesPanel.VerticalAlignment = VerticalAlignment.Center;

            var rulesGroupBox = new GroupBox();
            rulesGroupBox.Header = "Frag Rules";
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
            helpTextBlock.Text = "A \"multi-frag rail\" is a railgun shot that kills at least 2 players.";

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
                var rules = UDT_DLL.CreateCutByMultiRailArg(config);
                UDT_DLL.CutDemosByMultiRail(ref _app.ParseArg, filePaths, rules, UDT_DLL.CreateCutByPatternOptions(config, _app.PrivateConfig));
            }
            catch(Exception exception)
            {
                _app.LogError("Caught an exception while cutting demos: {0}", exception.Message);
            }
        }
    }
}