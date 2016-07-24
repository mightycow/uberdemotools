using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;


namespace Uber.DemoTools
{
    public class FlagCaptureDisplayInfo : CuttabbleByTimeDisplayInfo
    {
        public FlagCaptureDisplayInfo(int gameStateIndex, int startTimeMs, int endTimeMs, int durationMs, bool baseToBase, string player, string map)
        {
            GameStateIndex = gameStateIndex;
            GameState = gameStateIndex.ToString();
            StartAndEnd = App.FormatMinutesSeconds(startTimeMs / 1000) + " - " + App.FormatMinutesSeconds(endTimeMs / 1000);
            Duration = (((float)durationMs) / 1000.0f).ToString("F3") + "s";
            BaseToBase = baseToBase ? "yes" : "no";
            Player = player;
            Map = map.ToLower();
            StartTimeSec = startTimeMs / 1000;
            EndTimeSec = endTimeMs / 1000;
        }

        public override bool GetStartAndEndTimes(out int startTimeSec, out int endTimeSec)
        {
            startTimeSec = StartTimeSec;
            endTimeSec = EndTimeSec;

            return true;
        }

        public string GameState { get; set; }
        public string StartAndEnd { get; set; }
        public string Duration { get; set; }
        public string BaseToBase { get; set; }
        public string Player { get; set; }
        public string Map { get; set; }
        private int StartTimeSec;
        private int EndTimeSec;
    }

    public class FlagCapturesComponent : AppComponent
    {
        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return new List<DemoInfoListView> { _capturesListView }; } }
        public List<DemoInfoListView> InfoListViews { get { return new List<DemoInfoListView> { _capturesListView }; } }
        public ComponentType Type { get { return ComponentType.Captures; } }
        public bool MultiDemoMode { get { return false; } }

        public FlagCapturesComponent(App app)
        {
            _app = app;
            RootControl = CreateTab();
        }

        public void PopulateViews(DemoInfo demoInfo)
        {
            if(!demoInfo.Analyzed)
            {
                _capturesListView.ItemsSource = null;
                _capturesListView.SelectedIndex = -1;
                return;
            }

            var cutByTimeItem = new MenuItem();
            cutByTimeItem.Header = "Cut by Time (Ctrl+T)";
            cutByTimeItem.Command = _cutByCaptureCommand;
            cutByTimeItem.Click += (obj, args) => OnCutByTimeFromCommandContextClicked();
            App.AddKeyBinding(_capturesListView, Key.T, _cutByCaptureCommand, (obj, args) => OnCutByTimeFromCommandContextClicked());

            var contextMenu = new ContextMenu();
            contextMenu.Items.Add(cutByTimeItem);

            var captures = new ObservableCollection<ListViewItem>();
            foreach(var capture in demoInfo.FlagCaptures)
            {
                var item = new ListViewItem();
                item.Content = capture;
                item.ContextMenu = contextMenu;
                captures.Add(item);
            }

            _capturesListView.ItemsSource = captures;
            if(captures.Count > 0)
            {
                _capturesListView.SelectedIndex = 0;
            }
        }

        public void SaveToConfigObject(UdtConfig config)
        {
            // Nothing to do.
        }

        public void SaveToConfigObject(UdtPrivateConfig config)
        {
            // Nothing to do.
        }

        private App _app;
        private DemoInfoListView _capturesListView = null;
        private static RoutedCommand _cutByCaptureCommand = new RoutedCommand();
        private static RoutedCommand _copyCommand = new RoutedCommand();

        private FrameworkElement CreateTab()
        {
            var columnGS = new GridViewColumn();
            var headerGS = new GridViewColumnHeader();
            headerGS.ToolTip = "GameState Index";
            headerGS.Content = "GS";
            headerGS.Tag = "GameState";
            columnGS.Header = headerGS;
            columnGS.Width = 35;
            columnGS.DisplayMemberBinding = new Binding("GameState");

            var columnStartEndTime = new GridViewColumn();
            var headerStartEndTime = new GridViewColumnHeader();
            headerStartEndTime.Content = "Times";
            headerStartEndTime.Tag = "StartAndEnd";
            columnStartEndTime.Header = headerStartEndTime;
            columnStartEndTime.Width = 75;
            columnStartEndTime.DisplayMemberBinding = new Binding("StartAndEnd");

            var columnDuration = new GridViewColumn();
            var headerDuration = new GridViewColumnHeader();
            headerDuration.Content = "Duration";
            headerDuration.Tag = "Duration";
            columnDuration.Header = headerDuration;
            columnDuration.Width = 75;
            columnDuration.DisplayMemberBinding = new Binding("Duration");

            var columnBaseToBase = new GridViewColumn();
            var headerBaseToBase = new GridViewColumnHeader();
            headerBaseToBase.ToolTip = "Base Pick-Up";
            headerBaseToBase.Content = "BPU";
            headerBaseToBase.Tag = "BaseToBase";
            columnBaseToBase.Header = headerBaseToBase;
            columnBaseToBase.Width = 50;
            columnBaseToBase.DisplayMemberBinding = new Binding("BaseToBase");

            var columnPlayer = new GridViewColumn();
            var headerPlayer = new GridViewColumnHeader();
            headerPlayer.Content = "Player";
            headerPlayer.Tag = "Player";
            columnPlayer.Header = headerPlayer;
            columnPlayer.Width = 175;
            columnPlayer.DisplayMemberBinding = new Binding("Player");

            var columnMap = new GridViewColumn();
            var headerMap = new GridViewColumnHeader();
            headerMap.Content = "Map";
            headerMap.Tag = "Map";
            columnMap.Header = headerMap;
            columnMap.Width = 175;
            columnMap.DisplayMemberBinding = new Binding("Map");

            var commandsGridView = new GridView();
            commandsGridView.AllowsColumnReorder = false;
            commandsGridView.Columns.Add(columnGS);
            commandsGridView.Columns.Add(columnStartEndTime);
            commandsGridView.Columns.Add(columnDuration);
            commandsGridView.Columns.Add(columnBaseToBase);
            commandsGridView.Columns.Add(columnPlayer);
            commandsGridView.Columns.Add(columnMap);

            var commandsListView = new DemoInfoListView();
            _capturesListView = commandsListView;
            commandsListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            commandsListView.VerticalAlignment = VerticalAlignment.Stretch;
            commandsListView.Margin = new Thickness(5);
            commandsListView.View = commandsGridView;
            commandsListView.SelectionMode = SelectionMode.Extended;

            var infoPanelGroupBox = new GroupBox();
            infoPanelGroupBox.Header = "Flag Capture Events";
            infoPanelGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            infoPanelGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            infoPanelGroupBox.Margin = new Thickness(5);
            infoPanelGroupBox.Content = commandsListView;

            return infoPanelGroupBox;
        }

        private void OnCutByTimeFromCommandContextClicked()
        {
            _app.OnCutByTimeContextClicked(_capturesListView);
        }
    }
}