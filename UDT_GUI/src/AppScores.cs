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
    public class TeamScoreDisplayInfo : CuttabbleByTimeDisplayInfo
    {
        public TeamScoreDisplayInfo(int gameStateIndex, string time, int redScore, int blueScore)
        {
            GameStateIndex = gameStateIndex;
            GameState = gameStateIndex.ToString();
            Time = time;
            RedScore = redScore.ToString();
            BlueScore = blueScore.ToString();
        }

        public override string ToString()
        {
            return string.Format("[{0}] ({1}) {2} {3}", GameState, Time, RedScore, BlueScore);
        }

        public string GameState { get; set; }
        public string RedScore { get; set; }
        public string BlueScore { get; set; }
    }

    public class ScoreDisplayInfo : CuttabbleByTimeDisplayInfo
    {
        public ScoreDisplayInfo(int gameStateIndex, string time, int score1, int score2, string name1, string name2)
        {
            GameStateIndex = gameStateIndex;
            GameState = gameStateIndex.ToString();
            Time = time;
            Score1 = score1.ToString();
            Score2 = score2.ToString();
            Name1 = name1;
            Name2 = name2;
        }

        public override string ToString()
        {
            return string.Format("[{0}] ({1}) {2} {3} - {4} {5}", GameState, Time, Name1, Score1, Score2, Name2);
        }

        public string GameState { get; set; }
        public string Score1 { get; set; }
        public string Score2 { get; set; }
        public string Name1 { get; set; }
        public string Name2 { get; set; }
    }

    public class ScoresComponent : AppComponent
    {
        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return new List<DemoInfoListView> { _scoresListView, _teamScoresListView }; } }
        public List<DemoInfoListView> InfoListViews { get { return new List<DemoInfoListView> { _scoresListView, _teamScoresListView }; } }
        public ComponentType Type { get { return ComponentType.Scores; } }
        public bool MultiDemoMode { get { return false; } }

        public ScoresComponent(App app)
        {
            _app = app;
            RootControl = CreateTab();
        }

        public void PopulateViews(DemoInfo demoInfo)
        {
            if(!demoInfo.Analyzed)
            {
                _scoresListView.ItemsSource = null;
                _scoresListView.SelectedIndex = -1;
                _teamScoresListView.ItemsSource = null;
                _teamScoresListView.SelectedIndex = -1;
                _groupBox.Content = _scoresListView;
                return;
            }

            var activeListView = demoInfo.TeamGameType ? _teamScoresListView : _scoresListView;
            _groupBox.Content = activeListView;
            
            var cutByTimeItem = new MenuItem();
            cutByTimeItem.Header = "Cut by Time (Ctrl+T)";
            cutByTimeItem.Command = _cutByScoreCommand;
            cutByTimeItem.Click += (obj, args) => OnCutByTimeClicked();
            App.AddKeyBinding(activeListView, Key.T, _cutByScoreCommand, (obj, args) => OnCutByTimeClicked());

            var contextMenu = new ContextMenu();
            contextMenu.Items.Add(cutByTimeItem);

            var scores = new ObservableCollection<ListViewItem>();
            if(demoInfo.TeamGameType)
            {
                foreach(var score in demoInfo.TeamScores)
                {
                    var item = new ListViewItem();
                    item.Content = score;
                    item.ContextMenu = contextMenu;
                    scores.Add(item);
                }
            }
            else
            {
                foreach(var score in demoInfo.Scores)
                {
                    var item = new ListViewItem();
                    item.Content = score;
                    item.ContextMenu = contextMenu;
                    scores.Add(item);
                }
            }

            activeListView.ItemsSource = scores;
            if(scores.Count > 0)
            {
                activeListView.SelectedIndex = 0;
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
        private DemoInfoListView _scoresListView;
        private DemoInfoListView _teamScoresListView;
        private GroupBox _groupBox;
        private static RoutedCommand _cutByScoreCommand = new RoutedCommand();
        private static RoutedCommand _copyCommand = new RoutedCommand();

        private FrameworkElement CreateTab()
        {
            CreateScoresListView();
            CreateTeamScoresListView();

            var groupBox = new GroupBox();
            _groupBox = groupBox;
            groupBox.Header = "Scores";
            groupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            groupBox.VerticalAlignment = VerticalAlignment.Stretch;
            groupBox.Margin = new Thickness(5);
            groupBox.Content = _scoresListView;

            return groupBox;
        }

        private void CreateScoresListView()
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
            headerStartEndTime.Content = "Time";
            headerStartEndTime.Tag = "Time";
            columnStartEndTime.Header = headerStartEndTime;
            columnStartEndTime.Width = 40;
            columnStartEndTime.DisplayMemberBinding = new Binding("Time");

            var columnScore1 = new GridViewColumn();
            var headerScore1 = new GridViewColumnHeader();
            headerScore1.Content = "Score 1";
            headerScore1.Tag = "Score1";
            columnScore1.Header = headerScore1;
            columnScore1.Width = 75;
            columnScore1.DisplayMemberBinding = new Binding("Score1");

            var columnScore2 = new GridViewColumn();
            var headerScore2 = new GridViewColumnHeader();
            headerScore2.Content = "Score 2";
            headerScore2.Tag = "Score2";
            columnScore2.Header = headerScore2;
            columnScore2.Width = 75;
            columnScore2.DisplayMemberBinding = new Binding("Score2");

            var columnPlayer1 = new GridViewColumn();
            var headerPlayer1 = new GridViewColumnHeader();
            headerPlayer1.Content = "Player 1";
            headerPlayer1.Tag = "Player1";
            columnPlayer1.Header = headerPlayer1;
            columnPlayer1.Width = 175;
            columnPlayer1.DisplayMemberBinding = new Binding("Name1");

            var columnPlayer2 = new GridViewColumn();
            var headerPlayer2 = new GridViewColumnHeader();
            headerPlayer2.Content = "Player 2";
            headerPlayer2.Tag = "Player2";
            columnPlayer2.Header = headerPlayer2;
            columnPlayer2.Width = 175;
            columnPlayer2.DisplayMemberBinding = new Binding("Name2");

            var scoresGridView = new GridView();
            scoresGridView.AllowsColumnReorder = false;
            scoresGridView.Columns.Add(columnGS);
            scoresGridView.Columns.Add(columnStartEndTime);
            scoresGridView.Columns.Add(columnPlayer1);
            scoresGridView.Columns.Add(columnScore1);
            scoresGridView.Columns.Add(columnScore2);
            scoresGridView.Columns.Add(columnPlayer2);

            var scoresListView = new DemoInfoListView();
            _scoresListView = scoresListView;
            scoresListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            scoresListView.VerticalAlignment = VerticalAlignment.Stretch;
            scoresListView.Margin = new Thickness(5);
            scoresListView.View = scoresGridView;
            scoresListView.SelectionMode = SelectionMode.Extended;
        }

        private void CreateTeamScoresListView()
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
            headerStartEndTime.Content = "Time";
            headerStartEndTime.Tag = "Time";
            columnStartEndTime.Header = headerStartEndTime;
            columnStartEndTime.Width = 40;
            columnStartEndTime.DisplayMemberBinding = new Binding("Time");

            var columnScore1 = new GridViewColumn();
            var headerScore1 = new GridViewColumnHeader();
            headerScore1.Content = "RED Score";
            headerScore1.Tag = "Score1";
            columnScore1.Header = headerScore1;
            columnScore1.Width = 75;
            columnScore1.DisplayMemberBinding = new Binding("RedScore");

            var columnScore2 = new GridViewColumn();
            var headerScore2 = new GridViewColumnHeader();
            headerScore2.Content = "BLUE Score";
            headerScore2.Tag = "Score2";
            columnScore2.Header = headerScore2;
            columnScore2.Width = 75;
            columnScore2.DisplayMemberBinding = new Binding("BlueScore");

            var scoresGridView = new GridView();
            scoresGridView.AllowsColumnReorder = false;
            scoresGridView.Columns.Add(columnGS);
            scoresGridView.Columns.Add(columnStartEndTime);
            scoresGridView.Columns.Add(columnScore1);
            scoresGridView.Columns.Add(columnScore2);

            var scoresListView = new DemoInfoListView();
            _teamScoresListView = scoresListView;
            scoresListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            scoresListView.VerticalAlignment = VerticalAlignment.Stretch;
            scoresListView.Margin = new Thickness(5);
            scoresListView.View = scoresGridView;
            scoresListView.SelectionMode = SelectionMode.Extended;
        }

        private void OnCutByTimeClicked()
        {
            _app.OnCutByTimeContextClicked(_groupBox.Content as ListView);
        }
    }
}