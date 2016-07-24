using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Navigation;


namespace Uber.Builder
{
    public static class AppVersion
    {
        public const string Version = "0.1.1";
    }

    public class ProfileJob
    {
        public ProfileJob()
        {
        }

        public ProfileJob(string name, string args)
        {
            ExecutableName = name;
            Arguments = args;
        }

        public string ExecutableName;
        public string Arguments;
    }

    public class BuilderConfig
    {
        public string RootFolderPath = @"..\..\..\..";
        public string DLLHeaderFilePath = @"UDT_DLL\include\uberdemotools.h";
        public string GUIAppSourceFilePath = @"UDT_GUI\src\App.cs";
        public string ViewerAppSourceFilePath = @"UDT_DLL\src\viewer\viewer.cpp";
        public string ChangeLogDLLFilePath = @"changelog_dll.txt";
        public string ChangeLogGUIFilePath = @"changelog_gui.txt";
        public string ChangeLogViewerFilePath = @"changelog_viewer.txt";
        public string PremakeFilePath = @"UDT_DLL\premake\premake5.exe";
        public string DLLPremakeFolderPath = @"UDT_DLL\premake";
        public string DLLProjectFolderPath = @"UDT_DLL\.build";
        public string DLLOutputFolderPath = @"UDT_DLL\.bin";
        public string GUIOutputFolderPath = @"UDT_GUI\.bin";
        public string DemoFolderPath = @"demo_files";
        public string WinRARFilePath = @"C:\Program Files\WinRAR\WinRAR.exe";
        public string ResHackerFilePath = @"C:\Programs\Resource Hacker\ResourceHacker.exe";
        public string IconFilePath = @"UDT_GUI\UDT.ico";
        public List<string> CommandLineToolNames = new List<string>();
        public List<ProfileJob> ProfileJobs = new List<ProfileJob>();
    }

    public class Version
    {
        public override string ToString()
        {
            return string.Format("{0}.{1}.{2}", Major, Minor, Revision);
        }

        public int ToNumber()
        {
            return (Major * 10000) + (Minor * 100) + Revision;
        }

        public int Major;
        public int Minor;
        public int Revision;
    }

    public class SharedData
    {
        public readonly Version DLLVersion = new Version();
        public readonly Version GUIVersion = new Version();
        public readonly Version ViewerVersion = new Version();
        public readonly Version DLLVersionRequiredByGUI = new Version();
        public readonly Version ChangeLogDLLVersion = new Version();
        public readonly Version ChangeLogGUIVersion = new Version();
        public readonly Version ChangeLogViewerVersion = new Version();
        public bool VersionsChecked = false;
        public bool VersionsValid = false;
        public int SelectedVisualStudioVersion = 0;
        public bool EnablePGO = false;
        public bool ForceRebuild = false;
        public bool LogButDontRunProcesses = false;
        public readonly List<App.BoolDelegate> SelectedActions = new List<App.BoolDelegate>();
        public string TempFolderPath;
    }

    public static class VisualStudio
    {
        public class Version
        {
            public Version(string generator, string path, string version, int year)
            {
                PremakeGenerator = generator;
                Path = path;
                MSBuildVersion = version;
                Year = year;
            }

            public string PremakeGenerator { get; private set; }
            public string Path { get; private set; }
            public string MSBuildVersion { get; private set; }
            public int Year { get; private set; }
        }

        public static readonly Version[] Versions = new Version[]
        {
            new Version("vs2005", "VS80COMNTOOLS", "8.0", 2005),
            new Version("vs2008", "VS90COMNTOOLS", "9.0", 2008),
            new Version("vs2010", "VS100COMNTOOLS", "10.0", 2010),
            new Version("vs2012", "VS110COMNTOOLS", "11.0", 2012),
            new Version("vs2013", "VS120COMNTOOLS", "12.0", 2013),
            new Version("vs2015", "VS140COMNTOOLS", "14.0", 2015)
        };

        public enum Target
        {
            Debug,
            Release,
            ReleaseInst,
            ReleaseOpt,
            Count
        }

        public enum Architecture
        {
            X86,
            X64,
            Count
        }

        public static Version GetVersionByYear(int year)
        {
            foreach(var version in Versions)
            {
                if(version.Year == year)
                {
                    return version;
                }
            }

            return null;
        }

        public static string GetTargetString(Target target)
        {
            return target.ToString();
        }

        public static string GetArchitectureStringCPP(Architecture arch)
        {
            switch(arch)
            {
                case Architecture.X86: return "Win32";
                case Architecture.X64: return "x64";
                default: return "";
            }
        }

        public static string GetArchitectureStringCS(Architecture arch)
        {
            switch(arch)
            {
                case Architecture.X86: return "x86";
                case Architecture.X64: return "x64";
                default: return "";
            }
        }
    }

    public static class Premake
    {
        public static bool Generate(string premakeFilePath, string premakeFolderPath, string premakeGenerator)
        {
            return App.Instance.RunAndReadProcess(premakeFolderPath, premakeFilePath, premakeGenerator);
        }
    }

    public static class MSBuild
    {
        public static bool BuildCPP(VisualStudio.Version version, string projectOrSolutionFilePath, VisualStudio.Target target, VisualStudio.Architecture architecture, bool forceRebuild)
        {
            var workDir = Path.GetDirectoryName(projectOrSolutionFilePath);
            var msBuildPath = string.Format(@"C:\Program Files (x86)\MSBuild\{0}\Bin\MSBuild.exe", version.MSBuildVersion);
            var arguments = string.Format("/t:{0} /p:Configuration={1} /p:Platform={2} \"{3}\"",
                forceRebuild ? "Rebuild" : "Build",
                VisualStudio.GetTargetString(target),
                VisualStudio.GetArchitectureStringCPP(architecture),
                projectOrSolutionFilePath);

            return App.Instance.RunAndReadProcess(workDir, msBuildPath, arguments);
        }

