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
    public partial class App
    {
        private readonly string[] _puNames = new string[]
        {
            "Unknown",
            "Quad Damage",
            "Battle Suit",
            "Haste",
            "Invisibility",
            "Regeneration",
            "Flight",
            "Red Flag",
            "Blue Flag",
            "Neutral Flag",
            "Scout",
            "Guard",
            "Doubler",
            "Armor Regeneration",
            "Invulnerability"
        };

        private class PuRunDisplayInfo
        {
            public PuRunDisplayInfo(string time, string player, string pu, string duration, string kills, string teamKills, string selfKill)
            {
                Time = time;
                Player = player;
                Pu = pu;
                Duration = duration;
                Kills = kills;
                TeamKills = teamKills;
                SelfKill = selfKill;
            }

            public string Time { get; set; }
            public string Player { get; set; }
            public string Pu { get; set; }
            public string Duration { get; set; }
            public string Kills { get; set; }
            public string TeamKills { get; set; }
            public string SelfKill { get; set; }
        }

        private ListView _puRunsListView = null;

        private FrameworkElement CreateDemoPuRunTab()
        {
            var column0 = new GridViewColumn();
            var header0 = new GridViewColumnHeader();
            header0.Content = "Start";
            header0.Tag = "Time";
            column0.Header = header0;
            column0.Width = 40;
            column0.DisplayMemberBinding = new Binding("Time");

            var column1 = new GridViewColumn();
            var header1 = new GridViewColumnHeader();
            header1.Content = "Player";
            header1.Tag = "Player";
            column1.Header = header1;
            column1.Width = 140;
            column1.DisplayMemberBinding = new Binding("Player");

            var column2 = new GridViewColumn();
            var header2 = new GridViewColumnHeader();
            header2.Content = "Power-up";
            header2.Tag = "Pu";
            column2.Header = header2;
            column2.Width = 140;
            column2.DisplayMemberBinding = new Binding("Pu");

            var column3 = new GridViewColumn();
            var header3 = new GridViewColumnHeader();
            header3.Content = "Duration";
            header3.Tag = "Duration";
            column3.Header = header3;
            column3.Width = 75;
            column3.DisplayMemberBinding = new Binding("Duration");

            var column4 = new GridViewColumn();
            var header4 = new GridViewColumnHeader();
            header4.Content = "Kills";
            header4.Tag = "Kills";
            column4.Header = header4;
            column4.Width = 60;
            column4.DisplayMemberBinding = new Binding("Kills");

            var column5 = new GridViewColumn();
            var header5 = new GridViewColumnHeader();
            header5.Content = "Team Kills";
            header5.Tag = "TeamKills";
            column5.Header = header5;
            column5.Width = 75;
            column5.DisplayMemberBinding = new Binding("TeamKills");

            var column6 = new GridViewColumn();
            var header6 = new GridViewColumnHeader();
            header6.Content = "Self Kill?";
            header6.Tag = "SelfKill";
            column6.Header = header6;
            column6.Width = 75;
            column6.DisplayMemberBinding = new Binding("SelfKill");

            var demoEventsGridView = new GridView();
            demoEventsGridView.AllowsColumnReorder = false;
            demoEventsGridView.Columns.Add(column0);
            demoEventsGridView.Columns.Add(column1);
            demoEventsGridView.Columns.Add(column2);
            demoEventsGridView.Columns.Add(column3);
            demoEventsGridView.Columns.Add(column4);
            demoEventsGridView.Columns.Add(column5);
            demoEventsGridView.Columns.Add(column6);

            var eventsListView = new ListView();
            _puRunsListView = eventsListView;
            eventsListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            eventsListView.VerticalAlignment = VerticalAlignment.Stretch;
            eventsListView.Margin = new Thickness(5);
            eventsListView.View = demoEventsGridView;
            eventsListView.SelectionMode = SelectionMode.Single;
            eventsListView.Foreground = new SolidColorBrush(Colors.Black);
            InitPuRunsListViewCutBinding();

            var infoPanelGroupBox = new GroupBox();
            infoPanelGroupBox.Header = "Obituary Events";
            infoPanelGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            infoPanelGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            infoPanelGroupBox.Margin = new Thickness(5);
            infoPanelGroupBox.Content = eventsListView;

            return infoPanelGroupBox;
        }

        private void InitPuRunsListViewCutBinding()
        {
            var inputGesture = new KeyGesture(Key.T, ModifierKeys.Control);
            var inputBinding = new KeyBinding(_cutByPuRunCommand, inputGesture);

            var commandBinding = new CommandBinding();
            commandBinding.Command = _cutByPuRunCommand;
            commandBinding.Executed += (obj, args) => OnCutByTimeFromPuRunContextClicked();
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = true; };

            _puRunsListView.InputBindings.Add(inputBinding);
            _puRunsListView.CommandBindings.Add(commandBinding);
        }

        private void PopulatePuRunsListView(DemoInfo demoInfo)
        {
            var cutByTimeItem = new MenuItem();
            cutByTimeItem.Header = "Cut by Time (Ctrl+T)";
            cutByTimeItem.Command = _cutByPuRunCommand;
            cutByTimeItem.Click += (obj, args) => OnCutByTimeFromPuRunContextClicked();

            var eventsContextMenu = new ContextMenu();
            eventsContextMenu.Items.Add(cutByTimeItem);

            _puRunsListView.Items.Clear();
            foreach(var puRun in demoInfo.PuRuns)
            {
                var item = new ListViewItem();
                item.Content = puRun;
                item.ContextMenu = eventsContextMenu;
                _puRunsListView.Items.Add(item);
            }

            if(demoInfo.PuRuns.Count > 0)
            {
                _puRunsListView.SelectedIndex = 0;
            }
        }

        private void OnCutByTimeFromPuRunContextClicked()
        {
            int startOffset = _config.ChatCutStartOffset;
            int endOffset = _config.ChatCutEndOffset;
            if(!_config.SkipChatOffsetsDialog)
            {
                var dialog = new TimeOffsetsDialog(_window, _config.ChatCutStartOffset, _config.ChatCutEndOffset);
                if(!dialog.Valid)
                {
                    return;
                }

                startOffset = dialog.StartOffset;
                endOffset = dialog.EndOffset;
            }

            var item = _puRunsListView.SelectedItem as ListViewItem;
            if(item == null)
            {
                return;
            }

            var info = item.Content as PuRunDisplayInfo;
            if(info == null)
            {
                return;
            }

            int time = 0;
            if(!ParseMinutesSeconds(info.Time, out time))
            {
                return;
            }

            int duration = 0;
            if(!int.TryParse(info.Duration.Trim(new char[] { 's' }), out duration))
            {
                return;
            }

            var startTime = time - startOffset;
            var endTime = time + duration + endOffset;

            _startTimeEditBox.Text = FormatMinutesSeconds(startTime);
            _endTimeEditBox.Text = FormatMinutesSeconds(endTime);

            _tabControl.SelectedIndex = 3;
        }
    }
}