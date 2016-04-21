using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Media;


namespace Uber.DemoTools
{
    public class VdfNode
    {
        public VdfNode(VdfNode parent)
        {
            Parent = parent;
        }

        public VdfNode(VdfNode parent, string name)
        {
            Parent = parent;
            Name = name;
        }

        public VdfNode FindChild(string name)
        {
            return Children.Find(c => c.Name == name);
        }

        public VdfNode FindChild(params string[] names)
        {
            var node = this;
            foreach(var name in names)
            {
                node = node.FindChild(name);
                if(node == null)
                {
                    return null;
                }
            }

            return node;
        }

        public string GetUnescapedValue()
        {
            return Value.Replace(@"\\", @"\");
        }

        public string TryGetChildValue(string name, string errorValue)
        {
            var child = FindChild(name);
            if(child == null)
            {
                return errorValue;
            }

            return child.GetUnescapedValue();
        }

        public VdfNode Parent;
        public string Name = "";
        public string Value = "";
        public readonly List<VdfNode> Children = new List<VdfNode>();
        public bool IsValue = false;
    }

    public class VdfParser
    {
        public static VdfNode ParseVdfFile(string filePath)
        {
            return ParseVdfString(File.ReadAllText(filePath));
        }

        public static VdfNode ParseVdfString(string vdfString)
        {
            vdfString = CleanUpInput(vdfString);

            var rootNode = new VdfNode(null, "Root");
            var currentNode = new VdfNode(rootNode);
            bool readName = false;
            var i = 0;
            while(i < vdfString.Length)
            {
                var c = vdfString[i];
                if(c == '"')
                {
                    var endQuoteIdx = vdfString.IndexOf('"', i + 1);
                    var name = vdfString.Substring(i + 1, endQuoteIdx - i - 1);
                    if(readName)
                    {
                        currentNode.Value = name;
                        currentNode.IsValue = true;
                        currentNode = new VdfNode(currentNode.Parent);
                    }
                    else
                    {
                        currentNode.Name = name;
                        currentNode.Parent.Children.Add(currentNode);
                    }
                    readName = !readName;
                    i = endQuoteIdx + 1;
                }
                else if(c == '{')
                {
                    currentNode = new VdfNode(currentNode, currentNode.Name);
                    readName = false;
                    ++i;
                }
                else if(c == '}')
                {
                    if(currentNode.Parent == null)
                    {
                        break;
                    }
                    currentNode = new VdfNode(currentNode.Parent.Parent);
                    readName = false;
                    ++i;
                }
            }

            return rootNode;
        }

        private static string CleanUpInput(string input)
        {
            var inputLength = input.Length;
            var outputLength = 0;
            var output = new char[inputLength];
            for(int i = 0; i < inputLength; ++i)
            {
                char c = input[i];
                if(char.IsWhiteSpace(c))
                {
                    continue;
                }

                output[outputLength] = c;
                ++outputLength;
            }

            return new string(output, 0, outputLength);
        }
    }

    public class SteamUser
    {
        public string AccountName { get; set; }
        public string CurrentName { get; set; }
        public string SteamID { get; set; }
        public bool AutoLogIn { get; set; }
    }

    public class SteamDemoPath
    {
        public bool AccountSpecific = true;
        public string SteamID;
        public string Path;
    }

    public class SteamGame
    {
        public string Name;
        public string ShortName;
        public string Path;
        public readonly List<SteamDemoPath> DemoPaths = new List<SteamDemoPath>();
    }

    public class Steam
    {
        public readonly List<string> InstallPaths = new List<string>();
        public readonly List<SteamUser> Users = new List<SteamUser>();
        public readonly List<SteamGame> Games = new List<SteamGame>();