        public static bool BuildCS(VisualStudio.Version version, string projectOrSolutionFilePath, VisualStudio.Target target, VisualStudio.Architecture architecture, bool forceRebuild)
        {
            var workDir = Path.GetDirectoryName(projectOrSolutionFilePath);
            var msBuildPath = string.Format(@"C:\Program Files (x86)\MSBuild\{0}\Bin\MSBuild.exe", version.MSBuildVersion);
            var arguments = string.Format("/t:{0} /p:Configuration={1} /p:Platform={2} \"{3}\"",
                forceRebuild ? "Rebuild" : "Build",
                VisualStudio.GetTargetString(target),
                VisualStudio.GetArchitectureStringCS(architecture),
                projectOrSolutionFilePath);

            return App.Instance.RunAndReadProcess(workDir, msBuildPath, arguments);
        }
    }

    public static class WinRAR
    {
        public static bool CreateArchive(string workDir, string archiveFilePath, List<string> filesToArchive)
        {
            var listFilePath = Path.GetTempFileName();
            var listFileContent = new StringBuilder();
            foreach(var file in filesToArchive)
            {
                listFileContent.AppendLine(file);
            }
            File.WriteAllText(listFilePath, listFileContent.ToString());
            var arguments = string.Format("a -ep {0} @{1}", archiveFilePath, listFilePath);
            var result = App.Instance.RunAndReadProcess(workDir, App.Instance.Config.WinRARFilePath, arguments);
            File.Delete(listFilePath);

            return result;
        }

        public static bool AddToArchive(string workDir, string archiveFilePath, string path, string options)
        {
            var arguments = string.Format("a {0} {1} {2}", archiveFilePath, options, path);

            return App.Instance.RunAndReadProcess(workDir, App.Instance.Config.WinRARFilePath, arguments);
        }

        public static bool AddToArchive(string workDir, string archiveFilePath, string path)
        {
            var arguments = string.Format("a {0} {1}", archiveFilePath, path);

            return App.Instance.RunAndReadProcess(workDir, App.Instance.Config.WinRARFilePath, arguments);
        }
    }

    public class App
    {
        private readonly List<RadioButton> _visualStudioRadioButtons = new List<RadioButton>();
        private readonly List<CheckBox> _actionCheckBoxes = new List<CheckBox>();
        private TextBlock _statusTextBlock;
        private CheckBox _pgoCheckBox;
        private CheckBox _forceRebuildCheckBox;
        private CheckBox _logOnlyCheckBox;
        private ListBox _logListBox;
        private Window _window;
        private Application _application;
        private BuilderConfig _config;
        private SharedData _sharedData = new SharedData();
        private Thread _currentThread;

        private class Action
        {
            public Action(string desc, BoolDelegate handler)
            {
                GUIDescription = desc;
                Handler = handler;
            }

            public string GUIDescription;
            public BoolDelegate Handler;
        }

        private readonly List<Action> Actions = new List<Action>();

        // List of all C++ projects needed for a new release.
        private static readonly string[] ProjectNames = new string[]
        {
            "UDT",
            "UDT_cutter",
            "UDT_splitter",
            "UDT_timeshifter",
            "UDT_merger",
            "UDT_json",
            "UDT_captures",
            "UDT_converter",
            "viewer_data_gen",
            "UDT_viewer"
        };

        public delegate void VoidDelegate();
        public delegate bool BoolDelegate();

        public static App Instance { get; private set; }

        public SharedData Data { get; private set; }
        public BuilderConfig Config { get { return _config; } }

