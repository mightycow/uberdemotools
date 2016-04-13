using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;


namespace Uber.DemoTools
{
    public class FlagCaptureFilterComponent : AppComponent
    {
        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return null; } }
        public List<DemoInfoListView> InfoListViews { get { return null; } }
        public ComponentType Type { get { return ComponentType.FlagCaptureFilters; } }
        public bool MultiDemoMode { get { return true; } }

        public FlagCaptureFilterComponent(App app)
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
            if(App.GetTimeSeconds(_minCarryTimeTextBox.Text, out seconds))
            {
                config.FlagCaptureMinCarryTimeMs = seconds * 1000;
            }
            if(App.GetTimeSeconds(_maxCarryTimeTextBox.Text, out seconds))
            {
                config.FlagCaptureMaxCarryTimeMs = seconds * 1000;
            }
            config.FlagCaptureAllowBaseToBase = _allowBaseToBaseCheckBox.IsChecked ?? false;
            config.FlagCaptureAllowMissingToBase = _allowMissingToBaseCheckBox.IsChecked ?? false;
        }

        public void SaveToConfigObject(UdtPrivateConfig config)
        {
            // Nothing to do.
        }

        private App _app;
        private TextBox _minCarryTimeTextBox;
        private TextBox _maxCarryTimeTextBox;
        private CheckBox _allowBaseToBaseCheckBox;
        private CheckBox _allowMissingToBaseCheckBox;

        private FrameworkElement CreateTab()
        {
            var minCarryTimeTextBox = new TextBox();
            _minCarryTimeTextBox = minCarryTimeTextBox;
            minCarryTimeTextBox.Width = 50;
            minCarryTimeTextBox.Text = App.FormatMinutesSeconds(_app.Config.FlagCaptureMinCarryTimeMs / 1000);
            minCarryTimeTextBox.ToolTip = "seconds OR minutes:seconds";

            var maxCarryTimeTextBox = new TextBox();
            _maxCarryTimeTextBox = maxCarryTimeTextBox;
            maxCarryTimeTextBox.Width = 50;
            maxCarryTimeTextBox.Text = App.FormatMinutesSeconds(_app.Config.FlagCaptureMaxCarryTimeMs / 1000);
            maxCarryTimeTextBox.ToolTip = "seconds OR minutes:seconds";

            var allowBaseToBaseCheckBox = new CheckBox();
            _allowBaseToBaseCheckBox = allowBaseToBaseCheckBox;
            allowBaseToBaseCheckBox.VerticalAlignment = VerticalAlignment.Center;
            allowBaseToBaseCheckBox.IsChecked = _app.Config.FlagCaptureAllowBaseToBase;
            allowBaseToBaseCheckBox.Content = " Allow flag pick-ups from the flag's return position?";

            var allowMissingToBaseCheckBox = new CheckBox();
            _allowMissingToBaseCheckBox = allowMissingToBaseCheckBox;
            allowMissingToBaseCheckBox.VerticalAlignment = VerticalAlignment.Center;
            allowMissingToBaseCheckBox.IsChecked = _app.Config.FlagCaptureAllowMissingToBase;
            allowMissingToBaseCheckBox.Content = " Allow flag pick-ups not from the flag's return position?";

            var rulesPanelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            rulesPanelList.Add(App.CreateTuple("Min. Carry Time", minCarryTimeTextBox));
            rulesPanelList.Add(App.CreateTuple("Max. Carry Time", maxCarryTimeTextBox));
            rulesPanelList.Add(App.CreateTuple("Base Pick-ups?", allowBaseToBaseCheckBox));
            rulesPanelList.Add(App.CreateTuple("Non-base Pick-ups?", allowMissingToBaseCheckBox));

            var rulesPanel = WpfHelper.CreateDualColumnPanel(rulesPanelList, 120, 5);
            rulesPanel.HorizontalAlignment = HorizontalAlignment.Center;
            rulesPanel.VerticalAlignment = VerticalAlignment.Center;

            var rulesGroupBox = new GroupBox();
            rulesGroupBox.Header = "Capture Rules";
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

            var rootPanel = new WrapPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.Margin = new Thickness(5);
            rootPanel.Orientation = Orientation.Horizontal;
            rootPanel.Children.Add(rulesGroupBox);
            rootPanel.Children.Add(actionsGroupBox);
            
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
            if(!_app.Config.FlagCaptureAllowBaseToBase &&
                !_app.Config.FlagCaptureAllowMissingToBase)
            {
                _app.LogError("You disabled both base and non-base pick-ups. Please enable at least one of them to proceed.");
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

            _app.InitParseArg();

            try
            {
                var config = _app.Config;
                var rules = UDT_DLL.CreateCutByFlagCaptureArg(config);
                UDT_DLL.CutDemosByFlagCapture(ref _app.ParseArg, filePaths, rules, UDT_DLL.CreateCutByPatternOptions(config, _app.PrivateConfig));
            }
            catch(Exception exception)
            {
                _app.LogError("Caught an exception while cutting demos: {0}", exception.Message);
            }
        }
    }
}