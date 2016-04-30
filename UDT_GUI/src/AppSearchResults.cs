using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;


namespace Uber.DemoTools
{
    public class SearchResultFileDisplayInfo
    {
        public SearchResultFileDisplayInfo(string fileName, string matchCount, string patterns, uint fileIndex, DemoInfo demo)
        {
            FileName = fileName;
            MatchCount = matchCount;
            Patterns = patterns;
            FileIndex = fileIndex;
            Demo = demo;
        }

        public override string ToString()
        {
            return string.Format("{0}: [{1}] ({2})", FileName, MatchCount, Patterns);
        }

        public string FileName { get; set; }
        public string MatchCount { get; set; }
        public string Patterns { get; set; }
        public uint FileIndex { get; private set; }
        public DemoInfo Demo { get; private set; }
    }

    public class SearchResultCutDisplayInfo
    {
        public SearchResultCutDisplayInfo(string fileName, string gs, string start, string end, string patterns, UDT_DLL.udtPatternMatch match, DemoInfo demo)
        {
            FileName = fileName;
            GSIndex = gs;
            StartTime = start;
            EndTime = end;
            Patterns = patterns;
            Match = match;
            Demo = demo;
        }

        public override string ToString()
        {
            return string.Format("{0}: [{1}] {2} - {3} ({4})", FileName, GSIndex, StartTime, EndTime, Patterns);
        }

        public string FileName { get; set; }
        public string GSIndex { get; set; }
        public string StartTime { get; set; }
        public string EndTime { get; set; }
        public string Patterns { get; set; }
        public UDT_DLL.udtPatternMatch Match { get; private set; }
        public DemoInfo Demo { get; private set; }
    }

    public class CommandBindingWrapper
    {
        private RoutedCommand _command = new RoutedCommand();
        private CommandBinding _commandBinding;
        private KeyBinding _keyBinding;

        public delegate bool CanExecuteDelegate();
        public delegate void OnClickDelegate();

        public RoutedCommand RoutedCommand { get { return _command; } }
        public CommandBinding CommandBinding { get { return _commandBinding; } }
        public KeyBinding KeyBinding { get { return _keyBinding; } }

        public CommandBindingWrapper(UIElement uiElement, Key key, CanExecuteDelegate canExecute, OnClickDelegate onClick)
        {
            var keyGesture = new KeyGesture(key, ModifierKeys.Control);
            var keyBinding = new KeyBinding(_command, keyGesture);
            _keyBinding = keyBinding;
            var commandBinding = new CommandBinding();
            commandBinding.Command = _command;
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = canExecute(); };
            commandBinding.Executed += (obj, args) => onClick();
            _commandBinding = commandBinding;
            uiElement.CommandBindings.Add(_commandBinding);
            uiElement.InputBindings.Add(_keyBinding);
        }

