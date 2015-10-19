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
    public class FragEventDisplayInfo : CuttabbleByTimeDisplayInfo
    {
        public FragEventDisplayInfo(int gsIndex, string time, string attacker, string target, string mod)
        {
            GameStateIndex = gsIndex;
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

    public class FragEventsComponent : AppComponent
    {
        private static RoutedCommand _cutByFragCommand = new RoutedCommand();
        private static RoutedCommand _copyFragCommand = new RoutedCommand();
        private DemoInfoListView _fragEventsListView = null;
        private App _app = null;

        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return new List<DemoInfoListView> { _fragEventsListView }; } }
        public List<DemoInfoListView> InfoListViews { get { return new List<DemoInfoListView> { _fragEventsListView }; } }
        public ComponentType Type { get { return ComponentType.FragEvents; } }
        public bool MultiDemoMode { get { return false; } }

        public FragEventsComponent(App app)
        {
            _app = app;
            RootControl = CreateDemoFragTab();
        }

        public void PopulateViews(DemoInfo demoInfo)
        {
            if(!demoInfo.Analyzed)
            {
                _fragEventsListView.Items.Clear();
                return;
            }

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

        public void SaveToConfigObject(UdtConfig config)
        {
            // Nothing to do.
        }

        public void SaveToConfigObject(UdtPrivateConfig config)
        {
            // Nothing to do.
        }

        private FrameworkElement CreateDemoFragTab()
        {
            var columnGS = new GridViewColumn();
            var headerGS = new GridViewColumnHeader();
            headerGS.ToolTip = "GameState Index";
            headerGS.Content = "GS";
            headerGS.Tag = "GameStateIndex";
            columnGS.Header = headerGS;
            columnGS.Width = 35;
            columnGS.DisplayMemberBinding = new Binding("GameStateIndex");

            var columnTime = new GridViewColumn();
            var headerTime = new GridViewColumnHeader();
            headerTime.Content = "Time";
            headerTime.Tag = "Time";
            columnTime.Header = headerTime;
            columnTime.Width = 75;
            columnTime.DisplayMemberBinding = new Binding("Time");

            var columnAtt = new GridViewColumn();
            var headerAtt = new GridViewColumnHeader();
            headerAtt.Content = "Attacker";
            headerAtt.Tag = "Attacker";
            columnAtt.Header = headerAtt;
            columnAtt.Width = 155;
            columnAtt.DisplayMemberBinding = new Binding("Attacker");

            var columnTarget = new GridViewColumn();
            var headerTarget = new GridViewColumnHeader();
            headerTarget.Content = "Target";
            headerTarget.Tag = "Target";
            columnTarget.Header = headerTarget;
            columnTarget.Width = 155;
            columnTarget.DisplayMemberBinding = new Binding("Target");

            var columnMOD = new GridViewColumn();
            var headerMOD = new GridViewColumnHeader();
            headerMOD.Content = "Cause of Death"; // Mean of death in id terminology.
            headerMOD.Tag = "Mod";
            columnMOD.Header = headerMOD;
            columnMOD.Width = 155;
            columnMOD.DisplayMemberBinding = new Binding("Mod");

            var demoEventsGridView = new GridView();
            demoEventsGridView.AllowsColumnReorder = false;
            demoEventsGridView.Columns.Add(columnGS);
            demoEventsGridView.Columns.Add(columnTime);
            demoEventsGridView.Columns.Add(columnAtt);
            demoEventsGridView.Columns.Add(columnTarget);
            demoEventsGridView.Columns.Add(columnMOD);

            var eventsListView = new DemoInfoListView();
            _fragEventsListView = eventsListView;
            eventsListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            eventsListView.VerticalAlignment = VerticalAlignment.Stretch;
            eventsListView.Margin = new Thickness(5);
            eventsListView.View = demoEventsGridView;
            eventsListView.SelectionMode = SelectionMode.Extended;
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
            App.AddKeyBinding(_fragEventsListView, Key.T, _cutByFragCommand, (obj, args) => OnCutByTimeFromFragsContextClicked());
            App.AddKeyBinding(_fragEventsListView, Key.C, _copyFragCommand, (obj, args) => OnCopyFromFragsContextClicked());
        }

        private void OnCutByTimeFromFragsContextClicked()
        {
            _app.OnCutByTimeContextClicked(_fragEventsListView);
        }

        private void OnCopyFromFragsContextClicked()
        {
            App.CopyListViewRowsToClipboard(_fragEventsListView);
        }
    }
}