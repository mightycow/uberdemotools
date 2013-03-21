using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;


namespace Uber.DemoTools
{
    public class ChatRule
    {
        public string Operator = "Contains";
        public string Value = "";
        public bool CaseSensitive = false;
        public bool IgnoreColors = true;
    }

    public class UdtConfig
    {
        public List<ChatRule> ChatRules = new List<ChatRule>();
        public int ChatCutStartOffset = 10;
        public int ChatCutEndOffset = 10;
        public bool OutputToInputFolder = true;
        public string OutputFolder = "";
        public bool SkipChatOffsetsDialog = false;
        public bool SkipScanFoldersRecursivelyDialog = false;
        public bool ScanFoldersRecursively = false;
    }

    public partial class App
    {
        private const string GuiVersion = "0.2.0g";
        private readonly string DllVersion = Demo.GetVersion();

        private static readonly List<string> DemoExtensions = new List<string>
        {
            ".dm_68",
            ".dm_73"
        };

        private static readonly Dictionary<string, DemoProtocol> ProtocolFileExtDic = new Dictionary<string, DemoProtocol>
        {
            { ".dm_68", DemoProtocol.Dm68 },
            { ".dm_73", DemoProtocol.Dm73 }
        };

        private class TimedEventDisplayInfo
        {
            public string Time { get; set; }
        }

        private class ConfigStringSearchInfo
        {
            public ConfigStringSearchInfo(string key, string description)
            {
                Key = key;
                Description = description;
            }

            public string Key = "";
            public string Description = "";
        }

        private class ConfigStringDisplayInfo
        {
            public ConfigStringDisplayInfo(string description, string value)
            {
                Description = description;
                Value = value;
            }

            public string Description { get; set; }
            public string Value { get; set; }
        }
        
        private class DemoDisplayInfo
        {
            public string FileName { get; set; }
            public DemoInfo Demo { get; set; }
        }

        private class Xstats2Info
        {
            public List<WeaponStatsDisplayInfo> WeaponStats = new List<WeaponStatsDisplayInfo>(); // xstats2
            public List<StatDisplayInfo> DamageAndArmorStats = new List<StatDisplayInfo>(); // xstats2
            public string WeaponStatsPlayerName = "N/A"; // xstats2
        }

        private class PlayerInfo
        {
            public PlayerInfo(string name, int csIdx, int teamIdx)
            {
                Name = name;
                CsIndex = csIdx;
                TeamIndex = teamIdx;
            }

            public string Name = "N/A";
            public int CsIndex = -1;
            public int TeamIndex = -1;
        }

        private class DemoInfo
        {
            public static ConfigStringSearchInfo[] ConfigStringServerInfos = new ConfigStringSearchInfo[]
            {
                new ConfigStringSearchInfo("mapname", "Map Name"),
                new ConfigStringSearchInfo("mode_current", "Game Mode"),
                new ConfigStringSearchInfo("server_gameplay", "Gameplay"),
                new ConfigStringSearchInfo("game", "Game"),
                new ConfigStringSearchInfo("gamename", "Game Name"),
                new ConfigStringSearchInfo("gameversion", "Game Version"),
                new ConfigStringSearchInfo("sv_hostname", "Host Name"),
                new ConfigStringSearchInfo("GTV_CN", "GTV"),
                new ConfigStringSearchInfo("version", "Version"),
                new ConfigStringSearchInfo(".location", "Location"),
                new ConfigStringSearchInfo(".IRC", "IRC channel"),
                new ConfigStringSearchInfo(".admin", "Admin"),
                new ConfigStringSearchInfo("sv_allowDownload", "Allow Download"),
                new ConfigStringSearchInfo("sv_fps", "FPS"),
                new ConfigStringSearchInfo("g_gametype", "Game Type")
            };

            public static ConfigStringSearchInfo[] ConfigStringSystemInfos = new ConfigStringSearchInfo[]
            {
                new ConfigStringSearchInfo("g_synchronousClients", "Synchronous Clients"),
                new ConfigStringSearchInfo("timescale", "Time Scale"),
                new ConfigStringSearchInfo("fs_game", "Mod"),
                new ConfigStringSearchInfo("sv_pure", "Synchronous Clients"),
                new ConfigStringSearchInfo("sv_cheats", "Cheats")
            };

            public static ConfigStringSearchInfo[] ConfigStringCpmaGameInfo = new ConfigStringSearchInfo[]
            {
                new ConfigStringSearchInfo("tl", "Time Limit")
            };

            public string FilePath = "<invalid>";
            public DemoProtocol Protocol = DemoProtocol.Invalid;
            public List<Demo.ConfigStringInfo> DemoConfigStrings = null; // Raw data.
            public List<Demo.ChatStringInfo> DemoChatEvents = null; // Raw data.
            public List<Demo.ServerCommandInfo> DemoServerCommands = null; // Raw data.
            public List<Demo.GameStateInfo> DemoGameStates = new List<Demo.GameStateInfo>();
            public List<Demo.FragInfo> DemoFragEvents = new List<Demo.FragInfo>();
            public List<Demo.PuRunInfo> DemoPuRuns = new List<Demo.PuRunInfo>();
            public int DemoWarmupEndTimeMs = -1;
            public List<ConfigStringDisplayInfo> ConfigStrings = new List<ConfigStringDisplayInfo>();
            public List<ChatEventDisplayInfo> ChatEvents = new List<ChatEventDisplayInfo>();
            public List<FragEventDisplayInfo> FragEvents = new List<FragEventDisplayInfo>();
            public List<PuRunDisplayInfo> PuRuns = new List<PuRunDisplayInfo>();
            public List<Xstats2Info> Xstats2Infos = new List<Xstats2Info>();
            public List<PlayerInfo> Players = new List<PlayerInfo>();

            public static Regex ColorCodeRegEx = new Regex(@"\^.", RegexOptions.Compiled);
            public static Regex PlayerNameRegEx = new Regex(@"n\\([^\\]+)", RegexOptions.Compiled);
            public static Regex ConfigStringRegEx = new Regex(@"\\([^\\]+)", RegexOptions.Compiled);
        }

        private enum ConfigStringIndex
        {
            MaxPlayCount = 64,
            ServerInfo = 0, // Players_Blue Players_Red
            SystemInfo = 1,
            FirstPlayer_68 = 544,
            LastPlayer_68 = FirstPlayer_68 + MaxPlayCount - 1,
            FirstPlayer_73 = 529,
            LastPlayer_73 = FirstPlayer_73 + MaxPlayCount - 1,
            CpmaGameInfo = 672
        }

