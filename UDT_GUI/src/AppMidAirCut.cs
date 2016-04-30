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
            if(int.TryParse(_minDistanceEditBox.Text, out value))
            {
                config.MidAirCutMinDistance = value;
            }

            if(int.TryParse(_minAirTimeEditBox.Text, out value))
            {
                config.MidAirCutMinAirTimeMs = value;
            }

            config.MidAirCutAllowRocket = _allowRocketsCheckBox.IsChecked ?? false;
            config.MidAirCutAllowBFG = _allowBFGsCheckBox.IsChecked ?? false;
        }

        public void SaveToConfigObject(UdtPrivateConfig config)
        {
            // Nothing to do.
        }

        private App _app;
        private TextBox _minDistanceEditBox;
        private TextBox _minAirTimeEditBox;
        private CheckBox _allowRocketsCheckBox;
        private CheckBox _allowBFGsCheckBox;

        private FrameworkElement CreateTab()
        {
            var minDistanceEditBox = new TextBox();
            _minDistanceEditBox = minDistanceEditBox;
            minDistanceEditBox.Width = 60;
            minDistanceEditBox.Text = _app.Config.MidAirCutMinDistance.ToString();
            minDistanceEditBox.ToolTip = "The minimum distance between the position where the projectile was shot from and the impact position. Setting it to 0 will not filter based on distance.";

            var minAirTimeEditBox = new TextBox();
            _minAirTimeEditBox = minAirTimeEditBox;
            minAirTimeEditBox.Width = 60;
            minAirTimeEditBox.Text = _app.Config.MidAirCutMinAirTimeMs.ToString();
            minAirTimeEditBox.ToolTip = "The minimum amount of time the victim was in the air prior to the hit, in milli-seconds.";

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

            var rulesPanelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            rulesPanelList.Add(App.CreateTuple("Allow Rockets?", allowRocketsCheckBox));
            rulesPanelList.Add(App.CreateTuple("Allow BFG?", allowBFGsCheckBox));
            rulesPanelList.Add(App.CreateTuple("Min. Distance", minDistanceEditBox));
            rulesPanelList.Add(App.CreateTuple("Min. Air Time [ms]", minAirTimeEditBox));

            var rulesPanel = WpfHelper.CreateDualColumnPanel(rulesPanelList, 120, 5);
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
                "Recommended minimum value for victim air time: 300 ms" +
                "\nRecommended minimum value for projectile distance: 300 \"Quake units\"" + 
                "\n\nIf you set a low min. distance like 300, it would be recommended to set the air time to be at least 600." +
                "\nAlternatively, if you set a low min. air time like 300, it would be recommended to set the min. distance to at least 600.";

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