        public App(string[] cmdLineArgs)
        {
            Instance = this;
            Data = new SharedData();

            PresentationTraceSources.DataBindingSource.Switch.Level = SourceLevels.Critical;

            LoadConfig();

            Actions.Add(new Action("Generate the Visual Studio solution", ActionGenerateSolution));
            Actions.Add(new Action("Check version numbers for mismatches", ActionCheckVersions));
            Actions.Add(new Action("Build the GUI app and its updater", ActionBuildGUI));
            Actions.Add(new Action("Build the library and applications", ActionBuildLib));
            Actions.Add(new Action("Package the binaries", ActionPackage));

            var logListBox = new ListBox();
            _logListBox = logListBox;
            logListBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            logListBox.VerticalAlignment = VerticalAlignment.Stretch;
            logListBox.Margin = new Thickness(5);
            logListBox.Height = 300;
            logListBox.SelectionMode = SelectionMode.Extended;
            var listBoxStyle = new AlternatingListBoxBackground(Colors.White, Color.FromRgb(215, 215, 215));
            listBoxStyle.ApplyTo(logListBox);

            var logGroupBox = new GroupBox();
            logGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            logGroupBox.VerticalAlignment = VerticalAlignment.Bottom;
            logGroupBox.Margin = new Thickness(5);
            logGroupBox.Header = "Log";
            logGroupBox.Content = logListBox;

            var visualStudioVersionPanel = new StackPanel();
            visualStudioVersionPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            visualStudioVersionPanel.VerticalAlignment = VerticalAlignment.Stretch;
            visualStudioVersionPanel.Margin = new Thickness(5);
            visualStudioVersionPanel.Orientation = Orientation.Vertical;
            foreach(var version in VisualStudio.Versions)
            {
                var button = new RadioButton();
                button.HorizontalAlignment = HorizontalAlignment.Stretch;
                button.VerticalAlignment = VerticalAlignment.Stretch;
                button.Margin = new Thickness(5, 5, 5, 0);
                button.Content = string.Format("{0} ({1})", version.Year, version.MSBuildVersion);
                button.GroupName = "Visual Studio Version";
                visualStudioVersionPanel.Children.Add(button);
                _visualStudioRadioButtons.Add(button);
            }
            DetectVisualStudioVersions();

            var visualStudioVersionGroupBox = new GroupBox();
            visualStudioVersionGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            visualStudioVersionGroupBox.VerticalAlignment = VerticalAlignment.Top;
            visualStudioVersionGroupBox.Margin = new Thickness(5);
            visualStudioVersionGroupBox.Header = "Visual Studio";
            visualStudioVersionGroupBox.Content = visualStudioVersionPanel;

            var pgoCheckBox = new CheckBox();
            _pgoCheckBox = pgoCheckBox;
            pgoCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            pgoCheckBox.VerticalAlignment = VerticalAlignment.Top;
            pgoCheckBox.Margin = new Thickness(5);
            pgoCheckBox.Content = " Build with PGO";

            var forceRebuildCheckBox = new CheckBox();
            _forceRebuildCheckBox = forceRebuildCheckBox;
            forceRebuildCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            forceRebuildCheckBox.VerticalAlignment = VerticalAlignment.Top;
            forceRebuildCheckBox.Margin = new Thickness(5);
            forceRebuildCheckBox.Content = " Force Rebuild";

            var logOnlyCheckBox = new CheckBox();
            _logOnlyCheckBox = logOnlyCheckBox;
            logOnlyCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            logOnlyCheckBox.VerticalAlignment = VerticalAlignment.Top;
            logOnlyCheckBox.Margin = new Thickness(5);
            logOnlyCheckBox.Content = " Don't run Processes";

            var settingsPanel = new StackPanel();
            settingsPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            settingsPanel.VerticalAlignment = VerticalAlignment.Stretch;
            settingsPanel.Margin = new Thickness(5);
            settingsPanel.Orientation = Orientation.Vertical;
            settingsPanel.Children.Add(visualStudioVersionGroupBox);
            settingsPanel.Children.Add(pgoCheckBox);
            settingsPanel.Children.Add(forceRebuildCheckBox);
            settingsPanel.Children.Add(logOnlyCheckBox);

            var settingsGroupBox = new GroupBox();
            settingsGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            settingsGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            settingsGroupBox.Margin = new Thickness(5);
            settingsGroupBox.Header = "Settings";
            settingsGroupBox.Content = settingsPanel;

            var goButton = new Button();
            goButton.HorizontalAlignment = HorizontalAlignment.Stretch;
            goButton.VerticalAlignment = VerticalAlignment.Top;
            goButton.Margin = new Thickness(5);
            goButton.Height = 25;
            goButton.Content = "Go!";
            goButton.Click += (obj, args) => StartJobThread(JobThread, null);

            var actionsPanel = new StackPanel();
            actionsPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            actionsPanel.VerticalAlignment = VerticalAlignment.Stretch;
            actionsPanel.Margin = new Thickness(5);
            actionsPanel.Orientation = Orientation.Vertical;
            foreach(var action in Actions)
            {
                var checkBox = new CheckBox();
                checkBox.HorizontalAlignment = HorizontalAlignment.Left;
                checkBox.VerticalAlignment = VerticalAlignment.Top;
                checkBox.Margin = new Thickness(5, 5, 5, 3);
                checkBox.Content = " " + action.GUIDescription;
                actionsPanel.Children.Add(checkBox);
                _actionCheckBoxes.Add(checkBox);
            }
            actionsPanel.Children.Add(goButton);

            var actionsGroupBox = new GroupBox();
            actionsGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            actionsGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            actionsGroupBox.Margin = new Thickness(5);
            actionsGroupBox.Header = "Actions";
            actionsGroupBox.Content = actionsPanel;

            var statusTextBlock = new TextBlock();
            _statusTextBlock = statusTextBlock;
            statusTextBlock.HorizontalAlignment = HorizontalAlignment.Left;
            statusTextBlock.VerticalAlignment = VerticalAlignment.Top;
            statusTextBlock.Margin = new Thickness(5);

            var statusGroupBox = new GroupBox();
            statusGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            statusGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            statusGroupBox.Margin = new Thickness(5);
            statusGroupBox.Header = "Status";
            statusGroupBox.Content = statusTextBlock;

            var rootPanel = new DockPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.Margin = new Thickness(5);
            rootPanel.LastChildFill = true;
            rootPanel.Children.Add(logGroupBox);
            rootPanel.Children.Add(settingsGroupBox);
            rootPanel.Children.Add(actionsGroupBox);
            rootPanel.Children.Add(statusGroupBox);
            DockPanel.SetDock(logGroupBox, Dock.Bottom);
            DockPanel.SetDock(settingsGroupBox, Dock.Left);
            DockPanel.SetDock(actionsGroupBox, Dock.Left);
            DockPanel.SetDock(statusGroupBox, Dock.Left);

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

            LogInfo("Builder {0} is now operational!", AppVersion.Version);
            
            ProcessCommandLine(cmdLineArgs);

            var app = new Application();
            _application = app;
            app.ShutdownMode = ShutdownMode.OnLastWindowClose;
            app.Run(window);
        }

        private void ProcessCommandLine(string[] cmdLineArgs)
        {
        }

        private void OnQuit()
        {
            SaveConfig();
            _application.Shutdown();
        }

        private void LoadConfig()
        {
            Serializer.FromXml("Config.xml", out _config);

            var toolNames = Config.CommandLineToolNames;
            if(toolNames.Count == 0)
            {
                toolNames.Add("UDT_cutter");
                toolNames.Add("UDT_splitter");
                toolNames.Add("UDT_timeshifter");
                toolNames.Add("UDT_merger");
                toolNames.Add("UDT_json");
                toolNames.Add("UDT_captures");
                toolNames.Add("UDT_converter");
            }

            var profileJobs = Config.ProfileJobs;
            if(profileJobs.Count == 0)
            {
                profileJobs.Add(new ProfileJob("UDT_GUI.exe", "/ForceAnalyzeOnLoad /QuitAfterFirstJob /ForceSkipFolderScanDialog /ForceScanFoldersRecursively \"{1}\""));
                profileJobs.Add(new ProfileJob("UDT_viewer.exe", "\"{1}\\dm_91\\ctf_with_overtime.dm_91\" /ProfileMode"));
                profileJobs.Add(new ProfileJob("UDT_converter.exe", "-r -q -p=68 \"-o={0}\" \"{1}\\dm3\""));
                profileJobs.Add(new ProfileJob("UDT_converter.exe", "-r -q -p=68 \"-o={0}\" \"{1}\\dm_48\""));
                profileJobs.Add(new ProfileJob("UDT_converter.exe", "-r -q -p=91 \"-o={0}\" \"{1}\\dm_73\""));
                profileJobs.Add(new ProfileJob("UDT_converter.exe", "-r -q -p=91 \"-o={0}\" \"{1}\\dm_90\""));
                profileJobs.Add(new ProfileJob("UDT_captures.exe", "-r -q \"-o={0}\\captures.json\" \"{1}\""));
                profileJobs.Add(new ProfileJob("UDT_cutter.exe", "m -r -q \"-o={0}\" \"{1}\""));
                profileJobs.Add(new ProfileJob("UDT_json.exe", "-r -q \"-o={0}\" \"{1}\""));
            }
        }

        public void SaveConfig()
        {
            Serializer.ToXml("Config.xml", Config);
        }

