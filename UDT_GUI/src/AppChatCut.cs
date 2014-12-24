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
    public class ChatFiltersComponent : AppComponent
    {
        public FrameworkElement RootControl { get; private set; }
        public List<DemoInfoListView> AllListViews { get { return new List<DemoInfoListView> { _chatRulesListView }; } }
        public List<DemoInfoListView> InfoListViews { get { return null; } }
        public ComponentType Type { get { return ComponentType.ChatFilters; } }

        public ChatFiltersComponent(App app)
        {
            _app = app;
            RootControl = CreateCutByChatTab();
        }

        public void PopulateViews(DemoInfo demoInfo)
        { 
            // Nothing to do.
        }

        public void SaveToConfigObject(UdtConfig config)
        {
            // Nothing to do.
        }

        private App _app;

        private class ChatRuleDisplayInfo
        {
            public ChatRuleDisplayInfo(ChatRule rule)
            {
                Operator = rule.Operator;
                Pattern = rule.Value;
                CaseSensitive = rule.CaseSensitive;
                IgnoreColors = rule.IgnoreColors;
            }

            public string Operator { get; set; }
            public string Pattern { get; set; }
            public bool CaseSensitive { get; set; }
            public bool IgnoreColors { get; set; }
        }

        private DemoInfoListView _chatRulesListView = null;

        private FrameworkElement CreateCutByChatTab()
        {
            var chatRulesGridView = new GridView();
            chatRulesGridView.AllowsColumnReorder = false;
            chatRulesGridView.Columns.Add(new GridViewColumn { Header = "Operator", Width = 100, DisplayMemberBinding = new Binding("Operator") });
            chatRulesGridView.Columns.Add(new GridViewColumn { Header = "Pattern", Width = 175, DisplayMemberBinding = new Binding("Pattern") });
            chatRulesGridView.Columns.Add(new GridViewColumn { Header = "Case Sensitive", Width = 100, DisplayMemberBinding = new Binding("CaseSensitive") });
            chatRulesGridView.Columns.Add(new GridViewColumn { Header = "Ignore Colors", Width = 100, DisplayMemberBinding = new Binding("IgnoreColors") });

            var chatRulesListView = new DemoInfoListView();
            _chatRulesListView = chatRulesListView;
            chatRulesListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            chatRulesListView.VerticalAlignment = VerticalAlignment.Stretch;
            chatRulesListView.Margin = new Thickness(5);
            chatRulesListView.View = chatRulesGridView;
            chatRulesListView.SelectionMode = SelectionMode.Single;
            chatRulesListView.Width = 485;
            chatRulesListView.Foreground = new SolidColorBrush(Colors.Black);
            foreach(var rule in _app.Config.ChatRules)
            {
                chatRulesListView.Items.Add(new ChatRuleDisplayInfo(rule));
            }

            var addButton = new Button();
            addButton.Content = "Add";
            addButton.Width = 75;
            addButton.Height = 25;
            addButton.Margin = new Thickness(0, 0, 0, 5);
            addButton.Click += (obj, args) => OnAddChatRuleClicked();

            var editButton = new Button();
            editButton.Content = "Edit";
            editButton.Width = 75;
            editButton.Height = 25;
            editButton.Margin = new Thickness(0, 0, 0, 5);
            editButton.Click += (obj, args) => OnEditChatRuleClicked();

            var removeButton = new Button();
            removeButton.Content = "Remove";
            removeButton.Width = 75;
            removeButton.Height = 25;
            removeButton.Margin = new Thickness(0, 0, 0, 5);
            removeButton.Click += (obj, args) => OnRemoveChatRuleClicked();

            var chatRulesVerPanel = new StackPanel();
            chatRulesVerPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            chatRulesVerPanel.VerticalAlignment = VerticalAlignment.Stretch;
            chatRulesVerPanel.Margin = new Thickness(5);
            chatRulesVerPanel.Orientation = Orientation.Vertical;
            chatRulesVerPanel.Children.Add(addButton);
            chatRulesVerPanel.Children.Add(editButton);
            chatRulesVerPanel.Children.Add(removeButton);

            var chatRulesHorPanel = new StackPanel();
            chatRulesHorPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            chatRulesHorPanel.VerticalAlignment = VerticalAlignment.Stretch;
            chatRulesHorPanel.Margin = new Thickness(5);
            chatRulesHorPanel.Orientation = Orientation.Horizontal;
            chatRulesHorPanel.Children.Add(chatRulesListView);
            chatRulesHorPanel.Children.Add(chatRulesVerPanel);

            var chatRulesGroupBox = new GroupBox();
            chatRulesGroupBox.Header = "Chat Rules";
            chatRulesGroupBox.HorizontalAlignment = HorizontalAlignment.Stretch;
            chatRulesGroupBox.VerticalAlignment = VerticalAlignment.Stretch;
            chatRulesGroupBox.Margin = new Thickness(5);
            chatRulesGroupBox.Content = chatRulesHorPanel;

            var cutButton = new Button();
            cutButton.HorizontalAlignment = HorizontalAlignment.Left;
            cutButton.VerticalAlignment = VerticalAlignment.Top;
            cutButton.Content = "Cut!";
            cutButton.Width = 75;
            cutButton.Height = 25;
            cutButton.Margin = new Thickness(5);
            cutButton.Click += (obj, args) => OnCutByChatClicked();

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
                "The StartsWith pattern matching operator is currently applied to the start of the original chat command, not the start of the message portion itself." +
                "\nIt is therefore advised to use the Contains or EndsWith modes unless you have something very specific in mind (and know the Quake protocol well).";

            var helpGroupBox = new GroupBox();
            helpGroupBox.Margin = new Thickness(5);
            helpGroupBox.Header = "Help";
            helpGroupBox.Content = helpTextBlock;

            var rootPanel = new WrapPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.Margin = new Thickness(5);
            rootPanel.Orientation = Orientation.Horizontal;
            rootPanel.Children.Add(chatRulesGroupBox);
            rootPanel.Children.Add(actionsGroupBox);
            rootPanel.Children.Add(helpGroupBox);

            var scrollViewer = new ScrollViewer();
            scrollViewer.HorizontalAlignment = HorizontalAlignment.Stretch;
            scrollViewer.VerticalAlignment = VerticalAlignment.Stretch;
            scrollViewer.Margin = new Thickness(5);
            scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled;
            scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
            scrollViewer.Content = rootPanel;

            return scrollViewer; 
        }

        private void OnCutByChatClicked()
        {
            var demos = _app.SelectedDemos;
            if(demos == null)
            {
                _app.LogError("No demo was selected. Please select one to proceed.");
                return;
            }

            _app.SaveConfig();
            if(_app.Config.ChatRules.Count == 0)
            {
                _app.LogError("Not chat matching rules defined. Please add at least one to proceed.");
            }

            _app.DisableUiNonThreadSafe();
            _app.JoinJobThread();

            var filePaths = new List<string>();
            foreach(var demo in demos)
            {
                filePaths.Add(demo.FilePath);
            }

            _app.StartJobThread(DemoCutByChatThread, filePaths);
        }

        private void DemoCutByChatThread(object arg)
        {
            var filePaths = arg as List<string>;
            if(filePaths == null)
            {
                _app.LogError("Invalid thread argument type");
                return;
            }

            var outputFolder = _app.GetOutputFolder();
            var outputFolderPtr = Marshal.StringToHGlobalAnsi(outputFolder);
            _app.InitParseArg();
            _app.ParseArg.OutputFolderPath = outputFolderPtr;

            try
            {
                var config = _app.Config;
                UDT_DLL.CutDemosByChat(ref _app.ParseArg, filePaths, config.ChatRules, config.CutStartOffset, config.CutEndOffset, config.MaxThreadCount);
            }
            catch(Exception exception)
            {
                _app.LogError("Caught an exception while cutting demos: {0}", exception.Message);
            }

            Marshal.FreeHGlobal(outputFolderPtr);
        }

        private void OnAddChatRuleClicked()
        {
            var rule = new ChatRule();
            if(!ShowChatRuleEditDialog(rule))
            {
                return;
            }

            _app.Config.ChatRules.Add(rule);
            _chatRulesListView.Items.Add(new ChatRuleDisplayInfo(rule));
        }

        private void OnEditChatRuleClicked()
        {
            int idx = _chatRulesListView.SelectedIndex;
            if(idx < 0 || idx >= _chatRulesListView.Items.Count)
            {
                _app.LogWarning("No rule was selected. Editing nothing.");
                return;
            }

            var rule = _app.Config.ChatRules[idx];
            if(!ShowChatRuleEditDialog(rule))
            {
                return;
            }

            _chatRulesListView.Items[idx] = new ChatRuleDisplayInfo(rule);
        }

        private void OnRemoveChatRuleClicked()
        {
            int idx = _chatRulesListView.SelectedIndex;
            if(idx < 0 || idx >= _chatRulesListView.Items.Count)
            {
                _app.LogWarning("No rule was selected. Removed nothing.");
                return;
            }

            _chatRulesListView.Items.RemoveAt(idx);
            _app.Config.ChatRules.RemoveAt(idx);
        }

        private bool ShowChatRuleEditDialog(ChatRule chatRule)
        {
            var ruleNamesDic = new List<Tuple<string, string>>();
            ruleNamesDic.Add(new Tuple<string, string>("Contains", "Contains"));
            ruleNamesDic.Add(new Tuple<string, string>("Starts With", "StartsWith"));
            ruleNamesDic.Add(new Tuple<string, string>("Ends With", "EndsWith"));

            var operatorComboBox = new ComboBox();
            operatorComboBox.Width = 150;
            foreach(var ruleName in ruleNamesDic)
            {
                operatorComboBox.Items.Add(ruleName.Item1);
            }
            operatorComboBox.SelectedIndex = ruleNamesDic.FindIndex(r => r.Item2.ToLower() == chatRule.Operator.ToLower());

            var valueEditBox = new TextBox();
            valueEditBox.Width = 200;
            valueEditBox.Text = chatRule.Value;

            var caseCheckBox = new CheckBox();
            caseCheckBox.VerticalAlignment = VerticalAlignment.Center;
            caseCheckBox.Content = " The operator must respect case sensitivity";
            caseCheckBox.IsChecked = chatRule.CaseSensitive;

            var colorsCheckBox = new CheckBox();
            colorsCheckBox.VerticalAlignment = VerticalAlignment.Center;
            colorsCheckBox.Content = " Ignore the Quake 3 color codes (e.g. ^1 for red)";
            colorsCheckBox.IsChecked = chatRule.IgnoreColors;

            var panelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            panelList.Add(App.CreateTuple("Operator", operatorComboBox));
            panelList.Add(App.CreateTuple("Pattern", valueEditBox));
            panelList.Add(App.CreateTuple("Case Sensitive?", caseCheckBox));
            panelList.Add(App.CreateTuple("Ignore Colors?", colorsCheckBox));
            var rulePanel = WpfHelper.CreateDualColumnPanel(panelList, 100, 5);
            rulePanel.HorizontalAlignment = HorizontalAlignment.Center;
            rulePanel.VerticalAlignment = VerticalAlignment.Center;

            var ruleGroupBox = new GroupBox();
            ruleGroupBox.HorizontalAlignment = HorizontalAlignment.Center;
            ruleGroupBox.VerticalAlignment = VerticalAlignment.Center;
            ruleGroupBox.Margin = new Thickness(5);
            ruleGroupBox.Header = "Rule";
            ruleGroupBox.Content = rulePanel;

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
            rootPanel.Children.Add(ruleGroupBox);
            rootPanel.Children.Add(cancelButton);
            rootPanel.Children.Add(okButton);

            DockPanel.SetDock(ruleGroupBox, Dock.Top);
            DockPanel.SetDock(cancelButton, Dock.Right);
            DockPanel.SetDock(okButton, Dock.Right);

            var window = new Window();
            okButton.Click += (obj, args) => { window.DialogResult = true; window.Close(); };
            cancelButton.Click += (obj, args) => { window.DialogResult = false; window.Close(); };

            var appWindow = _app.MainWindow;
            window.WindowStyle = WindowStyle.ToolWindow;
            window.AllowsTransparency = false;
            window.Background = new SolidColorBrush(System.Windows.SystemColors.ControlColor);
            window.ShowInTaskbar = false;
            window.Title = "Chat Rule Editor";
            window.Content = rootPanel;
            window.Width = 420;
            window.Height = 280;
            window.Left = appWindow.Left + (appWindow.Width - window.Width) / 2;
            window.Top = appWindow.Top + (appWindow.Height - window.Height) / 2;
            window.ShowDialog();

            var result = window.DialogResult ?? false;
            if(!result)
            {
                return false;
            }

            chatRule.CaseSensitive = caseCheckBox.IsChecked ?? false;
            chatRule.IgnoreColors = colorsCheckBox.IsChecked ?? false;
            chatRule.Operator = ruleNamesDic.Find(r => r.Item1 == operatorComboBox.Text).Item2; ;
            chatRule.Value = valueEditBox.Text;

            return true;
        }
    }
}