        private DemoInfo CreateDemoInfo(string filePath, Demo demo)
        {
            var demoInfo = new DemoInfo();
            demoInfo.FilePath = filePath;
            demoInfo.Protocol = GetProtocolFromFilePath(filePath);
            demoInfo.DemoChatEvents = demo.GetChatStrings();
            demoInfo.DemoConfigStrings = demo.GetConfigStrings();
            demoInfo.DemoServerCommands = demo.GetServerCommands();
            demoInfo.DemoGameStates = demo.GetGameStates();
            demoInfo.DemoFragEvents = demo.GetFrags();
            demoInfo.DemoPuRuns = demo.GetPuRuns();
            demoInfo.DemoWarmupEndTimeMs = demo.WarmupEndTimeMs;

            var chatEvents = demoInfo.DemoChatEvents;
            var fragEvents = demoInfo.DemoFragEvents;
            var puRuns = demoInfo.DemoPuRuns;
            var configStrings = demoInfo.DemoConfigStrings;

            ExtractPlayerNames(demoInfo);

            var serverInfo = configStrings.Find(csi => csi.Index == (int)ConfigStringIndex.ServerInfo);
            if(serverInfo != null)
            {
                ExtractServerInfo(demoInfo, serverInfo.Value);
            }

            var systemInfo = configStrings.Find(csi => csi.Index == (int)ConfigStringIndex.SystemInfo);
            if(systemInfo != null)
            {
                ExtractSystemInfo(demoInfo, systemInfo.Value);
            }

            if(demoInfo.Protocol == DemoProtocol.Dm68)
            {
                var cpmaGameInfo = configStrings.Find(csi => csi.Index == (int)ConfigStringIndex.CpmaGameInfo);
                if(cpmaGameInfo != null)
                {
                    ExtractCpmaGameInfo(demoInfo, cpmaGameInfo.Value);
                }
            }

            foreach(var chatEvent in chatEvents)
            {
                int time = chatEvent.Time / 1000;
                int minutes = time / 60;
                int seconds = time % 60;
                var timeStr = string.Format("{0}:{1}", minutes, seconds.ToString("00"));
                var cleanMessage = DemoInfo.ColorCodeRegEx.Replace(chatEvent.Message, "");
                var nameAndMessage = cleanMessage.Split(new string[] { ": " }, 2, StringSplitOptions.None);
                if(nameAndMessage.Length != 2)
                {
                    continue;
                }

                var name = nameAndMessage[0];
                var message = nameAndMessage[1];
                if(demoInfo.Protocol == DemoProtocol.Dm73 && name.Length > 3)
                {
                    name = name.Substring(3);
                }

                demoInfo.ChatEvents.Add(new ChatEventDisplayInfo(timeStr, name, message));
            }

            foreach(var fragEvent in fragEvents)
            {
                int time = fragEvent.Time / 1000;
                int minutes = time / 60;
                int seconds = time % 60;
                var timeStr = string.Format("{0}:{1}", minutes, seconds.ToString("00"));

                var attacker = fragEvent.AttackerName;
                var target = fragEvent.TargetName;
                var mod = (fragEvent.Mod >= 0 && fragEvent.Mod < _meansOfDeath.Length) ? _meansOfDeath[fragEvent.Mod] : "unknown";

                demoInfo.FragEvents.Add(new FragEventDisplayInfo(timeStr, attacker, target, mod));
            }

            foreach(var puRun in puRuns)
            {
                int time = puRun.Time / 1000;
                int minutes = time / 60;
                int seconds = time % 60;
                var timeStr = string.Format("{0}:{1}", minutes, seconds.ToString("00"));

                int duration = puRun.Duration / 1000;
                var durationStr = string.Format("{0}s", duration);

                var player = puRun.PlayerName;
                var pu = (puRun.Pu >= 0 && puRun.Pu < _puNames.Length) ? _puNames[puRun.Pu] : "N/A";
                var kills = puRun.Kills.ToString();
                var teamKills = puRun.TeamKills.ToString();
                var selfKill = puRun.SelfKill != 0 ? "Yes" : "No";

                demoInfo.PuRuns.Add(new PuRunDisplayInfo(timeStr, player, pu, durationStr, kills, teamKills, selfKill));
            }

            if(demoInfo.Protocol == DemoProtocol.Dm68)
            {
                ExtractXstats2Info(demoInfo);
            }

            return demoInfo;
        }