        private void DetectVisualStudioVersions()
        {
            for(var i = 0; i < VisualStudio.Versions.Length; ++i)
            {
                var visualStudio = VisualStudio.Versions[i];
                var path = Environment.GetEnvironmentVariable(visualStudio.Path);
                var installed = !string.IsNullOrEmpty(path);
                _visualStudioRadioButtons[i].IsEnabled = installed;
                _visualStudioRadioButtons[i].IsChecked = installed;
            }
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

        public void LogWarning(string message, params object[] args)
        {
            LogMessageWithColor(SafeStringFormat(message, args), Color.FromRgb(255, 127, 0));
        }

        public void LogError(string message, params object[] args)
        {
            LogMessageWithColor(SafeStringFormat(message, args), Color.FromRgb(255, 0, 0));
        }

        public void LogInfo(string message, params object[] args)
        {
            LogMessageNoColor(SafeStringFormat(message, args));
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

        public static void GlobalLogInfo(string message, params object[] args)
        {
            App.Instance.LogInfo(message, args);
        }

        public static void GlobalLogWarning(string message, params object[] args)
        {
            App.Instance.LogWarning(message, args);
        }

        public static void GlobalLogError(string message, params object[] args)
        {
            App.Instance.LogError(message, args);
        }

        private void ReadStdOutCallback(string text)
        {
            ReadCallback(text, false);
        }

        private void ReadStdErrCallback(string text)
        {
            ReadCallback(text, true);
        }

        private void ReadCallback(string text, bool error)
        {
            var lines = text.Split(new[] { '\n' });
            foreach(var line in lines)
            {
                var fixedLine = line.Replace("\r", "");
                if(error)
                {
                    LogError(fixedLine);
                }
                else
                {
                    LogInfo(fixedLine);
                }
            }
        }

        public bool RunAndReadProcess(string workDir, string exePath, string arguments)
        {
            if(Data.LogButDontRunProcesses)
            {
                return RunAndReadProcessPrintOnly(workDir, exePath, arguments);
            }

            return RunAndReadProcessNormally(workDir, exePath, arguments);
        }

        private bool RunAndReadProcessPrintOnly(string workDir, string exePath, string arguments)
        {
            LogInfo("Builder> {0} {1}", exePath, arguments);

            return true;
        }

        private bool RunAndReadProcessNormally(string workDir, string exePath, string arguments)
        {
            LogInfo("Builder> {0} {1}", exePath, arguments);
            var info = ProcessLauncher.CreateProcessStartInfo(workDir, exePath, arguments);
            var process = Process.Start(info);

            return ProcessLauncher.ReadProcessOutputUntilDone(process, ReadStdOutCallback, ReadStdErrCallback);
        }

        public delegate void ObjectDelegate(object userData);

        private class JobThreadArg
        {
            public ObjectDelegate Function;
            public object Data;
        }

        public void StartJobThread(ObjectDelegate userFunction, object userData)
        {
            if(_currentThread != null)
            {
                _currentThread.Join();
            }

            SaveConfig();
            SaveSettings();
            DisableUI();
            var thread = new Thread(JobThreadEntryPoint);
            _currentThread = thread;

            var threadArg = new JobThreadArg();
            threadArg.Function = userFunction;
            threadArg.Data = userData;

            thread.Start(threadArg);
        }

        private void SaveSettings()
        {
            var visualStudioVersion = 0;
            for(var i = 0; i < _visualStudioRadioButtons.Count; ++i)
            {
                if(_visualStudioRadioButtons[i].IsChecked ?? false)
                {
                    visualStudioVersion = i;
                    break;
                }
            }

            Data.SelectedVisualStudioVersion = visualStudioVersion;
            Data.EnablePGO = _pgoCheckBox.IsChecked ?? false;
            Data.ForceRebuild = _forceRebuildCheckBox.IsChecked ?? false;
            Data.LogButDontRunProcesses = _logOnlyCheckBox.IsChecked ?? false;

            Data.SelectedActions.Clear();
            for(var i = 0; i < _actionCheckBoxes.Count; ++i)
            {
                if(_actionCheckBoxes[i].IsChecked ?? false)
                {
                    Data.SelectedActions.Add(Actions[i].Handler);
                }
            }
        }

        private void DisableUI()
        {
            _window.IsEnabled = false;
        }

        private void EnableUIThreadSafe()
        {
            VoidDelegate uiEnabler = delegate
            {
                _window.IsEnabled = true;
            };

            _window.Dispatcher.Invoke(uiEnabler);
        }

        private void JobThreadEntryPoint(object arg)
        {
            var threadArg = arg as JobThreadArg;
            if(threadArg == null)
            {
                return;
            }

            try
            {
                threadArg.Function(threadArg.Data);
            }
            catch(Exception exception)
            {
                EntryPoint.RaiseException(exception);
            }
            finally
            {
                EnableUIThreadSafe();
            }
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

                File.WriteAllText(saveFileDialog.FileName, GetLog());
            }
        }

        private void SetStatus(string message, params object[] args)
        {
            var formattedMessage = SafeStringFormat(message, args);
            VoidDelegate statusUpdater = delegate
            {
                _statusTextBlock.Text = formattedMessage;
            };

            _statusTextBlock.Dispatcher.Invoke(statusUpdater);
        }

        private void JobThread(object threadArg)
        {
            try
            {
                SetStatus("");

                foreach(var action in Data.SelectedActions)
                {
                    if(!action())
                    {
                        return;
                    }
                }

                SetStatus("Done!");
            }
            catch(Exception exception)
            {
                SetStatus("Exception caught: " + exception.Message);

                var lines = exception.ToString().Split(new[] { '\n' });
                foreach(var line in lines)
                {
                    LogError(line.Replace("\r", ""));
                }
            }
        }

        private bool ActionGenerateSolution()
        {
            var visualStudio = VisualStudio.Versions[Data.SelectedVisualStudioVersion];

            SetStatus("Generating project files for Visual C++ {0}", visualStudio.Year);
            if(!Premake.Generate(
                Path.Combine(_config.RootFolderPath, _config.PremakeFilePath),
                Path.Combine(_config.RootFolderPath, _config.DLLPremakeFolderPath),
                visualStudio.PremakeGenerator))
            {
                SetStatus("Failed to generate project files for Visual C++ {0}", visualStudio.Year);
                return false;
            }

            return true;
        }

        private bool ActionCheckVersions()
        {
            Data.VersionsChecked = true;
            Data.VersionsValid = false;
            SetStatus("Checking version numbers");

            if(!VersionParser.GetVersionFromDLLHeader(Data, Path.Combine(Config.RootFolderPath, Config.DLLHeaderFilePath)))
            {
                SetStatus("Failed to get the DLL version from the C header");
                return false;
            }

            if(!VersionParser.GetVersionFromViewerSource(Data, Path.Combine(Config.RootFolderPath, Config.ViewerAppSourceFilePath)))
            {
                SetStatus("Failed to get the viewer app version from the C++ source file");
                return false;
            }

            if(!VersionParser.GetVersionsFromCSAppSource(Data, Path.Combine(Config.RootFolderPath, Config.GUIAppSourceFilePath)))
            {
                SetStatus("Failed to get the GUI version from the C# source");
                return false;
            }

            if(!VersionParser.GetVersionFromGUIChangeLog(Data, Path.Combine(Config.RootFolderPath, Config.ChangeLogGUIFilePath)))
            {
                SetStatus("Failed to get the GUI version from the changelog");
                return false;
            }

            if(!VersionParser.GetVersionFromDLLChangeLog(Data, Path.Combine(Config.RootFolderPath, Config.ChangeLogDLLFilePath)))
            {
                SetStatus("Failed to get the DLL version from the changelog");
                return false;
            }

            if(!VersionParser.GetVersionFromViewerChangeLog(Data, Path.Combine(Config.RootFolderPath, Config.ChangeLogViewerFilePath)))
            {
                SetStatus("Failed to get the viewer version from the changelog");
                return false;
            }

            if(Data.DLLVersion.ToNumber() != Data.ChangeLogDLLVersion.ToNumber())
            {
                SetStatus("Library ({0}) and changelog ({1}) versions don't match!", Data.DLLVersion, Data.ChangeLogDLLVersion);
                return false;
            }

            if(Data.GUIVersion.ToNumber() != Data.ChangeLogGUIVersion.ToNumber())
            {
                SetStatus("GUI app ({0}) and changelog ({1}) versions don't match!", Data.GUIVersion, Data.ChangeLogGUIVersion);
                return false;
            }

            if(Data.DLLVersion.ToNumber() != Data.DLLVersionRequiredByGUI.ToNumber())
            {
                SetStatus("Library app ({0}) and GUI minimum library ({1}) versions don't match!", Data.DLLVersion, Data.DLLVersionRequiredByGUI);
                return false;
            }

            if(Data.ViewerVersion.ToNumber() != Data.ChangeLogViewerVersion.ToNumber())
            {
                SetStatus("Viewer app ({0}) and changelog ({1}) versions don't match!", Data.ViewerVersion, Data.ChangeLogViewerVersion);
                return false;
            }

            Data.VersionsValid = true;

            return true;
        }

        private bool ActionBuildLib()
        {
            return Data.EnablePGO ? BuildLibWithPGO() : BuildLibNormally();
        }

        private bool ActionBuildGUI()
        {
            var visualStudio = VisualStudio.Versions[Data.SelectedVisualStudioVersion];
            var solutionFilePath = Path.Combine(_config.RootFolderPath, "UDT_GUI", "UDT.sln");
            solutionFilePath = Path.GetFullPath(solutionFilePath);

            var forceRebuild = Data.ForceRebuild;
            Data.ForceRebuild = true;
            var result = BuildCS(visualStudio, solutionFilePath, VisualStudio.Target.Release);
            Data.ForceRebuild = forceRebuild;

            return result;
        }

        private bool ActionPackage()
        {
            if(!Data.VersionsChecked)
            {
                ActionCheckVersions();
            }

            if(!Data.VersionsValid)
            {
                SetStatus("Can't archive the new builds because some version numbers don't match");
                return false;
            }

            if(!File.Exists(Config.WinRARFilePath))
            {
                SetStatus("Invalid WinRAR.exe file path");
                return false;
            }

            SetStatus("Creating .zip archives");

            var libChangeLog = Path.GetFullPath(Path.Combine(Config.RootFolderPath, Config.ChangeLogDLLFilePath));
            var visualStudio = VisualStudio.Versions[Data.SelectedVisualStudioVersion];

            if(!CreateConArchive(visualStudio, "x86") || 
                !CreateConArchive(visualStudio, "x64") ||
                !CreateGUIArchive(visualStudio, "x86") ||
                !CreateGUIArchive(visualStudio, "x64") ||
                !CreateDevArchive(visualStudio, "x86") ||
                !CreateDevArchive(visualStudio, "x64") ||
                !CreateViewerArchives(visualStudio))
            {
                return false;
            }            

            return true;
        }

        private bool CreateConArchive(VisualStudio.Version visualStudio, string arch)
        {
            var workDir = Path.GetFullPath(Config.RootFolderPath);
            var libVersion = Data.DLLVersion.ToString();
            var libChangeLogFilePath = Path.GetFullPath(Path.Combine(Config.RootFolderPath, Config.ChangeLogDLLFilePath));
            var archiveFilePath = string.Format("udt_con_{0}_{1}.zip", libVersion, arch);
            var filePaths = new List<string>();
            filePaths.Add(libChangeLogFilePath);
            foreach(var toolName in Config.CommandLineToolNames)
            {
                var path = Path.Combine(Config.RootFolderPath, Config.DLLOutputFolderPath, visualStudio.PremakeGenerator, arch, "release", toolName + ".exe");
                filePaths.Add(Path.GetFullPath(path));
            }

            SetStatus("Creating archive {0}", archiveFilePath);
            if(!WinRAR.CreateArchive(workDir, archiveFilePath, filePaths))
            {
                SetStatus("Failed to create archive {0}", archiveFilePath);
                return false;
            }

            return true;
        }

        private bool CreateDevArchive(VisualStudio.Version visualStudio, string arch)
        {
            var workDir = Path.GetFullPath(Config.RootFolderPath);
            var libVersion = Data.DLLVersion.ToString();
            var archiveFilePath = string.Format("udt_dev_{0}_{1}.zip", libVersion, arch);
            var filePaths = new List<string>();
            filePaths.Add(Path.GetFullPath(Path.Combine(Config.RootFolderPath, Config.DLLHeaderFilePath)));
            filePaths.Add(Path.GetFullPath(Path.Combine(Config.RootFolderPath, Config.DLLOutputFolderPath, visualStudio.PremakeGenerator, arch, "release", "UDT.dll")));
            
            SetStatus("Creating archive {0}", archiveFilePath);
            if(!WinRAR.CreateArchive(workDir, archiveFilePath, filePaths))
            {
                SetStatus("Failed to create archive {0}", archiveFilePath);
                return false;
            }

            return true;
        }

        private bool CreateGUIArchive(VisualStudio.Version visualStudio, string arch)
        {
            var workDir = Path.GetFullPath(Config.RootFolderPath);
            var libVersion = Data.DLLVersion.ToString();
            var guiVersion = Data.GUIVersion.ToString();
            var libChangeLogFilePath = Path.GetFullPath(Path.Combine(Config.RootFolderPath, Config.ChangeLogDLLFilePath));
            var guiChangeLogFilePath = Path.GetFullPath(Path.Combine(Config.RootFolderPath, Config.ChangeLogGUIFilePath));
            var archiveFilePath = string.Format("udt_gui_{0}_dll_{1}_{2}.zip", guiVersion, libVersion, arch);
            var filePaths = new List<string>();
            filePaths.Add(Path.GetFullPath(Path.Combine(Config.RootFolderPath, Config.DLLOutputFolderPath, visualStudio.PremakeGenerator, arch, "release", "UDT.dll")));
            filePaths.Add(Path.GetFullPath(Path.Combine(Config.RootFolderPath, Config.GUIOutputFolderPath, arch, "release", "UDT_GUI.exe")));
            filePaths.Add(Path.GetFullPath(Path.Combine(Config.RootFolderPath, Config.GUIOutputFolderPath, arch, "release", "UDT_GUI_Updater.exe")));
            filePaths.Add(libChangeLogFilePath);
            filePaths.Add(guiChangeLogFilePath);

            SetStatus("Creating archive {0}", archiveFilePath);
            if(!WinRAR.CreateArchive(workDir, archiveFilePath, filePaths))
            {
                SetStatus("Failed to create archive {0}", archiveFilePath);
                return false;
            }

            return true;
        }

        private bool CreateViewerArchives(VisualStudio.Version visualStudio)
        {
            var tempDir = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName(), "viewer_data");
            Directory.CreateDirectory(tempDir);
            Data.TempFolderPath = tempDir;

            try
            {
                var relPath = Path.Combine(Config.RootFolderPath, Config.DLLOutputFolderPath, visualStudio.PremakeGenerator, "x86", "release", "viewer_data_gen.exe");
                var dataGenPath = Path.GetFullPath(relPath);
                var workDir = Path.GetFullPath(Config.RootFolderPath);
                var args = string.Format("-o={0} viewer_data", tempDir);
                if(!RunAndReadProcess(workDir, dataGenPath, args) ||
                    !CreateViewerArchive(visualStudio, "x86") ||
                    !CreateViewerArchive(visualStudio, "x64"))
                {
                    return false;
                }
            }
            finally
            {
                Directory.Delete(tempDir, true);
            }

            return true;
        }