        public CommandBindingWrapper(UIElement uiElement, CanExecuteDelegate canExecute, OnClickDelegate onClick)
        {
            var commandBinding = new CommandBinding();
            commandBinding.Command = _command;
            commandBinding.CanExecute += (obj, args) => { args.CanExecute = canExecute(); };
            commandBinding.Executed += (obj, args) => onClick();
            _commandBinding = commandBinding;
            uiElement.CommandBindings.Add(_commandBinding);
        }
    }

    public class SearchResultsComponent : AppComponent
    {
        private App _app;
        private TextBlock _resultsTextBlock;
        private TextBlock _noResultsTextBlock;
        private DemoInfoListView _fileResultsListView;
        private DemoInfoListView _cutResultsListView;
        private RadioButton _displayFilesRadioButton;
        private ScrollViewer _scrollViewer;
        private List<UDT_DLL.udtPatternMatch> _results;
        private List<DemoInfo> _resultDemos;
        private CommandBindingWrapper _fileCopyCommand;
        private CommandBindingWrapper _fileCutCommand;
        private CommandBindingWrapper _fileRevealCommand;
        private CommandBindingWrapper _fileSelectCommand;
        private CommandBindingWrapper _cutCopyCommand;
        private CommandBindingWrapper _cutCutCommand;
        private CommandBindingWrapper _cutRevealCommand;
        private CommandBindingWrapper _cutSelectCommand;

        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return new List<DemoInfoListView> { _fileResultsListView, _cutResultsListView }; } }
        public List<DemoInfoListView> InfoListViews { get { return new List<DemoInfoListView> { _fileResultsListView, _cutResultsListView }; } }
        public ComponentType Type { get { return ComponentType.SearchResults; } }
        public bool MultiDemoMode { get { return true; } }

        public SearchResultsComponent(App app)
        {
            _app = app;
            RootControl = CreateTab();
        }

        public void PopulateViews(DemoInfo demoInfo)
        {
            // Nothing to do.
        }

        public void SaveToConfigObject(UdtConfig config)
        {
            // Nothing to do.
        }

        public void SaveToConfigObject(UdtPrivateConfig config)
        {
            // Nothing to do.
        }

        public void UpdateResults(List<UDT_DLL.udtPatternMatch> results, List<DemoInfo> demos)
        {
            _cutResultsListView.Items.Clear();
            _fileResultsListView.Items.Clear();

            results.Sort((a, b) => a.StartTimeMs.CompareTo(b.StartTimeMs));
            results.StableSort((a, b) => a.GameStateIndex.CompareTo(b.GameStateIndex));
            results.StableSort((a, b) => a.DemoInputIndex.CompareTo(b.DemoInputIndex));

            _results = results;
            _resultDemos = demos;

            foreach(var result in results)
            {
                var index = result.DemoInputIndex;
                if(index >= demos.Count)
                {
                    continue;
                }

                var copyMenuItem = new MenuItem();
                copyMenuItem.Header = App.CreateContextMenuHeader("Copy to Clipboard", "(Ctrl+C)");
                copyMenuItem.Command = _cutCopyCommand.RoutedCommand;

                var cutMenuItem = new MenuItem();
                cutMenuItem.Header = "Apply Cut(s)";
                cutMenuItem.Command = _cutCutCommand.RoutedCommand;

                var revealMenuItem = new MenuItem();
                revealMenuItem.Header = "Reveal in File Explorer";
                revealMenuItem.Command = _cutRevealCommand.RoutedCommand;

                var selectMenuItem = new MenuItem();
                selectMenuItem.Header = "Select in Demo List";
                selectMenuItem.Command = _cutSelectCommand.RoutedCommand;

                var contextMenu = new ContextMenu();
                contextMenu.Items.Add(copyMenuItem);
                contextMenu.Items.Add(selectMenuItem);
                contextMenu.Items.Add(cutMenuItem);
                contextMenu.Items.Add(new Separator());
                contextMenu.Items.Add(revealMenuItem);

                var demo = demos[(int)index];
                var fileName = Path.GetFileNameWithoutExtension(demo.FilePath);
                var gs = result.GameStateIndex.ToString();
                var start = App.FormatMinutesSeconds(result.StartTimeMs / 1000);
                var end = App.FormatMinutesSeconds(result.EndTimeMs / 1000);
                var patterns = FormatPatterns(result.Patterns);
                var cutResult = new SearchResultCutDisplayInfo(fileName, gs, start, end, patterns, result, demo);
                var cutItem = new ListViewItem();
                cutItem.Content = cutResult;
                cutItem.ContextMenu = contextMenu;
                _cutResultsListView.Items.Add(cutItem);
            }

            uint currentFileIndex = uint.MaxValue;
            var fileResults = new List<UDT_DLL.udtPatternMatch>();
            foreach(var result in results)
            {
                var index = result.DemoInputIndex;
                if(index >= demos.Count)
                {
                    continue;
                }

                if(currentFileIndex != index)
                {
                    if(fileResults.Count > 0)
                    {
                        AddSingleFileResult(fileResults, demos);
                        fileResults.Clear();
                    }
                }
                fileResults.Add(result);
                currentFileIndex = index;
            }
            if(fileResults.Count > 0)
            {
                AddSingleFileResult(fileResults, demos);
            }

            var cutCount = _cutResultsListView.Items.Count;
            var fileCount = _fileResultsListView.Items.Count;
            _resultsTextBlock.Text = string.Format("Found {0} match{1} in {2} demo file{3}.", 
                cutCount, cutCount > 1 ? "es" : "", fileCount, fileCount > 1 ? "s" : "");

            var fileMode = _displayFilesRadioButton.IsChecked ?? false;
            if(fileMode)
            {
                OnFileModeChecked();
            }
            else
            {
                OnCutModeChecked();
            }
        }

        private void AddSingleFileResult(List<UDT_DLL.udtPatternMatch> results, List<DemoInfo> demos)
        {
            uint patternBits = 0;
            foreach(var result in results)
            {
                patternBits |= result.Patterns;
            }

            var copyMenuItem = new MenuItem();
            copyMenuItem.Header = App.CreateContextMenuHeader("Copy to Clipboard", "(Ctrl+C)");
            copyMenuItem.Command = _fileCopyCommand.RoutedCommand;

            var cutMenuItem = new MenuItem();
            cutMenuItem.Header = "Apply Cut(s)";
            cutMenuItem.Command = _fileCutCommand.RoutedCommand;

            var revealMenuItem = new MenuItem();
            revealMenuItem.Header = "Reveal in File Explorer";
            revealMenuItem.Command = _fileRevealCommand.RoutedCommand;

            var selectMenuItem = new MenuItem();
            selectMenuItem.Header = "Select in Demo List";
            selectMenuItem.Command = _fileSelectCommand.RoutedCommand;

            var contextMenu = new ContextMenu();
            contextMenu.Items.Add(copyMenuItem);
            contextMenu.Items.Add(selectMenuItem);
            contextMenu.Items.Add(cutMenuItem);
            contextMenu.Items.Add(new Separator());
            contextMenu.Items.Add(revealMenuItem);

            var fileIndex = results[0].DemoInputIndex;
            var demo = demos[(int)fileIndex];
            var fileName = Path.GetFileNameWithoutExtension(demo.FilePath);
            var matches = results.Count.ToString();
            var patterns = FormatPatterns(patternBits);
            var cutResult = new SearchResultFileDisplayInfo(fileName, matches, patterns, fileIndex, demo);
            var fileItem = new ListViewItem();
            fileItem.Content = cutResult;
            fileItem.ContextMenu = contextMenu;
            _fileResultsListView.Items.Add(fileItem);
        }

        private static readonly List<string> PatternNames = UDT_DLL.GetStringArray(UDT_DLL.udtStringArray.CutPatterns);

        private static string FormatPatterns(uint patternBits)
        {
            var usedNames = new List<string>();
            for(var i = 0; i < PatternNames.Count; ++i)
            {
                if((patternBits & (uint)(1 << i)) != 0)
                {
                    usedNames.Add(PatternNames[i]);
                }
            }

            return string.Join(", ", usedNames);
        }

        private FrameworkElement CreateTab()
        {
            var displayFilesRadioButton = new RadioButton();
            _displayFilesRadioButton = displayFilesRadioButton;
            displayFilesRadioButton.HorizontalAlignment = HorizontalAlignment.Left;
            displayFilesRadioButton.VerticalAlignment = VerticalAlignment.Top;
            displayFilesRadioButton.Margin = new Thickness(5);
            displayFilesRadioButton.Content = " Display results as files";
            displayFilesRadioButton.GroupName = "SearchResultsDisplayMode";
            displayFilesRadioButton.Checked += (obj, args) => OnFileModeChecked();

            var displayCutsRadioButton = new RadioButton();
            displayCutsRadioButton.HorizontalAlignment = HorizontalAlignment.Left;
            displayCutsRadioButton.VerticalAlignment = VerticalAlignment.Top;
            displayCutsRadioButton.Margin = new Thickness(5, 0, 5, 5);
            displayCutsRadioButton.Content = " Display results as file cuts";
            displayCutsRadioButton.GroupName = "SearchResultsDisplayMode";
            displayCutsRadioButton.Checked += (obj, args) => OnCutModeChecked();

            var displaySettingsStackPanel = new StackPanel();
            displaySettingsStackPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            displaySettingsStackPanel.VerticalAlignment = VerticalAlignment.Top;
            displaySettingsStackPanel.Margin = new Thickness(5);
            displaySettingsStackPanel.Orientation = Orientation.Vertical;
            displaySettingsStackPanel.Children.Add(displayFilesRadioButton);
            displaySettingsStackPanel.Children.Add(displayCutsRadioButton);

            var settingsGroupBox = new GroupBox();
            settingsGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            settingsGroupBox.VerticalAlignment = VerticalAlignment.Top;
            settingsGroupBox.Margin = new Thickness(5);
            settingsGroupBox.Header = "Settings";
            settingsGroupBox.Content = displaySettingsStackPanel;

            var fileResultsGridView = new GridView();
            fileResultsGridView.AllowsColumnReorder = false;
            fileResultsGridView.Columns.Add(CreateColumn(380, "File Name", "FileName"));
            fileResultsGridView.Columns.Add(CreateColumn(40, "Cuts", "MatchCount"));
            fileResultsGridView.Columns.Add(CreateColumn(130, "Patterns", "Patterns"));

            var fileResultsListView = new DemoInfoListView();
            _fileResultsListView = fileResultsListView;
            fileResultsListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            fileResultsListView.VerticalAlignment = VerticalAlignment.Stretch;
            fileResultsListView.Margin = new Thickness(5);
            fileResultsListView.View = fileResultsGridView;
            fileResultsListView.SelectionMode = SelectionMode.Extended;

            var cutResultsGridView = new GridView();
            cutResultsGridView.AllowsColumnReorder = false;
            cutResultsGridView.Columns.Add(CreateColumn(330, "File Name", "FileName"));
            cutResultsGridView.Columns.Add(CreateColumn(40, "GS", "GSIndex", "GameState Index"));
            cutResultsGridView.Columns.Add(CreateColumn(50, "Start", "StartTime"));
            cutResultsGridView.Columns.Add(CreateColumn(50, "End", "EndTime"));
            cutResultsGridView.Columns.Add(CreateColumn(80, "Patterns", "Patterns"));

            var cutResultsListView = new DemoInfoListView();
            _cutResultsListView = cutResultsListView;
            cutResultsListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            cutResultsListView.VerticalAlignment = VerticalAlignment.Stretch;
            cutResultsListView.Margin = new Thickness(5);
            cutResultsListView.View = cutResultsGridView;
            cutResultsListView.SelectionMode = SelectionMode.Extended;

            fileResultsListView.SetDemoAnalyzed(true);
            cutResultsListView.SetDemoAnalyzed(true);
            FixListViewMouseWheelHandling(fileResultsListView);
            FixListViewMouseWheelHandling(cutResultsListView);

            var noResultsTextBlock = new TextBlock();
            _noResultsTextBlock = noResultsTextBlock;
            noResultsTextBlock.HorizontalAlignment = HorizontalAlignment.Left;
            noResultsTextBlock.VerticalAlignment = VerticalAlignment.Center;
            noResultsTextBlock.Margin = new Thickness(5);
            noResultsTextBlock.Text = "Nothing was found!";

            var resultsTextBlock = new TextBlock();
            _resultsTextBlock = resultsTextBlock;
            resultsTextBlock.HorizontalAlignment = HorizontalAlignment.Left;
            resultsTextBlock.VerticalAlignment = VerticalAlignment.Center;
            resultsTextBlock.Margin = new Thickness(5);
            resultsTextBlock.Text = "";

            var resultsStackPanel = new StackPanel();
            resultsStackPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            resultsStackPanel.VerticalAlignment = VerticalAlignment.Stretch;
            resultsStackPanel.Margin = new Thickness(5);
            resultsStackPanel.Orientation = Orientation.Vertical;
            resultsStackPanel.Children.Add(noResultsTextBlock);
            resultsStackPanel.Children.Add(resultsTextBlock);
            resultsStackPanel.Children.Add(fileResultsListView);
            resultsStackPanel.Children.Add(cutResultsListView);

            var resultsGroupBox = new GroupBox();
            resultsGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            resultsGroupBox.VerticalAlignment = VerticalAlignment.Top;
            resultsGroupBox.Margin = new Thickness(5);
            resultsGroupBox.Header = "Search Results";
            resultsGroupBox.Content = resultsStackPanel;

            // Set the default selection and call OnFileModeChecked().
            displayFilesRadioButton.IsChecked = true;
            
            var rootPanel = new StackPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.Margin = new Thickness(5);
            rootPanel.Orientation = Orientation.Vertical;
            rootPanel.Children.Add(settingsGroupBox);
            rootPanel.Children.Add(resultsGroupBox);

            var scrollViewer = new ScrollViewer();
            _scrollViewer = scrollViewer;
            scrollViewer.HorizontalAlignment = HorizontalAlignment.Stretch;
            scrollViewer.VerticalAlignment = VerticalAlignment.Stretch;
            scrollViewer.Margin = new Thickness(0);
            scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled;
            scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
            scrollViewer.Content = rootPanel;

            InitListViewBindings();

            return scrollViewer;
        }

        private static GridViewColumn CreateColumn(int width, string title, string tagAndBinding, string toolTip)
        {
            var column = new GridViewColumn();
            var header = new GridViewColumnHeader();
            if(!string.IsNullOrWhiteSpace(toolTip))
            {
                header.ToolTip = toolTip;
            }
            header.Content = title;
            header.Tag = tagAndBinding;
            column.Header = header;
            column.Width = width;
            column.DisplayMemberBinding = new Binding(tagAndBinding);

            return column;
        }

        private static GridViewColumn CreateColumn(int width, string title, string tagAndBinding)
        {
            return CreateColumn(width, title, tagAndBinding, null);
        }

        private void OnFileModeChecked()
        {
            var hasResults = _cutResultsListView.Items.Count > 0 && _fileResultsListView.Items.Count > 0;
            _resultsTextBlock.Visibility = hasResults ? Visibility.Visible : Visibility.Collapsed;
            _noResultsTextBlock.Visibility = hasResults ? Visibility.Collapsed : Visibility.Visible;
            _fileResultsListView.Visibility = hasResults ? Visibility.Visible : Visibility.Collapsed;
            _cutResultsListView.Visibility = Visibility.Collapsed;
        }

        private void OnCutModeChecked()
        {
            var hasResults = _cutResultsListView.Items.Count > 0 && _fileResultsListView.Items.Count > 0;
            _resultsTextBlock.Visibility = hasResults ? Visibility.Visible : Visibility.Collapsed;
            _noResultsTextBlock.Visibility = hasResults ? Visibility.Collapsed : Visibility.Visible;
            _fileResultsListView.Visibility = Visibility.Collapsed;
            _cutResultsListView.Visibility = hasResults ? Visibility.Visible : Visibility.Collapsed;
        }

        private void FixListViewMouseWheelHandling(ListView listView)
        {
            listView.AddHandler(ListView.MouseWheelEvent, new RoutedEventHandler(MouseWheelHandler), true);
        }

        private void MouseWheelHandler(object sender, RoutedEventArgs args)
        {
            if(_scrollViewer == null)
            {
                return;
            }

            var info = args as MouseWheelEventArgs;
            var posDelta = (double)info.Delta;
            var curPos = _scrollViewer.VerticalOffset;
            _scrollViewer.ScrollToVerticalOffset(curPos - posDelta);
        }

        private void InitListViewBindings()
        {
            _fileCopyCommand = new CommandBindingWrapper(_fileResultsListView, Key.C, () => true, OnCopyFileResultToClipboardClicked);
            _fileCutCommand = new CommandBindingWrapper(_fileResultsListView, () => true, OnCutFileResultClicked);
            _fileRevealCommand = new CommandBindingWrapper(_fileResultsListView, CanExecuteFileRevealCommand, OnRevealFileResultClicked);
            _fileSelectCommand = new CommandBindingWrapper(_fileResultsListView, () => true, OnSelectFileResultClicked);

            _cutCopyCommand = new CommandBindingWrapper(_cutResultsListView, Key.C, () => true, OnCopyCutResultToClipboardClicked);
            _cutCutCommand = new CommandBindingWrapper(_cutResultsListView, () => true, OnCutCutResultClicked);
            _cutRevealCommand = new CommandBindingWrapper(_cutResultsListView, CanExecuteCutRevealCommand, OnRevealCutResultClicked);
            _cutSelectCommand = new CommandBindingWrapper(_cutResultsListView, () => true, OnSelectCutResultClicked);
        }

        private void OnCopyCutResultToClipboardClicked()
        {
            App.CopyListViewRowsToClipboard(_cutResultsListView);
        }

        private void OnCopyFileResultToClipboardClicked()
        {
            App.CopyListViewRowsToClipboard(_fileResultsListView);
        }

        private void OnCutCutResultClicked()
        {
            OnCutClicked(false);
        }

        private void OnCutFileResultClicked()
        {
            OnCutClicked(true);
        }

        private void OnRevealFileResultClicked()
        {
            try
            {
                var items = _fileResultsListView.SelectedItems;
                if(items.Count != 1)
                {
                    return;
                }
                var item = items[0];
                var listViewItem = item as ListViewItem;
                var displayInfo = listViewItem.Content as SearchResultFileDisplayInfo;
                Process.Start("explorer.exe", "/select," + _resultDemos[(int)displayInfo.FileIndex].FilePath);
            }
            catch(Exception)
            {
            }
        }

        private void OnRevealCutResultClicked()
        {
            try
            {
                var items = _cutResultsListView.SelectedItems;
                if(items.Count != 1)
                {
                    return;
                }
                var item = items[0];
                var listViewItem = item as ListViewItem;
                var displayInfo = listViewItem.Content as SearchResultCutDisplayInfo;
                Process.Start("explorer.exe", "/select," + _resultDemos[(int)displayInfo.Match.DemoInputIndex].FilePath);
            }
            catch(Exception)
            {
            }
        }

        private void OnSelectFileResultClicked()
        {
            try
            {
                var demos = new HashSet<DemoInfo>();
                var items = _fileResultsListView.SelectedItems;
                foreach(var item in items)
                {
                    var listViewItem = item as ListViewItem;
                    var displayInfo = listViewItem.Content as SearchResultFileDisplayInfo;
                    demos.Add(displayInfo.Demo);
                }
                _app.SelectDemos(demos);
            }
            catch(Exception)
            {
            }
        }

        private void OnSelectCutResultClicked()
        {
            try
            {
                var demos = new HashSet<DemoInfo>();
                var items = _cutResultsListView.SelectedItems;
                foreach(var item in items)
                {
                    var listViewItem = item as ListViewItem;
                    var displayInfo = listViewItem.Content as SearchResultCutDisplayInfo;
                    demos.Add(displayInfo.Demo);
                }
                _app.SelectDemos(demos);
            }
            catch(Exception)
            {
            }
        }

        private void OnCutClicked(bool fileMode)
        {
            var cutLists = fileMode ? GetFileMatches() : GetCutMatches();
            if(cutLists.FileCuts.Count == 0)
            {
                _app.LogError("No cut/file selected or none has a protocol that can be written");
                return;
            }

            _app.SaveBothConfigs();
            _app.DisableUiNonThreadSafe();
            _app.JoinJobThread();

            _app.StartJobThread(DemoCutThread, cutLists);
        }

        private class FileCuts
        {
            public string FilePath;
            public readonly List<UDT_DLL.Cut> Cuts = new List<UDT_DLL.Cut>();
        }

        private class CutLists
        {
            public readonly List<FileCuts> FileCuts = new List<FileCuts>();
        }

        private void DemoCutThread(object arg)
        {
            var cuts = arg as CutLists;
            if(cuts == null)
            {
                _app.LogError("Invalid thread argument");
                return;
            }

            _app.InitParseArg();
            _app.ParseArg.ProgressCb = (progress, context) => {};
            
            try
            {
                var progress = 0.0;
                var fileCount = cuts.FileCuts.Count;
                foreach(var fileCuts in cuts.FileCuts)
                {
                    UDT_DLL.CutDemoByTimes(_app.GetMainThreadContext(), ref _app.ParseArg, fileCuts.FilePath, fileCuts.Cuts);
                    progress += 1.0 / (double)fileCount;
                    _app.SetProgressThreadSafe(100.0 * progress);
                }
            }
            catch(Exception exception)
            {
                _app.LogError("Caught an exception while cutting demos: {0}", exception.Message);
            }
        }

        private CutLists GetFileMatches()
        {
            var items = _fileResultsListView.SelectedItems;
            if(items.Count == 0)
            {
                return null;
            }

            var result = new CutLists();
            foreach(var item in items)
            {
                var listViewItem = item as ListViewItem;
                if(listViewItem == null)
                {
                    continue;
                }

                var displayInfo = listViewItem.Content as SearchResultFileDisplayInfo;
                if(displayInfo == null)
                {
                    continue;
                }

                if(displayInfo.FileIndex >= (uint)_resultDemos.Count)
                {
                    continue;
                }

                var filePath = _resultDemos[(int)displayInfo.FileIndex].FilePath;
                if(!App.IsValidWriteProtocol(App.GetProtocolFromFilePath(filePath)))
                {
                    continue;
                }

                var matches = _results.FindAll(c => c.DemoInputIndex == displayInfo.FileIndex);
                var fileCuts = new FileCuts();
                fileCuts.FilePath = filePath;
                foreach(var match in matches)
                {
                    var cut = new UDT_DLL.Cut();
                    cut.GameStateIndex = (int)match.GameStateIndex;
                    cut.StartTimeMs = match.StartTimeMs;
                    cut.EndTimeMs = match.EndTimeMs;
                    fileCuts.Cuts.Add(cut);
                }
                result.FileCuts.Add(fileCuts);
            }

            return result;
        }

        private CutLists GetCutMatches()
        {
            var items = _cutResultsListView.SelectedItems;
            if(items.Count == 0)
            {
                return null;
            }

            uint currentFileIndex = uint.MaxValue;
            var fileCuts = new FileCuts();
            var result = new CutLists();
            foreach(var item in items)
            {
                var listViewItem = item as ListViewItem;
                if(listViewItem == null)
                {
                    continue;
                }

                var displayInfo = listViewItem.Content as SearchResultCutDisplayInfo;
                if(displayInfo == null)
                {
                    continue;
                }

                var match = displayInfo.Match;
                if(match.DemoInputIndex >= (uint)_resultDemos.Count)
                {
                    continue;
                }

                if(currentFileIndex != match.DemoInputIndex)
                {
                    if(fileCuts.Cuts.Count > 0)
                    {
                        fileCuts.FilePath = _resultDemos[(int)currentFileIndex].FilePath;
                        result.FileCuts.Add(fileCuts);
                        fileCuts = new FileCuts();
                    }
                }

                var cut = new UDT_DLL.Cut();
                cut.GameStateIndex = (int)match.GameStateIndex;
                cut.StartTimeMs = match.StartTimeMs;
                cut.EndTimeMs = match.EndTimeMs;
                fileCuts.Cuts.Add(cut);

                currentFileIndex = match.DemoInputIndex;
            }
            if(fileCuts.Cuts.Count > 0)
            {
                fileCuts.FilePath = _resultDemos[(int)currentFileIndex].FilePath;
                result.FileCuts.Add(fileCuts);
            }

            return result;
        }

        private bool CanExecuteFileRevealCommand()
        {
            var items = _fileResultsListView.SelectedItems;

            return items != null && items.Count == 1;
        }

        private bool CanExecuteCutRevealCommand()
        {
            var items = _cutResultsListView.SelectedItems;

            return items != null && items.Count == 1;
        }
    }
}