using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;


namespace Uber.DemoTools
{
    public class MidAirFiltersComponent : AppComponent
    {
        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return null; } }
        public List<DemoInfoListView> InfoListViews { get { return null; } }
        public ComponentType Type { get { return ComponentType.MidAirFilters; } }

        public MidAirFiltersComponent(App app)
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
            if(int.TryParse(_minDistanceEditBox.Text, out value))
            {
                config.MidAirCutMinDistance = value;
            }

            config.MidAirCutAllowRocket = _allowRocketsCheckBox.IsChecked ?? false;
            config.MidAirCutAllowGrenade = _allowGrenadesCheckBox.IsChecked ?? false;
            config.MidAirCutAllowBFG = _allowBFGsCheckBox.IsChecked ?? false;
        }

        public void SaveToConfigObject(UdtPrivateConfig config)
        {
            // Nothing to do.
        }

        private App _app;
        private TextBox _minDistanceEditBox;
        private CheckBox _allowRocketsCheckBox;
        private CheckBox _allowGrenadesCheckBox;
        private CheckBox _allowBFGsCheckBox;

        private FrameworkElement CreateTab()
        {
            var minDistanceEditBox = new TextBox();
            _minDistanceEditBox = minDistanceEditBox;
            minDistanceEditBox.Width = 60;
            minDistanceEditBox.Text = _app.Config.MidAirCutMinDistance.ToString();
            minDistanceEditBox.ToolTip = "The minimum distance between the position where the projectile was shot from and the impact position. Setting it to 0 will not filter based on distance.";

            var allowRocketsCheckBox = new CheckBox();
            _allowRocketsCheckBox = allowRocketsCheckBox;
            allowRocketsCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            allowRocketsCheckBox.VerticalAlignment = VerticalAlignment.Center;
            allowRocketsCheckBox.IsChecked = _app.Config.MidAirCutAllowRocket;

            var allowGrenadesCheckBox = new CheckBox();
            _allowGrenadesCheckBox = allowGrenadesCheckBox;
            allowGrenadesCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            allowGrenadesCheckBox.VerticalAlignment = VerticalAlignment.Center;
            allowGrenadesCheckBox.IsChecked = _app.Config.MidAirCutAllowGrenade;

            var allowBFGsCheckBox = new CheckBox();
            _allowBFGsCheckBox = allowBFGsCheckBox;
            allowBFGsCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            allowBFGsCheckBox.VerticalAlignment = VerticalAlignment.Center;
            allowBFGsCheckBox.IsChecked = _app.Config.MidAirCutAllowBFG;

            var rulesPanelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            rulesPanelList.Add(App.CreateTuple("Allow Rockets?", allowRocketsCheckBox));
            rulesPanelList.Add(App.CreateTuple("Allow Grenades?", allowGrenadesCheckBox));
            rulesPanelList.Add(App.CreateTuple("Allow BFG?", allowBFGsCheckBox));
            rulesPanelList.Add(App.CreateTuple("Min. Distance", minDistanceEditBox));

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
            /*
            var helpTextBlock = new TextBlock();
            helpTextBlock.Margin = new Thickness(5);
            helpTextBlock.TextWrapping = TextWrapping.WrapWithOverflow;
            helpTextBlock.Text =
                "Self-kills are suicides where the player shot himself with a weapon." +
                "\nDeaths can include 'suicides' where the killer is the world: lava, fall damage, hurt triggers, etc.";

            var helpGroupBox = new GroupBox();
            helpGroupBox.Margin = new Thickness(5);
            helpGroupBox.Header = "Help";
            helpGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            helpGroupBox.VerticalAlignment = VerticalAlignment.Top;
            helpGroupBox.Content = helpTextBlock;
            */
            var rootPanel = new WrapPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.Margin = new Thickness(5);
            rootPanel.Orientation = Orientation.Horizontal;
            rootPanel.Children.Add(rulesGroupBox);
            rootPanel.Children.Add(actionsGroupBox);
            //rootPanel.Children.Add(helpGroupBox);

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
            var demos = _app.SelectedDemos;
            if(demos == null)
            {
                _app.LogError("No demo was selected. Please select one to proceed.");
                return;
            }

            _app.SaveBothConfigs();

            var config = _app.Config;
            if(!config.MidAirCutAllowRocket && !config.MidAirCutAllowGrenade && !config.MidAirCutAllowBFG)
            {
                _app.LogError("You didn't check any weapon. Please check at least one to proceed.");
                return;
            }

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

            var outputFolder = _app.GetOutputFolder();
            var outputFolderPtr = Marshal.StringToHGlobalAnsi(outputFolder);
            _app.InitParseArg();
            _app.ParseArg.OutputFolderPath = outputFolderPtr;

            try
            {
                var config = _app.Config;
                var rules = UDT_DLL.CreateCutByMidAirArg(config);
                UDT_DLL.CutDemosByMidAir(ref _app.ParseArg, filePaths, rules, config.CutStartOffset, config.CutEndOffset, config.MaxThreadCount);
            }
            catch(Exception exception)
            {
                _app.LogError("Caught an exception while cutting demos: {0}", exception.Message);
            }

            Marshal.FreeHGlobal(outputFolderPtr);
        }
    }
}