        private bool CreateViewerArchive(VisualStudio.Version visualStudio, string arch)
        {
            var version = Data.ViewerVersion;
            var archiveFilePath = string.Format("udt_viewer_{0}_{1}.zip", version, arch);
            var viewerFolderPath = Path.GetFullPath(Path.Combine(Config.RootFolderPath, Config.DLLOutputFolderPath, visualStudio.PremakeGenerator, arch, "release"));
            var iconPath = Path.GetFullPath(Path.Combine(Config.RootFolderPath, Config.IconFilePath));
            var resHackArgs = string.Format("-addoverwrite \"UDT_viewer.exe\", \"UDT_viewer.exe\", \"{0}\", ICONGROUP, MAINICON, 0", iconPath);
            if(!RunAndReadProcess(viewerFolderPath, Config.ResHackerFilePath, resHackArgs))
            {
                SetStatus("Failed to set the viewer's icon before creating archive {0}", archiveFilePath);
                return false;
            }

            var workDir = Path.GetFullPath(Config.RootFolderPath);
            var changeLogFilePath = Path.GetFullPath(Path.Combine(Config.RootFolderPath, Config.ChangeLogViewerFilePath));
            var viewerPath = Path.Combine(viewerFolderPath, "UDT_viewer.exe");
            var filePaths = new List<string>();
            filePaths.Add(viewerPath);
            filePaths.Add(changeLogFilePath);
            
            SetStatus("Creating archive {0}", archiveFilePath);
            if(!WinRAR.CreateArchive(workDir, archiveFilePath, filePaths))
            {
                SetStatus("Failed to create archive {0}", archiveFilePath);
                return false;
            }

            if(!WinRAR.AddToArchive(workDir, archiveFilePath, Data.TempFolderPath, "-ep1") ||
                !WinRAR.AddToArchive(workDir, archiveFilePath, @"viewer_data\map_aliases.txt") ||
                !WinRAR.AddToArchive(workDir, archiveFilePath, @"viewer_data\deja_vu_sans.ttf") ||
                !WinRAR.AddToArchive(workDir, archiveFilePath, @"viewer_data\blender_icons.png") ||
                !WinRAR.AddToArchive(workDir, archiveFilePath, @"viewer_data\maps\*.png", "-apviewer_data -ep1"))
            {
                SetStatus("Failed to add files to archive {0}", archiveFilePath);
                return false;
            }
            
            return true;
        }

