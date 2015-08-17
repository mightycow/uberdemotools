using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;


namespace Uber.DemoTools
{
    public class DemoStatsField
    {
        public string Key = "";
        public string Value = "";
        public UDT_DLL.udtStatsCompMode ComparisonMode = UDT_DLL.udtStatsCompMode.NeitherWins;
        public int FieldBitIndex = 0;
        public int IntegerValue = 0;
    }

    public class StatsInfoGroup
    {
        public string Name = "N/A"; // What should be used as a column header.
        public readonly List<DemoStatsField> Fields = new List<DemoStatsField>();
    }

    public class DemoStatsInfo
    {
        public void AddGenericField(string key, string value)
        {
            if(value != null)
            {
                GenericFields.Add(new DemoStatsField { Key = key, Value = value });
            }
        }

        public void AddGenericField(string key, IntPtr stringValue)
        {
            if(stringValue != IntPtr.Zero)
            {
                GenericFields.Add(new DemoStatsField { Key = key, Value = UDT_DLL.SafeGetUTF8String(stringValue) });
            }
        }

        public readonly List<DemoStatsField> GenericFields = new List<DemoStatsField>();
        public readonly List<StatsInfoGroup> TeamStats = new List<StatsInfoGroup>();
        public readonly List<StatsInfoGroup> PlayerStats = new List<StatsInfoGroup>();
    }

    public class StatsComponent : AppComponent
    {
        private DemoInfoListView _matchInfoListView;
        private DemoInfoListView _teamStatsListView;
        private DemoInfoListView _playerStatsListView;
        private FrameworkElement _noStatsPanel;
        private FrameworkElement _statsPanel;
        private FrameworkElement _matchInfoPanel;
        private FrameworkElement _teamStatsPanel;
        private FrameworkElement _playerStatsPanel;
        private int _selectedStatsIndex = 0;
        private App _app;

