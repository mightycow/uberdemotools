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
    public class ChatEventDisplayInfo : CuttabbleByTimeDisplayInfo
    {
        public ChatEventDisplayInfo(int gsIndex, string time, string player, string message, bool teamMessage)
        {
            GameStateIndex = gsIndex;
            Time = time;
            Player = player;
            Message = message;
            _teamMessage = teamMessage;
        }

        public override string ToString()
        {
            return string.Format("[{0}] ({1}) <{2}> {3}", Time, TeamMessage, Player, Message);
        }

        public string Player { get; set; }
        public string Message { get; set; }
        public string TeamMessage { get { return _teamMessage ? "team" : "global"; } }

        private bool _teamMessage;
    }

    public class ChatEventsComponent : AppComponent
    {
        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return new List<DemoInfoListView> { _chatEventsListView }; } }
        public List<DemoInfoListView> InfoListViews { get { return new List<DemoInfoListView> { _chatEventsListView }; } }
        public ComponentType Type { get { return ComponentType.ChatEvents; } }
        public bool MultiDemoMode { get { return false; } }

        public ChatEventsComponent(App app)
        {
            _app = app;
            RootControl = CreateDemoChatTab();
        }

        public void PopulateViews(DemoInfo demoInfo)
        {
            if(!demoInfo.Analyzed)
            {
                _chatEventsListView.ItemsSource = null;
                _chatEventsListView.SelectedIndex = -1;
                return;
            }

            var cutByTimeItem = new MenuItem();
            cutByTimeItem.Header = "Cut by Time (Ctrl+T)";
            cutByTimeItem.Command = _cutByTimeCommand;
            cutByTimeItem.Click += (obj, args) => OnCutByTimeFromChatContextClicked();

            var copyItem = new MenuItem();
            copyItem.Header = "Copy to Clipboard (Ctrl+C)";
            copyItem.Command = _copyChatCommand;
            copyItem.Click += (obj, args) => OnCopyFromChatContextClicked();

            var eventsContextMenu = new ContextMenu();
            eventsContextMenu.Items.Add(cutByTimeItem);
            eventsContextMenu.Items.Add(copyItem);

            var chatEvents = new ObservableCollection<ListViewItem>();
            foreach(var chatEvent in demoInfo.ChatEvents)
            {
                var item = new ListViewItem();
                item.Content = chatEvent;
                item.ContextMenu = eventsContextMenu;
                chatEvents.Add(item);
            }

            _chatEventsListView.ItemsSource = chatEvents;
            if(chatEvents.Count > 0)
            {
                _chatEventsListView.SelectedIndex = 0;
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
        private static RoutedCommand _cutByTimeCommand = new RoutedCommand();
        private static RoutedCommand _copyChatCommand = new RoutedCommand();
        private DemoInfoListView _chatEventsListView = null;
        private bool _chatEventsAscending = true;
        private string _chatEventsLastProperty = "";

        private class ChatEventSorter : IComparer
        {
            private string _propertyName;

            public bool Ascending = true;
            public string PropertyName
            {
                get { return _propertyName; }
                set { if(value == _propertyName) Ascending = !Ascending; _propertyName = value; }
            }

            public ChatEventSorter(string propertyName, bool ascending = true)
            {
                PropertyName = propertyName;
                Ascending = ascending;
            }

            public int Compare(object a, object b)
            {
                try
                {
                    var a1 = a as ListViewItem;
                    var b1 = b as ListViewItem;
                    var a2 = a1.Content as ChatEventDisplayInfo;
                    var b2 = b1.Content as ChatEventDisplayInfo;
                    var x = Ascending ? a2 : b2;
                    var y = Ascending ? b2 : a2;

                    switch(_propertyName)
                    {
                        case "GameStateIndex":
                            return x.GameStateIndex.CompareTo(y.GameStateIndex);

                        case "Time":
                            return App.CompareTimeStrings(y.Time, x.Time);

                        case "TeamMessage":
                            return x.TeamMessage.CompareTo(y.TeamMessage);

                        case "Player":
                            return y.Player.CompareTo(x.Player);

                        case "Message":
                            return y.Message.CompareTo(x.Message);

                        default:
                            return 0;
                    }
                }
                catch(Exception)
                {
                    return 0;
                }
            }
        }

        private void OnChatEventColumnClicked(GridViewColumnHeader column)
        {
            if(column == null || _chatEventsListView.Items.Count == 0)
            {
                return;
            }

            var propertyName = column.Tag as string;
            if(propertyName == null)
            {
                return;
            }

            if(propertyName == _chatEventsLastProperty)
            {
                _chatEventsAscending = !_chatEventsAscending;
            }
            _chatEventsLastProperty = propertyName;

            var view = CollectionViewSource.GetDefaultView(_chatEventsListView.ItemsSource) as ListCollectionView;
            view.CustomSort = new ChatEventSorter(propertyName, _chatEventsAscending);
            _chatEventsListView.Items.Refresh();
        }

        private FrameworkElement CreateDemoChatTab()
        {
            var columnGS = new GridViewColumn();
            var headerGS = new GridViewColumnHeader();
            headerGS.ToolTip = "GameState Index";
            headerGS.Content = "GS";
            headerGS.Tag = "GameStateIndex";
            headerGS.Click += (obj, args) => OnChatEventColumnClicked(obj as GridViewColumnHeader);
            columnGS.Header = headerGS;
            columnGS.Width = 35;
            columnGS.DisplayMemberBinding = new Binding("GameStateIndex");

            var columnTime = new GridViewColumn();
            var headerTime = new GridViewColumnHeader();
            headerTime.Content = "Time";
            headerTime.Tag = "Time";
            headerTime.Click += (obj, args) => OnChatEventColumnClicked(obj as GridViewColumnHeader);
            columnTime.Header = headerTime;
            columnTime.Width = 75;
            columnTime.DisplayMemberBinding = new Binding("Time");

            var columnType = new GridViewColumn();
            var headerType = new GridViewColumnHeader();
            headerType.Content = "Scope";
            headerType.Tag = "TeamMessage";
            headerType.Click += (obj, args) => OnChatEventColumnClicked(obj as GridViewColumnHeader);
            columnType.Header = headerType;
            columnType.Width = 50;
            columnType.DisplayMemberBinding = new Binding("TeamMessage");

            var columnName = new GridViewColumn();
            var headerName = new GridViewColumnHeader();
            headerName.Content = "Player Name";
            headerName.Tag = "Player";
            headerName.Click += (obj, args) => OnChatEventColumnClicked(obj as GridViewColumnHeader);
            columnName.Header = headerName;
            columnName.Width = 120;
            columnName.DisplayMemberBinding = new Binding("Player");

            var columnMessage = new GridViewColumn();
            var headerMessage = new GridViewColumnHeader();
            headerMessage.Content = "Message";
            headerMessage.Tag = "Message";
            headerMessage.Click += (obj, args) => OnChatEventColumnClicked(obj as GridViewColumnHeader);
            columnMessage.Header = headerMessage;
            columnMessage.Width = 300;
            columnMessage.DisplayMemberBinding = new Binding("Message");

            var demoEventsGridView = new GridView();
            demoEventsGridView.AllowsColumnReorder = false;
            demoEventsGridView.Columns.Add(columnGS);
            demoEventsGridView.Columns.Add(columnTime);
            demoEventsGridView.Columns.Add(columnType);
            demoEventsGridView.Columns.Add(columnName);
            demoEventsGridView.Columns.Add(columnMessage);

            var eventsListView = new DemoInfoListView();
            _chatEventsListView = eventsListView;
            eventsListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            eventsListView.VerticalAlignment = VerticalAlignment.Stretch;
            eventsListView.Margin = new Thickness(5);
            eventsListView.View = demoEventsGridView;
            eventsListView.SelectionMode = SelectionMode.Extended;
            InitChatEventsListViewCutBinding();

            var infoPanelGroupBox = new GroupBox();
            infoPanelGroupBox.Header = "Chat Messages";
            infoPanelGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            infoPanelGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            infoPanelGroupBox.Margin = new Thickness(5);
            infoPanelGroupBox.Content = eventsListView;

            return infoPanelGroupBox;
        }

        private void InitChatEventsListViewCutBinding()
        {
            App.AddKeyBinding(_chatEventsListView, Key.T, _cutByTimeCommand, (obj, args) => OnCutByTimeFromChatContextClicked());
            App.AddKeyBinding(_chatEventsListView, Key.C, _copyChatCommand, (obj, args) => OnCopyFromChatContextClicked());
        }

        private void OnCutByTimeFromChatContextClicked()
        {
            _app.OnCutByTimeContextClicked(_chatEventsListView);
        }

        private void OnCopyFromChatContextClicked()
        {
            App.CopyListViewRowsToClipboard(_chatEventsListView);
        }
    }
}