        private bool BuildLibNormally()
        {
            var visualStudio = VisualStudio.Versions[Data.SelectedVisualStudioVersion];
            var solutionFilePath = Path.Combine(_config.RootFolderPath, _config.DLLProjectFolderPath, visualStudio.PremakeGenerator, "UDT.sln");
            solutionFilePath = Path.GetFullPath(solutionFilePath);

            return BuildCPP(visualStudio, solutionFilePath, VisualStudio.Target.Release);
        }

        private bool BuildLibWithPGO()
        {
            var visualStudio = VisualStudio.Versions[Data.SelectedVisualStudioVersion];

            return
                CreateViewerData() &&
                CopyGUIBinaries(visualStudio) &&
                CopyPGORunTime(visualStudio) &&
                BuildAllLibProjects(visualStudio);
        }

        private bool BuildAllLibProjects(VisualStudio.Version visualStudio)
        {
            var tempDir = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName());
            Directory.CreateDirectory(tempDir);
            Data.TempFolderPath = tempDir;

            try
            {
                foreach(var projectName in ProjectNames)
                {
                    var projectFilePath = Path.Combine(Config.RootFolderPath, Config.DLLProjectFolderPath, visualStudio.PremakeGenerator, projectName + ".vcxproj");
                    projectFilePath = Path.GetFullPath(projectFilePath);

                    if(ShouldEnablePGO(projectName))
                    {
                        if(!DeleteProfileData(visualStudio) ||
                            !BuildCPP(visualStudio, projectFilePath, VisualStudio.Target.ReleaseInst) ||
                            !ProfileProject(visualStudio, projectName) ||
                            !BuildCPP(visualStudio, projectFilePath, VisualStudio.Target.ReleaseOpt))
                        {
                            return false;
                        }
                    }
                    else if(!BuildCPP(visualStudio, projectFilePath, VisualStudio.Target.Release))
                    {
                        return false;
                    }
                }
            }
            finally
            {
                Directory.Delete(tempDir, true);
            }

