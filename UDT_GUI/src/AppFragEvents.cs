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
        private readonly string[] _meansOfDeath = new string[]
        {
            "Unknown",
            "Shotgun",
            "Gauntlet",
            "Machine Gun",
            "Grenade Launcher",
            "Grenade Splash",
            "Rocket Launcher",
            "Rocket Splash",
            "Plasma Gun",
            "Plasma Splash",
            "Railgun",
            "Lightning Gun",
            "BFG",
            "BFG Splash",
            "Water",
            "Slime",
            "Lava",
            "Crush",
            "Telefrag",
            "Fall",
            "Suicide",
            "Laser",
            "Trigger",
            "Grapple"
        };

        private class FragEventDisplayInfo : TimedEventDisplayInfo
        {
            public FragEventDisplayInfo(string time, string attacker, string target, string mod)
            {
                Time = time;
                Attacker = attacker;
                Target = target;
                Mod = mod;
            }

            public override string ToString()
            {
                return string.Format("[{0}] <{1}> {2} <{3}>", Time, Attacker, Mod, Target);
            }

            public string Attacker { get; set; }
            public string Target { get; set; }
            public string Mod { get; set; }
        }

        private ListView _fragEventsListView = null;

        private FrameworkElement CreateDemoFragTab()
        {
            var column0 = new GridViewColumn();
            var header0 = new GridViewColumnHeader();
            header0.Content = "Time";
            header0.Tag = "Time";
            column0.Header = header0;
            column0.Width = 75;
            column0.DisplayMemberBinding = new Binding("Time");

            var column1 = new GridViewColumn();
            var header1 = new GridViewColumnHeader();
            header1.Content = "Attacker";
            header1.Tag = "Attacker";
            column1.Header = header1;
            column1.Width = 175;
            column1.DisplayMemberBinding = new Binding("Attacker");

            var column2 = new GridViewColumn();
            var header2 = new GridViewColumnHeader();
            header2.Content = "Target";
            header2.Tag = "Target";
            column2.Header = header2;
            column2.Width = 175;
            column2.DisplayMemberBinding = new Binding("Target");

            var column3 = new GridViewColumn();
            var header3 = new GridViewColumnHeader();
            header3.Content = "Mean of Death";
            header3.Tag = "Mod";
            column3.Header = header3;
            column3.Width = 175;
            column3.DisplayMemberBinding = new Binding("Mod");

            var demoEventsGridView = new GridView();
            demoEventsGridView.AllowsColumnReorder = false;
            demoEventsGridView.Columns.Add(column0);
            demoEventsGridView.Columns.Add(column1);
            demoEventsGridView.Columns.Add(column2);
            demoEventsGridView.Columns.Add(column3);

            var eventsListView = new ListView();
            _fragEventsListView = eventsListView;
            eventsListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            eventsListView.VerticalAlignment = VerticalAlignment.Stretch;
            eventsListView.Margin = new Thickness(5);
            eventsListView.View = demoEventsGridView;
            eventsListView.SelectionMode = SelectionMode.Extended;
            eventsListView.Foreground = new SolidColorBrush(Colors.Black);
            InitFragEventsListViewCutBinding();

            var infoPanelGroupBox = new GroupBox();
            infoPanelGroupBox.Header = "Obituary Events";
            infoPanelGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            infoPanelGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            infoPanelGroupBox.Margin = new Thickness(5);
            infoPanelGroupBox.Content = eventsListView;

            return infoPanelGroupBox;
        }

        private void InitFragEventsListViewCutBinding()
        {
            AddKeyBinding(_fragEventsListView, Key.T, _cutByFragCommand, (obj, args) => OnCutByTimeFromFragsContextClicked());
            AddKeyBinding(_fragEventsListView, Key.C, _copyFragCommand, (obj, args) => OnCopyFromFragsContextClicked());
        }

        private void PopulateFragEventsListView(DemoInfo demoInfo)
        {
            var cutByTimeItem = new MenuItem();
            cutByTimeItem.Header = "Cut by Time (Ctrl+T)";
            cutByTimeItem.Command = _cutByFragCommand;
            cutByTimeItem.Click += (obj, args) => OnCutByTimeFromFragsContextClicked();

            var copyItem = new MenuItem();
            copyItem.Header = "Copy to Clipboard (Ctrl+C)";
            copyItem.Command = _copyFragCommand;
            copyItem.Click += (obj, args) => OnCopyFromFragsContextClicked();

            var eventsContextMenu = new ContextMenu();
            eventsContextMenu.Items.Add(cutByTimeItem);

            _fragEventsListView.Items.Clear();
            foreach(var fragEvent in demoInfo.FragEvents)
            {
                var item = new ListViewItem();
                item.Content = fragEvent;
                item.ContextMenu = eventsContextMenu;
                _fragEventsListView.Items.Add(item);
            }

            if(demoInfo.FragEvents.Count > 0)
            {
                _fragEventsListView.SelectedIndex = 0;
            }
        }

        private void OnCutByTimeFromFragsContextClicked()
        {
            OnCutByTimeContextClicked(_fragEventsListView);
        }

        private void OnCopyFromFragsContextClicked()
        {
            CopyListViewRowsToClipboard(_fragEventsListView);
        }
    }
}