        public static Steam GetSteamInfo()
        {
            var steamRegKey = Registry.CurrentUser.OpenSubKey(@"Software\Valve\Steam");
            if(steamRegKey == null)
            {
                return null;
            }

            var steamBasePath = steamRegKey.GetValue("SteamPath", "").ToString();
            steamBasePath = Path.GetFullPath(steamBasePath);
            if(!Directory.Exists(steamBasePath))
            {
                return null;
            }

            var steam = new Steam();
            steam.InstallPaths.Add(steamBasePath);

            var config = VdfParser.ParseVdfFile(Path.Combine(steamBasePath, @"config\config.vdf"));
            var steamNode = config.FindChild("InstallConfigStore", "Software", "Valve", "Steam");
            if(steamNode != null)
            {
                for(var i = 1; ; ++i)
                {
                    var installNode = steamNode.FindChild("BaseInstallFolder_" + i.ToString());
                    if(installNode == null)
                    {
                        break;
                    }

                    steam.InstallPaths.Add(Path.GetFullPath(installNode.GetUnescapedValue()));
                }
            }

            var logInUsers = VdfParser.ParseVdfFile(Path.Combine(steamBasePath, @"config\loginusers.vdf")).FindChild("users");
            if(logInUsers != null)
            {
                foreach(var user in logInUsers.Children)
                {
                    var userId = user.Name;
                    var accountName = user.TryGetChildValue("AccountName", "N/A");
                    var currentName = user.TryGetChildValue("PersonaName", "N/A");

                    var su = new SteamUser();
                    su.AccountName = accountName;
                    su.CurrentName = currentName;
                    su.SteamID = userId;
                    su.AutoLogIn = false;
                    steam.Users.Add(su);
                }
            }

            var steamAppData = VdfParser.ParseVdfFile(Path.Combine(steamBasePath, @"config\SteamAppData.vdf"));
            var autoLogInUser = steamAppData.FindChild("SteamAppData", "AutoLoginUser");
            if(autoLogInUser != null)
            {
                var autoLogInUserName = autoLogInUser.GetUnescapedValue();
                foreach(var user in steam.Users)
                {
                    if(user.AccountName == autoLogInUserName)
                    {
                        user.AutoLogIn = true;
                        break;
                    }
                }
            }

            GetGameInfo(steam, "Quake Live", "QL", "Quake Live", "282440", true);
            GetGameInfo(steam, "Quake 3", "Q3", "Quake 3 Arena", "2200", false);

            return steam;
        }