            return true;
        }

        private bool ShouldEnablePGO(string projectName)
        {
            projectName = projectName.ToLower();
            if(projectName == "udt")
            {
                return true;
            }

            var exeName = projectName + ".exe";

            return Config.ProfileJobs.Find(job => job.ExecutableName.ToLower() == exeName) != null;
        }

        private bool BuildCPP(VisualStudio.Version version, string solutionFilePath, VisualStudio.Target target)
        {
            var type = "";
            var forceRebuild = Data.ForceRebuild;
            switch(target)
            {
                case VisualStudio.Target.ReleaseInst: type = "instrumented "; break;
                case VisualStudio.Target.ReleaseOpt: type = "optimized "; forceRebuild = false; break;
                default: break;
            }

            var projectName = "";
            if(solutionFilePath.EndsWith(".vcxproj"))
            {
                projectName = Path.GetFileNameWithoutExtension(solutionFilePath) + " ";
            }

            SetStatus("Building {0}{1}x86 binaries with Visual C++ {2}", type, projectName, version.Year);
            if(!MSBuild.BuildCPP(version, solutionFilePath, target, VisualStudio.Architecture.X86, forceRebuild))
            {
                SetStatus("Failed to build {0}{1}x86 binaries with Visual C++ {2}", type, projectName, version.Year);
                return false;
            }

            SetStatus("Building {0}{1}x64 binaries with Visual C++ {2}", type, projectName, version.Year);
            if(!MSBuild.BuildCPP(version, solutionFilePath, target, VisualStudio.Architecture.X64, forceRebuild))
            {
                SetStatus("Failed to build {0}{1}x64 binaries with Visual C++ {2}", type, projectName, version.Year);
                return false;
            }

            return true;
        }

        private bool BuildCS(VisualStudio.Version version, string solutionFilePath, VisualStudio.Target target)
        {
            var forceRebuild = Data.ForceRebuild;
            SetStatus("Building x86 binaries with Visual C# {0}", version.Year);
            if(!MSBuild.BuildCS(version, solutionFilePath, target, VisualStudio.Architecture.X86, forceRebuild))
            {
                SetStatus("Failed to build x86 binaries with Visual C# {0}", version.Year);
                return false;
            }

            SetStatus("Building x64 binaries with Visual C# {0}", version.Year);
            if(!MSBuild.BuildCS(version, solutionFilePath, target, VisualStudio.Architecture.X64, forceRebuild))
            {
                SetStatus("Failed to build x64 binaries with Visual C# {0}", version.Year);
                return false;
            }

            return true;
        }

        private bool DeleteProfileData(VisualStudio.Version version, string arch)
        {
            var exeFolderPath = Path.Combine(_config.RootFolderPath, _config.DLLOutputFolderPath, version.PremakeGenerator, arch, "Release");
            exeFolderPath = Path.GetFullPath(exeFolderPath);
            if(!Directory.Exists(exeFolderPath))
            {
                SetStatus("Failed to find the {0} output directory", arch);
                return false;
            }

            foreach(var file in Directory.GetFiles(exeFolderPath, "*.pgd", SearchOption.TopDirectoryOnly))
            {
                File.Delete(file);
            }

            foreach(var file in Directory.GetFiles(exeFolderPath, "*.pgc", SearchOption.TopDirectoryOnly))
            {
                File.Delete(file);
            }

            return true;
        }

        private bool DeleteProfileData(VisualStudio.Version version)
        {
            return DeleteProfileData(version, "x86") && DeleteProfileData(version, "x64");
        }

        private bool ProfileProject(VisualStudio.Version version, string projectName, string arch)
        {
            var exeFolderPath = Path.Combine(Config.RootFolderPath, Config.DLLOutputFolderPath, version.PremakeGenerator, arch, "Release");
            exeFolderPath = Path.GetFullPath(exeFolderPath);
            if(!Directory.Exists(exeFolderPath))
            {
                SetStatus("Failed to find the {0} output directory", arch);
                return false;
            }

            var demoFolderPath = Path.GetFullPath(Path.Combine(Config.RootFolderPath, Config.DemoFolderPath));
            if(!Directory.Exists(exeFolderPath))
            {
                SetStatus("Failed to find the demo directory", arch);
                return false;
            }

            var profileJobs = Config.ProfileJobs.FindAll(job => IsProfileExecutableForThisProject(job.ExecutableName, projectName));
            foreach(var job in profileJobs)
            {
                var arguments = SafeStringFormat(job.Arguments, Data.TempFolderPath, demoFolderPath);
                var executablePath = Path.Combine(exeFolderPath, job.ExecutableName);

                SetStatus("Profiling {0} ({1})", job.ExecutableName, arch);
                if(!RunAndReadProcess(exeFolderPath, executablePath, arguments))
                {
                    SetStatus("Failed to profile {0} ({1})", job.ExecutableName, arch);
                    return false;
                }
            }

            return true;
        }

