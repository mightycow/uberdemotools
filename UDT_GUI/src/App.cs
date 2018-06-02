using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Navigation;


namespace Uber.DemoTools
{
    public class ChatRule
    {
        public string Operator = "Contains";
        public string Value = "";
        public bool CaseSensitive = false;
        public bool IgnoreColors = true;
        public bool SearchTeamMessages = false;
    }

    public class UdtConfig
    {
        public int CutStartOffset = 10;
        public int CutEndOffset = 10;
        public List<ChatRule> ChatRules = new List<ChatRule>();
        public bool OutputToInputFolder = true;
        public string OutputFolder = "";
        public bool SkipChatOffsetsDialog = false;
        public bool SkipScanFoldersRecursivelyDialog = false;
        public bool ScanFoldersRecursively = false;
        public int MaxThreadCount = 1;
        public bool MergeCutSectionsFromDifferentPatterns = true;
        public string InputFolder = "";
        public bool UseInputFolderAsDefaultBrowsingLocation = false;
        public bool OpenDemosFromInputFolderOnStartUp = false;
        public int PatternsSelectionBitMask = -1;
        public int FragCutMinFragCount = 3;
        public int FragCutTimeBetweenFrags = 5;
        public bool FragCutAllowSelfKills = false;
        public bool FragCutAllowTeamKills = false;
        public bool FragCutAllowAnyDeath = false;
        public int MidAirCutMinDistance = 300;
        public int MidAirCutMinAirTimeMs = 800;
        public bool MidAirCutAllowRocket = true;
        public bool MidAirCutAllowBFG = true;
        public bool AnalyzeOnLoad = true;
        public int MultiRailCutMinFragCount = 2;
        public bool ColorLogWarningsAndErrors = false;
        public int FlagCaptureMinCarryTimeMs = 0;
        public int FlagCaptureMaxCarryTimeMs = 10*60*1000; // 10 minutes.
        public bool FlagCaptureAllowBaseToBase = true;
        public bool FlagCaptureAllowMissingToBase = true;
        public float FlickRailMinSpeed = 800.0f; // Degrees/second.
        public int FlickRailMinSpeedSnaps = 2;
        public float FlickRailMinAngleDelta = 40.0f; // Degrees.
        public int FlickRailMinAngleDeltaSnaps = 2;
        public int MatchCutStartTimeOffsetMs = 10 * 1000;
        public int MatchCutEndTimeOffsetMs = 10 * 1000;
        public int TimeShiftSnapshotCount = 2;
        public string LastDemoOpenFolderPath = "";
        public uint JSONPlugInsEnabled = uint.MaxValue; // All enabled by default.
        public uint PerfStatsEnabled = 0; // All disabled by default.
        public uint CSharpPerfStatsEnabled = 0; // All disabled by default.
        public bool RunUpdaterAtStartUp = true;
    }

    public class UdtPrivateConfig
    {
        public Int32 PatternCutPlayerIndex = int.MinValue; // @NOTE: Some negative values have meaning already.
        public string PatternCutPlayerName = "";
        public UInt32 FragCutAllowedMeansOfDeaths = 0;
        public UDT_DLL.udtProtocol ConversionOutputProtocol = UDT_DLL.udtProtocol.Invalid;
        public bool QuitAfterFirstJob = false;
        public bool ForceAnalyzeOnLoad = false;
        public bool ForceSkipFolderScanDialog = false;
        public bool ForceScanFoldersRecursively = false;
    }

    public class CuttabbleByTimeDisplayInfo
    {
        public int GameStateIndex { get; set; }
        public string Time { get; set; }

        public virtual bool GetStartAndEndTimes(out int startTimeSec, out int endTimeSec)
        {
            startTimeSec = int.MaxValue;
            endTimeSec = int.MinValue;
            int timeSec = 0;
            if(!App.ParseMinutesSeconds(Time, out timeSec))
            {
                return false;
            }
            
            startTimeSec = timeSec;
            endTimeSec = timeSec;

            return true;
        }
    }

    public class Timeout
    {
        public Timeout(int startTimeMs, int endTimeMs)
        {
            StartTimeMs = startTimeMs;
            EndTimeMs = endTimeMs;
        }

        public int StartTimeMs = 0;
        public int EndTimeMs = 0;
    }

    public class MatchTimeInfo
    {
        public int GameStateIndex = 0;
        public int StartTimeMs = 0;
        public int EndTimeMs = 0;
        public int TimeLimit = 0; // In minutes, 0 when none is set.
        public bool HadOvertime = false;
        public bool RoundBasedMode = false;
        public readonly List<Timeout> TimeOuts = new List<Timeout>();
    }

    public class DemoInfo
    {
        // Always set.
        public int InputIndex = 0;
        public string FilePath = "?";
        public string Protocol = "?";
        public UDT_DLL.udtProtocol ProtocolNumber = UDT_DLL.udtProtocol.Invalid;

        // Only set when the demo was parsed.
        public bool Analyzed = false;
        public bool TeamGameType = false;
        public List<ChatEventDisplayInfo> ChatEvents = new List<ChatEventDisplayInfo>();
        public List<FragEventDisplayInfo> FragEvents = new List<FragEventDisplayInfo>();
        public List<Tuple<string, string>> Generic = new List<Tuple<string, string>>();
        public List<UInt32> GameStateFileOffsets = new List<UInt32>();
        public List<Tuple<int, int>> GameStateSnapshotTimesMs = new List<Tuple<int, int>>();
        public List<DemoStatsInfo> MatchStats = new List<DemoStatsInfo>();
        public List<CommandDisplayInfo> Commands = new List<CommandDisplayInfo>();
        public List<MatchTimeInfo> MatchTimes = new List<MatchTimeInfo>();
        public List<FlagCaptureDisplayInfo> FlagCaptures = new List<FlagCaptureDisplayInfo>();
        public List<ScoreDisplayInfo> Scores = new List<ScoreDisplayInfo>();
        public List<TeamScoreDisplayInfo> TeamScores = new List<TeamScoreDisplayInfo>();
    }

    public class DemoInfoListView : ListView
    {
        private Brush _originalBackground;
        private Brush _customBackground;

        public DemoInfoListView()
        {
            Initialized += (obj, arg) => { _originalBackground = Background; };
        }

        public void SetDemoAnalyzed(bool analyzed)
        {
            if(analyzed)
            {
                Background = _originalBackground;
                return;
            }

            if(_customBackground == null)
            {
                var label = new Label { Content = "Demo was not analyzed.", HorizontalAlignment = HorizontalAlignment.Center, VerticalAlignment = VerticalAlignment.Center };
                var brush = new VisualBrush(label) { Stretch = Stretch.None, Opacity = 0.5 };
                _customBackground = brush;
            }

            Background = _customBackground;
        }
    }

    public class App
    {
        private const string GuiVersion = "0.7.2";
        private const uint MinimumDllVersionMajor = 1;
        private const uint MinimumDllVersionMinor = 3;
        private const uint MinimumDllVersionRevision = 1;
        private readonly string DllVersion = UDT_DLL.GetVersion();

        private static readonly List<string> DemoExtensions = new List<string>
        {
            ".dm3",
            ".dm_48",
            ".dm_66",
            ".dm_67",
            ".dm_68",
            ".dm_73",
            ".dm_90",
            ".dm_91"
        };

        private static readonly List<string> DemoExtensionsQ3 = new List<string>
        {
            ".dm3",
            ".dm_48",
            ".dm_66",
            ".dm_67",
            ".dm_68"
        };

        private static readonly List<string> DemoExtensionsQL = new List<string>
        {
            ".dm_73",
            ".dm_90",
            ".dm_91"
        };

        private static readonly Dictionary<string, UDT_DLL.udtProtocol> ProtocolFileExtDic = new Dictionary<string, UDT_DLL.udtProtocol>
        {
            { ".dm3",   UDT_DLL.udtProtocol.Dm3  },
            { ".dm_48", UDT_DLL.udtProtocol.Dm48 },
            { ".dm_66", UDT_DLL.udtProtocol.Dm66 },
            { ".dm_67", UDT_DLL.udtProtocol.Dm67 },
            { ".dm_68", UDT_DLL.udtProtocol.Dm68 },
            { ".dm_73", UDT_DLL.udtProtocol.Dm73 },
            { ".dm_90", UDT_DLL.udtProtocol.Dm90 },
            { ".dm_91", UDT_DLL.udtProtocol.Dm91 }
        };

        private static readonly List<UDT_DLL.udtProtocol> ValidWriteProtocols = new List<UDT_DLL.udtProtocol>
        {
            { UDT_DLL.udtProtocol.Dm66 },
            { UDT_DLL.udtProtocol.Dm67 },
            { UDT_DLL.udtProtocol.Dm68 },
            { UDT_DLL.udtProtocol.Dm73 },
            { UDT_DLL.udtProtocol.Dm90 },
            { UDT_DLL.udtProtocol.Dm91 }
        };

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

        private UdtConfig _config = new UdtConfig();
        private UdtPrivateConfig _privateConfig = new UdtPrivateConfig();
        private Application _application = null;
        private Thread _jobThread = null;
        private Window _window = null;
        private ListView _demoListView = null;
        private Brush _demoListViewBackground = null;
        private ListView _infoListView = null;
        private ListBox _logListBox = null;
        private ProgressBar _progressBar = null;
        private Button _cancelJobButton = null;
        private GroupBox _progressGroupBox = null;
        private TextBlock _progressTimeElapsedTextBlock = null;
        private TextBlock _progressTimeRemainingTextBlock = null;
        private DockPanel _rootPanel = null;
        private TabControl _tabControl = null;
        private List<FrameworkElement> _rootElements = new List<FrameworkElement>();
        private List<DemoInfo> _demos = new List<DemoInfo>();
        private AlternatingListBoxBackground _altListBoxBg = null;
        private List<AppComponent> _appComponents = new List<AppComponent>();
        private AppComponent _cutByTimeComponent = null;
        private IntPtr _mainThreadContext = IntPtr.Zero;
        private bool _usingDarkTheme = false;
        private Stopwatch _threadedJobTimer = new Stopwatch();
        private Stopwatch _threadedJobTitleTimer = new Stopwatch();
        private static RoutedCommand _cutByChatCommand = new RoutedCommand();
        private static RoutedCommand _deleteDemoCommand = new RoutedCommand();
        private static RoutedCommand _splitDemoCommand = new RoutedCommand();
        private static RoutedCommand _revealDemoCommand = new RoutedCommand();
        private static RoutedCommand _analyzeDemoCommand = new RoutedCommand();
        private static RoutedCommand _convertDemo68Command = new RoutedCommand();
        private static RoutedCommand _convertDemo90Command = new RoutedCommand();
        private static RoutedCommand _mergeDemosCommand = new RoutedCommand();
        private static RoutedCommand _JSONExportCommand = new RoutedCommand();
        private static RoutedCommand _selectAllDemosCommand = new RoutedCommand();
        private static RoutedCommand _showDemoInfoCommand = new RoutedCommand();
        private static RoutedCommand _clearLogCommand = new RoutedCommand();
        private static RoutedCommand _copyLogCommand = new RoutedCommand();
        private static RoutedCommand _copyChatCommand = new RoutedCommand();
        private static RoutedCommand _copyFragCommand = new RoutedCommand();
        private IntPtr _cancelOperation = IntPtr.Zero;

