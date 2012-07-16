using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;


namespace Uber.DemoTools
{
    public partial class App
    {
        private class WeaponStatsDisplayInfo
        {
            public string Weapon { get; set; }
            public string Acc { get; set; }
            public string Hits { get; set; }
            public string Atts { get; set; }
            public string Kills { get; set; }
            public string Deaths { get; set; }
            public string Take { get; set; }
            public string Drop { get; set; }
        }

        private class WeaponStatsColumnInfo
        {
            public WeaponStatsColumnInfo(string name, int width)
            {
                Name = name;
                Width = width;
            }

            public string Name;
            public int Width;
        }

        private static WeaponStatsColumnInfo[] _weaponStatsColumns = new WeaponStatsColumnInfo[]
        {
            new WeaponStatsColumnInfo("Weapon", 165),
            new WeaponStatsColumnInfo("Acc", 60),
            new WeaponStatsColumnInfo("Hits", 60),
            new WeaponStatsColumnInfo("Atts", 60),
            new WeaponStatsColumnInfo("Kills", 60),
            new WeaponStatsColumnInfo("Deaths", 60),
            new WeaponStatsColumnInfo("Take", 60),
            new WeaponStatsColumnInfo("Drop", 60)
        };

        /*
        xstats2 weapon bits
        ga -   2
        mg -   4
        sg -   8
        gl -  16
        rl -  32
        lg -  64
        rg - 128
        pg - 256
        bf - 512
        */

        [Flags]
        private enum WeaponStatsFlags
        {
            Gauntlet = 1 << 1, // OK
            MachingeGun = 1 << 2, // OK
            Shotgun = 1 << 3, // OK
            GrenadeLauncher = 1 << 4, // OK
            RocketLauncher = 1 << 5, // OK
            LightningGun = 1 << 6, // OK
            Railgun = 1 << 7, // OK
            PlasmaGun = 1 << 8, // OK
            BFG = 1 << 9 // OK
        }

        private class WeaponStatsSearchInfo
        {
            public WeaponStatsSearchInfo(string weapon, WeaponStatsFlags flagMask)
            {
                WeaponName = weapon;
                FlagMask = (int)flagMask;
            }

            public string WeaponName;
            public int FlagMask;
        }

        private static WeaponStatsSearchInfo[] WeaponStatsInfos = new WeaponStatsSearchInfo[]
        {
            new WeaponStatsSearchInfo("Gauntlet", WeaponStatsFlags.Gauntlet),
            new WeaponStatsSearchInfo("MachineGun", WeaponStatsFlags.MachingeGun),
            new WeaponStatsSearchInfo("Shotgun", WeaponStatsFlags.Shotgun),
            new WeaponStatsSearchInfo("G.Launcher", WeaponStatsFlags.GrenadeLauncher),
            new WeaponStatsSearchInfo("R.Launcher", WeaponStatsFlags.RocketLauncher),
            new WeaponStatsSearchInfo("LightningGun", WeaponStatsFlags.LightningGun),
            new WeaponStatsSearchInfo("Railgun", WeaponStatsFlags.Railgun),
            new WeaponStatsSearchInfo("Plasma", WeaponStatsFlags.PlasmaGun),
            new WeaponStatsSearchInfo("BFG", WeaponStatsFlags.BFG)
        };

        private static string[] xstats2EndStatsInfos = new string[]
        {
            "Damage Given",
            "Damage Received",
            "Armor Taken",
            "Health Taken",
            "MHs taken",
            "RAs taken",
            "YAs taken",
            "GAs taken"
        };

        private class StatDisplayInfo
        {
            public StatDisplayInfo(string name, string value)
            {
                Name = name;
                Value = value;
            }

            public string Name { get; set; }
            public string Value { get; set; }
        }

        private class Xstats2WidgetInfo
        {
            public GroupBox WeaponStatsGroupBox = null;
            public ListView WeaponStatsListView = null;
            public GroupBox DamageStatsGroupBox = null;
            public ListView DamageStatsListView = null;
        }

        private List<Xstats2WidgetInfo> _xstats2WidgetInfos = new List<Xstats2WidgetInfo>();
        private FrameworkElement _xstats2SorryNothingFound = null;
        private WrapPanel _statsRootPanel = null;

        private const int MaxXstats2Displayed = 4;

