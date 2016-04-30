using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;


namespace Uber.DemoTools
{
    public class TimeShiftModifierComponent : AppComponent
    {
        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return null; } }
        public List<DemoInfoListView> InfoListViews { get { return null; } }
        public ComponentType Type { get { return ComponentType.TimeShiftModifier; } }
        public bool MultiDemoMode { get { return true; } }

        public TimeShiftModifierComponent(App app)
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
            config.TimeShiftSnapshotCount = _snapshotCountComboBox.SelectedIndex + 1;
        }

        public void SaveToConfigObject(UdtPrivateConfig config)
        {
            // Nothing to do.
        }

        private App _app;
        private ComboBox _snapshotCountComboBox;

        private FrameworkElement CreateTab()
        {
            var snapshotCount = _app.Config.TimeShiftSnapshotCount;
            snapshotCount = Math.Max(snapshotCount, 1);
            snapshotCount = Math.Min(snapshotCount, 8);

            var snapshotCountComboBox = new ComboBox();
            _snapshotCountComboBox = snapshotCountComboBox;
            snapshotCountComboBox.HorizontalAlignment = HorizontalAlignment.Left;
            snapshotCountComboBox.VerticalAlignment = VerticalAlignment.Top;
            snapshotCountComboBox.Margin = new Thickness(5);
            snapshotCountComboBox.Width = 40;
            for(var i = 1; i <= 8; ++i)
            {
                snapshotCountComboBox.Items.Add(i);
            }
            snapshotCountComboBox.SelectedIndex = snapshotCount - 1;

            var settingsPanelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            settingsPanelList.Add(App.CreateTuple("Snapshot Count", snapshotCountComboBox));

            var settingsPanel = WpfHelper.CreateDualColumnPanel(settingsPanelList, 120, 0);
            settingsPanel.HorizontalAlignment = HorizontalAlignment.Center;
            settingsPanel.VerticalAlignment = VerticalAlignment.Center;

            var settingsGroupBox = new GroupBox();
            settingsGroupBox.Header = "Settings";
            settingsGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            settingsGroupBox.VerticalAlignment = VerticalAlignment.Top;
            settingsGroupBox.Content = settingsPanel;

            var goButton = new Button();
            goButton.HorizontalAlignment = HorizontalAlignment.Left;
            goButton.VerticalAlignment = VerticalAlignment.Top;
            goButton.Content = "Go!";
            goButton.Width = 75;
            goButton.Height = 25;
            goButton.Margin = new Thickness(5);
            goButton.Click += (obj, args) => OnGoClicked();

            var actionsGroupBox = new GroupBox();
            actionsGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            actionsGroupBox.VerticalAlignment = VerticalAlignment.Top;
            actionsGroupBox.Margin = new Thickness(5);
            actionsGroupBox.Header = "Actions";
            actionsGroupBox.Content = goButton;
            
            var helpTextBlock = new TextBlock();
            helpTextBlock.Margin = new Thickness(5);
            helpTextBlock.TextWrapping = TextWrapping.WrapWithOverflow;
            helpTextBlock.Text =
                "This modifier will move the position of the non-first-person view players back in time by the specified amount of snapshots.\n" + 
                "The main purpose of this is to make the demo look more like what the player whose view was recorded in first-person actually saw, which is desirable for movie-making.\n" + 
                "\n" +
                "For LG tracking, 1 snapshot should do.\n" +
                "For rail shots, 1 + (first-person player ping / snapshot duration) should do.\n" + 
                "Depending on cpma/q3mme doing the playback and cg_smoothClients being on/off, you might want to add 1 to the snapshot count.";

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
            rootPanel.Children.Add(settingsGroupBox);
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

        private void OnGoClicked()
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
                UDT_DLL.TimeShiftDemos(ref _app.ParseArg, filePaths, _app.Config.MaxThreadCount, _app.Config.TimeShiftSnapshotCount);
            }
            catch(Exception exception)
            {
                _app.LogError("Caught an exception while modifying demos: {0}", exception.Message);
            }
        }
    }
}