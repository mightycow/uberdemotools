using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;


namespace Uber.DemoTools
{
    public class FlickRailFiltersComponent : AppComponent
    {
        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return null; } }
        public List<DemoInfoListView> InfoListViews { get { return null; } }
        public ComponentType Type { get { return ComponentType.FlickRailFilters; } }
        public bool MultiDemoMode { get { return true; } }

        public FlickRailFiltersComponent(App app)
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
            float value;
            if(float.TryParse(_minSpeedEditBox.Text, out value))
            {
                config.FlickRailMinSpeed = value;
            }
            if(float.TryParse(_minAngleDeltaEditBox.Text, out value))
            {
                config.FlickRailMinAngleDelta = value;
            }

            config.FlickRailMinSpeedSnaps = _minSpeedSnapsComboBox.SelectedIndex + 2;
            config.FlickRailMinAngleDeltaSnaps = _minAngleDeltaSnapsComboBox.SelectedIndex + 2;
        }

        public void SaveToConfigObject(UdtPrivateConfig config)
        {
            // Nothing to do.
        }

        private App _app;
        private TextBox _minSpeedEditBox;
        private TextBox _minAngleDeltaEditBox;
        private ComboBox _minSpeedSnapsComboBox;
        private ComboBox _minAngleDeltaSnapsComboBox;

        private FrameworkElement CreateTab()
        {
            var config = _app.Config;

            var minSpeedEditBox = new TextBox();
            _minSpeedEditBox = minSpeedEditBox;
            minSpeedEditBox.HorizontalAlignment = HorizontalAlignment.Left;
            minSpeedEditBox.VerticalAlignment = VerticalAlignment.Top;
            minSpeedEditBox.Width = 60;
            minSpeedEditBox.Text = config.FlickRailMinSpeed.ToString();
            minSpeedEditBox.ToolTip = "Angular velocity threshold, in degrees per second.";

            var minAngleDeltaEditBox = new TextBox();
            _minAngleDeltaEditBox = minAngleDeltaEditBox;
            minAngleDeltaEditBox.HorizontalAlignment = HorizontalAlignment.Left;
            minAngleDeltaEditBox.VerticalAlignment = VerticalAlignment.Top;
            minAngleDeltaEditBox.Width = 60;
            minAngleDeltaEditBox.Text = config.FlickRailMinAngleDelta.ToString();
            minAngleDeltaEditBox.ToolTip = "Absolute angle difference threshold, in degrees.";

            _minSpeedSnapsComboBox = CreateSnapshotCountComboBox(config.FlickRailMinSpeedSnaps);
            _minAngleDeltaSnapsComboBox = CreateSnapshotCountComboBox(config.FlickRailMinAngleDeltaSnaps);

            var rulesPanelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            rulesPanelList.Add(App.CreateTuple("Min. Speed", minSpeedEditBox));
            rulesPanelList.Add(App.CreateTuple("Speed Snapshots", _minSpeedSnapsComboBox));
            rulesPanelList.Add(App.CreateTuple("Min. Angle Delta", minAngleDeltaEditBox));
            rulesPanelList.Add(App.CreateTuple("Angle Delta Snapshots", _minAngleDeltaSnapsComboBox));

            var rulesPanel = WpfHelper.CreateDualColumnPanel(rulesPanelList, 120, 5);
            rulesPanel.HorizontalAlignment = HorizontalAlignment.Center;
            rulesPanel.VerticalAlignment = VerticalAlignment.Center;

            var rulesGroupBox = new GroupBox();
            rulesGroupBox.Header = "Flick Rail Rules";
            rulesGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            rulesGroupBox.VerticalAlignment = VerticalAlignment.Top;
            rulesGroupBox.Margin = new Thickness(5);
            rulesGroupBox.Content = rulesPanel;

            var actionsGroupBox = CutByPatternComponent.CreateActionsGroupBox(UDT_DLL.udtPatternType.FlickRails);
            
            var helpTextBlock = new TextBlock();
            helpTextBlock.Margin = new Thickness(5);
            helpTextBlock.TextWrapping = TextWrapping.WrapWithOverflow;
            helpTextBlock.Text =
                "Recommended minimum value for angular velocity:" +
                "\n- 800 degrees/second for a fast flick shot" +
                "\n- 500 degrees/second for a reasonably fast flick shot" + 
                "\n\nRecommended minimum angle delta for a good flick shot:" + 
                "\n- 50 degrees if over the duration of 2 snapshots in Quake 3 (snapshot duration: usually 33 ms)" +
                "\n- 37 degrees if over the duration of 2 snapshots in Quake Live (snapshot duration: 25 ms)" + 
                "\n\nNote that:" + 
                "\n- Min. Angle Delta is linearly dependent on Angle Delta Snapshots" +
                "\n- Min. Speed is independent of Speed Snapshots" +
                "\n\nIf you don't care about the minimum angle, set Min. Angle Delta to 0 and just tune the Min. Speed value.";

            rulesPanelList.Add(App.CreateTuple("Min. Speed", minSpeedEditBox));
            rulesPanelList.Add(App.CreateTuple("Speed Snapshots", _minSpeedSnapsComboBox));
            rulesPanelList.Add(App.CreateTuple("Min. Angle Delta", minAngleDeltaEditBox));
            rulesPanelList.Add(App.CreateTuple("Angle Delta Snapshots", _minAngleDeltaSnapsComboBox));

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

        private ComboBox CreateSnapshotCountComboBox(int configValue)
        {
            configValue = Math.Max(configValue, 2);
            configValue = Math.Min(configValue, 4);

            var comboBox = new ComboBox();
            comboBox.HorizontalAlignment = HorizontalAlignment.Left;
            comboBox.VerticalAlignment = VerticalAlignment.Top;
            comboBox.Width = 40;
            comboBox.Items.Add(2);
            comboBox.Items.Add(3);
            comboBox.Items.Add(4);
            comboBox.SelectedIndex = configValue - 2;

            return comboBox;
        }
    }
}