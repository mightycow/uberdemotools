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
        private class ChatEventDisplayInfo : TimedEventDisplayInfo
        {
            public ChatEventDisplayInfo(string time, string player, string message)
            {
                Time = time;
                Player = player;
                Message = message;
            }

            public override string ToString()
            {
                return string.Format("[{0}] <{1}> {2}", Time, Player, Message);
            }

            public string Player { get; set; }
            public string Message { get; set; }
        }

        private ListView _chatEventsListView = null;
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
                        case "Time":
                            return App.CompareTimeStrings(y.Time, x.Time);

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
            if(column == null)
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
            var column0 = new GridViewColumn();
            var header0 = new GridViewColumnHeader();
            header0.Content = "Time";
            header0.Tag = "Time";
            header0.Click += (obj, args) => OnChatEventColumnClicked(obj as GridViewColumnHeader);
            column0.Header = header0;
            column0.Width = 75;
            column0.DisplayMemberBinding = new Binding("Time");

            var column1 = new GridViewColumn();
            var header1 = new GridViewColumnHeader();
            header1.Content = "Player Name";
            header1.Tag = "Player";
            header1.Click += (obj, args) => OnChatEventColumnClicked(obj as GridViewColumnHeader);
            column1.Header = header1;
            column1.Width = 175;
            column1.DisplayMemberBinding = new Binding("Player");

            var column2 = new GridViewColumn();
            var header2 = new GridViewColumnHeader();
            header2.Content = "Message";
            header2.Tag = "Message";
            header2.Click += (obj, args) => OnChatEventColumnClicked(obj as GridViewColumnHeader);
            column2.Header = header2;
            column2.Width = 340;
            column2.DisplayMemberBinding = new Binding("Message");

            var demoEventsGridView = new GridView();
            demoEventsGridView.AllowsColumnReorder = false;
            demoEventsGridView.Columns.Add(column0);
            demoEventsGridView.Columns.Add(column1);
            demoEventsGridView.Columns.Add(column2);

            var eventsListView = new ListView();
            _chatEventsListView = eventsListView;
            eventsListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            eventsListView.VerticalAlignment = VerticalAlignment.Stretch;
            eventsListView.Margin = new Thickness(5);
            eventsListView.View = demoEventsGridView;
            eventsListView.SelectionMode = SelectionMode.Extended;
            InitChatEventsListViewCutBinding();

            var infoPanelGroupBox = new GroupBox();
            infoPanelGroupBox.Header = "Demo Chat Events";
            infoPanelGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            infoPanelGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            infoPanelGroupBox.Margin = new Thickness(5);
            infoPanelGroupBox.Content = eventsListView;

            return infoPanelGroupBox;
        }

        private static void AddKeyBinding(UIElement element, Key key, RoutedCommand command, ExecutedRoutedEventHandler callback)
        {
            var inputGesture = new KeyGesture(key, ModifierKeys.Control);
            var inputBinding = new KeyBinding(command, inputGesture);

            var commandBinding = new CommandBinding();
            commandBinding.Command = command;
            commandBinding.Executed += callback;
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = true; };

            element.InputBindings.Add(inputBinding);
            element.CommandBindings.Add(commandBinding);
        }

        private void InitChatEventsListViewCutBinding()
        {
            AddKeyBinding(_chatEventsListView, Key.T, _cutByChatCommand, (obj, args) => OnCutByTimeFromChatContextClicked());
            AddKeyBinding(_chatEventsListView, Key.C, _copyChatCommand, (obj, args) => OnCopyFromChatContextClicked());
        }

        private void PopulateChatEventsListView(DemoInfo demoInfo)
        {
            var cutByTimeItem = new MenuItem();
            cutByTimeItem.Header = "Cut by Time (Ctrl+T)";
            cutByTimeItem.Command = _cutByChatCommand;
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

        private void OnCutByTimeFromChatContextClicked()
        {
            OnCutByTimeContextClicked(_chatEventsListView);
        }

        private void OnCopyFromChatContextClicked()
        {
            CopyListViewRowsToClipboard(_chatEventsListView);
        }
    }
}