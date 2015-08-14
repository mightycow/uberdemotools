using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;


namespace Uber.DemoTools
{
    public class StatsComponent : AppComponent
    {
        private DemoInfoListView _teamStatsListView;
        private DemoInfoListView _playerStatsListView;
        private FrameworkElement _noDataMessageElement;
        private GroupBox _teamStatsGroupBox;
        private GroupBox _playerStatsGroupBox;
        private App _app;

        private const int KeyColumnWidth = 150;

        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return new List<DemoInfoListView> { _teamStatsListView, _playerStatsListView }; } }
        public List<DemoInfoListView> InfoListViews { get { return new List<DemoInfoListView> { _teamStatsListView, _playerStatsListView }; } }
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
            _teamStatsGroupBox.Visibility = showStats ? Visibility.Visible : Visibility.Collapsed;
            _playerStatsGroupBox.Visibility = showStats ? Visibility.Visible : Visibility.Collapsed;
            _noDataMessageElement.Visibility = showStats ? Visibility.Collapsed : Visibility.Visible;
            if(!showStats)
            {
                _teamStatsListView.Items.Clear();
                _playerStatsListView.Items.Clear();
                return;
            }

            // @TODO:
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
            _noDataMessageElement = noDataTextBlock;
            noDataTextBlock.HorizontalAlignment = HorizontalAlignment.Center;
            noDataTextBlock.VerticalAlignment = VerticalAlignment.Center;
            noDataTextBlock.Text = "This demo was not analyzed.";
            noDataTextBlock.Visibility = Visibility.Collapsed;

            var teamStatsListView = CreateTeamStatsListView();
            var playerStatsListView = CreatePlayerStatsListView();
            _teamStatsListView = teamStatsListView;
            _playerStatsListView = playerStatsListView;

            var teamStatsGroupBox = new GroupBox();
            _teamStatsGroupBox = teamStatsGroupBox;
            teamStatsGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            teamStatsGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            teamStatsGroupBox.Margin = new Thickness(5);
            teamStatsGroupBox.Header = "Team Scores and Stats";
            teamStatsGroupBox.Content = teamStatsListView;

            var playerStatsGroupBox = new GroupBox();
            _playerStatsGroupBox = playerStatsGroupBox;
            playerStatsGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            playerStatsGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            playerStatsGroupBox.Margin = new Thickness(5);
            playerStatsGroupBox.Header = "Player Scores and Stats";
            playerStatsGroupBox.Content = playerStatsListView;

            var rootPanel = new StackPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.Margin = new Thickness(5);
            rootPanel.Orientation = Orientation.Vertical;
            rootPanel.Children.Add(noDataTextBlock);
            rootPanel.Children.Add(teamStatsGroupBox);
            rootPanel.Children.Add(playerStatsGroupBox);

            var infoPanelGroupBox = new GroupBox();
            infoPanelGroupBox.Header = "Scores and Stats";
            infoPanelGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            infoPanelGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            infoPanelGroupBox.Margin = new Thickness(5);
            infoPanelGroupBox.Content = rootPanel;

            return infoPanelGroupBox;
        }

        private DemoInfoListView CreateTeamStatsListView()
        {
            var columnKey = new GridViewColumn();
            var headerKey = new GridViewColumnHeader();
            headerKey.Content = "Key";
            headerKey.Tag = "Key";
            columnKey.Header = headerKey;
            columnKey.Width = KeyColumnWidth;

            var columnRed = new GridViewColumn();
            var headerRed = new GridViewColumnHeader();
            headerRed.Content = "Red";
            headerRed.Tag = "Red";
            columnRed.Header = headerRed;
            columnRed.Width = 200;

            var columnBlue = new GridViewColumn();
            var headerBlue = new GridViewColumnHeader();
            headerBlue.Content = "Blue";
            headerBlue.Tag = "Blue";
            columnBlue.Header = headerBlue;
            columnBlue.Width = 200;

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

            var gridView = new GridView();
            gridView.AllowsColumnReorder = true;
            gridView.Columns.Add(columnKey);

            var listView = new DemoInfoListView();
            listView.HorizontalAlignment = HorizontalAlignment.Stretch;
            listView.VerticalAlignment = VerticalAlignment.Stretch;
            listView.Margin = new Thickness(5);
            listView.View = gridView;
            listView.SelectionMode = SelectionMode.Extended;

            return listView;
        }
    }
}