        private bool GetConfigString(string values, string key, out string result)
        {
            result = "";

            var idx = values.IndexOf(key + @"\");
            if(idx < 0)
            {
                return false;
            }
            idx += key.Length;

            var match = DemoInfo.ConfigStringRegEx.Match(values, idx);
            if(!match.Success)
            {
                return false;
            }

            result = match.Groups[1].Value;

            return true;
        }

        private void ExtractServerInfo(DemoInfo demoInfo, string values)
        {
            string value = "";
            foreach(var info in DemoInfo.ConfigStringServerInfos)
            {
                if(GetConfigString(values, info.Key, out value))
                {
                    demoInfo.ConfigStrings.Add(new ConfigStringDisplayInfo(info.Description, value));
                }
            }
        }

        private void ExtractSystemInfo(DemoInfo demoInfo, string values)
        {
            string value = "";
            foreach(var info in DemoInfo.ConfigStringSystemInfos)
            {
                if(GetConfigString(values, info.Key, out value))
                {
                    demoInfo.ConfigStrings.Add(new ConfigStringDisplayInfo(info.Description, value));
                }
            }
        }

        private void ExtractCpmaGameInfo(DemoInfo demoInfo, string values)
        {
            string value = "";
            foreach(var info in DemoInfo.ConfigStringCpmaGameInfo)
            {
                if(GetConfigString(values, info.Key, out value))
                {
                    demoInfo.ConfigStrings.Add(new ConfigStringDisplayInfo(info.Description, value));
                }
            }
        }

        private enum TeamIndex
        {
            Free,
            Red,
            Blue,
            Spectator
        }

        private static int GetConfigStringFirstPlayerId(DemoInfo demoInfo)
        {
            return demoInfo.Protocol == DemoProtocol.Dm68 ? (int)ConfigStringIndex.FirstPlayer_68 : (int)ConfigStringIndex.FirstPlayer_73;
        }

        private static int GetConfigStringLastPlayerId(DemoInfo demoInfo)
        {
            return demoInfo.Protocol == DemoProtocol.Dm68 ? (int)ConfigStringIndex.LastPlayer_68 : (int)ConfigStringIndex.LastPlayer_73;
        }

        private void ExtractPlayerNames(DemoInfo demoInfo)
        {
            var allPlayers = demoInfo.Players;
            var players = new List<PlayerInfo>();
            var spectators = new List<PlayerInfo>();
            var redPlayers = new List<PlayerInfo>();
            var bluePlayers = new List<PlayerInfo>();

            var playerConfigs = demoInfo.DemoConfigStrings.FindAll(csi => csi.Index >= GetConfigStringFirstPlayerId(demoInfo) && csi.Index <= GetConfigStringLastPlayerId(demoInfo));
            foreach(var cs in playerConfigs)
            {
                var match = DemoInfo.PlayerNameRegEx.Match(cs.Value);
                if(!match.Success)
                {
                    continue;
                }

                var playerName = DemoInfo.ColorCodeRegEx.Replace(match.Groups[1].Value, "");
                var teamIdxStr = "";
                if(!GetConfigString(cs.Value, "t", out teamIdxStr))
                {
                    continue;
                }

                var teamIdx = 0;
                if(!int.TryParse(teamIdxStr, out teamIdx))
                {
                    continue;
                }

                allPlayers.Add(new PlayerInfo(playerName, cs.Index, teamIdx));

                switch(teamIdx)
                {
                    case (int)TeamIndex.Free:
                        players.Add(new PlayerInfo(playerName, cs.Index, teamIdx));
                        break;

                    case (int)TeamIndex.Spectator:
                        spectators.Add(new PlayerInfo(playerName, cs.Index, teamIdx));
                        break;

                    case (int)TeamIndex.Blue:
                        bluePlayers.Add(new PlayerInfo(playerName, cs.Index, teamIdx));
                        break;
                    
                    case (int)TeamIndex.Red:
                        redPlayers.Add(new PlayerInfo(playerName, cs.Index, teamIdx));
                        break;

                    default:
                        players.Add(new PlayerInfo(playerName, cs.Index, teamIdx));
                        break;
                }
            }

            if(redPlayers.Count > 0 && bluePlayers.Count > 0)
            {
                demoInfo.ConfigStrings.Add(new ConfigStringDisplayInfo("Red Team", FormatPlayerList(redPlayers)));
                demoInfo.ConfigStrings.Add(new ConfigStringDisplayInfo("Blue Team", FormatPlayerList(bluePlayers)));
            }
            else
            {
                demoInfo.ConfigStrings.Add(new ConfigStringDisplayInfo("Players", FormatPlayerList(players)));
            }

            demoInfo.ConfigStrings.Add(new ConfigStringDisplayInfo("Spectators", FormatPlayerList(spectators)));
        }

        private string FormatPlayerList(List<PlayerInfo> players)
        {
            if(players.Count == 0)
            {
                return "";
            }

            var list = players[0].Name;
            for(int i = 1; i < players.Count; ++i)
            {
                list += " | ";
                list += players[i].Name;
            }

            return list;
        }

        private void ExtractXstats2Info(DemoInfo demoInfo)
        {
            var commands = demoInfo.DemoServerCommands;
            var configStrings = demoInfo.DemoConfigStrings;

            foreach(var command in commands)
            {
                if(!command.Command.StartsWith("xstats2 "))
                {
                    continue;
                }

                var tokens = command.Command.Split(new char[] { ' ' });
                if(tokens.Length < 3)
                {
                    LogWarning("xtats2: Too few arguments");
                    continue;
                }

                int flags = -1;
                if(!int.TryParse(tokens[2], out flags))
                {
                    LogWarning("xtats2: Unreadable weapon flags");
                    continue;
                }

                int weaponCount = 0;
                foreach(var searchInfo in WeaponStatsInfos)
                {
                    if((flags & searchInfo.FlagMask) != 0)
                    {
                        ++weaponCount;
                    }
                }

                if(tokens.Length < 3 + 6 * weaponCount)
                {
                    LogWarning("xtats2: Too few arguments");
                    continue;
                }

                var xstats2Info = new Xstats2Info();

                int idx = 3;
                foreach(var searchInfo in WeaponStatsInfos)
                {
                    if((flags & searchInfo.FlagMask) == 0)
                    {
                        continue;
                    }

                    var info = new WeaponStatsDisplayInfo();
                    info.Weapon = searchInfo.WeaponName;
                    info.Hits = FormatWeaponStat(tokens[idx++]);
                    info.Atts = FormatWeaponStat(tokens[idx++]);
                    info.Kills = FormatWeaponStat(tokens[idx++]);
                    info.Deaths = FormatWeaponStat(tokens[idx++]);
                    info.Take = FormatWeaponStat(tokens[idx++]);
                    info.Drop = FormatWeaponStat(tokens[idx++]);
                    info.Acc = ComputeAccuracy(info.Hits, info.Atts);
                    xstats2Info.WeaponStats.Add(info);
                }

                if(!ParseDamageAndArmourStats(xstats2Info, tokens, idx))
                {
                    continue;
                }

                if(!ExtractPlayerName(demoInfo, xstats2Info, tokens, configStrings))
                {
                    continue;
                }

                demoInfo.Xstats2Infos.Add(xstats2Info);
            }
        }

        private bool ParseDamageAndArmourStats(Xstats2Info demoInfo, string[] tokens, int idx)
        {
            if(idx + xstats2EndStatsInfos.Length >= tokens.Length)
            {
                LogWarning("xtats2: Too few arguments");
                return false;
            }

            foreach(var statName in xstats2EndStatsInfos)
            {
                demoInfo.DamageAndArmorStats.Add(new StatDisplayInfo(statName, tokens[idx++]));
            }

            return true;
        }

        private bool ExtractPlayerName(DemoInfo demoInfo, Xstats2Info statsInfo, string[] tokens, List<Demo.ConfigStringInfo> configStrings)
        {
            int playerId = -1;
            if(!int.TryParse(tokens[1], out playerId))
            {
                LogWarning("xtats2: Unreadable player ID");
                return false;
            }

            int csIdx = configStrings.FindIndex(cs => cs.Index == GetConfigStringFirstPlayerId(demoInfo) + playerId);
            if(csIdx < 0)
            {
                LogWarning("xtats2: Invalid player ID");
                return false;
            }

            // Extract the player's name.
            var match = DemoInfo.PlayerNameRegEx.Match(configStrings[csIdx].Value);
            if(!match.Success)
            {
                LogWarning("xtats2: Failed to extract the player's name from the matching config string");
                return false;
            }

            statsInfo.WeaponStatsPlayerName = DemoInfo.ColorCodeRegEx.Replace(match.Groups[1].Value, "");

            return true;
        }

        private bool ExtractPlayerName(DemoInfo demoInfo, int playerIdx, out string playerName)
        {
            playerName = "N/A";

            var cs = demoInfo.DemoConfigStrings.Find(csi => csi.Index == GetConfigStringFirstPlayerId(demoInfo) + playerIdx);
            if(cs == null)
            {
                return false;
            }

            // Extract the player's name.
            var match = DemoInfo.PlayerNameRegEx.Match(cs.Value);
            if(!match.Success)
            {
                return false;
            }

            playerName = DemoInfo.ColorCodeRegEx.Replace(match.Groups[1].Value, "");

            return true;
        }
        
        private UdtConfig _config = new UdtConfig();
        private Application _application = null;
        private Thread _jobThread = null;
        private Window _window = null;
        private ListView _demoListView = null;
        private Brush _demoListViewBackground = null;
        private ListView _infoListView = null;
        private ListBox _logListBox = null;
        private ProgressBar _progressBar = null;
        private Button _cancelJobButton = null;
        private int _cancelJobValue = 0;
        private GroupBox _progressGroupBox = null;
        private DockPanel _rootPanel = null;
        private TabControl _tabControl = null;
        private List<FrameworkElement> _rootElements = new List<FrameworkElement>();
        private List<DemoInfo> _demos = new List<DemoInfo>();
        private AlternatingListBoxBackground _altListBoxBg = null;
        private static RoutedCommand _cutByChatCommand = new RoutedCommand();
        private static RoutedCommand _cutByFragCommand = new RoutedCommand();
        private static RoutedCommand _cutByPuRunCommand = new RoutedCommand();
        private static RoutedCommand _deleteDemoCommand = new RoutedCommand();
        private static RoutedCommand _showDemoInfoCommand = new RoutedCommand();
        private static RoutedCommand _clearLogCommand = new RoutedCommand();
        private static RoutedCommand _copyLogCommand = new RoutedCommand();
        private static RoutedCommand _copyChatCommand = new RoutedCommand();
        private static RoutedCommand _copyFragCommand = new RoutedCommand();

        public App(string[] cmdLineArgs)
        {
            PresentationTraceSources.DataBindingSource.Switch.Level = SourceLevels.Critical;

            LoadConfig();

            _altListBoxBg = new AlternatingListBoxBackground(Colors.White, Color.FromRgb(223, 223, 223));

            var manageDemosTab = new TabItem();
            manageDemosTab.Header = "Manage Demos";
            manageDemosTab.Content = CreateManageDemosTab();

            var demosTab = new TabItem();
            demosTab.Header = "Info";
            demosTab.Content = CreateDemoInfoTab();

            var demoChatTab = new TabItem();
            demoChatTab.Header = "Chat";
            demoChatTab.Content = CreateDemoChatTab();
            
            var cutTimeTab = new TabItem();
            cutTimeTab.Header = "Cut by Time";
            cutTimeTab.Content = CreateCutByTimeTab();

            var cutChatTab = new TabItem();
            cutChatTab.Header = "Cut by Chat";
            cutChatTab.Content = CreateCutByChatTab();

            var demoFragsTab = new TabItem();
            demoFragsTab.Header = "Obituaries";
            demoFragsTab.Content = CreateDemoFragTab();

            var demoPuRunTab = new TabItem();
            demoPuRunTab.Header = "PU Runs";
            demoPuRunTab.Content = CreateDemoPuRunTab();

            var statsTab = new TabItem();
            statsTab.Header = "Stats";
            statsTab.Content = CreateStatsTab();

            var settingsTab = new TabItem();
            settingsTab.Header = "Settings";
            settingsTab.Content = CreateSettingsTab();
            
            var tabControl = new TabControl();
            _tabControl = tabControl;
            tabControl.HorizontalAlignment = HorizontalAlignment.Stretch;
            tabControl.VerticalAlignment = VerticalAlignment.Stretch;
            tabControl.Margin = new Thickness(5);
            tabControl.Items.Add(manageDemosTab);
            tabControl.Items.Add(demosTab);
            tabControl.Items.Add(demoChatTab);
            tabControl.Items.Add(cutTimeTab);
            tabControl.Items.Add(cutChatTab);
            tabControl.Items.Add(demoFragsTab);
            tabControl.Items.Add(demoPuRunTab);
            tabControl.Items.Add(statsTab);
            tabControl.Items.Add(settingsTab);
            tabControl.SelectionChanged += (obj, args) => OnTabSelectionChanged();

            var exitMenuItem = new MenuItem();
            exitMenuItem.Header = "E_xit";
            exitMenuItem.Click += (obj, arg) => OnQuit();
            exitMenuItem.ToolTip = new ToolTip { Content = "Close the program" };

            var openDemoMenuItem = new MenuItem();
            openDemoMenuItem.Header = "Open Demo(s)...";
            openDemoMenuItem.Click += (obj, arg) => OnOpenDemo();
            openDemoMenuItem.ToolTip = new ToolTip { Content = "Open a demo file for processing/analysis" };

            var openFolderMenuItem = new MenuItem();
            openFolderMenuItem.Header = "Open Demo Folder...";
            openFolderMenuItem.Click += (obj, arg) => OnOpenDemoFolder();
            openFolderMenuItem.ToolTip = new ToolTip { Content = "Open a demo folder for processing/analysis" };

            var fileMenuItem = new MenuItem();
            fileMenuItem.Header = "_File";
            fileMenuItem.Items.Add(openDemoMenuItem);
            fileMenuItem.Items.Add(openFolderMenuItem);
            fileMenuItem.Items.Add(new Separator());
            fileMenuItem.Items.Add(exitMenuItem);

            var clearLogMenuItem = new MenuItem();
            clearLogMenuItem.Header = "Clear";
            clearLogMenuItem.Click += (obj, arg) => ClearLog();
            clearLogMenuItem.ToolTip = new ToolTip { Content = "Clears the log box" };

            var copyLogMenuItem = new MenuItem();
            copyLogMenuItem.Header = "Copy Everything";
            copyLogMenuItem.Click += (obj, arg) => CopyLog();
            copyLogMenuItem.ToolTip = new ToolTip { Content = "Copies the log to the Windows clipboard" };

            var copyLogSelectionMenuItem = new MenuItem();
            copyLogSelectionMenuItem.Header = "Copy Selection";
            copyLogSelectionMenuItem.Click += (obj, arg) => CopyLogSelection();
            copyLogSelectionMenuItem.ToolTip = new ToolTip { Content = "Copies the selected log line to the Windows clipboard" };

            var saveLogMenuItem = new MenuItem();
            saveLogMenuItem.Header = "Save to File...";
            saveLogMenuItem.Click += (obj, arg) => SaveLog();
            saveLogMenuItem.ToolTip = new ToolTip { Content = "Saves the log to the specified file" };

            var logMenuItem = new MenuItem();
            logMenuItem.Header = "_Log";
            logMenuItem.Items.Add(clearLogMenuItem);
            logMenuItem.Items.Add(copyLogMenuItem);
            logMenuItem.Items.Add(copyLogSelectionMenuItem);
            logMenuItem.Items.Add(new Separator());
            logMenuItem.Items.Add(saveLogMenuItem);

            var aboutMenuItem = new MenuItem();
            aboutMenuItem.Header = "_About";
            aboutMenuItem.Click += (obj, arg) => ShowAboutWindow();
            aboutMenuItem.ToolTip = new ToolTip { Content = "Learn more about this awesome application" };

            var helpMenuItem = new MenuItem();
            helpMenuItem.Header = "_Help";
            helpMenuItem.Items.Add(aboutMenuItem);

            var mainMenu = new Menu();
            mainMenu.IsMainMenu = true;
            mainMenu.Items.Add(fileMenuItem);
            mainMenu.Items.Add(logMenuItem);
            mainMenu.Items.Add(helpMenuItem);

            var logListBox = new ListBox();
            _logListBox = logListBox;
            _logListBox.SelectionMode = SelectionMode.Single;
            logListBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            logListBox.VerticalAlignment = VerticalAlignment.Stretch;
            logListBox.Margin = new Thickness(5);
            logListBox.Height = 150;
            InitLogListBoxClearCommand();
            InitLogListBoxCopyCommand();
            InitLogListBoxContextualMenu();

            var logGroupBox = new GroupBox();
            logGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            logGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            logGroupBox.Margin = new Thickness(5);
            logGroupBox.Header = "Log";
            logGroupBox.Content = logListBox;

            var progressBar = new ProgressBar();
            _progressBar = progressBar;
            progressBar.HorizontalAlignment = HorizontalAlignment.Stretch;
            progressBar.VerticalAlignment = VerticalAlignment.Bottom;
            progressBar.Margin = new Thickness(5);
            progressBar.Height = 20;
            progressBar.Minimum = 0;
            progressBar.Maximum = 100;
            progressBar.Value = 0;

            var cancelJobButton = new Button();
            _cancelJobButton = cancelJobButton;
            cancelJobButton.HorizontalAlignment = HorizontalAlignment.Right;
            cancelJobButton.VerticalAlignment = VerticalAlignment.Center;
            cancelJobButton.Width = 70;
            cancelJobButton.Height = 25;
            cancelJobButton.Content = "Cancel";
            cancelJobButton.Click += (obj, args) => OnCancelJobClicked();

            var progressPanel = new DockPanel();
            progressPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            progressPanel.VerticalAlignment = VerticalAlignment.Bottom;
            progressPanel.LastChildFill = true;
            progressPanel.Children.Add(cancelJobButton);
            progressPanel.Children.Add(progressBar);
            DockPanel.SetDock(cancelJobButton, Dock.Right);

            var progressGroupBox = new GroupBox();
            _progressGroupBox = progressGroupBox;
            progressGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            progressGroupBox.VerticalAlignment = VerticalAlignment.Center;
            progressGroupBox.Margin = new Thickness(5, 0, 5, 0);
            progressGroupBox.Header = "Progress";
            progressGroupBox.Content = progressPanel;
            progressGroupBox.Visibility = Visibility.Collapsed;

            var demoGridView = new GridView();
            demoGridView.AllowsColumnReorder = false;
            demoGridView.Columns.Add(new GridViewColumn { Header = "File Name", Width = 290, DisplayMemberBinding = new Binding("FileName") });

            var demoListView = new ListView();
            _demoListView = demoListView;
            _demoListViewBackground = demoListView.Background;
            demoListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            demoListView.VerticalAlignment = VerticalAlignment.Stretch;
            demoListView.Margin = new Thickness(5);
            demoListView.Width = 300;
            demoListView.AllowDrop = true;
            demoListView.View = demoGridView;
            demoListView.SelectionMode = SelectionMode.Extended;
            demoListView.DragEnter += OnDemoListBoxDragEnter;
            demoListView.Drop += OnDemoListBoxDragDrop;
            demoListView.SelectionChanged += (obj, args) => OnDemoListSelectionChanged();
            demoListView.Initialized += (obj, arg) => { _demoListViewBackground = _demoListView.Background; };
            demoListView.Foreground = new SolidColorBrush(Colors.Black);
            InitDemoListDeleteCommand();
            
            var demoListGroupBox = new GroupBox();
            demoListGroupBox.Header = "Demo List";
            demoListGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            demoListGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            demoListGroupBox.Margin = new Thickness(5);
            demoListGroupBox.Content = demoListView;

            var centerPartPanel = new DockPanel();
            centerPartPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            centerPartPanel.VerticalAlignment = VerticalAlignment.Stretch;
            centerPartPanel.Margin = new Thickness(5);
            centerPartPanel.Children.Add(demoListGroupBox);
            centerPartPanel.Children.Add(tabControl);
            centerPartPanel.LastChildFill = true;
            DockPanel.SetDock(demoListGroupBox, Dock.Left);

            var statusBarTextBox = new TextBox();
            statusBarTextBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            statusBarTextBox.VerticalAlignment = VerticalAlignment.Bottom;
            statusBarTextBox.IsEnabled = true;
            statusBarTextBox.IsReadOnly = true;
            statusBarTextBox.Background = new SolidColorBrush(System.Windows.SystemColors.ControlColor);
            statusBarTextBox.Text = Quotes.GetRandomQuote();

            var rootPanel = new DockPanel();
            _rootPanel = rootPanel;
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.LastChildFill = true;
            AddRootElement(statusBarTextBox);
            AddRootElement(logGroupBox);
            AddRootElement(progressGroupBox);
            AddRootElement(mainMenu);
            AddRootElement(centerPartPanel);
            DockPanel.SetDock(mainMenu, Dock.Top);
            DockPanel.SetDock(centerPartPanel, Dock.Top);
            DockPanel.SetDock(statusBarTextBox, Dock.Bottom);
            DockPanel.SetDock(logGroupBox, Dock.Bottom);
            DockPanel.SetDock(progressGroupBox, Dock.Bottom);
            _rootElements.Remove(progressGroupBox); // Only the cancel button can remain active at all times.

            _altListBoxBg.ApplyTo(_infoListView);
            _altListBoxBg.ApplyTo(_chatRulesListView);
            _altListBoxBg.ApplyTo(_chatEventsListView);
            _altListBoxBg.ApplyTo(_fragEventsListView);
            _altListBoxBg.ApplyTo(_puRunsListView);
            _altListBoxBg.ApplyTo(_demoListView);
            _altListBoxBg.ApplyTo(_logListBox);
            foreach(var info in _xstats2WidgetInfos)
            {
                _altListBoxBg.ApplyTo(info.WeaponStatsListView);
                _altListBoxBg.ApplyTo(info.DamageStatsListView);
            }
            _logListBox.Resources.Add(SystemColors.HighlightBrushKey, new SolidColorBrush(Color.FromRgb(255, 255, 191)));

            var label = new Label { Content = "You can drag'n'drop files and folders here.", HorizontalAlignment = HorizontalAlignment.Center, VerticalAlignment = VerticalAlignment.Center };
            var brush = new VisualBrush(label) { Stretch = Stretch.None, Opacity = 0.5 };
            _demoListView.Background = brush;

            var window = new Window();
#if UDT_DOT_NET_40
            TextOptions.SetTextRenderingMode(window, TextRenderingMode.ClearType);
            TextOptions.SetTextHintingMode(window, TextHintingMode.Fixed);
            TextOptions.SetTextFormattingMode(window, TextFormattingMode.Display);
#endif
            _window = window;
            window.Closing += (obj, args) => OnQuit();
            window.WindowStyle = WindowStyle.SingleBorderWindow;
            window.AllowsTransparency = false;
            window.Background = new SolidColorBrush(System.Windows.SystemColors.ControlColor);
            window.ShowInTaskbar = true;
            window.Title = "UDT";
            window.Content = rootPanel;
            window.Width = 1024;
            window.Height = 768;
            window.MinWidth = 1024;
            window.MinHeight = 768;

            LogInfo("UDT is now operational!");
            LogInfo("GUI version: " + GuiVersion);
            LogInfo("DLL version: " + DllVersion);

#if UDT_DOT_NET_40
            const string DotNetVersion = "4.0";
#else
            const string DotNetVersion = "3.5";
#endif

#if UDT_DOT_NET_CLIENT_PROFILE
            const string DotNetProfile = " Client Profile";
#else
            const string DotNetProfile = "";
#endif

            LogInfo("Running on .NET " + DotNetVersion + DotNetProfile);

            ProcessCommandLine(cmdLineArgs);

            var app = new Application();
            _application = app;
            app.ShutdownMode = ShutdownMode.OnLastWindowClose;
            app.Run(window);
        }

        private void ProcessCommandLine(string[] cmdLineArgs)
        {
            var filePaths = new List<string>();
            var folderPaths = new List<string>();
            foreach(var argument in cmdLineArgs)
            {
                var arg = argument;
                if(Path.GetExtension(arg).ToLower() == ".lnk")
                {
                    string realPath;
                    if(Shortcut.ResolveShortcut(out realPath, argument))
                    {
                        arg = realPath;
                    }
                }

                if(File.Exists(arg) && IsDemoPath(arg))
                {
                    filePaths.Add(Path.GetFullPath(arg));
                }
                else if(Directory.Exists(arg))
                {
                    folderPaths.Add(Path.GetFullPath(arg));
                }
            }

            AddDemos(filePaths, folderPaths);
        }

        private void AddDemos(List<string> filePaths, List<string> folderPaths)
        {
            var filteredFilePaths = new List<string>();
            foreach(var filePath in filePaths)
            {
                if(!File.Exists(filePath))
                {
                    continue;
                }

                var absFilePath = Path.GetFullPath(filePath).ToLower();

                bool exists = false;
                foreach(var demo in _demos)
                {
                    var demoAbsFilePath = Path.GetFullPath(demo.FilePath).ToLower();
                    if(demoAbsFilePath == absFilePath)
                    {
                        exists = true;
                        break;
                    }
                }

                if(!exists)
                {
                    filteredFilePaths.Add(filePath);
                }
            }

            if(folderPaths.Count > 0)
            {
                var recursive = false;

                if(_config.SkipScanFoldersRecursivelyDialog)
                {
                    recursive = _config.ScanFoldersRecursively;
                }
                else
                {
                    var result = MessageBox.Show("You dropped folder paths. Do you want to add all demos in subdirectories too?", "Scan for Demos Recursively?", MessageBoxButton.YesNoCancel, MessageBoxImage.Question);
                    if(result == MessageBoxResult.Cancel)
                    {
                        return;
                    }

                    recursive = result == MessageBoxResult.Yes;
                }

                var searchOption = recursive ? SearchOption.AllDirectories : SearchOption.TopDirectoryOnly;

                foreach(var folderPath in folderPaths)
                {
                    foreach(var demoExtension in DemoExtensions)
                    {
                        var searchPattern = "*" + demoExtension;
                        var demoPaths = Directory.GetFiles(folderPath, searchPattern, searchOption);
                        filteredFilePaths.AddRange(demoPaths);
                    }
                }
            }

            if(filteredFilePaths.Count == 0)
            {
                return;
            }

            AddDemosImpl(filteredFilePaths);
        }

        private void InitDemoListDeleteCommand()
        {
            var inputGesture = new KeyGesture(Key.Delete, ModifierKeys.None);
            var inputBinding = new KeyBinding(_deleteDemoCommand, inputGesture);
            var commandBinding = new CommandBinding();
            commandBinding.Command = _deleteDemoCommand;
            commandBinding.Executed += (obj, args) => OnRemoveDemoClicked();
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = true; };
            _demoListView.InputBindings.Add(inputBinding);
            _demoListView.CommandBindings.Add(commandBinding);
        }

