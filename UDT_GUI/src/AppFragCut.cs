using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Media;


namespace Uber.DemoTools
{
    public class FilterGroupBox
    {
        public FrameworkElement RootElement { get; private set; }

        private List<CheckBox> _checkBoxes = new List<CheckBox>();

        public FilterGroupBox(string header, UDT_DLL.udtStringArray stringArrayId, int columnCount = 2)
        {
            var enableAllButton = new Button();
            enableAllButton.HorizontalAlignment = HorizontalAlignment.Left;
            enableAllButton.VerticalAlignment = VerticalAlignment.Top;
            enableAllButton.Content = "Check All";
            enableAllButton.Width = 75;
            enableAllButton.Height = 25;
            enableAllButton.Margin = new Thickness(5);
            enableAllButton.Click += (obj, args) => SetAllChecked(true);

            var disableAllButton = new Button();
            disableAllButton.HorizontalAlignment = HorizontalAlignment.Left;
            disableAllButton.VerticalAlignment = VerticalAlignment.Top;
            disableAllButton.Content = "Uncheck All";
            disableAllButton.Width = 75;
            disableAllButton.Height = 25;
            disableAllButton.Margin = new Thickness(5);
            disableAllButton.Click += (obj, args) => SetAllChecked(false);

            var rootPanel = new StackPanel();
            rootPanel.Margin = new Thickness(5);
            rootPanel.Orientation = Orientation.Vertical;
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;

            var panel = new Grid();
            panel.Margin = new Thickness(5);
            panel.HorizontalAlignment = HorizontalAlignment.Stretch;
            panel.VerticalAlignment = VerticalAlignment.Stretch;

            const int rowOffset = 1; // One additional row for the EnableAll/DisableAll buttons.
            var itemNames = UDT_DLL.GetStringArray(stringArrayId);
            var rowCount = (itemNames.Count + columnCount - 1) / columnCount;
            var maxCheckBoxesPerColumn = rowCount;
            rowCount += rowOffset;

            for(var i = 0; i < rowCount; ++i)
            {
                panel.RowDefinitions.Add(new RowDefinition());
            }

            for(var i = 0; i < columnCount; ++i)
            {
                panel.ColumnDefinitions.Add(new ColumnDefinition());
            }

            panel.Children.Add(enableAllButton);
            panel.Children.Add(disableAllButton);
            Grid.SetRow(enableAllButton, 0);
            Grid.SetColumn(enableAllButton, 0);
            Grid.SetRow(disableAllButton, 0);
            Grid.SetColumn(disableAllButton, 1);

            for(var i = 0; i < itemNames.Count; ++i)
            {
                var name = itemNames[i];
                var rowIdx = i % maxCheckBoxesPerColumn;
                var columnIdx = i / maxCheckBoxesPerColumn;
                rowIdx += rowOffset;

                var checkBox = new CheckBox();
                checkBox.Margin = new Thickness(5, 0, 5, 5);
                checkBox.HorizontalAlignment = HorizontalAlignment.Left;
                checkBox.VerticalAlignment = VerticalAlignment.Center;
                checkBox.IsChecked = true;
                checkBox.Content = " " + name.Substring(0, 1).ToUpper() + name.Substring(1);
                Grid.SetRow(checkBox, rowIdx);
                Grid.SetColumn(checkBox, columnIdx);
                _checkBoxes.Add(checkBox);

                panel.Children.Add(checkBox);
            }

            rootPanel.Children.Add(panel);

            var groupBox = new GroupBox();
            groupBox.Header = header;
            groupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            groupBox.VerticalAlignment = VerticalAlignment.Stretch;
            groupBox.Margin = new Thickness(5);
            groupBox.Content = rootPanel;

            RootElement = groupBox;
        }

        public UInt32 GetBitMask()
        {
            UInt32 result = 0;
            for(var i = 0; i < _checkBoxes.Count; ++i)
            {
                var isChecked = _checkBoxes[i].IsChecked ?? false;
                if(isChecked)
                {
                    result |= (UInt32)(1 << i);
                }
            }

            return result;
        }

        private void SetAllChecked(bool isChecked)
        {
            foreach(var checkBox in _checkBoxes)
            {
                checkBox.IsChecked = isChecked;
            }
        }
    }