        private const int KeyColumnWidth = 150;

        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return new List<DemoInfoListView> { _matchInfoListView, _teamStatsListView, _playerStatsListView }; } }
        public List<DemoInfoListView> InfoListViews { get { return new List<DemoInfoListView> { _matchInfoListView, _teamStatsListView, _playerStatsListView }; } }
        public ComponentType Type { get { return ComponentType.Stats; } }
        public bool MultiDemoMode { get { return false; } }

        public StatsComponent(App app)
        {
            _app = app;
            RootControl = CreateTab();
        }

        public void PopulateViews(DemoInfo demoInfo)
        {
            var showStats = demoInfo.Analyzed;
            _statsPanel.Visibility = showStats ? Visibility.Visible : Visibility.Collapsed;
            _noStatsPanel.Visibility = showStats ? Visibility.Collapsed : Visibility.Visible;
            _matchInfoListView.Items.Clear();
            _teamStatsListView.Items.Clear();
            _playerStatsListView.Items.Clear();
            if(!showStats)
            {
                ShowMatchInfo(false);
                ShowTeamStats(false);
                ShowPlayerStats(false);
                return;
            }

            if(demoInfo.MatchStats.Count == 0)
            {
                ShowMatchInfo(false);
                ShowTeamStats(false);
                ShowPlayerStats(false);
                return;
            }

            if(_selectedStatsIndex < 0 || _selectedStatsIndex >= demoInfo.MatchStats.Count)
            {
                _selectedStatsIndex = 0;
            }

            var stats = demoInfo.MatchStats[_selectedStatsIndex];
            ShowMatchInfo(stats.GenericFields.Count > 0);
            ShowTeamStats(stats.TeamStats.Count > 0);
            ShowPlayerStats(stats.PlayerStats.Count > 0);

            foreach(var field in stats.GenericFields)
            {
                _matchInfoListView.Items.Add(new string[] { field.Key, field.Value });
            }
            
            if(stats.TeamStats.Count == 2 &&
               stats.TeamStats[0].Fields.Count == stats.TeamStats[1].Fields.Count)
            {
                var fieldCount = stats.TeamStats[0].Fields.Count;
                for(var i = 0; i < fieldCount; ++i)
                {
                    var field0 = stats.TeamStats[0].Fields[i];
                    var field1 = stats.TeamStats[1].Fields[i];
                    _teamStatsListView.Items.Add(new string[] { field0.Key, field0.Value, field1.Value });
                }
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

        private FrameworkElement CreateTab()
        {
            var noDataTextBlock = new TextBlock();
            _noStatsPanel = noDataTextBlock;
            noDataTextBlock.HorizontalAlignment = HorizontalAlignment.Center;
            noDataTextBlock.VerticalAlignment = VerticalAlignment.Center;
            noDataTextBlock.Text = "This demo was not analyzed.";
            noDataTextBlock.Visibility = Visibility.Collapsed;

            var matchInfoListView = CreateMatchInfoListView();
            var teamStatsListView = CreateTeamStatsListView();
            var playerStatsListView = CreatePlayerStatsListView();
            _matchInfoListView = matchInfoListView;
            _teamStatsListView = teamStatsListView;
            _playerStatsListView = playerStatsListView;

            var matchInfoGroupBox = new GroupBox();
            _matchInfoPanel = matchInfoGroupBox;
            matchInfoGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            matchInfoGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            matchInfoGroupBox.Margin = new Thickness(5);
            matchInfoGroupBox.Header = "Match Info";
            matchInfoGroupBox.Content = matchInfoListView;

            var teamStatsGroupBox = new GroupBox();
            _teamStatsPanel = teamStatsGroupBox;
            teamStatsGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            teamStatsGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            teamStatsGroupBox.Margin = new Thickness(5);
            teamStatsGroupBox.Header = "Team Scores and Stats";
            teamStatsGroupBox.Content = teamStatsListView;

            var playerStatsGroupBox = new GroupBox();
            _playerStatsPanel = playerStatsGroupBox;
            playerStatsGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            playerStatsGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            playerStatsGroupBox.Margin = new Thickness(5);
            playerStatsGroupBox.Header = "Player Scores and Stats";
            playerStatsGroupBox.Content = playerStatsListView;

            var statsPanel = new StackPanel();
            statsPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            statsPanel.VerticalAlignment = VerticalAlignment.Stretch;
            statsPanel.Margin = new Thickness(5);
            statsPanel.Orientation = Orientation.Vertical;
            statsPanel.Children.Add(matchInfoGroupBox);
            statsPanel.Children.Add(teamStatsGroupBox);
            statsPanel.Children.Add(playerStatsGroupBox);

            var rootPanel = new StackPanel();
            _statsPanel = statsPanel;
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.Margin = new Thickness(5);
            rootPanel.Orientation = Orientation.Vertical;
            rootPanel.Children.Add(_noStatsPanel);
            rootPanel.Children.Add(_statsPanel);

            var statsGroupBox = new GroupBox();
            statsGroupBox.Header = "Scores and Stats";
            statsGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            statsGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            statsGroupBox.Margin = new Thickness(5);
            statsGroupBox.Content = rootPanel;

            var scrollViewer = new ScrollViewer();
            scrollViewer.HorizontalAlignment = HorizontalAlignment.Stretch;
            scrollViewer.VerticalAlignment = VerticalAlignment.Stretch;
            scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Auto;
            scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
            scrollViewer.Content = statsGroupBox;

            return scrollViewer; 
        }

        private DemoInfoListView CreateMatchInfoListView()
        {
            var columnKey = new GridViewColumn();
            var headerKey = new GridViewColumnHeader();
            headerKey.Content = "Key";
            headerKey.Tag = "Key";
            columnKey.Header = headerKey;
            columnKey.Width = KeyColumnWidth;
            columnKey.DisplayMemberBinding = new Binding("[0]");

            var columnValue = new GridViewColumn();
            var headerValue = new GridViewColumnHeader();
            headerValue.Content = "Value";
            headerValue.Tag = "Value";
            columnValue.Header = headerValue;
            columnValue.Width = 400;
            columnValue.DisplayMemberBinding = new Binding("[1]");

            var gridView = new GridView();
            gridView.AllowsColumnReorder = false;
            gridView.Columns.Add(columnKey);
            gridView.Columns.Add(columnValue);

            var listView = new DemoInfoListView();
            listView.HorizontalAlignment = HorizontalAlignment.Stretch;
            listView.VerticalAlignment = VerticalAlignment.Stretch;
            listView.Margin = new Thickness(5);
            listView.View = gridView;
            listView.SelectionMode = SelectionMode.Extended;

            return listView;
        }

        private DemoInfoListView CreateTeamStatsListView()
        {
            var columnKey = new GridViewColumn();
            var headerKey = new GridViewColumnHeader();
            headerKey.Content = "Key";
            headerKey.Tag = "Key";
            columnKey.Header = headerKey;
            columnKey.Width = KeyColumnWidth;
            columnKey.DisplayMemberBinding = new Binding("[0]");

            var columnRed = new GridViewColumn();
            var headerRed = new GridViewColumnHeader();
            headerRed.Content = "Red";
            headerRed.Tag = "Red";
            columnRed.Header = headerRed;
            columnRed.Width = 200;
            columnRed.DisplayMemberBinding = new Binding("[1]");

            var columnBlue = new GridViewColumn();
            var headerBlue = new GridViewColumnHeader();
            headerBlue.Content = "Blue";
            headerBlue.Tag = "Blue";
            columnBlue.Header = headerBlue;
            columnBlue.Width = 200;
            columnBlue.DisplayMemberBinding = new Binding("[2]");

            var gridView = new GridView();
            gridView.AllowsColumnReorder = false;
            gridView.Columns.Add(columnKey);
            gridView.Columns.Add(columnRed);
            gridView.Columns.Add(columnBlue);

            var listView = new DemoInfoListView();
            listView.HorizontalAlignment = HorizontalAlignment.Stretch;
            listView.VerticalAlignment = VerticalAlignment.Stretch;
            listView.Margin = new Thickness(5);
            listView.View = gridView;
            listView.SelectionMode = SelectionMode.Extended;

            return listView;
        }

        private DemoInfoListView CreatePlayerStatsListView()
        {
            var columnKey = new GridViewColumn();
            var headerKey = new GridViewColumnHeader();
            headerKey.Content = "Key";
            headerKey.Tag = "Key";
            columnKey.Header = headerKey;
            columnKey.Width = KeyColumnWidth;
            columnKey.DisplayMemberBinding = new Binding("[0]");

            var gridView = new GridView();
            gridView.AllowsColumnReorder = false;
            gridView.Columns.Add(columnKey);

            var listView = new DemoInfoListView();
            listView.HorizontalAlignment = HorizontalAlignment.Stretch;
            listView.VerticalAlignment = VerticalAlignment.Stretch;
            listView.Margin = new Thickness(5);
            listView.View = gridView;
            listView.SelectionMode = SelectionMode.Extended;

            return listView;
        }

        private void ShowMatchInfo(bool show)
        {
            _matchInfoPanel.Visibility = show ? Visibility.Visible : Visibility.Collapsed;
        }

        private void ShowTeamStats(bool show)
        {
            _teamStatsPanel.Visibility = show ? Visibility.Visible : Visibility.Collapsed;
        }

        private void ShowPlayerStats(bool show)
        {
            _playerStatsPanel.Visibility = show ? Visibility.Visible : Visibility.Collapsed;
        }
    }
}