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
                "Recommended minimum value for angular velocity: 800 degrees/second for a good flick railgun shot.";

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
            if(config.FlickRailMinSpeed <= 0.0f && config.FlickRailMinAngleDelta <= 0.0f)
            {
                _app.LogError("Both thresholds are negative or zero, which will match all railgun frags.");
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
                var rules = UDT_DLL.CreateCutByFlickRailArg(config);
                UDT_DLL.CutDemosByFlickRail(ref _app.ParseArg, filePaths, rules, UDT_DLL.CreateCutByPatternOptions(config, _app.PrivateConfig));
            }
            catch(Exception exception)
            {
                _app.LogError("Caught an exception while cutting demos: {0}", exception.Message);
            }

            Marshal.FreeHGlobal(outputFolderPtr);
        }
    }
}