        private bool ProfileProject(VisualStudio.Version version, string projectName)
        {
            return ProfileProject(version, projectName, "x86") && ProfileProject(version, projectName, "x64");
        }

        private bool IsProfileExecutableForThisProject(string exeName, string projectName)
        {
            exeName = exeName.ToLower();
            projectName = projectName.ToLower();
            if(exeName == "udt_gui.exe" && projectName == "udt")
            {
                return true;
            }
            if(exeName == projectName + ".exe")
            {
                return true;
            }

            return false;
        }

        private bool CopyGUIBinaries(VisualStudio.Version version, string arch)
        {
            var exeFolderPath = Path.Combine(_config.RootFolderPath, _config.DLLOutputFolderPath, version.PremakeGenerator, arch, "Release");
            exeFolderPath = Path.GetFullPath(exeFolderPath);
            if(!Directory.Exists(exeFolderPath))
            {
                SetStatus("Failed to find the {0} DLL output directory", arch);
                return false;
            }

            var guiExeFolderPath = Path.Combine(_config.RootFolderPath, _config.GUIOutputFolderPath, arch, "Release");
            guiExeFolderPath = Path.GetFullPath(guiExeFolderPath);
            if(!Directory.Exists(guiExeFolderPath))
            {
                SetStatus("Failed to find the {0} GUI output directory", arch);
                return false;
            }

            var sourcePath = Path.Combine(guiExeFolderPath, "UDT_GUI.exe");
            var destPath = Path.Combine(exeFolderPath, "UDT_GUI.exe");
            CopyFileWithOverride(sourcePath, destPath);

            return true;
        }

        private static void CopyFileWithOverride(string sourcePath, string destPath)
        {
            if(File.Exists(destPath))
            {
                File.Delete(destPath);
            }
            File.Copy(sourcePath, destPath);
        }

        private bool CopyGUIBinaries(VisualStudio.Version version)
        {
            return CopyGUIBinaries(version, "x86") && CopyGUIBinaries(version, "x64");
        }

        private bool CopyPGORunTime(VisualStudio.Version version, bool x86)
        {
            var dllName = string.Format("pgort{0}.dll", version.MSBuildVersion.Replace(".", ""));
            var vsPath = Environment.GetEnvironmentVariable(version.Path);
            if(!Directory.Exists(vsPath))
            {
                SetStatus("Failed to find the Visual Studio {0} directory", version.Year);
                return false;
            }

            var targetName = x86 ? "x86" : "x64";
            var pgoPath = Path.GetFullPath(Path.Combine(vsPath, x86 ? @"..\..\VC\bin" : @"..\..\VC\bin\amd64", dllName));
            var exeFolderPath = Path.GetFullPath(Path.Combine(Config.RootFolderPath, Config.DLLOutputFolderPath, version.PremakeGenerator, targetName, "Release"));
            if(!Directory.Exists(exeFolderPath))
            {
                SetStatus("Failed to find the {0} DLL output directory", targetName);
                return false;
            }

            var destPath = Path.Combine(exeFolderPath, dllName);
            if(!File.Exists(destPath))
            {
                File.Copy(pgoPath, destPath);
            }

            return true;
        }

        private bool CopyPGORunTime(VisualStudio.Version version)
        {
            return CopyPGORunTime(version, true) && CopyPGORunTime(version, false);
        }

        private bool CreateViewerData()
        {
            // We create the new folder in the x86\release, then copy over to x64\release.

            try
            {
                var visualStudio = VisualStudio.Versions[Data.SelectedVisualStudioVersion];
                var dataPathX86 = FullPath(Config.DLLOutputFolderPath, visualStudio.PremakeGenerator, @"x86\release\viewer_data");
                var dataPathX64 = FullPath(Config.DLLOutputFolderPath, visualStudio.PremakeGenerator, @"x64\release\viewer_data");
                var dataGenPath = FullPath(Config.DLLOutputFolderPath, visualStudio.PremakeGenerator, @"x64\release\viewer_data_gen.exe");
                if(Directory.Exists(dataPathX86))
                {
                    Directory.Delete(dataPathX86, true);
                }
                if(Directory.Exists(dataPathX64))
                {
                    Directory.Delete(dataPathX64, true);
                }

                Directory.CreateDirectory(dataPathX86);
                var workDir = Path.GetFullPath(Config.RootFolderPath);
                var args = string.Format("-o={0} viewer_data", dataPathX86);
                if(!RunAndReadProcess(workDir, dataGenPath, args))
                {
                    SetStatus("Failed to create viewer data with viewer_data_gen.exe");
                    return false;
                }

                var fileToCopy = new List<string>();
                fileToCopy.AddRange(Directory.GetFiles(FullPath(@"viewer_data\maps"), "*.png", SearchOption.TopDirectoryOnly));
                fileToCopy.Add(FullPath(@"viewer_data\map_aliases.txt"));
                fileToCopy.Add(FullPath(@"viewer_data\deja_vu_sans.ttf"));
                fileToCopy.Add(FullPath(@"viewer_data\blender_icons.png"));
                CopyFilesToDirectory(fileToCopy, dataPathX86);

                Microsoft.VisualBasic.FileIO.FileSystem.CopyDirectory(dataPathX86, dataPathX64);
            }
            catch(Exception exception)
            {
                SetStatus("Failed to create viewer data, see the log");
                LogError("Failed to create viewer data: {0}", exception.Message);
                LogError(exception.StackTrace);
                return false;
            }

            return true;
        }

        private string FullPath(string relativePath)
        {
            return Path.GetFullPath(Path.Combine(Config.RootFolderPath, relativePath));
        }

        private string FullPath(params string[] relativePaths)
        {
            var newPathsArray = new string[relativePaths.Length + 1];
            newPathsArray[0] = Config.RootFolderPath;
            var i = 1;
            foreach(var path in relativePaths)
            {
                newPathsArray[i++] = path;
            }

            return Path.GetFullPath(Path.Combine(newPathsArray));
        }

        private static void CopyFilesToDirectory(List<string> filePaths, string fullDestFolderPath)
        {
            foreach(var filePath in filePaths)
            {
                var fileName = Path.GetFileName(filePath);
                var destFilePath = Path.Combine(fullDestFolderPath, fileName);
                File.Copy(filePath, destFilePath);
            }
        }
    }
}