        private void InitLogListBoxClearCommand()
        {
            var inputGesture = new KeyGesture(Key.X, ModifierKeys.Control);
            var inputBinding = new KeyBinding(_clearLogCommand, inputGesture);
            var commandBinding = new CommandBinding();
            commandBinding.Command = _clearLogCommand;
            commandBinding.Executed += (obj, args) => ClearLog();
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = true; };
            _logListBox.InputBindings.Add(inputBinding);
            _logListBox.CommandBindings.Add(commandBinding);
        }

        private void InitLogListBoxCopyCommand()
        {
            var inputGesture = new KeyGesture(Key.C, ModifierKeys.Control);
            var inputBinding = new KeyBinding(_copyLogCommand, inputGesture);
            var commandBinding = new CommandBinding();
            commandBinding.Command = _copyLogCommand;
            commandBinding.Executed += (obj, args) => CopyLogSelection();
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = true; };
            _logListBox.InputBindings.Add(inputBinding);
            _logListBox.CommandBindings.Add(commandBinding);
        }

        private void InitLogListBoxContextualMenu()
        {
            var clearLogMenuItem = new MenuItem();
            clearLogMenuItem.Header = "Clear (Ctrl-X)";
            clearLogMenuItem.Command = _clearLogCommand;
            clearLogMenuItem.Click += (obj, args) => ClearLog();

            var copyLogMenuItem = new MenuItem();
            copyLogMenuItem.Header = "Copy (Ctrl-C)";
            copyLogMenuItem.Command = _copyLogCommand;
            copyLogMenuItem.Click += (obj, args) => CopyLogSelection();

            var contextMenu = new ContextMenu();
            contextMenu.Items.Add(clearLogMenuItem);
            contextMenu.Items.Add(copyLogMenuItem);

            _logListBox.ContextMenu = contextMenu;
        }

        private void OnShowDemoInfo()
        {
            VoidDelegate tabSetter = delegate
            {
                _tabControl.SelectedIndex = 1;
            };
            _tabControl.Dispatcher.Invoke(tabSetter);
        }

        private void AddRootElement(FrameworkElement element)
        {
            _rootPanel.Children.Add(element);
            _rootElements.Add(element);
        }

        private void OnQuit()
        {
            if(_jobThread != null)
            {
                _jobThread.Join();
            }

            SaveConfig();
            _application.Shutdown();
        }

        private void OnTabSelectionChanged()
        {
            var singleMode = (_tabControl.SelectedIndex == 1) || (_tabControl.SelectedIndex == 2);
            _demoListView.SelectionMode = singleMode ? SelectionMode.Single : SelectionMode.Extended;
        }

        private void OnOpenDemo()
        {
            using(var openFileDialog = new System.Windows.Forms.OpenFileDialog())
            {
                openFileDialog.CheckPathExists = true;
                openFileDialog.Multiselect = true;
                openFileDialog.InitialDirectory = System.Environment.GetFolderPath(Environment.SpecialFolder.Desktop);
                openFileDialog.Filter = "Quake 3 demos (*.dm_68)|*.dm_68|Quake Live demos (*.dm_73)|*.dm_73"; // @TODO: Construct the filter programmatically.
                if(openFileDialog.ShowDialog() != System.Windows.Forms.DialogResult.OK)
                {
                    return;
                }

                var filePaths = new List<string>();
                filePaths.AddRange(openFileDialog.FileNames);
                AddDemos(filePaths, new List<string>());
            }
        }

        private void OnOpenDemoFolder()
        {
            using(var openFolderDialog = new System.Windows.Forms.FolderBrowserDialog())
            {
                openFolderDialog.Description = "Browse for a folder containing demo files";
                openFolderDialog.ShowNewFolderButton = true;
                openFolderDialog.RootFolder = Environment.SpecialFolder.Desktop;
                if(openFolderDialog.ShowDialog() != System.Windows.Forms.DialogResult.OK)
                {
                    return;
                }

                AddDemos(new List<string>(), new List<string> { openFolderDialog.SelectedPath });
            }
        }

        private void ShowAboutWindow()
        {
            var textPanelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            textPanelList.Add(CreateTuple("Version", "1.3.3.7"));
            textPanelList.Add(CreateTuple("Developer", "myT"));
            var textPanel = WpfHelper.CreateDualColumnPanel(textPanelList, 100, 1);

            var image = new System.Windows.Controls.Image();
            image.HorizontalAlignment = HorizontalAlignment.Right;
            image.VerticalAlignment = VerticalAlignment.Top;
            image.Margin = new Thickness(5);
            image.Stretch = Stretch.None;
            image.Source = UDT.Properties.Resources.UDTIcon.ToImageSource();

            var rootPanel = new StackPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.Margin = new Thickness(5);
            rootPanel.Orientation = Orientation.Horizontal;
            rootPanel.Children.Add(textPanel);
            rootPanel.Children.Add(image);

            var window = new Window();
            window.WindowStyle = WindowStyle.ToolWindow;
            window.ResizeMode = ResizeMode.NoResize;
            window.Background = new SolidColorBrush(System.Windows.SystemColors.ControlColor);
            window.ShowInTaskbar = false;
            window.Title = "About UberDemoTools";
            window.Content = rootPanel;
            window.Width = 240;
            window.Height = 100;
            window.Left = _window.Left + (_window.Width - window.Width) / 2;
            window.Top = _window.Top + (_window.Height - window.Height) / 2;
            window.Icon = UDT.Properties.Resources.UDTIcon.ToImageSource();
            window.ShowDialog();
        }

        private static Tuple<FrameworkElement, FrameworkElement> CreateTuple(string a, FrameworkElement b)
        {
            return new Tuple<FrameworkElement, FrameworkElement>(new Label { Content = a }, b);
        }

        private static Tuple<FrameworkElement, FrameworkElement> CreateTuple(string a, string b)
        {
            return new Tuple<FrameworkElement, FrameworkElement>(new Label { Content = a }, new Label { Content = b });
        }

        private FrameworkElement CreateDemoInfoTab()
        {
            var configStringGridView = new GridView();
            configStringGridView.AllowsColumnReorder = false;
            configStringGridView.Columns.Add(new GridViewColumn { Header = "Name", Width = 150, DisplayMemberBinding = new Binding("Description") });
            configStringGridView.Columns.Add(new GridViewColumn { Header = "Value", Width = 400, DisplayMemberBinding = new Binding("Value") });

            var infoListView = new ListView();
            _infoListView = infoListView;
            infoListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            infoListView.VerticalAlignment = VerticalAlignment.Stretch;
            infoListView.Margin = new Thickness(5);
            infoListView.View = configStringGridView;
            infoListView.SelectionMode = SelectionMode.Single;
            infoListView.Foreground = new SolidColorBrush(Colors.Black);

            var infoPanelGroupBox = new GroupBox();
            infoPanelGroupBox.Header = "Demo Information";
            infoPanelGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            infoPanelGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            infoPanelGroupBox.Margin = new Thickness(5);
            infoPanelGroupBox.Content = infoListView;

            return infoPanelGroupBox;
        }

        private FrameworkElement CreateManageDemosTab()
        {
            var addButton = new Button();
            addButton.Content = "Add File(s)";
            addButton.Width = 75;
            addButton.Height = 25;
            addButton.Margin = new Thickness(5);
            addButton.Click += (obj, args) => OnOpenDemo();

            var addFolderButton = new Button();
            addFolderButton.Content = "Add Folder";
            addFolderButton.Width = 75;
            addFolderButton.Height = 25;
            addFolderButton.Margin = new Thickness(5);
            addFolderButton.Click += (obj, args) => OnOpenDemoFolder();

            var removeButton = new Button();
            removeButton.Content = "Remove";
            removeButton.Width = 75;
            removeButton.Height = 25;
            removeButton.Margin = new Thickness(5);
            removeButton.Click += (obj, args) => OnRemoveDemoClicked();

            var buttonPanel = new StackPanel();
            buttonPanel.HorizontalAlignment = HorizontalAlignment.Left;
            buttonPanel.VerticalAlignment = VerticalAlignment.Top;
            buttonPanel.Margin = new Thickness(5);
            buttonPanel.Orientation = Orientation.Vertical;
            buttonPanel.Children.Add(addButton);
            buttonPanel.Children.Add(addFolderButton);
            buttonPanel.Children.Add(removeButton);

            var buttonGroupBox = new GroupBox();
            buttonGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            buttonGroupBox.VerticalAlignment = VerticalAlignment.Top;
            buttonGroupBox.Margin = new Thickness(5);
            buttonGroupBox.Header = "Demo List Actions";
            buttonGroupBox.Content = buttonPanel;

            return buttonGroupBox;
        }

        private void PopulateInfoListView(DemoInfo demoInfo)
        {
            _infoListView.Items.Clear();
            _infoListView.Items.Add(new ConfigStringDisplayInfo("Folder Path", Path.GetDirectoryName(demoInfo.FilePath) ?? "N/A"));
            _infoListView.Items.Add(new ConfigStringDisplayInfo("File Name", Path.GetFileNameWithoutExtension(demoInfo.FilePath) ?? "N/A"));
            _infoListView.Items.Add(new ConfigStringDisplayInfo("Game Start", FormatMinutesSeconds(demoInfo.DemoWarmupEndTimeMs / 1000)));
            foreach(var configInfo in demoInfo.ConfigStrings)
            {
                _infoListView.Items.Add(configInfo);
            }
        }

        private void OnDemoListSelectionChanged()
        {
            int idx = _demoListView.SelectedIndex;
            if(idx < 0 || idx >= _demos.Count)
            {
                return;
            }

            var demoInfo = _demos[idx];

            PopulateInfoListView(demoInfo);
            PopulateChatEventsListView(demoInfo);
            PopulateFragEventsListView(demoInfo);
            PopulatePuRunsListView(demoInfo);
            PopulateStats(demoInfo);
        }

        private void OnDemoListBoxDragEnter(object sender, DragEventArgs e)
        {
            bool dataPresent = e.Data.GetDataPresent(DataFormats.FileDrop);

            e.Effects = dataPresent ? DragDropEffects.All : DragDropEffects.None;
        }

        private static bool IsDemoPath(string path)
        {
            var extension = Path.GetExtension(path).ToLower();

            return DemoExtensions.Contains(extension);
        }

        private void OnDemoListBoxDragDrop(object sender, DragEventArgs e)
        {
            var droppedPaths = (string[])e.Data.GetData(DataFormats.FileDrop, false);
            var droppedFilePaths = new List<string>();
            var droppedFolderPaths = new List<string>();

            foreach(var droppedPath in droppedPaths)
            {
                var path = droppedPath;
                if(Path.GetExtension(path).ToLower() == ".lnk")
                {
                    string realPath;
                    if(Shortcut.ResolveShortcut(out realPath, path))
                    {
                        path = realPath;
                    }
                }

                if(File.Exists(path) && IsDemoPath(path))
                {
                    droppedFilePaths.Add(path);
                }
                else if(Directory.Exists(path))
                {
                    droppedFolderPaths.Add(path);
                }
            }

            AddDemos(droppedFilePaths, droppedFolderPaths);
        }

        private void DisableUiNonThreadSafe()
        {
            _progressGroupBox.Visibility = Visibility.Visible;
            _progressBar.Value = 0;
            foreach(var element in _rootElements)
            {
                element.IsEnabled = false;
            }
        }

        private void EnableUiThreadSafe()
        {
            VoidDelegate guiResetter = delegate
            {
                _progressGroupBox.Visibility = Visibility.Collapsed;
                _progressBar.Value = 0;
                foreach(var element in _rootElements)
                {
                    element.IsEnabled = true;
                }
            };

            _cancelJobValue = 0;
            _window.Dispatcher.Invoke(guiResetter);
        }

        private void AddDemosImpl(List<string> filePaths)
        {
            if(filePaths.Count == 0)
            {
                return;
            }

            DisableUiNonThreadSafe();
            _demoListView.Background = _demoListViewBackground;

            if(_jobThread != null)
            {
                _jobThread.Join();
            }

            _jobThread = new Thread(DemoAddThread);
            _jobThread.Start(filePaths);
        }

        private void DemoAddThread(object arg)
        {
            try
            {
                DemoAddThreadImpl(arg);
            }
            catch(Exception exception)
            {
                EntryPoint.RaiseException(exception);
            }
        }

        delegate void VoidDelegate();

        private void AddDemo(DemoDisplayInfo info)
        {
            var inputGesture = new MouseGesture(MouseAction.LeftDoubleClick, ModifierKeys.None);
            var inputBinding = new MouseBinding(_showDemoInfoCommand, inputGesture);
            var commandBinding = new CommandBinding();
            commandBinding.Command = _showDemoInfoCommand;
            commandBinding.Executed += (obj, args) => OnShowDemoInfo();
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = true; };

            var removeDemoItem = new MenuItem();
            removeDemoItem.Header = "Remove (del)";
            removeDemoItem.Command = _deleteDemoCommand;
            removeDemoItem.Click += (obj, args) => OnRemoveDemoClicked();

            var demosContextMenu = new ContextMenu();
            demosContextMenu.Items.Add(removeDemoItem);

            var item = new ListViewItem();
            item.Content = info;
            item.ContextMenu = demosContextMenu;
            item.InputBindings.Add(inputBinding);
            item.CommandBindings.Add(commandBinding);

            _demoListView.Items.Add(item);
        }

        private void DemoAddThreadImpl(object arg)
        {
            var filePaths = (List<string>)arg;
            var fileLengths = new List<long>();

            long totalBytesToParse = 0;
            foreach(var filePath in filePaths)
            {
                var length = new FileInfo(filePath).Length;
                fileLengths.Add(length);
                totalBytesToParse += length;
            }

            var progress = 0.0;
            var progressStep = 100.0 / (double)totalBytesToParse;

            var timer = new Stopwatch();
            timer.Start();

            for(int i = 0; i < filePaths.Count; ++i)
            {
                var filePath = filePaths[i];
                var fileLength = fileLengths[i];
                var demoDisplayInfo = new DemoDisplayInfo();
                demoDisplayInfo.Demo = new DemoInfo();
                demoDisplayInfo.FileName = Path.GetFileNameWithoutExtension(filePath);

                var protocol = GetProtocolFromFilePath(filePath);
                if(protocol == DemoProtocol.Invalid)
                {
                    LogError("Unrecognized protocol for demo '{0}'", Path.GetFileName(filePath));
                    progress += (double)fileLength * progressStep;
                    SetProgressThreadSafe(progress);    
                    continue;
                }

                Demo.ProgressCallback progressCb = (progressPc) =>
                {
                    if(timer.ElapsedMilliseconds < 50)
                    {
                        return _cancelJobValue;
                    }

                    timer.Stop();
                    timer.Reset();
                    timer.Start();

                    var realProgress = progress + (double)fileLength * progressStep * (double)progressPc;
                    SetProgressThreadSafe(realProgress);

                    return _cancelJobValue;
                };

                VoidDelegate itemAdder = delegate { AddDemo(demoDisplayInfo); };
                _demoListView.Dispatcher.Invoke(itemAdder);

                DemoInfo demoInfo = null;
                try
                {
                    var demo = new Demo(protocol);
                    demo.Parse(filePath, progressCb, DemoLoggingCallback);
                    demoInfo = CreateDemoInfo(filePath, demo);
                    demo.Destroy();
                }
                catch(SEHException exception)
                {
                    LogError("Caught an exception while parsing demo '{0}': {1}", demoDisplayInfo.FileName, exception.Message);
                    progress += (double)fileLength * progressStep;
                    SetProgressThreadSafe(progress);
                    VoidDelegate itemRemover = delegate { RemoveListViewItem(demoDisplayInfo, _demoListView); };
                    _demoListView.Dispatcher.Invoke(itemRemover);
                    continue;
                }

                if(_cancelJobValue != 0)
                {
                    VoidDelegate itemRemover = delegate { RemoveListViewItem(demoDisplayInfo, _demoListView); };
                    _demoListView.Dispatcher.Invoke(itemRemover);
                    break;
                }

                _demos.Add(demoInfo);

                progress += (double)fileLength * progressStep;
                SetProgressThreadSafe(progress);

                demoDisplayInfo.Demo = demoInfo;
            }

            EnableUiThreadSafe();
        }

        private static void RemoveListViewItem<T>(T info, ListView listView) where T : class
        {
            int idx = -1;
            for(int i = 0; i < listView.Items.Count; ++i)
            {
                var listViewItem = listView.Items[i] as ListViewItem;
                if(listViewItem == null)
                {
                    continue;
                }

                var itemContent = listViewItem.Content as T;
                if(itemContent != null)
                {
                    idx = i;
                    break;
                }
            }

            if(idx != -1)
            {
                listView.Items.RemoveAt(idx);
            }
        }

        private static T FindAnchestor<T>(DependencyObject current) where T : DependencyObject
        {
            do
            {
                if(current is T)
                {
                    return (T)current;
                }
                current = VisualTreeHelper.GetParent(current);
            }
            while(current != null);

            return null;
        }

        private void LoadConfig()
        {
            Serializer.FromXml("Config.xml", out _config);
        }

        private void SaveConfig()
        {
            int time = 0;
            if(GetOffsetSeconds(_startTimeOffsetEditBox.Text, out time))
            {
                _config.ChatCutStartOffset = time;
            }
            if(GetOffsetSeconds(_endTimeOffsetEditBox.Text, out time))
            {
                _config.ChatCutEndOffset = time;
            }

            _config.OutputToInputFolder = _outputModeCheckBox.IsChecked ?? false;
            _config.OutputFolder = _outputFolderTextBox.Text;

            Serializer.ToXml("Config.xml", _config);
        }

        public static int CompareTimeStrings(string a, string b)
        {
            int a2 = -1;
            if(!GetTimeSeconds(a, out a2))
            {
                return a.CompareTo(b);
            }

            int b2 = -1;
            if(!GetTimeSeconds(b, out b2))
            {
                return a.CompareTo(b);
            }

            return a2.CompareTo(b2);
        }

        private static bool GetTimeSeconds(string text, out int time)
        {
            time = -1;

            var minutesSecondsRegEx = new Regex(@"(\d+):(\d+)");
            var match = minutesSecondsRegEx.Match(text);
            if(match.Success)
            {
                int m = int.Parse(match.Groups[1].Value);
                int s = int.Parse(match.Groups[2].Value);
                time = m * 60 + s;

                return true;
            }

            var secondsRegEx = new Regex(@"(\d+)");
            match = secondsRegEx.Match(text);
            if(secondsRegEx.IsMatch(text))
            {
                time = int.Parse(match.Groups[1].Value);

                return true;
            }

            return false;
        }

        private static bool GetOffsetSeconds(string text, out int time)
        {
            time = -1;

            var secondsRegEx = new Regex(@"(\d+)");
            var match = secondsRegEx.Match(text);
            if(secondsRegEx.IsMatch(text))
            {
                time = int.Parse(match.Groups[1].Value);

                return true;
            }

            return false;
        }

        private void OnRemoveDemoClicked()
        {
            if(_demoListView.SelectedItems.Count == 0)
            {
                LogWarning("You must select 1 or more demos from the list before you can remove them");
                return;
            }

            var demoIndices = new List<int>();
            foreach(var item in _demoListView.SelectedItems)
            {
                var listViewItem = item as ListViewItem;
                if(listViewItem == null)
                {
                    continue;
                }

                var displayInfo = listViewItem.Content as DemoDisplayInfo;
                if(displayInfo == null)
                {
                    continue;
                }

                var index = _demos.FindIndex(d => d == displayInfo.Demo);
                if(index >= 0 && index < _demos.Count)
                {
                    demoIndices.Add(index);
                }
            }

            demoIndices.Sort();
            demoIndices.Reverse();

            foreach(var demoIndex in demoIndices)
            {
                _demoListView.Items.RemoveAt(demoIndex);
                _demos.RemoveAt(demoIndex);
            }
        }

        private void OnCutByTimeContextClicked(ListView listView)
        {
            var items = listView.SelectedItems;
            if(items.Count == 0)
            {
                return;
            }

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

            int startTime = int.MaxValue;
            int endTime = int.MinValue;
            foreach(var item in items)
            {
                var listViewItem = item as ListViewItem;
                if(listViewItem == null)
                {
                    continue;
                }

                var info = listViewItem.Content as TimedEventDisplayInfo;
                if(info == null)
                {
                    continue;
                }

                int time = 0;
                if(!ParseMinutesSeconds(info.Time, out time))
                {
                    continue;
                }

                startTime = Math.Min(startTime, time);
                endTime = Math.Max(endTime, time);
            }

            if(startTime == int.MaxValue && endTime == int.MinValue)
            {
                return;
            }

            startTime -= startOffset;
            endTime += endOffset;

            _startTimeEditBox.Text = FormatMinutesSeconds(startTime);
            _endTimeEditBox.Text = FormatMinutesSeconds(endTime);

            _tabControl.SelectedIndex = 3;
        }

        private static void CopyListViewRowsToClipboard(ListView listView)
        {
            var stringBuilder = new StringBuilder();

            foreach(var item in listView.SelectedItems)
            {
                var listViewItem = item as ListViewItem;
                if(listViewItem == null)
                {
                    continue;
                }

                var realItem = listViewItem.Content;
                if(realItem == null)
                {
                    continue;
                }

                stringBuilder.AppendLine(realItem.ToString());
            }

            var allRows = stringBuilder.ToString();
            var allRowsFixed = allRows.TrimEnd(new char[] { '\r', '\n' });

            Clipboard.SetDataObject(allRowsFixed, true);
        }

        private void OnCancelJobClicked()
        {
            _cancelJobValue = 1;
            LogWarning("Job cancelled!");
        }

        private string GenerateOutputFilePath(string inFilePath, string startTime, string endTime)
        {
            string result = "";
            VoidDelegate delayedCall = delegate { result = GenerateOutputFilePathImpl(inFilePath, startTime, endTime); };
            _window.Dispatcher.Invoke(delayedCall);

            return result;
        }

        private string GenerateOutputFilePathImpl(string inFilePath, string startTime, string endTime)
        {
            var outputSame = _outputModeCheckBox.IsChecked ?? false;
            if(!outputSame)
            {
                

                var outputFolder = _outputFolderTextBox.Text;
                if(Directory.Exists(outputFolder))
                {
                    return Path.Combine(outputFolder, Path.GetFileNameWithoutExtension(inFilePath)) + "_cut_" + startTime + "_" + endTime + Path.GetExtension(inFilePath);
                }
                else
                {
                    _outputModeCheckBox.IsChecked = true;
                    LogWarning("The selected output folder {0} is invalid, switching to the 'output at input folder' mode for you...", outputFolder);
                }
            }

            return Path.Combine(Path.GetDirectoryName(inFilePath), Path.GetFileNameWithoutExtension(inFilePath)) + "_cut_" + startTime + "_" + endTime + Path.GetExtension(inFilePath);
        }

        private void SetProgressThreadSafe(double value)
        {
            VoidDelegate valueSetter = delegate { _progressBar.Value = value; _progressBar.InvalidateVisual(); };
            _progressBar.Dispatcher.Invoke(valueSetter);
        }

        private static DemoProtocol GetProtocolFromFilePath(string filePath)
        {
            var extension = Path.GetExtension(filePath).ToLower();
            if(!ProtocolFileExtDic.ContainsKey(extension))
            {
                return DemoProtocol.Invalid;
            }

            var prot = ProtocolFileExtDic[extension];

            return ProtocolFileExtDic[extension];
        }

        private void LogMessage(string message, Color color)
        {
            VoidDelegate itemAdder = delegate 
            {
                var label = new Label();
                label.Foreground = new SolidColorBrush(color);
                label.Content = message;
                _logListBox.Items.Add(label);
                _logListBox.ScrollIntoView(label); 
            };

            _logListBox.Dispatcher.Invoke(itemAdder);
        }

        private void LogInfo(string message, params object[] args)
        {
            LogMessage(string.Format(message, args), Color.FromRgb(0, 0, 0));
        }

        private void LogWarning(string message, params object[] args)
        {
            LogMessage(string.Format(message, args), Color.FromRgb(255, 127, 0));
        }

        private void LogError(string message, params object[] args)
        {
            LogMessage(string.Format(message, args), Color.FromRgb(255, 0, 0));
        }

        private void DemoLoggingCallback(int logLevel, string message)
        {
            switch(logLevel)
            {
                case 3:
                    LogError("Critical error: " + message);
                    LogError("It is highly recommended you restart the application to not work with corrupt data in memory");
                    break;

                case 2:
                    LogError(message);
                    break;

                case 1:
                    LogWarning(message);
                    break;

                default:
                    LogInfo(message);
                    break;
            }
        }

        private string GetLog()
        {
            var stringBuilder = new StringBuilder();

            foreach(var item in _logListBox.Items)
            {
                var label = item as Label;
                if(label == null)
                {
                    continue;
                }

                var line = label.Content as string;
                if(line == null)
                {
                    continue;
                }

                stringBuilder.AppendLine(line);
            }

            return stringBuilder.ToString();
        }

        private void ClearLog()
        {
            _logListBox.Items.Clear();
        }

        private void CopyLog()
        {
            Clipboard.SetDataObject(GetLog(), true);
        }

        private void CopyLogSelection()
        {
            var label = _logListBox.SelectedItem as Label;
            if(label == null)
            {
                return;
            }

            var line = label.Content as string;
            if(line == null)
            {
                return;
            }

            Clipboard.SetDataObject(line, true);
        }

        private void SaveLog()
        {
            using(var saveFileDialog = new System.Windows.Forms.SaveFileDialog())
            {
                saveFileDialog.InitialDirectory = System.Environment.GetFolderPath(Environment.SpecialFolder.Desktop);
                saveFileDialog.Filter = "text file (*.txt)|*.txt";
                if(saveFileDialog.ShowDialog() != System.Windows.Forms.DialogResult.OK)
                {
                    return;
                }

                File.WriteAllText(saveFileDialog.FileName,  GetLog());
            }
        }
    }
}