        private static void GetGameInfo(Steam steam, string name, string shortName, string folderName, string appId, bool perAccountDemos)
        {
            var steamRegKey = Registry.CurrentUser.OpenSubKey(@"Software\Valve\Steam\Apps\" + appId);
            if(steamRegKey == null)
            {
                return;   
            }

            var installedKeyValue = (int)steamRegKey.GetValue("Installed", 0);
            if(installedKeyValue != 1)
            {
                return;
            }

            var path = "";
            var pathFound = false;
            foreach(var folder in steam.InstallPaths)
            {
                path = Path.Combine(folder, @"SteamApps\common\" + folderName);
                if(Directory.Exists(path))
                {
                    pathFound = true;
                    break;
                }
            }
            if(!pathFound)
            {
                return;
            }

            var game = new SteamGame();
            game.Name = name;
            game.ShortName = shortName;
            game.Path = path;
            steam.Games.Add(game);

            if(perAccountDemos)
            {
                foreach(var user in steam.Users)
                {
                    var demoPath = Path.Combine(path, user.SteamID + @"\baseq3\demos");
                    if(Directory.Exists(demoPath))
                    {
                        var demoPathInfo = new SteamDemoPath();
                        demoPathInfo.AccountSpecific = true;
                        demoPathInfo.SteamID = user.SteamID;
                        demoPathInfo.Path = demoPath;
                        game.DemoPaths.Add(demoPathInfo);
                    }
                }
            }
            else
            {
                var demoPath = Path.Combine(path, @"\baseq3\demos");
                if(Directory.Exists(demoPath))
                {
                    var demoPathInfo = new SteamDemoPath();
                    demoPathInfo.AccountSpecific = false;
                    demoPathInfo.SteamID = "";
                    demoPathInfo.Path = demoPath;
                    game.DemoPaths.Add(demoPathInfo);
                }
            }
        }
    }

    public class SteamDialog
    {
        public SteamDialog(Window parent)
        {
            var steam = Steam.GetSteamInfo();
            if(steam == null)
            {
                MessageBox.Show("Failed to find a Steam installation", "Steam not found", MessageBoxButton.OK, MessageBoxImage.Information);
                return;
            }

            var installPathsPanel = CreateVerticalPanel();
            for(var i = 0; i < steam.InstallPaths.Count; ++i)
            {
                var installPath = steam.InstallPaths[i];
                var title = i == 0 ? "Main" : "Extra #" + i.ToString();
                var pathTextBox = new TextBox();
                pathTextBox.Text = installPath;
                pathTextBox.IsReadOnly = true;
                var row = CreateRow(50, title, pathTextBox);
                installPathsPanel.Children.Add(row);
            }
            var installPathsGroupBox = CreateGroupBox("Steam Folders", installPathsPanel);
            if(installPathsPanel.Children.Count == 0)
            {
                installPathsGroupBox.Visibility = Visibility.Collapsed;
            }

            var usersGridView = new GridView();
            usersGridView.AllowsColumnReorder = false;
            usersGridView.Columns.Add(new GridViewColumn { Header = "Account Name", Width = 125, DisplayMemberBinding = new Binding("AccountName") });
            usersGridView.Columns.Add(new GridViewColumn { Header = "Current Name", Width = 125, DisplayMemberBinding = new Binding("CurrentName") });
            usersGridView.Columns.Add(new GridViewColumn { Header = "Steam ID", Width = 200, DisplayMemberBinding = new Binding("SteamID") });
            usersGridView.Columns.Add(new GridViewColumn { Header = "Auto Log-In", Width = 100, DisplayMemberBinding = new Binding("AutoLogIn") });

            var usersListView = new DemoInfoListView();
            usersListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            usersListView.VerticalAlignment = VerticalAlignment.Stretch;
            usersListView.Margin = new Thickness(5);
            usersListView.View = usersGridView;
            usersListView.SelectionMode = SelectionMode.Single;
            foreach(var user in steam.Users)
            {
                usersListView.Items.Add(user);
            }

            var copySteamIdButton = CreateButton("Copy ID to Clipboard", (obj, args) => CopySteamIdToClipboard(usersListView));
            copySteamIdButton.HorizontalAlignment = HorizontalAlignment.Left;
            copySteamIdButton.Margin = new Thickness(5);
            var usersPanel = CreateVerticalPanel();
            usersPanel.Margin = new Thickness(0);
            usersPanel.Children.Add(usersListView);
            usersPanel.Children.Add(copySteamIdButton);

            var usersGroupBox = CreateGroupBox("Steam Users", usersPanel);
            if(usersListView.Items.Count == 0)
            {
                usersGroupBox.Visibility = Visibility.Collapsed;
            }

            var gamePathsPanel = CreateVerticalPanel();
            foreach(var game in steam.Games)
            {
                var pathTextBox = new TextBox();
                pathTextBox.Text = game.Path;
                pathTextBox.IsReadOnly = true;
                var row = CreateRow(100, game.Name, pathTextBox);
                gamePathsPanel.Children.Add(row);
            }
            var gamePathsGroupBox = CreateGroupBox("Game Folders", gamePathsPanel);
            if(gamePathsPanel.Children.Count == 0)
            {
                gamePathsGroupBox.Visibility = Visibility.Collapsed;
            }

            var demoPathsGridView = new GridView();
            demoPathsGridView.AllowsColumnReorder = false;
            demoPathsGridView.Columns.Add(new GridViewColumn { Header = "Game", Width = 50, DisplayMemberBinding = new Binding("Game") });
            demoPathsGridView.Columns.Add(new GridViewColumn { Header = "Path", Width = 425, DisplayMemberBinding = new Binding("Path") });
            demoPathsGridView.Columns.Add(new GridViewColumn { Header = "Account", Width = 125, DisplayMemberBinding = new Binding("Account") });

            var demoPathsListView = new DemoInfoListView();
            demoPathsListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            demoPathsListView.VerticalAlignment = VerticalAlignment.Stretch;
            demoPathsListView.Margin = new Thickness(5);
            demoPathsListView.View = demoPathsGridView;
            demoPathsListView.SelectionMode = SelectionMode.Single;
            foreach(var game in steam.Games)
            {
                foreach(var demoPath in game.DemoPaths)
                {
                    var item = new DemoFolder();
                    item.Game = game.ShortName;
                    item.Path = demoPath.Path;
                    item.Account = "";
                    if(demoPath.AccountSpecific)
                    {
                        var user = steam.Users.Find(u => u.SteamID == demoPath.SteamID);
                        if(user != null)
                        {
                            item.Account = user.AccountName;
                        }
                    }
                    demoPathsListView.Items.Add(item);
                }
            }

            var copyButton = CreateButton("Copy Path to Clipboard", (obj, args) => CopyPathToClipboard(demoPathsListView));
            var setAsInputButton = CreateButton("Set as UDT Input Path", (obj, args) => SetAsInputFolder(demoPathsListView));
            var setAsOutputButton = CreateButton("Set as UDT Output Path", (obj, args) => SetAsOutputFolder(demoPathsListView));
            var openButton = CreateButton("Open in File Explorer", (obj, args) => OpenDemoPath(demoPathsListView));

            var demoPathsButtonRow = CreateHorizontalPanel();
            demoPathsButtonRow.Children.Add(copyButton);
            demoPathsButtonRow.Children.Add(setAsInputButton);
            demoPathsButtonRow.Children.Add(setAsOutputButton);
            demoPathsButtonRow.Children.Add(openButton);

            var demoPathsPanel = CreateVerticalPanel();
            demoPathsPanel.Margin = new Thickness(0);
            demoPathsPanel.Children.Add(demoPathsListView);
            demoPathsPanel.Children.Add(demoPathsButtonRow);

            var demoPathsGroupBox = CreateGroupBox("Demo Folders", demoPathsPanel);
            if(demoPathsListView.Items.Count == 0)
            {
                demoPathsGroupBox.Visibility = Visibility.Collapsed;
            }

            var rootPanel = CreateVerticalPanel();
            rootPanel.Children.Add(installPathsGroupBox);
            rootPanel.Children.Add(usersGroupBox);
            rootPanel.Children.Add(gamePathsGroupBox);
            rootPanel.Children.Add(demoPathsGroupBox);
            
            var window = new Window();
            window.Owner = parent;
            window.WindowStyle = WindowStyle.ToolWindow;
            window.AllowsTransparency = false;
            window.Background = new SolidColorBrush(System.Windows.SystemColors.ControlColor);
            window.ShowInTaskbar = false;
            window.SizeToContent = SizeToContent.WidthAndHeight;
            window.MinWidth = 400;
            window.MinHeight = 300;
            window.MaxWidth = 1024;
            window.MaxHeight = 768;
            window.ResizeMode = ResizeMode.NoResize;
            window.Icon = UDT.Properties.Resources.UDTIcon.ToImageSource();
            window.Title = "Steam Info";
            window.Content = rootPanel;
            window.Loaded += (obj, args) =>
            {
                window.Left = parent.Left + (parent.Width - window.Width) / 2;
                window.Top = parent.Top + (parent.Height - window.Height) / 2;
            };
            window.ShowDialog();
        }

        private static FrameworkElement CreateRow(int width, string title, FrameworkElement element)
        {
            var block = new TextBlock();
            block.VerticalAlignment = VerticalAlignment.Center;
            block.Width = width;
            block.Text = title;

            var row = new StackPanel();
            row.HorizontalAlignment = HorizontalAlignment.Stretch;
            row.VerticalAlignment = VerticalAlignment.Center;
            row.Orientation = Orientation.Horizontal;
            row.Children.Add(block);
            row.Children.Add(element);

            return row;
        }

        private static GroupBox CreateGroupBox(string title, FrameworkElement content)
        {
            var groupBox = new GroupBox();
            groupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            groupBox.VerticalAlignment = VerticalAlignment.Stretch;
            groupBox.Margin = new Thickness(5);
            groupBox.Header = title;
            groupBox.Content = content;

            return groupBox;
        }

        private static StackPanel CreateVerticalPanel()
        {
            var panel = new StackPanel();
            panel.HorizontalAlignment = HorizontalAlignment.Stretch;
            panel.VerticalAlignment = VerticalAlignment.Stretch;
            panel.Orientation = Orientation.Vertical;
            panel.Margin = new Thickness(5);

            return panel;
        }

        private static StackPanel CreateHorizontalPanel()
        {
            var panel = new StackPanel();
            panel.HorizontalAlignment = HorizontalAlignment.Stretch;
            panel.VerticalAlignment = VerticalAlignment.Stretch;
            panel.Orientation = Orientation.Horizontal;
            panel.Margin = new Thickness(5);

            return panel;
        }

        private static Button CreateButton(string title, RoutedEventHandler clickHandler)
        {
            var button = new Button();
            button.Margin = new Thickness(0, 0, 5, 0);
            button.Width = 150;
            button.Height = 25;
            button.Content = title;
            button.Click += clickHandler;

            return button;
        }

        private static T GetListViewItem<T>(ListView listView) where T : class
        {
            var item = listView.SelectedItem as T;
            if(item == null)
            {
                if(listView.Items.Count == 1)
                {
                    return listView.Items[0] as T;
                }

                NoItemSelected();
                return null;
            }

            return item;
        }

        private static void CopyPathToClipboard(ListView listView)
        {
            var item = GetListViewItem<DemoFolder>(listView);
            if(item == null)
            {
                return;
            }

            Clipboard.SetText(item.Path, TextDataFormat.UnicodeText);
        }

        private static void SetAsInputFolder(ListView listView)
        {
            var item = GetListViewItem<DemoFolder>(listView);
            if(item == null)
            {
                return;
            }

            var x = App.Instance.SelectedDemo;
            App.Instance.SetInputFolderPath(item.Path);
        }

        private static void SetAsOutputFolder(ListView listView)
        {
            var item = GetListViewItem<DemoFolder>(listView);
            if(item == null)
            {
                return;
            }

            App.Instance.SetOutputFolderPath(item.Path);
        }

        private static void OpenDemoPath(ListView listView)
        {
            var item = GetListViewItem<DemoFolder>(listView);
            if(item == null)
            {
                return;
            }

            try
            {
                Process.Start(item.Path);
            }
            catch(Exception)
            {
            }
        }

        private static void CopySteamIdToClipboard(ListView listView)
        {
            var item = GetListViewItem<SteamUser>(listView);
            if(item == null)
            {
                return;
            }

            Clipboard.SetText(item.SteamID, TextDataFormat.UnicodeText);
        }

        private static void NoItemSelected()
        {
            MessageBox.Show("No item selected in the list view.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
        }

        private class DemoFolder
        {
            public string Game { get; set; }
            public string Path { get; set; }
            public string Account { get; set; }
        }
    }
}