        public UDT_DLL.udtParseArg ParseArg = new UDT_DLL.udtParseArg();

        static public App Instance { get; private set; }

        public UdtConfig Config
        {
            get { return _config; }
        }

        public UdtPrivateConfig PrivateConfig
        {
            get { return _privateConfig; }
        }

        public Window MainWindow
        {
            get { return _window; }
        }

        public DemoInfo SelectedDemo
        {
            get
            {
                var index = _demoListView.SelectedIndex;
                return (index == -1) ? null : _demos[index];
            }
        }

        public List<DemoInfo> SelectedDemos
        {
            get
            {
                var items = _demoListView.SelectedItems;
                if(items.Count == 0)
                {
                    return null;
                }

                var demos = new List<DemoInfo>();
                foreach(var item in items)
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

                    demos.Add(displayInfo.Demo);
                }

                return demos;
            }
        }

        public List<DemoInfo> SelectedWriteDemos
        {
            get
            {
                return GetSelectedWriteDemos(false);
            }
        }

        public List<DemoInfo> GetSelectedWriteDemos(bool silent)
        {
            var demos = SelectedDemos;
            if(demos == null)
            {
                if(!silent) LogError("No demo was selected");
                return null;
            }

            demos = demos.FindAll(d => IsValidWriteProtocol(d.ProtocolNumber));
            if(demos.Count == 0)
            {
                if(!silent) LogError("No selected demo had a protocol version compatible with the requested operation");
                return null;
            }

            return demos;
        }

        private static Color MultiplyRGBSpace(Color color, float value)
        {
            var rF = Math.Min((color.R / 255.0f) * value, 1.0f);
            var gF = Math.Min((color.G / 255.0f) * value, 1.0f);
            var bF = Math.Min((color.B / 255.0f) * value, 1.0f);
            var r = (byte)(rF * 255.0f);
            var g = (byte)(gF * 255.0f);
            var b = (byte)(bF * 255.0f);

            return Color.FromRgb(r, g, b);
        }

        public App(string[] cmdLineArgs)
        {
            Instance = this;

            var mutex = Updater.UpdaterHelper.TryOpenNamedMutex();
            if(mutex != null)
            {
                // Updater still running, try waiting for a bit.
                Thread.Sleep(1000);
                mutex = Updater.UpdaterHelper.TryOpenNamedMutex();
                if(mutex != null)
                {
                    // No luck. Might be that the user was a bit impatient. :)
                    Process.GetCurrentProcess().Kill();
                }
            }

            var updaterFileName = Updater.UpdaterHelper.ExeFileName;
            var updaterFileNameNewExt = updaterFileName + Updater.UpdaterHelper.NewFileExtension;
            var updaterFileNameOldExt = updaterFileName + Updater.UpdaterHelper.OldFileExtension;
            if(File.Exists(updaterFileNameNewExt))
            {
                if(File.Exists(updaterFileName))
                {
                    TryDeleteFile(updaterFileName);
                }
                TryMoveFile(updaterFileNameNewExt, updaterFileName);
            }
            TryDeleteFile(updaterFileNameNewExt);
            TryDeleteFile(updaterFileNameOldExt);

            PresentationTraceSources.DataBindingSource.Switch.Level = SourceLevels.Critical;

            UDT_DLL.SetFatalErrorHandler(FatalErrorHandler);
            UDT_DLL.InitLibrary();

            var desiredVersion = new UDT_DLL.Version(MinimumDllVersionMajor, MinimumDllVersionMinor, MinimumDllVersionRevision);
            var dllVersion = UDT_DLL.GetVersionNumbers();
            var displayVersionWarning = false;
            var dllVersionComparison = dllVersion.CompareTo(desiredVersion);
            if(dllVersionComparison < 0)
            {
                var message = string.Format("UDT.dll version is {0} but {1} is required at a minimum.", DllVersion, desiredVersion.ToString());
                MessageBox.Show(message, "UDT_GUI: UDT.dll too old", MessageBoxButton.OK, MessageBoxImage.Error);
                Process.GetCurrentProcess().Kill();
            }
            else if(dllVersionComparison > 0)
            {
                displayVersionWarning = true;
            }

            _cancelOperation = Marshal.AllocHGlobal(4);
            Marshal.WriteInt32(_cancelOperation, 0);

            LoadConfig();
            if(Config.RunUpdaterAtStartUp)
            {
                CheckForUpdate(true);
            }

            var listItemColor1 = SystemColors.WindowColor;
            var listItemColor2 = listItemColor1;
            var listColorSum = (int)listItemColor1.R + (int)listItemColor1.G + (int)listItemColor1.B;
            _usingDarkTheme = listColorSum < 385;
            if(_usingDarkTheme)
            {
                if(listColorSum < 96)
                {
                    listItemColor2 = Color.Add(listItemColor2, Color.FromRgb(31, 31, 31));
                    
                }
                else
                {
                    listItemColor2 = MultiplyRGBSpace(listItemColor1, 1.15f);
                }
            }
            else
            {
                listItemColor2 = MultiplyRGBSpace(listItemColor1, 0.87f);
            }
            _altListBoxBg = new AlternatingListBoxBackground(listItemColor1, listItemColor2);

            var manageDemosTab = new TabItem();
            manageDemosTab.Header = "Manage";
            manageDemosTab.Content = CreateManageDemosTab();
            
            var infoTab = new TabItem();
            infoTab.Header = "Info";
            infoTab.Content = CreateInfoTab();

            _cutByTimeComponent = new CutByTimeComponent(this);
            _appComponents.Add(_cutByTimeComponent);
            var cutByTimeTab = new TabItem();
            cutByTimeTab.Header = "Cut by Time";
            cutByTimeTab.Content = _cutByTimeComponent.RootControl;

            var cutByPattern = new CutByPatternComponent(this);
            _appComponents.Add(cutByPattern);
            var cutByPatternTab = new TabItem();
            cutByPatternTab.Header = "Pattern Search";
            cutByPatternTab.Content = cutByPattern.RootControl;

            var modifiers = new ModifierComponent(this);
            _appComponents.Add(modifiers);
            var modifiersTab = new TabItem();
            modifiersTab.Header = "Modifiers";
            modifiersTab.Content = modifiers.RootControl;

            var settings = new AppSettingsComponent(this);
            _appComponents.Add(settings);
            var settingsTab = new TabItem();
            settingsTab.Header = "Settings";
            settingsTab.Content = settings.RootControl;

            var patterns = new PatternsComponent(this);
            _appComponents.Add(patterns);
            var patternsTab = new TabItem();
            patternsTab.Header = "Patterns";
            patternsTab.Content = patterns.RootControl;

            var searchResults = new SearchResultsComponent(this);
            _appComponents.Add(searchResults);
            var searchResultsTab = new TabItem();
            searchResultsTab.Header = "Search Results";
            searchResultsTab.Content = searchResults.RootControl;
            
            var tabControl = new TabControl();
            _tabControl = tabControl;
            tabControl.HorizontalAlignment = HorizontalAlignment.Stretch;
            tabControl.VerticalAlignment = VerticalAlignment.Stretch;
            tabControl.Margin = new Thickness(5);
            tabControl.Items.Add(manageDemosTab);
            tabControl.Items.Add(infoTab);
            tabControl.Items.Add(cutByTimeTab);
            tabControl.Items.Add(cutByPatternTab);
            tabControl.Items.Add(patternsTab);
            tabControl.Items.Add(modifiersTab);
            tabControl.Items.Add(searchResultsTab);
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

            var viewHelpMenuItem = new MenuItem();
            viewHelpMenuItem.Header = "_View Online Help";
            viewHelpMenuItem.Click += (obj, arg) => ShowHelpPage();
            viewHelpMenuItem.ToolTip = new ToolTip { Content = "Open a new tab with the help in your default browser" };

            var aboutMenuItem = new MenuItem();
            aboutMenuItem.Header = "_About";
            aboutMenuItem.Click += (obj, arg) => ShowAboutWindow();
            aboutMenuItem.ToolTip = new ToolTip { Content = "Learn more about this awesome application" };

            var updateMenuItem = new MenuItem();
            updateMenuItem.Header = "Check for _Update";
            updateMenuItem.Click += (obj, arg) => CheckForUpdate(false);
            updateMenuItem.ToolTip = new ToolTip { Content = "See if a newer version is available" };

#if DEBUG
            var forceUpdateMenuItem = new MenuItem();
            forceUpdateMenuItem.Header = "_Force Update";
            forceUpdateMenuItem.Click += (obj, arg) => CheckForUpdateFromFakeOldVersion();
            forceUpdateMenuItem.ToolTip = new ToolTip { Content = "Force an update for testing purposes" };
#endif

            var helpMenuItem = new MenuItem();
            helpMenuItem.Header = "_Help";
            helpMenuItem.Items.Add(viewHelpMenuItem);
            helpMenuItem.Items.Add(new Separator());
            helpMenuItem.Items.Add(updateMenuItem);
#if DEBUG
            helpMenuItem.Items.Add(forceUpdateMenuItem);
#endif
            helpMenuItem.Items.Add(new Separator());
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
            logListBox.SelectionMode = SelectionMode.Extended;
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

            var progressTimeElapsedTextBlock = new TextBlock();
            _progressTimeElapsedTextBlock = progressTimeElapsedTextBlock;
            progressTimeElapsedTextBlock.Margin = new Thickness(5, 0, 0, 0);
            progressTimeElapsedTextBlock.Text = "Time elapsed: ";

            var progressTimeRemainingTextBlock = new TextBlock();
            _progressTimeRemainingTextBlock = progressTimeRemainingTextBlock;
            progressTimeRemainingTextBlock.Text = "Time remaining: ";
            progressTimeRemainingTextBlock.HorizontalAlignment = HorizontalAlignment.Center;

            var progressTimeRow = new Grid();
            progressTimeRow.RowDefinitions.Add(new RowDefinition());
            progressTimeRow.ColumnDefinitions.Add(new ColumnDefinition());
            progressTimeRow.ColumnDefinitions.Add(new ColumnDefinition());
            progressTimeRow.ColumnDefinitions.Add(new ColumnDefinition());
            progressTimeRow.Children.Add(progressTimeElapsedTextBlock);
            progressTimeRow.Children.Add(progressTimeRemainingTextBlock);
            Grid.SetRow(progressTimeElapsedTextBlock, 0);
            Grid.SetColumn(progressTimeElapsedTextBlock, 0);
            Grid.SetRow(progressTimeRemainingTextBlock, 0);
            Grid.SetColumn(progressTimeRemainingTextBlock, 1);

            var progressPanel = new DockPanel();
            progressPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            progressPanel.VerticalAlignment = VerticalAlignment.Bottom;
            progressPanel.LastChildFill = true;
            progressPanel.Children.Add(progressTimeRow);
            progressPanel.Children.Add(cancelJobButton);
            progressPanel.Children.Add(progressBar);
            DockPanel.SetDock(cancelJobButton, Dock.Right);
            DockPanel.SetDock(progressTimeRow, Dock.Top);

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
            InitDemoListDeleteCommand();
            InitDemoListSplitCommand();
            InitDemoListRevealCommand();
            InitDemoListAnalyzeCommand();
            InitDemoListConversion68Command();
            InitDemoListConversion90Command();
            InitDemoListMergeCommand();
            InitDemoListJSONExportCommand();
            InitDemoListSelectAllCommand();
            InitDemoListContextMenu();
            
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
            _altListBoxBg.ApplyTo(_demoListView);
            _altListBoxBg.ApplyTo(_logListBox);
            foreach(var component in _appComponents)
            {
                var listViews = component.AllListViews;
                if(listViews == null)
                {
                    continue;
                }

                foreach(var listView in listViews)
                {
                    _altListBoxBg.ApplyTo(listView);
                }
            }

            var label = new Label { Content = "You can drag'n'drop files and folders here.", HorizontalAlignment = HorizontalAlignment.Center, VerticalAlignment = VerticalAlignment.Center };
            var brush = new VisualBrush(label) { Stretch = Stretch.None, Opacity = 0.5 };
            _demoListView.Background = brush;

            var window = new Window();

            TextOptions.SetTextRenderingMode(window, TextRenderingMode.ClearType);
            TextOptions.SetTextHintingMode(window, TextHintingMode.Fixed);
            TextOptions.SetTextFormattingMode(window, TextFormattingMode.Display);

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

#if UDT_X64
            const string arch = "x64";
#else
            const string arch = "x86";
#endif
            LogInfo("UDT {0} is now operational!", arch);
            LogInfo("GUI version: " + GuiVersion);
            LogInfo("DLL version: " + DllVersion);
            if(displayVersionWarning)
            {
                LogWarning("Expected DLL version was {0}. Compatibility is not guaranteed.", desiredVersion.ToString());
            }

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
                else if(arg.ToLower() == "/quitafterfirstjob")
                {
                    _privateConfig.QuitAfterFirstJob = true;
                }
                else if(arg.ToLower() == "/forceanalyzeonload")
                {
                    _privateConfig.ForceAnalyzeOnLoad = true;
                }
                else if(arg.ToLower() == "/forceskipfolderscandialog")
                {
                    _privateConfig.ForceSkipFolderScanDialog = true;
                }
                else if(arg.ToLower() == "/forcescanfoldersrecursively")
                {
                    _privateConfig.ForceScanFoldersRecursively = true;
                }
            }

