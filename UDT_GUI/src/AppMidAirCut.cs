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
        public bool MultiDemoMode { get { return true; } }

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
            if(int.TryParse(_minProjectileTimeEditBox.Text, out value))
            {
                config.MidAirCutMinProjectileTimeMs = value;
            }

            if(int.TryParse(_minAirTimeEditBox.Text, out value))
            {
                config.MidAirCutMinAirTimeMs = value;
            }

            if(int.TryParse(_minGroundDistanceEditBox.Text, out value))
            {
                config.MidAirCutMinGroundDistance = value;
            }

            config.MidAirCutAllowRocket = _allowRocketsCheckBox.IsChecked ?? false;
            config.MidAirCutAllowBFG = _allowBFGsCheckBox.IsChecked ?? false;
            config.MidAirCutAllowGrenade = _allowGrenadesCheckBox.IsChecked ?? false;
        }

        public void SaveToConfigObject(UdtPrivateConfig config)
        {
            // Nothing to do.
        }

        private App _app;
        private TextBox _minProjectileTimeEditBox;
        private TextBox _minAirTimeEditBox;
        private TextBox _minGroundDistanceEditBox;
        private CheckBox _allowRocketsCheckBox;
        private CheckBox _allowBFGsCheckBox;
        private CheckBox _allowGrenadesCheckBox;

        private FrameworkElement CreateTab()
        {
            var minProjectileTimeEditBox = new TextBox();
            _minProjectileTimeEditBox = minProjectileTimeEditBox;
            minProjectileTimeEditBox.Width = 60;
            minProjectileTimeEditBox.Text = _app.Config.MidAirCutMinProjectileTimeMs.ToString();
            minProjectileTimeEditBox.ToolTip = "The minimum amount of time the projectile was in existence prior to the hit, in milli-seconds.";

            var minAirTimeEditBox = new TextBox();
            _minAirTimeEditBox = minAirTimeEditBox;
            minAirTimeEditBox.Width = 60;
            minAirTimeEditBox.Text = _app.Config.MidAirCutMinAirTimeMs.ToString();
            minAirTimeEditBox.ToolTip = "The minimum amount of time the victim was in the air prior to the hit, in milli-seconds.";

            var minGroundDistanceEditBox = new TextBox();
            _minGroundDistanceEditBox = minGroundDistanceEditBox;
            minGroundDistanceEditBox.Width = 60;
            minGroundDistanceEditBox.Text = _app.Config.MidAirCutMinGroundDistance.ToString();
            minGroundDistanceEditBox.ToolTip = "The minimum distance from the 'ground' the victim was at impact time, in Quake units.";

            var allowRocketsCheckBox = new CheckBox();
            _allowRocketsCheckBox = allowRocketsCheckBox;
            allowRocketsCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            allowRocketsCheckBox.VerticalAlignment = VerticalAlignment.Center;
            allowRocketsCheckBox.IsChecked = _app.Config.MidAirCutAllowRocket;

            var allowBFGsCheckBox = new CheckBox();
            _allowBFGsCheckBox = allowBFGsCheckBox;
            allowBFGsCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            allowBFGsCheckBox.VerticalAlignment = VerticalAlignment.Center;
            allowBFGsCheckBox.IsChecked = _app.Config.MidAirCutAllowBFG;

            var allowGrenadesCheckBox = new CheckBox();
            _allowGrenadesCheckBox = allowGrenadesCheckBox;
            allowGrenadesCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            allowGrenadesCheckBox.VerticalAlignment = VerticalAlignment.Center;
            allowGrenadesCheckBox.IsChecked = _app.Config.MidAirCutAllowGrenade;

            var rulesPanelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            rulesPanelList.Add(App.CreateTuple("Allow Rockets?", allowRocketsCheckBox));
            rulesPanelList.Add(App.CreateTuple("Allow BFG?", allowBFGsCheckBox));
            rulesPanelList.Add(App.CreateTuple("CPMA: Allow Grenades?", allowGrenadesCheckBox));
            rulesPanelList.Add(App.CreateTuple("Min. Projectile Time [ms]", minProjectileTimeEditBox));
            rulesPanelList.Add(App.CreateTuple("Min. Victim Air Time [ms]", minAirTimeEditBox));
            rulesPanelList.Add(App.CreateTuple("CPMA: Min. Ground Distance", minGroundDistanceEditBox));

            var rulesPanel = WpfHelper.CreateDualColumnPanel(rulesPanelList, 160, 5);
            rulesPanel.HorizontalAlignment = HorizontalAlignment.Center;
            rulesPanel.VerticalAlignment = VerticalAlignment.Center;

            var rulesGroupBox = new GroupBox();
            rulesGroupBox.Header = "Frag Rules";
            rulesGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            rulesGroupBox.VerticalAlignment = VerticalAlignment.Top;
            rulesGroupBox.Margin = new Thickness(5);
            rulesGroupBox.Content = rulesPanel;

            var actionsGroupBox = CutByPatternComponent.CreateActionsGroupBox(UDT_DLL.udtPatternType.MidAirFrags);
            
            var helpTextBlock = new TextBlock();
            helpTextBlock.Margin = new Thickness(5);
            helpTextBlock.TextWrapping = TextWrapping.WrapWithOverflow;
            helpTextBlock.Text =
                "\"Allow Grenades?\" and \"Min. Ground Distance\" are available only for CPMA 1.50+ demos and will be ignored for other demos.\n\n" +
                "Notes for CPMA 1.50+:\n" +
                "All numbers extracted from the demos are much more reliable.\n" +
                "All direct hits against players in water/slime/lava are filtered out.\n" +
                "The \"ground distance\" is the vertical distance from the player's collision model to the \"ground\", " + 
                "so it doesn't account for horizontal movement and try to predict on which surface the victim was going to land on.\n" +
                "The \"ground\" can be a solid surface, water, lava, slime or a player clip.";

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
    }
}