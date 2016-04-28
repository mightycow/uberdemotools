using System;
using System.Collections.Generic;
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
        public SearchResultFileDisplayInfo(string fileName, string matchCount, string patterns)
        {
            FileName = fileName;
            MatchCount = matchCount;
            Patterns = patterns;
        }

        public override string ToString()
        {
            return "@TODO:";
        }

        public string FileName { get; set; }
        public string MatchCount { get; set; }
        public string Patterns { get; set; }
    }

    public class SearchResultCutDisplayInfo
    {
        public SearchResultCutDisplayInfo(string fileName, string gs, string start, string end, string patterns)
        {
            FileName = fileName;
            GSIndex = gs;
            StartTime = start;
            EndTime = end;
            Patterns = patterns;
        }

        public override string ToString()
        {
            return "@TODO:";
        }

        public string FileName { get; set; }
        public string GSIndex { get; set; }
        public string StartTime { get; set; }
        public string EndTime { get; set; }
        public string Patterns { get; set; }
    }

    public class SearchResultsComponent : AppComponent
    {
        private App _app;
        private TextBlock _noResultsTextBlock;
        private DemoInfoListView _fileResultsListView;
        private DemoInfoListView _cutResultsListView;
        private RadioButton _displayFilesRadioButton;
        private ScrollViewer _scrollViewer;

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

        public void UpdateResults(List<UDT_DLL.udtPatternMatch> results, List<string> filePaths)
        {
            _cutResultsListView.Items.Clear();
            _fileResultsListView.Items.Clear();

            results.Sort((a, b) => a.StartTimeMs.CompareTo(b.StartTimeMs));
            results.StableSort((a, b) => a.GameStateIndex.CompareTo(b.GameStateIndex));
            results.StableSort((a, b) => a.DemoInputIndex.CompareTo(b.DemoInputIndex));

            foreach(var result in results)
            {
                var index = result.DemoInputIndex;
                if(index >= filePaths.Count)
                {
                    continue;
                }

                var fileName = Path.GetFileNameWithoutExtension(filePaths[(int)index]);
                var gs = result.GameStateIndex.ToString();
                var start = App.FormatMinutesSeconds(result.StartTimeMs / 1000);
                var end = App.FormatMinutesSeconds(result.EndTimeMs / 1000);
                var patterns = FormatPatterns(result.Patterns);
                var cutResult = new SearchResultCutDisplayInfo(fileName, gs, start, end, patterns);
                var cutItem = new ListViewItem();
                cutItem.Content = cutResult;
                _cutResultsListView.Items.Add(cutItem);
            }

            uint currentFileIndex = uint.MaxValue;
            var fileResults = new List<UDT_DLL.udtPatternMatch>();
            foreach(var result in results)
            {
                var index = result.DemoInputIndex;
                if(index >= filePaths.Count)
                {
                    continue;
                }

                if(currentFileIndex != index)
                {
                    if(fileResults.Count > 0)
                    {
                        AddSingleFileResult(fileResults, filePaths);
                        fileResults.Clear();
                    }
                }
                fileResults.Add(result);
                currentFileIndex = index;
            }
            if(fileResults.Count > 0)
            {
                AddSingleFileResult(fileResults, filePaths);
            }

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

        private void AddSingleFileResult(List<UDT_DLL.udtPatternMatch> results, List<string> filePaths)
        {
            uint patternBits = 0;
            foreach(var result in results)
            {
                patternBits |= result.Patterns;
            }

            var fileName = Path.GetFileNameWithoutExtension(filePaths[(int)results[0].DemoInputIndex]);
            var matches = results.Count.ToString();
            var patterns = FormatPatterns(patternBits);
            var cutResult = new SearchResultFileDisplayInfo(fileName, matches, patterns);
            var fileItem = new ListViewItem();
            fileItem.Content = cutResult;
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
            fileResultsGridView.Columns.Add(CreateColumn(300, "File Name", "FileName"));
            fileResultsGridView.Columns.Add(CreateColumn(75, "Matches", "MatchCount"));
            fileResultsGridView.Columns.Add(CreateColumn(200, "Patterns", "Patterns"));

            var fileResultsListView = new DemoInfoListView();
            _fileResultsListView = fileResultsListView;
            fileResultsListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            fileResultsListView.VerticalAlignment = VerticalAlignment.Stretch;
            fileResultsListView.Margin = new Thickness(5);
            fileResultsListView.View = fileResultsGridView;
            fileResultsListView.SelectionMode = SelectionMode.Extended;

            var cutResultsGridView = new GridView();
            cutResultsGridView.AllowsColumnReorder = false;
            cutResultsGridView.Columns.Add(CreateColumn(300, "File Name", "FileName"));
            cutResultsGridView.Columns.Add(CreateColumn(40, "GS", "GSIndex", "GameState Index"));
            cutResultsGridView.Columns.Add(CreateColumn(50, "Start", "StartTime"));
            cutResultsGridView.Columns.Add(CreateColumn(50, "End", "EndTime"));
            cutResultsGridView.Columns.Add(CreateColumn(150, "Patterns", "Patterns"));

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

            var resultsStackPanel = new StackPanel();
            resultsStackPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            resultsStackPanel.VerticalAlignment = VerticalAlignment.Stretch;
            resultsStackPanel.Margin = new Thickness(5);
            resultsStackPanel.Orientation = Orientation.Vertical;
            resultsStackPanel.Children.Add(noResultsTextBlock);
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
            _noResultsTextBlock.Visibility = hasResults ? Visibility.Collapsed : Visibility.Visible;
            _fileResultsListView.Visibility = hasResults ? Visibility.Visible : Visibility.Collapsed;
            _cutResultsListView.Visibility = Visibility.Collapsed;
        }

        private void OnCutModeChecked()
        {
            var hasResults = _cutResultsListView.Items.Count > 0 && _fileResultsListView.Items.Count > 0;
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
    }
}