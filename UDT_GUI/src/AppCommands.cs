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
    public class CommandDisplayInfo
    {
        public CommandDisplayInfo(int gameStateIndex, string gameState, string time, string command, string value)
        {
            GameStateIndex = gameStateIndex;
            GameState = gameState;
            Time = time;
            Command = command;
            Value = value;
        }

        public override string ToString()
        {
            return string.Format("[{0}] ({1}) <{2}> {3}", GameState, Time, Command, Value);
        }

        public int GameStateIndex { get; private set; }
        public string GameState { get; set; }
        public string Time { get; set; }
        public string Command { get; set; }
        public string Value { get; set; }
    }

    public class CommandsComponent : AppComponent
    {
        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return new List<DemoInfoListView> { _commandsListView }; } }
        public List<DemoInfoListView> InfoListViews { get { return new List<DemoInfoListView> { _commandsListView }; } }
        public ComponentType Type { get { return ComponentType.Commands; } }
        public bool MultiDemoMode { get { return false; } }

        public CommandsComponent(App app)
        {
            _app = app;
            RootControl = CreateTab();
        }

        public void PopulateViews(DemoInfo demoInfo)
        {
            if(!demoInfo.Analyzed)
            {
                _commandsListView.ItemsSource = null;
                _commandsListView.SelectedIndex = -1;
                return;
            }

            var commands = new ObservableCollection<CommandDisplayInfo>();
            foreach(var command in demoInfo.Commands)
            {
                commands.Add(command);
            }

            _commandsListView.ItemsSource = commands;
            if(commands.Count > 0)
            {
                _commandsListView.SelectedIndex = 0;
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
        private DemoInfoListView _commandsListView = null;

        private FrameworkElement CreateTab()
        {
            var columnGS = new GridViewColumn();
            var headerGS = new GridViewColumnHeader();
            headerGS.ToolTip = "GameState Index";
            headerGS.Content = "GS";
            headerGS.Tag = "GameState";
            columnGS.Header = headerGS;
            columnGS.Width = 25;
            columnGS.DisplayMemberBinding = new Binding("GameState");

            var columnTime = new GridViewColumn();
            var headerTime = new GridViewColumnHeader();
            headerTime.Content = "Time";
            headerTime.Tag = "Time";
            columnTime.Header = headerTime;
            columnTime.Width = 50;
            columnTime.DisplayMemberBinding = new Binding("Time");

            var columnCommand = new GridViewColumn();
            var headerCommand = new GridViewColumnHeader();
            headerCommand.Content = "Command";
            headerCommand.Tag = "Command";
            columnCommand.Header = headerCommand;
            columnCommand.Width = 75;
            columnCommand.DisplayMemberBinding = new Binding("Command");

            var columnValue = new GridViewColumn();
            var headerValue = new GridViewColumnHeader();
            headerValue.Content = "Value";
            headerValue.Tag = "Value";
            columnValue.Header = headerValue;
            columnValue.Width = 400;
            columnValue.DisplayMemberBinding = new Binding("Value");

            var commandsGridView = new GridView();
            commandsGridView.AllowsColumnReorder = false;
            commandsGridView.Columns.Add(columnGS);
            commandsGridView.Columns.Add(columnTime);
            commandsGridView.Columns.Add(columnCommand);
            commandsGridView.Columns.Add(columnValue);

            var commandsListView = new DemoInfoListView();
            _commandsListView = commandsListView;
            commandsListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            commandsListView.VerticalAlignment = VerticalAlignment.Stretch;
            commandsListView.Margin = new Thickness(5);
            commandsListView.View = commandsGridView;
            commandsListView.SelectionMode = SelectionMode.Extended;

            var infoPanelGroupBox = new GroupBox();
            infoPanelGroupBox.Header = "Config Strings and Commands";
            infoPanelGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            infoPanelGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            infoPanelGroupBox.Margin = new Thickness(5);
            infoPanelGroupBox.Content = commandsListView;

            return infoPanelGroupBox;
        }
    }
}