    public class FragSequenceFiltersComponent : AppComponent
    {
        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return null; } }
        public List<DemoInfoListView> InfoListViews { get { return null; } }
        public ComponentType Type { get { return ComponentType.ChatFilters; } }

        public FragSequenceFiltersComponent(App app)
        {
            _app = app;
            RootControl = CreateCutByFragTab();
        }

        public void PopulateViews(DemoInfo demoInfo)
        { 
            // Nothing to do.
        }

        public void SaveToConfigObject(UdtConfig config)
        {
            int intValue = 0;

            if(int.TryParse(_minFragCountEditBox.Text, out intValue))
            {
                _app.Config.FragCutMinFragCount = intValue;
            }

            if(int.TryParse(_timeBetweenFragsEditBox.Text, out intValue))
            {
                _app.Config.FragCutTimeBetweenFrags = intValue;
            }

            _app.Config.FragCutAllowSelfKills = _allowSelfKillsCheckBox.IsChecked.HasValue && _allowSelfKillsCheckBox.IsChecked.Value;
            _app.Config.FragCutAllowTeamKills = _allowTeamKillsCheckBox.IsChecked.HasValue && _allowTeamKillsCheckBox.IsChecked.Value;
            _app.Config.FragCutAllowAnyDeath = _allowAnyDeathCheckBox.IsChecked.HasValue && _allowAnyDeathCheckBox.IsChecked.Value;
        }

        private App _app;
        private TextBox _minFragCountEditBox = null;
        private TextBox _timeBetweenFragsEditBox = null;
        private CheckBox _allowSelfKillsCheckBox = null;
        private CheckBox _allowTeamKillsCheckBox = null;
        private CheckBox _allowAnyDeathCheckBox = null;
        private RadioButton _autoPlayerSelectionRadioButton = null;
        private RadioButton _manualPlayerSelectionRadioButton = null;
        private ComboBox _playerIndexComboBox = null;
        private FilterGroupBox _playerMODFilters = null;

        private FrameworkElement CreateCutByFragTab()
        {
            _playerMODFilters = new FilterGroupBox("Means of Death Filters", UDT_DLL.udtStringArray.PlayerMeansOfDeath, 3);
            // @TODO: Power-up Filters

            var minFragCountEditBox = new TextBox();
            _minFragCountEditBox = minFragCountEditBox;
            minFragCountEditBox.Width = 40;
            minFragCountEditBox.Text = _app.Config.FragCutMinFragCount.ToString();

            var timeBetweenFragsEditBox = new TextBox();
            _timeBetweenFragsEditBox = timeBetweenFragsEditBox;
            timeBetweenFragsEditBox.Width = 40;
            timeBetweenFragsEditBox.Text = _app.Config.FragCutTimeBetweenFrags.ToString();

            var allowSelfKillsCheckBox = new CheckBox();
            _allowSelfKillsCheckBox = allowSelfKillsCheckBox;
            allowSelfKillsCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            allowSelfKillsCheckBox.VerticalAlignment = VerticalAlignment.Center;
            allowSelfKillsCheckBox.IsChecked = _app.Config.FragCutAllowSelfKills;
            allowSelfKillsCheckBox.ToolTip = "Self kills are suicides where the player shot himself with a weapon";

            var allowTeamKillsCheckBox = new CheckBox();
            _allowTeamKillsCheckBox = allowTeamKillsCheckBox;
            allowTeamKillsCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            allowTeamKillsCheckBox.VerticalAlignment = VerticalAlignment.Center;
            allowTeamKillsCheckBox.IsChecked = _app.Config.FragCutAllowTeamKills;

            var allowAnyDeathCheckBox = new CheckBox();
            _allowAnyDeathCheckBox = allowAnyDeathCheckBox;
            allowAnyDeathCheckBox.HorizontalAlignment = HorizontalAlignment.Left;
            allowAnyDeathCheckBox.VerticalAlignment = VerticalAlignment.Center;
            allowAnyDeathCheckBox.IsChecked = _app.Config.FragCutAllowAnyDeath;
            allowAnyDeathCheckBox.ToolTip = "This includes suicides where the killer is the world: lava, fall damage, hurt triggers, etc";

            var rulesPanelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            rulesPanelList.Add(App.CreateTuple("Min. Frag Count", minFragCountEditBox));
            rulesPanelList.Add(App.CreateTuple("Time Between Frags", timeBetweenFragsEditBox));
            rulesPanelList.Add(App.CreateTuple("Allow Self Kills?", allowSelfKillsCheckBox));
            rulesPanelList.Add(App.CreateTuple("Allow Any Death?", allowAnyDeathCheckBox));
            rulesPanelList.Add(App.CreateTuple("Allow Team Kills?", allowTeamKillsCheckBox));
            
            var rulesPanel = WpfHelper.CreateDualColumnPanel(rulesPanelList, 120, 5);
            rulesPanel.HorizontalAlignment = HorizontalAlignment.Center;
            rulesPanel.VerticalAlignment = VerticalAlignment.Center;

            var rulesGroupBox = new GroupBox();
            rulesGroupBox.Header = "Frag Rules";
            rulesGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            rulesGroupBox.VerticalAlignment = VerticalAlignment.Top;
            rulesGroupBox.Margin = new Thickness(5);
            rulesGroupBox.Content = rulesPanel;

            var autoPlayerSelectionRadioButton = new RadioButton();
            autoPlayerSelectionRadioButton.HorizontalAlignment = HorizontalAlignment.Left;
            autoPlayerSelectionRadioButton.VerticalAlignment = VerticalAlignment.Center;
            _autoPlayerSelectionRadioButton = autoPlayerSelectionRadioButton;
            autoPlayerSelectionRadioButton.GroupName = "PlayerSelection";
            autoPlayerSelectionRadioButton.Content = " Recording Player";
            autoPlayerSelectionRadioButton.ToolTip = "The player who recorded the demo";
            autoPlayerSelectionRadioButton.IsChecked = true;

            var manualPlayerSelectionRadioButton = new RadioButton();
            manualPlayerSelectionRadioButton.HorizontalAlignment = HorizontalAlignment.Left;
            manualPlayerSelectionRadioButton.VerticalAlignment = VerticalAlignment.Center;
            _manualPlayerSelectionRadioButton = manualPlayerSelectionRadioButton;
            manualPlayerSelectionRadioButton.GroupName = "PlayerSelection";
            manualPlayerSelectionRadioButton.Content = " This Player";
            manualPlayerSelectionRadioButton.IsChecked = false;

            var playerIndexComboBox = new ComboBox();
            playerIndexComboBox.HorizontalAlignment = HorizontalAlignment.Left;
            playerIndexComboBox.VerticalAlignment = VerticalAlignment.Center;
            _playerIndexComboBox = playerIndexComboBox;
            playerIndexComboBox.Margin = new Thickness(5, 0, 0, 0);
            playerIndexComboBox.Width = 40;
            for(var i = 0; i < 64; ++i)
            {
                playerIndexComboBox.Items.Add(i.ToString());
            }
            playerIndexComboBox.SelectedIndex = 0;

            var manualPlayerSelectionRow = new StackPanel();
            manualPlayerSelectionRow.Margin = new Thickness(0, 5, 0, 0);
            manualPlayerSelectionRow.Orientation = Orientation.Horizontal;
            manualPlayerSelectionRow.HorizontalAlignment = HorizontalAlignment.Stretch;
            manualPlayerSelectionRow.VerticalAlignment = VerticalAlignment.Stretch;
            manualPlayerSelectionRow.Children.Add(manualPlayerSelectionRadioButton);
            manualPlayerSelectionRow.Children.Add(playerIndexComboBox);

            var playerSelectionPanel = new StackPanel();
            playerSelectionPanel.Margin = new Thickness(10);
            playerSelectionPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            playerSelectionPanel.VerticalAlignment = VerticalAlignment.Stretch;
            playerSelectionPanel.Children.Add(autoPlayerSelectionRadioButton);
            playerSelectionPanel.Children.Add(manualPlayerSelectionRow);

            var playerSelectionGroupBox = new GroupBox();
            playerSelectionGroupBox.Margin = new Thickness(5);
            playerSelectionGroupBox.Header = "Player Selection";
            playerSelectionGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            playerSelectionGroupBox.VerticalAlignment = VerticalAlignment.Top;
            playerSelectionGroupBox.Content = playerSelectionPanel;

            var cutButton = new Button();
            cutButton.HorizontalAlignment = HorizontalAlignment.Left;
            cutButton.VerticalAlignment = VerticalAlignment.Top;
            cutButton.Content = "Cut!";
            cutButton.Width = 75;
            cutButton.Height = 25;
            cutButton.Margin = new Thickness(5);
            cutButton.Click += (obj, args) => OnCutByFragClicked();

            var actionsGroupBox = new GroupBox();
            actionsGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            actionsGroupBox.VerticalAlignment = VerticalAlignment.Top;
            actionsGroupBox.Margin = new Thickness(5);
            actionsGroupBox.Header = "Actions";
            actionsGroupBox.Content = cutButton;
            
            var helpTextBlock = new TextBlock();
            helpTextBlock.Margin = new Thickness(5);
            helpTextBlock.TextWrapping = TextWrapping.WrapWithOverflow;
            helpTextBlock.Text = 
                "Self-kills are suicides where the player shot himself with a weapon." +
                "\nDeaths can include 'suicides' where the killer is the world: lava, fall damage, hurt triggers, etc.";
            
            var helpGroupBox = new GroupBox();
            helpGroupBox.Margin = new Thickness(5);
            helpGroupBox.Header = "Help";
            helpGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            helpGroupBox.VerticalAlignment = VerticalAlignment.Top;
            helpGroupBox.Content = helpTextBlock;
            
            var rootPanel = new WrapPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.Margin = new Thickness(5);
            rootPanel.Orientation = Orientation.Horizontal;
            rootPanel.Children.Add(rulesGroupBox);
            rootPanel.Children.Add(playerSelectionGroupBox);
            rootPanel.Children.Add(actionsGroupBox);
            rootPanel.Children.Add(helpGroupBox);
            rootPanel.Children.Add(new WpfHelper.WrapPanelNewLine());
            rootPanel.Children.Add(_playerMODFilters.RootElement);

            var scrollViewer = new ScrollViewer();
            scrollViewer.HorizontalAlignment = HorizontalAlignment.Stretch;
            scrollViewer.VerticalAlignment = VerticalAlignment.Stretch;
            scrollViewer.Margin = new Thickness(5);
            scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled;
            scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
            scrollViewer.Content = rootPanel;

            return scrollViewer; 
        }

        private class ThreadArg
        {
            public List<string> FilePaths = null;
            public int PlayerIndex = -1;
            public UInt32 AllowedMeansOfDeaths = 0;
        }

        private void OnCutByFragClicked()
        {
            var demos = _app.SelectedDemos;
            if(demos == null)
            {
                _app.LogError("No demo was selected. Please select one to proceed.");
                return;
            }

            var playerIndex = -1;
            var manualMode = _manualPlayerSelectionRadioButton.IsChecked ?? false;
            if(manualMode)
            {
                playerIndex = _playerIndexComboBox.SelectedIndex;
            }

            UInt32 allowedPlayerMODs = _playerMODFilters.GetBitMask();
            if(allowedPlayerMODs == 0)
            {
                _app.LogError("You didn't check any Mean of Death. Please check at least one to proceed.");
                return;
            }

            _app.SaveConfig();
            if(_app.Config.FragCutMinFragCount < 2)
            {
                _app.LogError("'Min. Frag Count' must be 2 or higher.");
                return;
            }
            if(_app.Config.FragCutTimeBetweenFrags < 1)
            {
                _app.LogError("'Time Between Frags' must be strictly positive.");
                return;
            }

            _app.DisableUiNonThreadSafe();
            _app.JoinJobThread();

            var filePaths = new List<string>();
            foreach(var demo in demos)
            {
                filePaths.Add(demo.FilePath);
            }

            var threadArg = new ThreadArg();
            threadArg.FilePaths = filePaths;
            threadArg.PlayerIndex = playerIndex;
            threadArg.AllowedMeansOfDeaths = allowedPlayerMODs;

            _app.StartJobThread(DemoCutByFragThread, threadArg);
        }

        private void DemoCutByFragThread(object arg)
        {
            var threadArg = arg as ThreadArg;
            if(threadArg == null)
            {
                _app.LogError("Invalid thread argument type");
                return;
            }

            var filePaths = threadArg.FilePaths;
            if(filePaths == null)
            {
                _app.LogError("Invalid thread argument data");
                return;
            }

            var outputFolder = _app.GetOutputFolder();
            var outputFolderPtr = Marshal.StringToHGlobalAnsi(outputFolder);
            _app.InitParseArg();
            _app.ParseArg.OutputFolderPath = outputFolderPtr;

            var config = _app.Config;
            UInt32 flags = 0;
            if(config.FragCutAllowAnyDeath)
            {
                flags |= (UInt32)UDT_DLL.udtCutByFragArgFlags.AllowDeaths;
            }
            if(config.FragCutAllowSelfKills)
            {
                flags |= (UInt32)UDT_DLL.udtCutByFragArgFlags.AllowSelfKills;
            }
            if(config.FragCutAllowTeamKills)
            {
                flags |= (UInt32)UDT_DLL.udtCutByFragArgFlags.AllowTeamKills;
            }

            try
            {
                var rules = new UDT_DLL.udtCutByFragArg();
                rules.MinFragCount = (UInt32)config.FragCutMinFragCount;
                rules.TimeBetweenFragsSec = (UInt32)config.FragCutTimeBetweenFrags;
                rules.TimeMode = 0; // @TODO:
                rules.Flags = flags;
                rules.PlayerIndex = (Int32)threadArg.PlayerIndex;
                rules.AllowedMeansOfDeaths = threadArg.AllowedMeansOfDeaths;
                UDT_DLL.CutDemosByFrag(ref _app.ParseArg, filePaths, rules, config.CutStartOffset, config.CutEndOffset, config.MaxThreadCount);
            }
            catch(Exception exception)
            {
                _app.LogError("Caught an exception while cutting demos: {0}", exception.Message);
            }

            Marshal.FreeHGlobal(outputFolderPtr);
        }
    }
}