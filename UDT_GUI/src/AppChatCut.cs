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
        public bool MultiDemoMode { get { return true; } }

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

        public void SaveToConfigObject(UdtPrivateConfig config)
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
                SearchTeam = rule.SearchTeamMessages;
            }

            public string Operator { get; set; }
            public string Pattern { get; set; }
            public bool CaseSensitive { get; set; }
            public bool IgnoreColors { get; set; }
            public bool SearchTeam { get; set; }
        }

        private DemoInfoListView _chatRulesListView = null;

        private FrameworkElement CreateCutByChatTab()
        {
            var chatRulesGridView = new GridView();
            chatRulesGridView.AllowsColumnReorder = false;
            chatRulesGridView.Columns.Add(new GridViewColumn { Header = "Operator", Width = 90, DisplayMemberBinding = new Binding("Operator") });
            chatRulesGridView.Columns.Add(new GridViewColumn { Header = "Pattern", Width = 155, DisplayMemberBinding = new Binding("Pattern") });
            chatRulesGridView.Columns.Add(new GridViewColumn { Header = "Case Sens.", Width = 80, DisplayMemberBinding = new Binding("CaseSensitive") });
            chatRulesGridView.Columns.Add(new GridViewColumn { Header = "No Colors", Width = 80, DisplayMemberBinding = new Binding("IgnoreColors") });
            chatRulesGridView.Columns.Add(new GridViewColumn { Header = "Team", Width = 70, DisplayMemberBinding = new Binding("SearchTeam") });

            var chatRulesListView = new DemoInfoListView();
            _chatRulesListView = chatRulesListView;
            chatRulesListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            chatRulesListView.VerticalAlignment = VerticalAlignment.Stretch;
            chatRulesListView.Margin = new Thickness(5);
            chatRulesListView.View = chatRulesGridView;
            chatRulesListView.SelectionMode = SelectionMode.Single;
            chatRulesListView.Width = 485;
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

            var actionsGroupBox = CutByPatternComponent.CreateActionsGroupBox(UDT_DLL.udtPatternType.Chat);

            var helpTextBlock = new TextBlock();
            helpTextBlock.Margin = new Thickness(5);
            helpTextBlock.TextWrapping = TextWrapping.WrapWithOverflow;
            helpTextBlock.Text =
                "A cut section is created if the chat message matches any of the rules." +
                "\nIn other words, rules are logically ORed, not ANDed:" +
                "\ncreate_cut = match(message, rule_1) OR match(message, rule_2) OR ... OR match(message, rule_N)";

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

            var teamMessagesCheckBox = new CheckBox();
            teamMessagesCheckBox.VerticalAlignment = VerticalAlignment.Center;
            teamMessagesCheckBox.Content = " Search team chat too?";
            teamMessagesCheckBox.IsChecked = chatRule.SearchTeamMessages;

            var panelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            panelList.Add(App.CreateTuple("Operator", operatorComboBox));
            panelList.Add(App.CreateTuple("Pattern", valueEditBox));
            panelList.Add(App.CreateTuple("Case Sensitive?", caseCheckBox));
            panelList.Add(App.CreateTuple("Ignore Colors?", colorsCheckBox));
            panelList.Add(App.CreateTuple("Team Chat?", teamMessagesCheckBox));
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
            window.Owner = _app.MainWindow;
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
            chatRule.SearchTeamMessages = teamMessagesCheckBox.IsChecked ?? false;
            chatRule.Operator = ruleNamesDic.Find(r => r.Item1 == operatorComboBox.Text).Item2; ;
            chatRule.Value = valueEditBox.Text;

            return true;
        }
    }
}