        private FrameworkElement CreateStatsTab()
        {
            var xstats2SorryNothingFound = new Label();
            _xstats2SorryNothingFound = xstats2SorryNothingFound;
            xstats2SorryNothingFound.HorizontalAlignment = HorizontalAlignment.Center;
            xstats2SorryNothingFound.VerticalAlignment = VerticalAlignment.Center;
            xstats2SorryNothingFound.Margin = new Thickness(10);
            xstats2SorryNothingFound.Content = "Didn't find any readable end-game stats server command in this demo (e.g. xstats2).";
            xstats2SorryNothingFound.Visibility = Visibility.Collapsed;

            for(int i = 0; i < MaxXstats2Displayed; ++i)
            {
                var weaponStatsWidth = 10;
                var weaponStatsGridView = new GridView();
                weaponStatsGridView.AllowsColumnReorder = false;
                foreach(var columnInfo in _weaponStatsColumns)
                {
                    weaponStatsGridView.Columns.Add(new GridViewColumn { Header = columnInfo.Name, Width = columnInfo.Width, DisplayMemberBinding = new Binding(columnInfo.Name) });
                    weaponStatsWidth += columnInfo.Width;
                }

                var weaponStatsListView = new ListView();
                weaponStatsListView.HorizontalAlignment = HorizontalAlignment.Stretch;
                weaponStatsListView.VerticalAlignment = VerticalAlignment.Stretch;
                weaponStatsListView.Margin = new Thickness(5);
                weaponStatsListView.Width = weaponStatsWidth;
                weaponStatsListView.View = weaponStatsGridView;
                weaponStatsListView.SelectionMode = SelectionMode.Single;

                var weaponStatsGroupBox = new GroupBox();
                weaponStatsGroupBox.Header = "Weapon Stats for N/A";
                weaponStatsGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
                weaponStatsGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
                weaponStatsGroupBox.Margin = new Thickness(5);
                weaponStatsGroupBox.Content = weaponStatsListView;
                weaponStatsGroupBox.Visibility = Visibility.Collapsed;

                var damageStatsGridView = new GridView();
                damageStatsGridView.AllowsColumnReorder = false;
                damageStatsGridView.Columns.Add(new GridViewColumn { Header = "Name", Width = 200, DisplayMemberBinding = new Binding("Name") });
                damageStatsGridView.Columns.Add(new GridViewColumn { Header = "Value", Width = 100, DisplayMemberBinding = new Binding("Value") });

                var damageStatsListView = new ListView();
                damageStatsListView.HorizontalAlignment = HorizontalAlignment.Stretch;
                damageStatsListView.VerticalAlignment = VerticalAlignment.Stretch;
                damageStatsListView.Margin = new Thickness(5);
                damageStatsListView.Width = 310;
                damageStatsListView.View = damageStatsGridView;
                damageStatsListView.SelectionMode = SelectionMode.Single;

                var damageStatsGroupBox = new GroupBox();
                damageStatsGroupBox.Header = "Additional Stats for N/A";
                damageStatsGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
                damageStatsGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
                damageStatsGroupBox.Margin = new Thickness(5);
                damageStatsGroupBox.Content = damageStatsListView;
                damageStatsGroupBox.Visibility = Visibility.Collapsed;

                var info = new Xstats2WidgetInfo();
                info.DamageStatsGroupBox = damageStatsGroupBox;
                info.DamageStatsListView = damageStatsListView;
                info.WeaponStatsGroupBox = weaponStatsGroupBox;
                info.WeaponStatsListView = weaponStatsListView;
                _xstats2WidgetInfos.Add(info);
            }

            var rootPanel = new WrapPanel();
            _statsRootPanel = rootPanel;
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.Margin = new Thickness(5);
            rootPanel.Orientation = Orientation.Horizontal;
            rootPanel.Children.Add(xstats2SorryNothingFound);
            foreach(var info in _xstats2WidgetInfos)
            {
                rootPanel.Children.Add(info.WeaponStatsGroupBox);
                rootPanel.Children.Add(info.DamageStatsGroupBox);
            }

            var scrollViewer = new ScrollViewer();
            scrollViewer.HorizontalAlignment = HorizontalAlignment.Stretch;
            scrollViewer.VerticalAlignment = VerticalAlignment.Stretch;
            scrollViewer.Margin = new Thickness(5);
            scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled;
            scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
            scrollViewer.Content = rootPanel;

            return scrollViewer;
        }

        private void PopulateStats(DemoInfo demoInfo)
        {
            for(int i = 0; i < MaxXstats2Displayed; ++i)
            {
                var info2 = _xstats2WidgetInfos[i];
                var visibility = Visibility.Collapsed;
                info2.WeaponStatsGroupBox.Visibility = visibility;
                info2.DamageStatsGroupBox.Visibility = visibility;
            }

            _xstats2SorryNothingFound.Visibility = demoInfo.Xstats2Infos.Count > 0 ? Visibility.Collapsed : Visibility.Visible;

            int statsCount = Math.Min(demoInfo.Xstats2Infos.Count, MaxXstats2Displayed);
            for(int i = 0; i < statsCount; ++i)
            {
                var info = demoInfo.Xstats2Infos[i];
                var info2 = _xstats2WidgetInfos[i];

                var visibility = info.WeaponStats.Count > 0 ? Visibility.Visible : Visibility.Collapsed;
                info2.WeaponStatsGroupBox.Visibility = visibility;
                info2.WeaponStatsGroupBox.Header = "Weapon Stats for Player " + info.WeaponStatsPlayerName;
                info2.WeaponStatsListView.Items.Clear();
                foreach(var weaponStat in info.WeaponStats)
                {
                    info2.WeaponStatsListView.Items.Add(weaponStat);
                }

                info2.DamageStatsGroupBox.Visibility = visibility;
                info2.DamageStatsGroupBox.Header = "Additional Stats for Player " + info.WeaponStatsPlayerName;
                info2.DamageStatsListView.Items.Clear();
                foreach(var extraStats in info.DamageAndArmorStats)
                {
                    info2.DamageStatsListView.Items.Add(extraStats);
                }
            }
        }

        private static string ComputeAccuracy(string hits, string shots)
        {
            int h = 0;
            if(!int.TryParse(hits, out h))
            {
                return "";
            }

            int s = 0;
            if(!int.TryParse(shots, out s))
            {
                return "";
            }

            if(s == 0)
            {
                return "";
            }

            int x = (1000 * h) / s;
            int a = x / 10;
            int b = x % 10;

            return a.ToString() + "." + b.ToString();
        }

        private static string FormatWeaponStat(string number)
        {
            return number == "0" ? "" : number;
        }
    }
}