            if(cmdLineArgs.Length == 0 && 
                _config.OpenDemosFromInputFolderOnStartUp &&
                !string.IsNullOrWhiteSpace(_config.InputFolder) &&
                Directory.Exists(_config.InputFolder))
            {
                folderPaths.Add(_config.InputFolder);
            }

            AddDemos(filePaths, folderPaths);
        }

        public static FrameworkElement CreateContextMenuHeader(string left, string right)
        {
            var leftItem = new TextBlock { Text = left, Margin = new Thickness(0, 0, 10, 0) };
            var rightItem = new TextBlock { Text = right, HorizontalAlignment = HorizontalAlignment.Right };

            var panel = new DockPanel();
            panel.LastChildFill = true;
            panel.Children.Add(leftItem);
            panel.Children.Add(rightItem);
            DockPanel.SetDock(leftItem, Dock.Left);
            DockPanel.SetDock(rightItem, Dock.Right);

            return panel;
        }

        private void InitDemoListContextMenu()
        {
            var removeDemoItem = new MenuItem();
            removeDemoItem.Header = CreateContextMenuHeader("Remove Selected", "(Delete)");
            removeDemoItem.Command = _deleteDemoCommand;

            var splitDemoItem = new MenuItem();
            splitDemoItem.Header = "Split Selected";
            splitDemoItem.Command = _splitDemoCommand;
            splitDemoItem.Click += (obj, args) => OnSplitDemoClicked();

            var revealDemoItem = new MenuItem();
            revealDemoItem.Header = "Reveal in File Explorer";
            revealDemoItem.Command = _revealDemoCommand;
            revealDemoItem.Click += (obj, args) => OnRevealDemoClicked();

            var analyzeDemoItem = new MenuItem();
            analyzeDemoItem.Header = "Analyze Selected";
            analyzeDemoItem.Command = _analyzeDemoCommand;
            analyzeDemoItem.Click += (obj, args) => OnAnalyzeDemoClicked();

            var convertDemo68Item = new MenuItem();
            convertDemo68Item.Header = "Convert to *.dm__68";
            convertDemo68Item.Command = _convertDemo68Command;
            convertDemo68Item.Click += (obj, args) => OnConvertDemosClicked(UDT_DLL.udtProtocol.Dm68);

            var convertDemo90Item = new MenuItem();
            convertDemo90Item.Header = "Convert to *.dm__90";
            convertDemo90Item.Command = _convertDemo90Command;
            convertDemo90Item.Click += (obj, args) => OnConvertDemosClicked(UDT_DLL.udtProtocol.Dm90);

            var mergeDemosItem = new MenuItem();
            mergeDemosItem.Header = "Merge Selected";
            mergeDemosItem.Command = _mergeDemosCommand;
            mergeDemosItem.Click += (obj, args) => OnMergeDemosClicked();

            var jsonExportItem = new MenuItem();
            jsonExportItem.Header = "Save Analysis to .JSON";
            jsonExportItem.Command = _JSONExportCommand;
            jsonExportItem.Click += (obj, args) => OnExportDemosToJSONClicked();

            var selectAllDemosItem = new MenuItem();
            selectAllDemosItem.Header = CreateContextMenuHeader("Select All", "(Ctrl+A)");
            selectAllDemosItem.Command = _selectAllDemosCommand;
            selectAllDemosItem.Click += (obj, args) => { if(_demoListView.SelectionMode != SelectionMode.Single) _demoListView.SelectAll(); };

            var demosContextMenu = new ContextMenu();
            demosContextMenu.Items.Add(analyzeDemoItem);
            demosContextMenu.Items.Add(convertDemo68Item);
            demosContextMenu.Items.Add(convertDemo90Item);
            demosContextMenu.Items.Add(mergeDemosItem);
            demosContextMenu.Items.Add(jsonExportItem);
            demosContextMenu.Items.Add(removeDemoItem);
            demosContextMenu.Items.Add(new Separator());
            demosContextMenu.Items.Add(splitDemoItem);
            demosContextMenu.Items.Add(new Separator());
            demosContextMenu.Items.Add(revealDemoItem);
            demosContextMenu.Items.Add(new Separator());
            demosContextMenu.Items.Add(selectAllDemosItem);

            _demoListView.ContextMenu = demosContextMenu;
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

                if(_privateConfig.ForceSkipFolderScanDialog || _config.SkipScanFoldersRecursivelyDialog)
                {
                    recursive = _privateConfig.ForceScanFoldersRecursively || _config.ScanFoldersRecursively;
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
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = _demoListView.SelectedItems.Count > 0; };
            commandBinding.Executed += (obj, args) => OnRemoveDemoClicked();
            _demoListView.InputBindings.Add(inputBinding);
            _demoListView.CommandBindings.Add(commandBinding);
        }

        private bool CanExecuteAnalyzeCommand()
        {
            var demos = SelectedDemos;
            if(demos == null)
            {
                return false;
            }

            return demos.Exists(d => !d.Analyzed);
        }

        private void InitDemoListAnalyzeCommand()
        {
            var commandBinding = new CommandBinding();
            commandBinding.Command = _analyzeDemoCommand;
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = CanExecuteAnalyzeCommand(); };
            _demoListView.CommandBindings.Add(commandBinding);
        }

        private bool CanExecuteConvertCommand(UDT_DLL.udtProtocol outputFormat)
        {
            var demos = SelectedDemos;
            if(demos == null)
            {
                return false;
            }

            return demos.Exists(d => IsValidInputFormatForConverter(outputFormat, d.ProtocolNumber));
        }

        private bool CanExecuteMergeCommand()
        {
            var demos = GetSelectedWriteDemos(true);
            if(demos == null)
            {
                return false;
            }

            if(demos.Count < 2)
            {
                return false;
            }

            var firstProtocol = demos[0].ProtocolNumber;
            for(var i = 1; i < demos.Count; ++i)
            {
                if(demos[i].ProtocolNumber != firstProtocol)
                {
                    return false;
                }
            }

            return true;
        }

        private void InitDemoListConversion68Command()
        {
            var commandBinding = new CommandBinding();
            commandBinding.Command = _convertDemo68Command;
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = CanExecuteConvertCommand(UDT_DLL.udtProtocol.Dm68); };
            _demoListView.CommandBindings.Add(commandBinding);
        }

        private void InitDemoListConversion90Command()
        {
            var commandBinding = new CommandBinding();
            commandBinding.Command = _convertDemo90Command;
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = CanExecuteConvertCommand(UDT_DLL.udtProtocol.Dm90); };
            _demoListView.CommandBindings.Add(commandBinding);
        }

        private void InitDemoListMergeCommand()
        {
            var commandBinding = new CommandBinding();
            commandBinding.Command = _mergeDemosCommand;
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = CanExecuteMergeCommand(); };
            _demoListView.CommandBindings.Add(commandBinding);
        }

        private void InitDemoListJSONExportCommand()
        {
            var commandBinding = new CommandBinding();
            commandBinding.Command = _JSONExportCommand;
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = true; };
            _demoListView.CommandBindings.Add(commandBinding);
        }

        private bool CanExecuteSplitCommand()
        {
            var demos = GetSelectedWriteDemos(true);
            if(demos == null || demos.Count > 1)
            {
                return false;
            }
            var demo = demos[0];

            return demo.GameStateFileOffsets.Count > 1 || !demo.Analyzed;
        }

        private void InitDemoListSplitCommand()
        {
            var commandBinding = new CommandBinding();
            commandBinding.Command = _splitDemoCommand;
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = CanExecuteSplitCommand(); };
            _demoListView.CommandBindings.Add(commandBinding);
        }

        private bool CanExecuteRevealCommand()
        {
            var demos = SelectedDemos;

            return demos != null && demos.Count == 1;
        }

        private void InitDemoListRevealCommand()
        {
            var commandBinding = new CommandBinding();
            commandBinding.Command = _revealDemoCommand;
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = CanExecuteRevealCommand(); };
            _demoListView.CommandBindings.Add(commandBinding);
        }

        private void InitDemoListSelectAllCommand()
        {
            var commandBinding = new CommandBinding();
            commandBinding.Command = _selectAllDemosCommand;
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = _demos.Count > 0; };
            _demoListView.CommandBindings.Add(commandBinding);
        }

        private void InitLogListBoxClearCommand()
        {
            var commandBinding = new CommandBinding();
            commandBinding.Command = _clearLogCommand;
            commandBinding.Executed += (obj, args) => ClearLog();
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = true; };
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
            clearLogMenuItem.Header = "Clear";
            clearLogMenuItem.Command = _clearLogCommand;
            clearLogMenuItem.Click += (obj, args) => ClearLog();

            var copyLogMenuItem = new MenuItem();
            copyLogMenuItem.Header = CreateContextMenuHeader("Copy", "Ctrl+C");
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
            Marshal.WriteInt32(_cancelOperation, 1);
            if(_currentJob != null)
            {
                _currentJob.Cancel();
            }
            SaveConfig();
            _application.Shutdown();
        }

        private void OnTabSelectionChanged()
        {
            var tabIndex = _tabControl.SelectedIndex;
            var multiMode = tabIndex == 0 || tabIndex > 1;
            var tabItems = _tabControl.Items;
            if(tabIndex > 1)
            {
                var selectedItem = _tabControl.Items[tabIndex];
                var tabComponent = _appComponents.Find(c => IsMatchingTab(c, selectedItem));
                if(tabComponent != null)
                {
                    multiMode = tabComponent.MultiDemoMode;
                }
            }
  
            _demoListView.SelectionMode = multiMode ? SelectionMode.Extended : SelectionMode.Single;
        }

        private bool IsMatchingTab(AppComponent component, object tabItem)
        {
            var tab = tabItem as TabItem;
            if(tab == null)
            {
                return false;
            }

            return component.RootControl == tab.Content;
        }

        private void OnOpenDemo()
        {
            using(var openFileDialog = new System.Windows.Forms.OpenFileDialog())
            {
                var extensionsQ3 = "*" + DemoExtensionsQ3[0];
                for(var i = 1; i < DemoExtensionsQ3.Count; ++i)
                {
                    extensionsQ3 += ";*" + DemoExtensionsQ3[i];
                }

                var extensionsQL = "*" + DemoExtensionsQL[0];
                for(var i = 1; i < DemoExtensionsQL.Count; ++i)
                {
                    extensionsQL += ";*" + DemoExtensionsQL[i];
                }

                var folderPath = GetDefaultBrowsingFolder();
                openFileDialog.CheckPathExists = true;
                openFileDialog.Multiselect = true;
                openFileDialog.InitialDirectory = folderPath ?? Environment.GetFolderPath(Environment.SpecialFolder.Desktop);
                openFileDialog.Filter = string.Format("Quake 3 demos ({0})|{0}|Quake Live demos ({1})|{1}", extensionsQ3, extensionsQL);
                if(openFileDialog.ShowDialog() != System.Windows.Forms.DialogResult.OK)
                {
                    return;
                }

                if(openFileDialog.FileNames.Length > 0)
                {
                    SaveBrowsingFolder(Path.GetDirectoryName(openFileDialog.FileNames[0]));
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
                var folderPath = GetDefaultBrowsingFolder();
                openFolderDialog.Description = "Browse for a folder containing demo files";
                openFolderDialog.ShowNewFolderButton = true;
                if(folderPath != null)
                {
                    openFolderDialog.SelectedPath = folderPath;
                }
                if(openFolderDialog.ShowDialog() != System.Windows.Forms.DialogResult.OK)
                {
                    return;
                }

                SaveBrowsingFolder(openFolderDialog.SelectedPath);
                AddDemos(new List<string>(), new List<string> { openFolderDialog.SelectedPath });
            }
        }

        private void ShowHelpPage()
        {
            try
            {
                Process.Start(@"https://github.com/mightycow/uberdemotools/blob/develop/README.md");
            }
            catch(Exception exception)
            {
                LogError("Failed to open the online help: " + exception.Message);
            }
        }

        private void CheckForUpdate(bool fromStartUp)
        {
#if UDT_X64
            const string arch = "x64";
#else
            const string arch = "x86";
#endif

            try
            {
                var startUpArg = fromStartUp ? Updater.UpdaterHelper.NoMessageBoxIfCurrentArg : "blabla";
                var arguments = string.Join(" ", DllVersion, GuiVersion, arch, Process.GetCurrentProcess().Id.ToString(), startUpArg);
                Process.Start("UDT_GUI_Updater.exe", arguments);
            }
            catch(Exception exception)
            {
                if(!fromStartUp)
                {
                    LogError("Failed to open the updater: " + exception.Message);
                }
            }
        }

#if DEBUG
        private void CheckForUpdateFromFakeOldVersion()
        {
#if UDT_X64
            const string arch = "x64";
#else
            const string arch = "x86";
#endif

            try
            {
                var arguments = string.Join(" ", "0.1.0", "0.1.0", arch, Process.GetCurrentProcess().Id.ToString(), "blabla");
                Process.Start("UDT_GUI_Updater.exe", arguments);
            }
            catch(Exception exception)
            {
                LogError("Failed to open the updater: " + exception.Message);
            }
        }
#endif

        private void ShowAboutWindow()
        {
            var udtIcon = new System.Windows.Controls.Image();
            udtIcon.HorizontalAlignment = HorizontalAlignment.Right;
            udtIcon.VerticalAlignment = VerticalAlignment.Top;
            udtIcon.Margin = new Thickness(10, 0, 0, 0);
            udtIcon.Stretch = Stretch.None;
            udtIcon.Source = UDT.Properties.Resources.UDTIcon.ToImageSource();

            var guiVersion = new TextBlock { Text = "UDT GUI Version " + GuiVersion };
            var dllVersion = new TextBlock { Text = "UDT DLL Version " + DllVersion };
            guiVersion.Margin = new Thickness(0, 10, 0, 0);
            dllVersion.Margin = new Thickness(0, 5, 0, 0);

            var udtPanel = new DockPanel();
            udtPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            udtPanel.VerticalAlignment = VerticalAlignment.Top;
            udtPanel.LastChildFill = false;
            udtPanel.Children.Add(udtIcon);
            udtPanel.Children.Add(guiVersion);
            udtPanel.Children.Add(dllVersion);
            DockPanel.SetDock(udtIcon, Dock.Right);
            DockPanel.SetDock(guiVersion, Dock.Top);
            DockPanel.SetDock(dllVersion, Dock.Top);

            var zipStorerIcon = new System.Windows.Controls.Image();
            zipStorerIcon.HorizontalAlignment = HorizontalAlignment.Right;
            zipStorerIcon.VerticalAlignment = VerticalAlignment.Top;
            zipStorerIcon.Margin = new Thickness(10, 0, 0, 0);
            zipStorerIcon.Stretch = Stretch.None;
            zipStorerIcon.Source = UDT.Properties.Resources.ZipStorerIcon.ToImageSource();

            const string ZipStorerUrl = "https://zipstorer.codeplex.com/";
            var zipStorerCredit = new TextBlock { Text = "The updater uses ZipStorer by Jaime Olivares" };
            var zipStorerHyperLink = new Hyperlink(new Run(ZipStorerUrl));
            zipStorerHyperLink.NavigateUri = new Uri(ZipStorerUrl);
            zipStorerHyperLink.RequestNavigate += (obj, args) => HandleLinkClick(args);
            var zipStorerLink = new TextBlock();
            zipStorerLink.Inlines.Add(zipStorerHyperLink);
            zipStorerCredit.Margin = new Thickness(0, 5, 0, 0);
            zipStorerLink.Margin = new Thickness(0, 5, 0, 0);

            var zipStorerPanel = new DockPanel();
            zipStorerPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            zipStorerPanel.VerticalAlignment = VerticalAlignment.Top;
            zipStorerPanel.LastChildFill = false;
            zipStorerPanel.Margin = new Thickness(0, 10, 0, 0);
            zipStorerPanel.Children.Add(zipStorerIcon);
            zipStorerPanel.Children.Add(zipStorerCredit);
            zipStorerPanel.Children.Add(zipStorerLink);
            DockPanel.SetDock(zipStorerIcon, Dock.Right);
            DockPanel.SetDock(zipStorerCredit, Dock.Top);
            DockPanel.SetDock(zipStorerLink, Dock.Top);

            var rootPanel = new StackPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Top;
            rootPanel.Margin = new Thickness(10);
            rootPanel.Orientation = Orientation.Vertical;
            rootPanel.Children.Add(udtPanel);
            rootPanel.Children.Add(zipStorerPanel);

            var window = new Window();
            window.Owner = MainWindow;
            window.WindowStyle = WindowStyle.ToolWindow;
            window.ResizeMode = ResizeMode.NoResize;
            window.Background = new SolidColorBrush(System.Windows.SystemColors.ControlColor);
            window.ShowInTaskbar = false;
            window.Title = "About UberDemoTools";
            window.Content = rootPanel;
            window.SizeToContent = SizeToContent.WidthAndHeight;
            window.ResizeMode = ResizeMode.NoResize;
            window.Icon = UDT.Properties.Resources.UDTIcon.ToImageSource();
            window.Loaded += (obj, args) => 
            { 
                window.Left = _window.Left + (_window.Width - window.Width) / 2; 
                window.Top = _window.Top + (_window.Height - window.Height) / 2; 
            };
            window.ShowDialog();
        }

        private static void HandleLinkClick(RequestNavigateEventArgs args)
        {
            try
            {
                Process.Start(new ProcessStartInfo(args.Uri.AbsoluteUri));
            }
            catch(Exception)
            {
            }
            args.Handled = true;
        }

        public static Tuple<FrameworkElement, FrameworkElement> CreateTuple(FrameworkElement a, FrameworkElement b)
        {
            return new Tuple<FrameworkElement, FrameworkElement>(a, b);
        }

        public static Tuple<FrameworkElement, FrameworkElement> CreateTuple(string a, FrameworkElement b)
        {
            return new Tuple<FrameworkElement, FrameworkElement>(new Label { Content = a }, b);
        }

        public static Tuple<FrameworkElement, FrameworkElement> CreateTuple(FrameworkElement a, string b)
        {
            return new Tuple<FrameworkElement, FrameworkElement>(a, new Label { Content = b });
        }

        public static Tuple<FrameworkElement, FrameworkElement> CreateTuple(string a, string b)
        {
            return new Tuple<FrameworkElement, FrameworkElement>(new Label { Content = a }, new Label { Content = b });
        }

        private FrameworkElement CreateInfoTab()
        {
            var generalInfoTab = new TabItem();
            generalInfoTab.Header = "General";
            generalInfoTab.Content = CreateGeneralInfoTab();

            var chatEvents = new ChatEventsComponent(this);
            _appComponents.Add(chatEvents);
            var chatTab = new TabItem();
            chatTab.Header = "Chat";
            chatTab.Content = chatEvents.RootControl;

            var deathEvents = new FragEventsComponent(this);
            _appComponents.Add(deathEvents);
            var deathsTab = new TabItem();
            deathsTab.Header = "Obituaries";
            deathsTab.Content = deathEvents.RootControl;

            var stats = new StatsComponent(this);
            _appComponents.Add(stats);
            var statsTab = new TabItem();
            statsTab.Header = "Stats";
            statsTab.Content = stats.RootControl;

            var commands = new CommandsComponent(this);
            _appComponents.Add(commands);
            var commandsTab = new TabItem();
            commandsTab.Header = "Commands";
            commandsTab.Content = commands.RootControl;

            var scores = new ScoresComponent(this);
            _appComponents.Add(scores);
            var scoresTab = new TabItem();
            scoresTab.Header = "Scores";
            scoresTab.Content = scores.RootControl;

            var captures = new FlagCapturesComponent(this);
            _appComponents.Add(captures);
            var capturesTab = new TabItem();
            capturesTab.Header = "Captures";
            capturesTab.Content = captures.RootControl;

            var tabControl = new TabControl();
            tabControl.HorizontalAlignment = HorizontalAlignment.Stretch;
            tabControl.VerticalAlignment = VerticalAlignment.Stretch;
            tabControl.Margin = new Thickness(5);
            tabControl.Items.Add(generalInfoTab);
            tabControl.Items.Add(chatTab);
            tabControl.Items.Add(deathsTab);
            tabControl.Items.Add(statsTab);
            tabControl.Items.Add(commandsTab);
            tabControl.Items.Add(scoresTab);
            tabControl.Items.Add(capturesTab);

            return tabControl;
        }

        private FrameworkElement CreateGeneralInfoTab()
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

            var demoListButtonPanel = new StackPanel();
            demoListButtonPanel.HorizontalAlignment = HorizontalAlignment.Left;
            demoListButtonPanel.VerticalAlignment = VerticalAlignment.Top;
            demoListButtonPanel.Margin = new Thickness(5);
            demoListButtonPanel.Orientation = Orientation.Vertical;
            demoListButtonPanel.Children.Add(addButton);
            demoListButtonPanel.Children.Add(addFolderButton);
            demoListButtonPanel.Children.Add(removeButton);

            var demoListButtonGroupBox = new GroupBox();
            demoListButtonGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            demoListButtonGroupBox.VerticalAlignment = VerticalAlignment.Top;
            demoListButtonGroupBox.Margin = new Thickness(5);
            demoListButtonGroupBox.Header = "Demo List Actions";
            demoListButtonGroupBox.Content = demoListButtonPanel;

            var splitButton = new Button();
            splitButton.Content = "Split";
            splitButton.Width = 75;
            splitButton.Height = 25;
            splitButton.Margin = new Thickness(5);
            splitButton.Click += (obj, args) => OnSplitDemoClicked();

            var demoButtonPanel = new StackPanel();
            demoButtonPanel.HorizontalAlignment = HorizontalAlignment.Left;
            demoButtonPanel.VerticalAlignment = VerticalAlignment.Top;
            demoButtonPanel.Margin = new Thickness(5);
            demoButtonPanel.Orientation = Orientation.Vertical;
            demoButtonPanel.Children.Add(splitButton);

            var demoButtonGroupBox = new GroupBox();
            demoButtonGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            demoButtonGroupBox.VerticalAlignment = VerticalAlignment.Top;
            demoButtonGroupBox.Margin = new Thickness(5);
            demoButtonGroupBox.Header = "Per-demo Actions";
            demoButtonGroupBox.Content = demoButtonPanel;

            var analyzeButton = new Button();
            analyzeButton.Content = "Analyze";
            analyzeButton.Width = 75;
            analyzeButton.Height = 25;
            analyzeButton.Margin = new Thickness(5);
            analyzeButton.ToolTip = "Read the demos for information used to populate the Info tab";
            analyzeButton.Click += (obj, args) => OnAnalyzeDemoClicked();

            var convert68Button = new Button();
            convert68Button.Content = "=> *.dm__68";
            convert68Button.Width = 75;
            convert68Button.Height = 25;
            convert68Button.Margin = new Thickness(5);
            convert68Button.ToolTip = "Convert Q3 *.dm_3 and *.dm_48 demos to Q3 *.dm_68 demos";
            convert68Button.Click += (obj, args) => OnConvertDemosClicked(UDT_DLL.udtProtocol.Dm68);

            var convert91Button = new Button();
            convert91Button.Content = "=> *.dm__91";
            convert91Button.Width = 75;
            convert91Button.Height = 25;
            convert91Button.Margin = new Thickness(5);
            convert91Button.ToolTip = "Convert QL *.dm_73 and *.dm_90 demos to QL *.dm_91 demos";
            convert91Button.Click += (obj, args) => OnConvertDemosClicked(UDT_DLL.udtProtocol.Dm91);

            var mergeDemosButton = new Button();
            mergeDemosButton.Content = "Merge";
            mergeDemosButton.Width = 75;
            mergeDemosButton.Height = 25;
            mergeDemosButton.Margin = new Thickness(5);
            mergeDemosButton.ToolTip = "Merge multiple demos into a single output demo\nThis is for demos of the same match only";
            mergeDemosButton.Click += (obj, args) => OnMergeDemosClicked();

            var jsonExportButton = new Button();
            jsonExportButton.Content = "=> .JSON";
            jsonExportButton.Width = 75;
            jsonExportButton.Height = 25;
            jsonExportButton.Margin = new Thickness(5);
            jsonExportButton.ToolTip = "Export demo analysis data to .JSON files\nYou can enable/disable analyzers in the Settings tab";
            jsonExportButton.Click += (obj, args) => OnExportDemosToJSONClicked();

            var multiDemoActionButtonsPanel = new StackPanel();
            multiDemoActionButtonsPanel.HorizontalAlignment = HorizontalAlignment.Left;
            multiDemoActionButtonsPanel.VerticalAlignment = VerticalAlignment.Top;
            multiDemoActionButtonsPanel.Margin = new Thickness(5);
            multiDemoActionButtonsPanel.Orientation = Orientation.Vertical;
            multiDemoActionButtonsPanel.Children.Add(analyzeButton);
            multiDemoActionButtonsPanel.Children.Add(convert68Button);
            multiDemoActionButtonsPanel.Children.Add(convert91Button);
            multiDemoActionButtonsPanel.Children.Add(mergeDemosButton);
            multiDemoActionButtonsPanel.Children.Add(jsonExportButton);

            var multiDemoActionButtonsGroupBox = new GroupBox();
            multiDemoActionButtonsGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            multiDemoActionButtonsGroupBox.VerticalAlignment = VerticalAlignment.Top;
            multiDemoActionButtonsGroupBox.Margin = new Thickness(5);
            multiDemoActionButtonsGroupBox.Header = "Multi-demo Actions";
            multiDemoActionButtonsGroupBox.Content = multiDemoActionButtonsPanel;

            var steamButton = new Button();
            steamButton.Content = "Steam Info...";
            steamButton.Width = 90;
            steamButton.Height = 25;
            steamButton.Margin = new Thickness(5);
            steamButton.ToolTip = "Display Steam information: user accounts, Q3 and QL demos paths";
            steamButton.Click += (obj, args) => OnSteamInfoClicked();

            var extraActionButtonsPanel = new StackPanel();
            extraActionButtonsPanel.HorizontalAlignment = HorizontalAlignment.Left;
            extraActionButtonsPanel.VerticalAlignment = VerticalAlignment.Top;
            extraActionButtonsPanel.Margin = new Thickness(5);
            extraActionButtonsPanel.Orientation = Orientation.Vertical;
            extraActionButtonsPanel.Children.Add(steamButton);

            var extraActionButtonsGroupBox = new GroupBox();
            extraActionButtonsGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            extraActionButtonsGroupBox.VerticalAlignment = VerticalAlignment.Top;
            extraActionButtonsGroupBox.Margin = new Thickness(5);
            extraActionButtonsGroupBox.Header = "Extra Stuff";
            extraActionButtonsGroupBox.Content = extraActionButtonsPanel;

            var helpTextBlock = new TextBlock();
            helpTextBlock.Margin = new Thickness(5);
            helpTextBlock.TextWrapping = TextWrapping.WrapWithOverflow;
            helpTextBlock.Text =
                "In UDT, splitting means creating one file per GameState message when there is more than one.\n" +
                "\nIn Quake's network protocol, the GameState message is sent to the client when a map is (re-)loaded." +
                "\nThe usual case is that the demo only contains one GameState message at the very beginning of the file." +
                "\nBecause that specific message is never delta-encoded or dependent on previous data, demo content can be copied as-is without any advanced parsing nor processing.\n" +
                "\nOn the other hand, cutting at specific timestamps does requires that all messages be decoded in order and re-encoded accordingly and is therefore a much more costly operation.";

            var helpGroupBox = new GroupBox();
            helpGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            helpGroupBox.VerticalAlignment = VerticalAlignment.Top;
            helpGroupBox.Margin = new Thickness(5);
            helpGroupBox.Header = "Help";
            helpGroupBox.Content = helpTextBlock;

            var rootPanel = new WrapPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.Margin = new Thickness(5);
            rootPanel.Orientation = Orientation.Horizontal;
            rootPanel.Children.Add(demoListButtonGroupBox);
            rootPanel.Children.Add(multiDemoActionButtonsGroupBox);
            rootPanel.Children.Add(demoButtonGroupBox);
            rootPanel.Children.Add(extraActionButtonsGroupBox);
            rootPanel.Children.Add(helpGroupBox);

            return rootPanel;
        }

        public void SetInputFolderPath(string path)
        {
            var settings = _appComponents.Find(c => c is AppSettingsComponent) as AppSettingsComponent;
            if(settings == null)
            {
                return;
            }

            Config.InputFolder = path;
            SaveConfig();
            settings.SetInputFolderPath(path);
        }

        public void SetOutputFolderPath(string path)
        {
            var settings = _appComponents.Find(c => c is AppSettingsComponent) as AppSettingsComponent;
            if(settings == null)
            {
                return;
            }

            Config.OutputFolder = path;
            SaveConfig();
            settings.SetOutputFolderPath(path);
        }

        private void PopulateInfoListView(DemoInfo demoInfo)
        {
            _infoListView.Items.Clear();
            _infoListView.Items.Add(new ConfigStringDisplayInfo("Folder Path", Path.GetDirectoryName(demoInfo.FilePath) ?? "N/A"));
            _infoListView.Items.Add(new ConfigStringDisplayInfo("File Name", Path.GetFileNameWithoutExtension(demoInfo.FilePath) ?? "N/A"));
            _infoListView.Items.Add(new ConfigStringDisplayInfo("Protocol", demoInfo.Protocol));

            if(!demoInfo.Analyzed)
            {
                _infoListView.Items.Add(new ConfigStringDisplayInfo("", "This demo wasn't analyzed."));
                return;
            }

            foreach(var tuple in demoInfo.Generic)
            {
                _infoListView.Items.Add(new ConfigStringDisplayInfo(tuple.Item1, tuple.Item2));
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

            foreach(var tab in _appComponents)
            {
                var views = tab.InfoListViews;
                if(views != null)
                {
                    foreach(var view in views)
                    {
                        view.SetDemoAnalyzed(demoInfo.Analyzed);
                    }
                }
            }

            foreach(var tab in _appComponents)
            {
                tab.PopulateViews(demoInfo);
            }
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

        public void DisableUiNonThreadSafe()
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

            _window.Dispatcher.Invoke(guiResetter);
        }

        private void AddDemosImpl(List<string> filePaths)
        {
            if(filePaths.Count == 0)
            {
                return;
            }

            _demoListView.Background = _demoListViewBackground;
            var oldDemoCount = _demos.Count;

            var newDemos = new List<DemoInfo>();
            foreach(var filePath in filePaths)
            {
                var demoInfo = new DemoInfo();
                demoInfo.InputIndex = -1;
                demoInfo.FilePath = filePath;
                demoInfo.Protocol = Path.GetExtension(filePath);
                demoInfo.ProtocolNumber = UDT_DLL.GetProtocolFromFilePath(filePath);

                var demoDisplayInfo = new DemoDisplayInfo();
                demoDisplayInfo.Demo = demoInfo;
                demoDisplayInfo.FileName = Path.GetFileNameWithoutExtension(filePath);
                _demos.Add(demoInfo);
                newDemos.Add(demoInfo);

                AddDemoToListView(demoDisplayInfo);
            }

            if(oldDemoCount == 0)
            {
                _demoListView.SelectedIndex = 0;
            }

            if(_privateConfig.ForceAnalyzeOnLoad || _config.AnalyzeOnLoad)
            {
                AnalyzeDemos(newDemos);
            }
        }

        private void AnalyzeDemos(List<DemoInfo> demos)
        {
            demos = demos.FindAll(d => !d.Analyzed);
            if(demos.Count == 0)
            {
                LogError("All the selected demos were already analyzed.");
                return;
            }

            DisableUiNonThreadSafe();

            SaveBothConfigs();

            JoinJobThread();
            StartJobThread(DemoAnalyzeThread, demos);
        }

        private bool IsValidInputFormatForConverter(UDT_DLL.udtProtocol outputFormat, UDT_DLL.udtProtocol inputFormat)
        {
            if( (outputFormat == UDT_DLL.udtProtocol.Dm68 && inputFormat == UDT_DLL.udtProtocol.Dm3) ||
                (outputFormat == UDT_DLL.udtProtocol.Dm68 && inputFormat == UDT_DLL.udtProtocol.Dm48) ||
                (outputFormat == UDT_DLL.udtProtocol.Dm91 && inputFormat == UDT_DLL.udtProtocol.Dm73) ||
                (outputFormat == UDT_DLL.udtProtocol.Dm91 && inputFormat == UDT_DLL.udtProtocol.Dm90))
            {
                return true;
            }

            return false;
        }

        private class DemoConvertThreadArg
        {
            public List<DemoInfo> Demos;
        }

        private void OnConvertDemosClicked(UDT_DLL.udtProtocol outputFormat)
        {
            var demos = SelectedDemos;
            if(demos == null || demos.Count == 0)
            {
                LogError("No demo selected. Please select at least one to proceed.");
                return;
            }

            demos = demos.FindAll(d => IsValidInputFormatForConverter(outputFormat, d.ProtocolNumber));
            if(demos.Count == 0)
            {
                LogError("All of the selected demos are either in the target format or an unsupported input format.");
                return;
            }
            
            DisableUiNonThreadSafe();

            SaveBothConfigs();
            PrivateConfig.ConversionOutputProtocol = outputFormat;

            var threadData = new DemoConvertThreadArg();
            threadData.Demos = demos;

            JoinJobThread();
            StartJobThread(DemoConvertThread, threadData);
        }

        private void OnMergeDemosClicked()
        {
            var demos = SelectedWriteDemos;
            if(demos == null)
            {
                return;
            }
            if(demos.Count < 2)
            {
                LogError("No enough demos selected. Please select at least two to proceed.");
            }

            var firstProtocol = demos[0].ProtocolNumber;
            for(var i = 1; i < demos.Count; ++i)
            {
                if(demos[i].ProtocolNumber != firstProtocol)
                {
                    LogError("Protocol mismatch detected. All demos must be using the protocol.");
                    return;
                }
            }

            // Select the primary demo.
            var dialog = new DemoMergingDialog(_window, demos);
            if(dialog.SelectionIndex < 0)
            {
                return;
            }

            // If the selected demo isn't already the first, swap it with the first.
            if(dialog.SelectionIndex != 0)
            {
                var item = demos[dialog.SelectionIndex];
                demos[dialog.SelectionIndex] = demos[0];
                demos[0] = item;
            }

            DisableUiNonThreadSafe();

            JoinJobThread();
            StartJobThread(DemoMergeThread, demos);
        }

        private void OnExportDemosToJSONClicked()
        {
            var demos = SelectedDemos;
            if(demos == null || demos.Count == 0)
            {
                LogError("No demo selected. Please select at least one to proceed.");
                return;
            }

            if(Config.JSONPlugInsEnabled == 0)
            {
                LogError("No analyzer selected. Please select at least one to proceed.");
                return;
            }

            DisableUiNonThreadSafe();
            SaveBothConfigs();

            JoinJobThread();
            StartJobThread(JSONDemoExportThread, demos);
        }

        private void OnSteamInfoClicked()
        {
            new SteamDialog(_window);
        }

        public delegate void VoidDelegate();

        private void AddDemoToListView(DemoDisplayInfo info)
        {
            var inputGesture = new MouseGesture(MouseAction.LeftDoubleClick, ModifierKeys.None);
            var inputBinding = new MouseBinding(_showDemoInfoCommand, inputGesture);
            var commandBinding = new CommandBinding();
            commandBinding.Command = _showDemoInfoCommand;
            commandBinding.Executed += (obj, args) => OnShowDemoInfo();
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = true; };

            var item = new ListViewItem();
            item.Content = info;
            item.InputBindings.Add(inputBinding);
            item.CommandBindings.Add(commandBinding);

            _demoListView.Items.Add(item);
        }

        public string GetOutputFolder()
        {
            if(_config.OutputToInputFolder || 
                string.IsNullOrWhiteSpace(_config.OutputFolder) || 
                !Directory.Exists(_config.OutputFolder))
            {
                return null;
            }

            return _config.OutputFolder;
        }

        public string GetDefaultBrowsingFolder()
        {
            if(_config.UseInputFolderAsDefaultBrowsingLocation &&
                !string.IsNullOrWhiteSpace(_config.InputFolder) &&
                Directory.Exists(_config.InputFolder))
            {
                return _config.InputFolder;
            }

            if(!string.IsNullOrWhiteSpace(_config.LastDemoOpenFolderPath) &&
                Directory.Exists(_config.LastDemoOpenFolderPath))
            {
                return _config.LastDemoOpenFolderPath;
            }

            return null;
        }

        public void SaveBrowsingFolder(string folderPath)
        {
            _config.LastDemoOpenFolderPath = folderPath;
        }

        public void InitParseArg()
        {
            if(ParseArg.OutputFolderPath != IntPtr.Zero)
            {
                Marshal.FreeHGlobal(ParseArg.OutputFolderPath);
            }

            var outputFolder = GetOutputFolder();
            Marshal.WriteInt32(_cancelOperation, 0);
            ParseArg.CancelOperation = _cancelOperation;
            ParseArg.PerformanceStats = IntPtr.Zero; // @TODO: Do we really want stats for the small jobs?
            ParseArg.MessageCb = DemoLoggingCallback;
            ParseArg.ProgressCb = DemoProgressCallback;
            ParseArg.ProgressContext = IntPtr.Zero;
            ParseArg.FileOffset = 0;
            ParseArg.GameStateIndex = 0;
            ParseArg.OutputFolderPath = IntPtr.Zero;
            ParseArg.PlugInCount = 0;
            ParseArg.PlugIns = IntPtr.Zero;
            ParseArg.Flags = 0;
            ParseArg.MinProgressTimeMs = 100;
            if(outputFolder != null)
            {
                ParseArg.OutputFolderPath = UDT_DLL.StringToHGlobalUTF8(outputFolder);
            }
        }

        private void DemoAnalyzeThread(object arg)
        {
            var demos = arg as List<DemoInfo>;
            if(demos == null)
            {
                LogError("Invalid thread argument type");
                return;
            }

            InitParseArg();

            var filePaths = new List<string>();
            foreach(var demo in demos)
            {
                filePaths.Add(demo.FilePath);
            }

            List<DemoInfo> newDemos = null;
            try
            {
                newDemos = UDT_DLL.ParseDemos(ref ParseArg, filePaths, _config.MaxThreadCount);
            }
            catch(Exception exception)
            {
                LogError("Caught an exception while parsing demos: {0}", exception.Message);
                newDemos = null;
            }

            if(newDemos == null || newDemos.Count == 0)
            {
                return;
            }

            foreach(var newDemo in newDemos)
            {
                if(newDemo == null)
                {
                    continue;
                }

                var i = newDemo.InputIndex;
                demos[i].Analyzed = true;
                demos[i].TeamGameType = newDemo.TeamGameType;
                demos[i].ChatEvents = newDemo.ChatEvents;
                demos[i].FragEvents = newDemo.FragEvents;
                demos[i].GameStateFileOffsets = newDemo.GameStateFileOffsets;
                demos[i].GameStateSnapshotTimesMs = newDemo.GameStateSnapshotTimesMs;
                demos[i].Generic = newDemo.Generic;
                demos[i].InputIndex = newDemo.InputIndex;
                demos[i].Protocol = newDemo.Protocol;
                demos[i].ProtocolNumber = newDemo.ProtocolNumber;
                demos[i].FilePath = newDemo.FilePath;
                demos[i].MatchStats = newDemo.MatchStats;
                demos[i].Commands = newDemo.Commands;
                demos[i].MatchTimes = newDemo.MatchTimes;
                demos[i].FlagCaptures = newDemo.FlagCaptures;
                demos[i].Scores = newDemo.Scores;
                demos[i].TeamScores = newDemo.TeamScores;
            }

            VoidDelegate infoUpdater = delegate { OnDemoListSelectionChanged(); };
            _window.Dispatcher.Invoke(infoUpdater);
        }

        private void DemoConvertThread(object arg)
        {
            var threadData = arg as DemoConvertThreadArg;
            if(threadData == null)
            {
                LogError("Invalid thread argument type");
                return;
            }

            var demos = threadData.Demos;

            InitParseArg();

            var filePaths = new List<string>();
            foreach(var demo in demos)
            {
                filePaths.Add(demo.FilePath);
            }

            try
            {
                UDT_DLL.ConvertDemos(ref ParseArg, PrivateConfig.ConversionOutputProtocol, filePaths, _config.MaxThreadCount);
            }
            catch(Exception exception)
            {
                LogError("Caught an exception while converting demos: {0}", exception.Message);
            }
        }

        private void DemoMergeThread(object arg)
        {
            var demos = arg as List<DemoInfo>;
            if(demos == null)
            {
                LogError("Invalid thread argument type");
                return;
            }

            InitParseArg();

            var filePaths = new List<string>();
            foreach(var demo in demos)
            {
                filePaths.Add(demo.FilePath);
            }

            try
            {
                UDT_DLL.MergeDemos(ref ParseArg, filePaths);
            }
            catch(Exception exception)
            {
                LogError("Caught an exception while merging demos: {0}", exception.Message);
            }
        }

        private void JSONDemoExportThread(object arg)
        {
            var demos = arg as List<DemoInfo>;
            if(demos == null)
            {
                LogError("Invalid thread argument type");
                return;
            }

            var filePaths = new List<string>();
            foreach(var demo in demos)
            {
                filePaths.Add(demo.FilePath);
            }

            InitParseArg();

            var plugIns = new List<UInt32>();
            for(int i = 0; i < (int)UDT_DLL.udtParserPlugIn.Count; ++i)
            {
                if(BitManip.IsBitSet(Config.JSONPlugInsEnabled, i))
                {
                    plugIns.Add((UInt32)i);
                }
            }

            try
            {
                UDT_DLL.ExportDemosDataToJSON(ref ParseArg, filePaths, Config.MaxThreadCount, plugIns.ToArray());
            }
            catch(Exception exception)
            {
                LogError("Caught an exception while exporting demo analysis data to .JSON files: {0}", exception.Message);
            }
        }

        public static string FormatPerformanceTimeUs(long elapsedUs)
        {
            if(elapsedUs <= 0)
            {
                return "< 1ms";
            }

            var usecTotal = elapsedUs;
            if(usecTotal < 1000)
            {
                return (usecTotal / 1000.0).ToString("0.0") + "ms";
            }

            if(usecTotal < 1000000)
            {
                return (usecTotal / 1000).ToString() + "ms";
            }

            var secTotal = usecTotal / 1000000;
            if(secTotal < 10)
            {
                var secs = usecTotal / 1000000.0;
                return secs.ToString(".00") + "s";
            }

            if(secTotal < 100)
            {
                var secs = usecTotal / 1000000.0;
                return secs.ToString(".0") + "s";
            }

            return secTotal.ToString() + "s";
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

        public void SaveConfig()
        {
            foreach(var component in _appComponents)
            {
                component.SaveToConfigObject(_config);
            }

            Serializer.ToXml("Config.xml", _config);
        }

        public void SavePrivateConfig()
        {
            foreach(var component in _appComponents)
            {
                component.SaveToConfigObject(_privateConfig);
            }
        }

        public void SaveBothConfigs()
        {
            SaveConfig();
            SavePrivateConfig();
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

        private static Regex MinutesSecondsRegEx = new Regex(@"(\d+):(\d+)", RegexOptions.Compiled);
        private static Regex SecondsRegEx = new Regex(@"(\d+)", RegexOptions.Compiled);

        public static bool GetTimeSeconds(string text, out int time)
        {
            time = -1;

            foreach(var c in text)
            {
                if(!char.IsDigit(c) && 
                    c != ':' &&
                    !char.IsWhiteSpace(c))
                {
                    return false;
                }
            }

            var match = MinutesSecondsRegEx.Match(text);
            if(match.Success)
            {
                var m = int.Parse(match.Groups[1].Value);
                var s = int.Parse(match.Groups[2].Value);
                time = m * 60 + s;
                return true;
            }

            match = SecondsRegEx.Match(text);
            if(match.Success)
            {
                time = int.Parse(match.Groups[1].Value);
                return true;
            }

            return false;
        }

        public static bool GetOffsetSeconds(string text, out int time)
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
            var selectedCount = _demoListView.SelectedItems.Count;
            if(selectedCount == 0)
            {
                LogWarning("You must select 1 or more demos from the list before you can remove them");
                return;
            }

            if(selectedCount == _demos.Count)
            {
                // Let the most common case be super duper fast.
                _demoListView.Items.Clear();
                _demos.Clear();
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

        private void DemoSplitThread(object arg)
        {
            var filePath = arg as string;
            if(filePath == null)
            {
                LogError("Invalid thread argument type");
                return;
            }

            InitParseArg();

            try
            {
                LogInfo("Splitting demo: {0}", filePath);
                UDT_DLL.SplitDemo(GetMainThreadContext(), ref ParseArg, filePath);
            }
            catch(Exception exception)
            {
                LogError("Caught an exception while splitting a demo: {0}", exception.Message);
            }
        }

        private void OnSplitDemoClicked()
        {
            var demo = SelectedDemo;
            if(demo == null)
            {
                LogError("No demo selected. Please select one to proceed.");
                return;
            }

            if(!IsValidWriteProtocol(demo.ProtocolNumber))
            {
                LogError("The selected demo is using a protocol that UDT can't write.");
                return;
            }

            if(demo.Analyzed && demo.GameStateFileOffsets.Count == 1)
            {
                LogError("The selected demo only has 1 game state message. There's nothing to split.");
                return;
            }

            DisableUiNonThreadSafe();
            _demoListView.Background = _demoListViewBackground;

            JoinJobThread();
            StartJobThread(DemoSplitThread, demo.FilePath);
        }

        private void OnRevealDemoClicked()
        {
            var demo = SelectedDemo;
            if(demo == null)
            {
                LogError("No demo selected. Please select one to proceed.");
                return;
            }

            try
            {
                Process.Start("explorer.exe", "/select," + demo.FilePath);
            }
            catch(Exception exception)
            {
                LogError("Failed to open demo in the file explorer: " + exception.Message);
            }
        }

        private void OnAnalyzeDemoClicked()
        {
            var demos = SelectedDemos;
            if(demos == null || demos.Count == 0)
            {
                LogError("No demo selected. Please select at least one to proceed.");
                return;
            }

            AnalyzeDemos(demos);
        }

        public static bool ParseMinutesSeconds(string time, out int totalSeconds)
        {
            totalSeconds = -1;

            int colonIdx = time.IndexOf(':');
            if(colonIdx < 0)
            {
                return false;
            }

            int minutes = -1;
            if(!int.TryParse(time.Substring(0, colonIdx), out minutes))
            {
                return false;
            }

            int seconds = -1;
            if(!int.TryParse(time.Substring(colonIdx + 1), out seconds))
            {
                return false;
            }

            totalSeconds = 60 * minutes + seconds;

            return true;
        }

        public static string FormatMinutesSeconds(int totalSeconds)
        {
            var minutes = totalSeconds / 60;
            var seconds = totalSeconds % 60;

            return minutes.ToString() + ":" + seconds.ToString("00");
        }

        public void OnCutByTimeContextClicked(ListView listView)
        {
            var items = listView.SelectedItems;
            if(items.Count == 0)
            {
                return;
            }

            var cutByTimeComponent = _cutByTimeComponent as CutByTimeComponent;
            if(cutByTimeComponent == null)
            {
                return;
            }

            int gsIndex = 0;
            int startTime = int.MaxValue;
            int endTime = int.MinValue;
            foreach(var item in items)
            {
                var listViewItem = item as ListViewItem;
                if(listViewItem == null)
                {
                    continue;
                }

                var info = listViewItem.Content as CuttabbleByTimeDisplayInfo;
                if(info == null)
                {
                    continue;
                }

                int startTimeLocal = int.MaxValue;
                int endTimeLocal = int.MinValue;
                if(!info.GetStartAndEndTimes(out startTimeLocal, out endTimeLocal))
                {
                    continue;
                }

                gsIndex = info.GameStateIndex;
                startTime = Math.Min(startTime, startTimeLocal);
                endTime = Math.Max(endTime, endTimeLocal);
            }

            if(startTime == int.MaxValue && endTime == int.MinValue)
            {
                return;
            }

            SetCutByTimeFields(gsIndex, startTime, endTime);

            var tabIdx = 0;
            foreach(var item in _tabControl.Items)
            {
                var tabItem = item as TabItem;
                if(tabItem == null)
                {
                    continue;
                }

                if(tabItem.Content == cutByTimeComponent.RootControl)
                {
                    _tabControl.SelectedIndex = tabIdx;
                    break;
                }

                ++tabIdx;                
            }
        }

        public bool SetCutByTimeFields(int gsIndex, int firstTime, int lastTime)
        {
            var cutByTimeComponent = _cutByTimeComponent as CutByTimeComponent;
            if(cutByTimeComponent == null)
            {
                return false;
            }

            int startOffset = _config.CutStartOffset;
            int endOffset = _config.CutEndOffset;
            if(!_config.SkipChatOffsetsDialog)
            {
                var dialog = new TimeOffsetsDialog(_window, _config.CutStartOffset, _config.CutEndOffset);
                if(!dialog.Valid)
                {
                    return false;
                }

                startOffset = dialog.StartOffset;
                endOffset = dialog.EndOffset;
            }

            var startTime = firstTime - startOffset;
            var endTime = lastTime + endOffset;

            cutByTimeComponent.SetCutInfo(gsIndex, startTime, endTime);

            return true;
        }

        public static void CopyListViewRowsToClipboard(ListView listView)
        {
            var stringBuilder = new StringBuilder();

            var items = CreateSortedList(listView.SelectedItems, listView.Items);
            foreach(var item in items)
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

        private MultithreadedJob _currentJob;

        public bool CreateAndProcessJob(ref UDT_DLL.udtParseArg parseArg, List<string> filePaths, int maxThreadCount, int maxBatchSize, MultithreadedJob.JobExecuter jobExecuter)
        {
            _currentJob = new MultithreadedJob();
            _currentJob.Process(ref parseArg, filePaths, maxThreadCount, maxBatchSize, jobExecuter);
            _currentJob.FreeResources();

            return true;
        }

        public void UpdateSearchResults(List<UDT_DLL.udtPatternMatch> results, List<DemoInfo> demos)
        {
            var resultsComponent = _appComponents.Find(c => c is SearchResultsComponent) as SearchResultsComponent;
            if(resultsComponent == null)
            {
                return;
            }

            VoidDelegate guiUpdater = delegate
            {
                resultsComponent.UpdateResults(results, demos);
            };
            _window.Dispatcher.Invoke(guiUpdater);

            VoidDelegate tabSetter = delegate
            {
                var tabIndex = 0;
                for(var i = 0; i < _tabControl.Items.Count; ++i)
                {
                    if(IsMatchingTab(resultsComponent, _tabControl.Items[i]))
                    {
                        tabIndex = i;
                        break;
                    }
                }
                _tabControl.SelectedIndex = tabIndex;
            };
            _tabControl.Dispatcher.Invoke(tabSetter);
        }

        private void OnCancelJobClicked()
        {
            // @NOTE: Not all job types will require an instance of MultithreadedJob.
            Marshal.WriteInt32(_cancelOperation, 1);
            if(_currentJob != null)
            {
                _currentJob.Cancel();
            }
            LogWarning("Job canceled!");
        }

        private static string FormatProgressTimeFromSeconds(long totalSeconds)
        {
            var hours = totalSeconds / 3600;
            var secondsInLastHour = totalSeconds - 3600 * hours;
            var minutes = secondsInLastHour / 60;
            var seconds = secondsInLastHour - minutes * 60;

            var result = "";
            if(hours > 0)
            {
                result += hours.ToString() + "h ";
            }
            if(minutes > 0)
            {
                result += minutes.ToString() + "m ";
            }
            result += seconds.ToString() + "s";

            return result;
        }

        public void SetProgressThreadSafe(double value)
        {
            var elapsedTimeMs = _threadedJobTimer.ElapsedMilliseconds;
            var elapsed = FormatProgressTimeFromSeconds(elapsedTimeMs / 1000);
            var remaining = "?";
            if(elapsedTimeMs >= 200 && value > 0.0)
            {
                var totalTimeSeconds = (elapsedTimeMs / 1000.0) * (100.0 / value);
                var remainingTimeSeconds = totalTimeSeconds - (elapsedTimeMs / 1000.0);
                remaining = remainingTimeSeconds < 1.0 ? 
                    "< 1s" : 
                    FormatProgressTimeFromSeconds((long)remainingTimeSeconds);
            }

            VoidDelegate valueSetter = delegate 
            {
                _progressTimeElapsedTextBlock.Text = "Time elapsed: " + elapsed;
                _progressTimeRemainingTextBlock.Text = "Estimated time remaining: " + remaining;
                _progressBar.Value = value; 
                _progressBar.InvalidateVisual();
                if(_threadedJobTitleTimer.ElapsedMilliseconds >= 200)
                {
                    _window.Title = value.ToString("F1") + "% - UDT";
                    _threadedJobTitleTimer.Restart();
                }
            };
            _window.Dispatcher.Invoke(valueSetter);
        }

        public static UDT_DLL.udtProtocol GetProtocolFromFilePath(string filePath)
        {
            var extension = Path.GetExtension(filePath).ToLower();
            if(!ProtocolFileExtDic.ContainsKey(extension))
            {
                return UDT_DLL.udtProtocol.Invalid;
            }

            var prot = ProtocolFileExtDic[extension];

            return ProtocolFileExtDic[extension];
        }

        public static bool IsValidWriteProtocol(UDT_DLL.udtProtocol protocol)
        {
            foreach(var writeProtocol in ValidWriteProtocols)
            {
                if(writeProtocol == protocol)
                {
                    return true;
                }
            }

            return false;
        }

        private static void ScrollListBoxAllTheWayDown(ListBox listBox, object lastItem)
        {
            if(VisualTreeHelper.GetChildrenCount(listBox) > 0)
            {
                var border = VisualTreeHelper.GetChild(listBox, 0) as Decorator;
                if(border != null)
                {
                    var scrollViewer = border.Child as ScrollViewer;
                    if(scrollViewer != null)
                    {
                        scrollViewer.ScrollToBottom();
                        return;
                    }
                }
            }

            // No luck, we use the next best thing.
            listBox.ScrollIntoView(lastItem); 
        }

        private void LogMessageNoColor(string message)
        {
            VoidDelegate itemAdder = delegate 
            {
                _logListBox.Items.Add(message);
                ScrollListBoxAllTheWayDown(_logListBox, message);
            };

            _logListBox.Dispatcher.Invoke(itemAdder);
        }

        public void LogWarningNoColor(string message, params object[] args)
        {
            LogMessageNoColor("WARNING: " + SafeStringFormat(message, args));
        }

        public void LogErrorNoColor(string message, params object[] args)
        {
            LogMessageNoColor("ERROR: " + SafeStringFormat(message, args));
        }

        private void LogMessageWithColor(string message, Color color)
        {
            VoidDelegate itemAdder = delegate
            {
                var textBlock = new TextBlock();
                textBlock.Text = message;
                textBlock.Foreground = new SolidColorBrush(color);
                _logListBox.Items.Add(textBlock);
                ScrollListBoxAllTheWayDown(_logListBox, textBlock);
            };

            _logListBox.Dispatcher.Invoke(itemAdder);
        }

        public void LogWarningWithColor(string message, params object[] args)
        {
            LogMessageWithColor(SafeStringFormat(message, args), Color.FromRgb(255, 127, 0));
        }

        public void LogErrorWithColor(string message, params object[] args)
        {
            LogMessageWithColor(SafeStringFormat(message, args), Color.FromRgb(255, 0, 0));
        }

        public void LogInfo(string message, params object[] args)
        {
            LogMessageNoColor(SafeStringFormat(message, args));
        }

        public void LogWarning(string message, params object[] args)
        {
            if(Config.ColorLogWarningsAndErrors)
            {
                LogWarningWithColor(message, args);
            }
            else
            {
                LogWarningNoColor(message, args);
            }
        }

        public void LogError(string message, params object[] args)
        {
            if(Config.ColorLogWarningsAndErrors)
            {
                LogErrorWithColor(message, args);
            }
            else
            {
                LogErrorNoColor(message, args);
            }
        }

        static private string SafeStringFormat(string format, params object[] args)
        {
            if(args == null || args.Length == 0)
            {
                return format;
            }
            
            try
            {
                return string.Format(format, args);
            }
            catch
            {
                var builder = new StringBuilder();
                builder.Append("string.Format of this failed: ");
                builder.Append(format);
                for(var i = 0; i < args.Length; ++i)
                {
                    builder.Append(", arg");
                    builder.Append(i.ToString());
                    builder.Append("=");
                    builder.Append(args[i]);
                }

                return builder.ToString();
            }
        }

        static public void GlobalLogInfo(string message, params object[] args)
        {
            App.Instance.LogInfo(message, args);
        }

        static public void GlobalLogWarning(string message, params object[] args)
        {
            App.Instance.LogWarning(message, args);
        }

        static public void GlobalLogError(string message, params object[] args)
        {
            App.Instance.LogError(message, args);
        }

        public void DemoLoggingCallback(int logLevel, string message)
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

        public void DemoProgressCallback(float progress, IntPtr userData)
        {
            SetProgressThreadSafe(100.0 * (double)progress);
        }

        static private string GetTextFromLogItem(object item)
        {
            if(item == null)
            {
                return null;
            }

            if(item is string)
            {
                return item as string;
            }

            if(item is TextBlock)
            {
                return (item as TextBlock).Text;
            }

            return null;
        }

        private string GetLog()
        {
            var stringBuilder = new StringBuilder();

            foreach(var item in _logListBox.Items)
            {
                var line = GetTextFromLogItem(item);
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

        public static List<object> CreateSortedList(System.Collections.IList items, System.Collections.IList originalItems)
        {
            var itemList = new List<object>();
            itemList.Capacity = items.Count;
            foreach(var item in items)
            {
                itemList.Add(item);
            }

            itemList.Sort((a, b) => originalItems.IndexOf(a).CompareTo(originalItems.IndexOf(b)));

            return itemList;
        }

        public void SelectDemos(IEnumerable<DemoInfo> demos)
        {
            var selectedItems = new List<ListViewItem>();
            foreach(var item in _demoListView.Items)
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

                foreach(var demo in demos)
                {
                    if(demo == displayInfo.Demo)
                    {
                        selectedItems.Add(listViewItem);
                        break;
                    }
                }
            }

            if(selectedItems.Count == 0)
            {
                return;
            }

            _demoListView.SelectedItems.Clear();
            foreach(var selectedItem in selectedItems)
            {
                _demoListView.SelectedItems.Add(selectedItem);
            }
            _demoListView.Refresh();
        }

        private void CopyLogSelection()
        {
            var stringBuilder = new StringBuilder();

            var items = CreateSortedList(_logListBox.SelectedItems, _logListBox.Items);
            foreach(var item in items)
            {
                var line = GetTextFromLogItem(item);
                if(line == null)
                {
                    continue;
                }

                stringBuilder.AppendLine(line);
            }
          
            Clipboard.SetDataObject(stringBuilder.ToString(), true);
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

        private void FatalErrorHandler(string errorMessage)
        {
            throw new Exception(errorMessage);
        }

        public void JoinJobThread()
        {
            if(_jobThread != null)
            {
                _jobThread.Join();
            }
        }

        private class JobThreadData
        {
            public ParameterizedThreadStart UserFunction;
            public object UserData;
        }

        public void StartJobThread(ParameterizedThreadStart entryPoint, object userData)
        {
            _threadedJobTimer.Restart();
            _threadedJobTitleTimer.Restart();
            var udtData = new JobThreadData();
            udtData.UserFunction = entryPoint;
            udtData.UserData = userData;
            _jobThread = new Thread(RealJobThreadEntryPoint);
            _jobThread.Start(udtData);
        }

        private void RealJobThreadEntryPoint(object udtData)
        {
            var data = udtData as JobThreadData;
            if(data == null)
            {
                return;
            }

            if(Debugger.IsAttached)
            {
                data.UserFunction(data.UserData);

                EnableUiThreadSafe();

                VoidDelegate uiResetter = delegate
                {
                    _window.Title = "UDT";
                    _demoListView.Focus();
                };
                _window.Dispatcher.Invoke(uiResetter);

                return;
            }

            try
            {
                data.UserFunction(data.UserData);
            }
            catch(Exception exception)
            {
                EntryPoint.RaiseException(exception);
            }
            finally
            {
                EnableUiThreadSafe();

                VoidDelegate uiResetter = delegate 
                {
                    _window.Title = "UDT";
                    _demoListView.Focus();
                    if(_privateConfig.QuitAfterFirstJob)
                    {
                        OnQuit();
                    }
                };
                _window.Dispatcher.Invoke(uiResetter);
            }
        }

        public IntPtr GetMainThreadContext()
        {
            if(_mainThreadContext == IntPtr.Zero)
            {
                _mainThreadContext = UDT_DLL.CreateContext();
            }

            return _mainThreadContext;
        }

        public static void AddKeyBinding(UIElement element, Key key, RoutedCommand command, ExecutedRoutedEventHandler callback)
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

        // True if the file doesn't exist at the end of the call.
        private static bool TryDeleteFile(string filePath)
        {
            try
            {
                if(File.Exists(filePath))
                {
                    File.Delete(filePath);
                }
                return true;
            }
            catch(Exception)
            {
            }

            return false;
        }

        // True if the source file existed and was renamed.
        private static bool TryMoveFile(string filePath, string newFilePath)
        {
            try
            {
                if(File.Exists(filePath))
                {
                    File.Move(filePath, newFilePath);
                    return true;
                }   
            }
            catch(Exception)
            {
            }

            return false;
        }
    }

    public class DemoMergingDialog
    {
        private int _selectionIndex = -1;

        public int SelectionIndex
        {
            get { return _selectionIndex; }
        }

        public DemoMergingDialog(Window parent, List<DemoInfo> demos)
        {
            var demosComboBox = new ComboBox();
            demosComboBox.Margin = new Thickness(5);
            foreach(var demo in demos)
            {
                demosComboBox.Items.Add(Path.GetFileName(demo.FilePath));
            }
            demosComboBox.SelectedIndex = 0;

            var helpText = new TextBlock();
            helpText.Margin = new Thickness(5);
            helpText.Text = "Select the demo whose first-person/free-float camera view we use for the output demo.";
            helpText.TextAlignment = TextAlignment.Left;
            helpText.TextWrapping = TextWrapping.Wrap;

            var selectionGroupBox = new GroupBox();
            selectionGroupBox.Header = "Demo Selection";
            selectionGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            selectionGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            selectionGroupBox.Margin = new Thickness(5);
            selectionGroupBox.Content = demosComboBox;

            var okButton = new Button();
            okButton.Content = "OK";
            okButton.Width = 75;
            okButton.Height = 25;
            okButton.Margin = new Thickness(5);
            okButton.HorizontalAlignment = HorizontalAlignment.Right;

            var cancelButton = new Button();
            cancelButton.Content = "Cancel";
            cancelButton.Width = 75;
            cancelButton.Height = 25;
            cancelButton.Margin = new Thickness(5);
            cancelButton.HorizontalAlignment = HorizontalAlignment.Right;

            var rootPanel = new DockPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Center;
            rootPanel.VerticalAlignment = VerticalAlignment.Center;
            rootPanel.Children.Add(helpText);
            rootPanel.Children.Add(selectionGroupBox);
            rootPanel.Children.Add(cancelButton);
            rootPanel.Children.Add(okButton);

            DockPanel.SetDock(helpText, Dock.Top);
            DockPanel.SetDock(selectionGroupBox, Dock.Top);
            DockPanel.SetDock(cancelButton, Dock.Right);
            DockPanel.SetDock(okButton, Dock.Right);

            var window = new Window();
            okButton.Click += (obj, args) => { window.DialogResult = true; window.Close(); };
            cancelButton.Click += (obj, args) => { window.DialogResult = false; window.Close(); };

            window.Owner = parent;
            window.WindowStyle = WindowStyle.ToolWindow;
            window.AllowsTransparency = false;
            window.Background = new SolidColorBrush(System.Windows.SystemColors.ControlColor);
            window.ShowInTaskbar = false;
            window.Width = 240;
            window.Height = 180;
            window.Left = parent.Left + (parent.Width - window.Width) / 2;
            window.Top = parent.Top + (parent.Height - window.Height) / 2;
            window.Icon = UDT.Properties.Resources.UDTIcon.ToImageSource();
            window.Title = "Primary Demo Selection";
            window.Content = rootPanel;
            window.ResizeMode = ResizeMode.NoResize;
            window.ShowDialog();

            var valid = window.DialogResult ?? false;
            if(!valid)
            {
                return;
            }

            _selectionIndex = demosComboBox.SelectedIndex;
        }
    }
}