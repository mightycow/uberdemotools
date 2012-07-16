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
    public partial class App
    {
        private class CutByChatInfo
        {
            public List<DemoDisplayInfo> Demos = null;
            public int StartOffset = 0;
            public int EndOffset = 0;
        }

        private class ChatRuleDisplayInfo
        {
            public ChatRuleDisplayInfo(ChatRule rule)
            {
                Operator = rule.Operator;
                Value = rule.Value;
                CaseSensitive = rule.CaseSensitive;
                IgnoreColors = rule.IgnoreColors;
            }

            public string Operator { get; set; }
            public string Value { get; set; }
            public bool CaseSensitive { get; set; }
            public bool IgnoreColors { get; set; }
        }

        private ListView _chatRulesListView = null;
        private TextBox _startTimeOffsetEditBox = null;
        private TextBox _endTimeOffsetEditBox = null;

        private FrameworkElement CreateCutByChatTab()
        {
            var chatRulesGridView = new GridView();
            chatRulesGridView.AllowsColumnReorder = false;
            chatRulesGridView.Columns.Add(new GridViewColumn { Header = "Operator", Width = 100, DisplayMemberBinding = new Binding("Operator") });
            chatRulesGridView.Columns.Add(new GridViewColumn { Header = "Value", Width = 175, DisplayMemberBinding = new Binding("Value") });
            chatRulesGridView.Columns.Add(new GridViewColumn { Header = "Case Sensitive", Width = 100, DisplayMemberBinding = new Binding("CaseSensitive") });
            chatRulesGridView.Columns.Add(new GridViewColumn { Header = "Ignore Colors", Width = 100, DisplayMemberBinding = new Binding("IgnoreColors") });

            var chatRulesListView = new ListView();
            _chatRulesListView = chatRulesListView;
            chatRulesListView.HorizontalAlignment = HorizontalAlignment.Stretch;
            chatRulesListView.VerticalAlignment = VerticalAlignment.Stretch;
            chatRulesListView.Margin = new Thickness(5);
            chatRulesListView.View = chatRulesGridView;
            chatRulesListView.SelectionMode = SelectionMode.Single;
            chatRulesListView.Width = 485;
            foreach(var rule in _config.ChatRules)
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

            var startTimeOffsetEditBox = new TextBox();
            _startTimeOffsetEditBox = startTimeOffsetEditBox;
            startTimeOffsetEditBox.Width = 40;
            startTimeOffsetEditBox.Text = _config.ChatCutStartOffset.ToString();
            startTimeOffsetEditBox.ToolTip = "How many seconds before the chat event do we start the cut?";

            var endTimeOffsetEditBox = new TextBox();
            _endTimeOffsetEditBox = endTimeOffsetEditBox;
            endTimeOffsetEditBox.Width = 40;
            endTimeOffsetEditBox.Text = _config.ChatCutEndOffset.ToString();
            endTimeOffsetEditBox.ToolTip = "How many seconds after the chat event do we start the cut?";

            var panelList = new List<Tuple<FrameworkElement, FrameworkElement>>();
            panelList.Add(CreateTuple("Start Time Offset", startTimeOffsetEditBox));
            panelList.Add(CreateTuple("End Time Offset", endTimeOffsetEditBox));
            var optionsPanel = WpfHelper.CreateDualColumnPanel(panelList, 100, 5);
            optionsPanel.HorizontalAlignment = HorizontalAlignment.Center;
            optionsPanel.VerticalAlignment = VerticalAlignment.Center;
            
            var timeOffsetsGroupBox = new GroupBox();
            timeOffsetsGroupBox.Header = "Time Offsets";
            timeOffsetsGroupBox.HorizontalAlignment = HorizontalAlignment.Left;
            timeOffsetsGroupBox.VerticalAlignment = VerticalAlignment.Top;
            timeOffsetsGroupBox.Margin = new Thickness(5);
            timeOffsetsGroupBox.Content = optionsPanel;

            var rootPanel = new WrapPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.Margin = new Thickness(5);
            rootPanel.Orientation = Orientation.Horizontal;
            rootPanel.Children.Add(chatRulesGroupBox);
            rootPanel.Children.Add(timeOffsetsGroupBox);
            rootPanel.Children.Add(actionsGroupBox);

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
            if(_demoListView.SelectedItems.Count == 0)
            {
                return;
            }

            DisableUiNonThreadSafe();

            if(_jobThread != null)
            {
                _jobThread.Join();
            }

            var demos = new List<DemoDisplayInfo>();
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

                demos.Add(displayInfo);
            }

            int startOffset = -1;
            if(!GetOffsetSeconds(_startTimeOffsetEditBox.Text, out startOffset))
            {
                return;
            }

            int endOffset = -1;
            if(!GetOffsetSeconds(_endTimeOffsetEditBox.Text, out endOffset))
            {
                return;
            }

            _config.ChatCutStartOffset = startOffset;
            _config.ChatCutEndOffset = endOffset;

            var info = new CutByChatInfo();
            info.Demos = demos;
            info.StartOffset = startOffset;
            info.EndOffset = endOffset;

            _jobThread = new Thread(DemoCutByChatThread);
            _jobThread.Start(info);
        }

        private void DemoCutByChatThread(object arg)
        {
            try
            {
                DemoCutByChatThreadImpl(arg);
            }
            catch(Exception exception)
            {
                EntryPoint.RaiseException(exception);
            }
        }

        private class CutByChatDemoInfo
        {
            public DemoDisplayInfo Info = null;
            public int DemoParseTime = 0;
            public int CutCount = 0;
            public List<int> GsParseTimes = new List<int>();
            public List<List<DemoCut>> GsCutsList = new List<List<DemoCut>>();
        }

        private void DemoCutByChatThreadImpl(object arg)
        {
            var threadInfo = (CutByChatInfo)arg;
            var demos = threadInfo.Demos;

            int totalParseTime = 0;
            var demoList = new List<CutByChatDemoInfo>();

            foreach(var info in demos)
            {
                var cutSections = new List<Tuple<int, int>>();
                var mergedCutSections = new List<Tuple<int, int>>();
                var inFilePath = info.Demo.FilePath;

                var chatEntries = info.Demo.DemoChatEvents;
                foreach(var chatEntry in chatEntries)
                {
                    var nameAndMessage = chatEntry.Message.Split(new string[] { ": " }, 2, StringSplitOptions.None);
                    if(nameAndMessage.Length != 2 || !IsChatMessageMatching(nameAndMessage[1]))
                    {
                        continue;
                    }

                    int msgTime = chatEntry.Time;
                    int startTime = msgTime - threadInfo.StartOffset * 1000;
                    int endTime = msgTime + threadInfo.EndOffset * 1000;
                    cutSections.Add(Tuple.Create(startTime, endTime));
                }

                MergeRanges(cutSections, mergedCutSections);
                if(mergedCutSections.Count == 0)
                {
                    LogInfo("Processing demo {0}... no chat match found", Path.GetFileNameWithoutExtension(info.Demo.FilePath));
                    continue;
                }

                var cuts = new List<DemoCut>();
                foreach(var cutSection in mergedCutSections)
                {
                    var startTimeDisplay = FormatMinutesSeconds(cutSection.Item1 / 1000);
                    var endTimeDisplay = FormatMinutesSeconds(cutSection.Item2 / 1000);
                    var startTime = startTimeDisplay.Replace(":", "");
                    var endTime = endTimeDisplay.Replace(":", "");
                    var outFilePath = GenerateOutputFilePath(inFilePath, startTime, endTime);

                    var cut = new DemoCut();
                    cut.FilePath = outFilePath;
                    cut.StartTimeMs = cutSection.Item1;
                    cut.EndTimeMs = cutSection.Item2;
                    cuts.Add(cut);
                }

                var gameStates = info.Demo.DemoGameStates;
                int demoParseTime = 0;
                List<int> gsParseTimes = new List<int>();
                List<List<DemoCut>> gsCutsList = new List<List<DemoCut>>();
                Demo.CreateCutList(cuts, gameStates, ref gsCutsList, ref gsParseTimes, ref demoParseTime);

                totalParseTime += demoParseTime;

                int cutCount = 0;
                foreach(var gsCuts in gsCutsList)
                {
                    cutCount += gsCuts.Count;
                }

                if(cutCount == 0)
                {
                    continue;
                }

                var info2 = new CutByChatDemoInfo();
                info2.Info = info;
                info2.CutCount = cutCount;
                info2.DemoParseTime = demoParseTime;
                info2.GsParseTimes = gsParseTimes;
                info2.GsCutsList = gsCutsList;
                demoList.Add(info2);
            }

            double progressScale = 100.0 / (double)totalParseTime;
            double progress = 0.0;

            foreach(var demoInfo in demoList)
            {
                var info = demoInfo.Info;
                var cutCount = demoInfo.CutCount;
                var inFilePath = info.Demo.FilePath;
                var protocol = GetProtocolFromFilePath(inFilePath);

                LogInfo("Processing demo {0}... with {1} cut{2}", Path.GetFileNameWithoutExtension(inFilePath), cutCount, cutCount == 1 ? "" : "s");

                if(demoInfo.GsCutsList.Count != demoInfo.GsParseTimes.Count)
                {
                    LogError("Cut/parse time count mismatch.");
                    continue;
                }

                if(demoInfo.GsCutsList.Count == 0)
                {
                    LogError("Cut/parse time count is zero.");
                    continue;
                }

                for(int i = 0; i < demoInfo.GsCutsList.Count; ++i)
                {
                    var cuts = demoInfo.GsCutsList[i];
                    var duration = (double)demoInfo.GsParseTimes[i];
                    var gameState = demoInfo.Info.Demo.DemoGameStates[i];
                    if(cuts.Count == 0)
                    {
                        // We can safely skip this.
                        continue;
                    }

                    var timer = new Stopwatch();
                    timer.Start();

                    Demo.ProgressCallback progressCb = (progressPc) =>
                    {
                        if(timer.ElapsedMilliseconds < 50)
                        {
                            return _cancelJobValue;
                        }

                        timer.Stop();
                        timer.Reset();
                        timer.Start();

                        var realProgress = progress + progressScale * duration * (double)progressPc;
                        SetProgressThreadSafe(realProgress);

                        return _cancelJobValue;
                    };

                    try
                    {
                        Demo.Cut(protocol, inFilePath, progressCb, DemoLoggingCallback, cuts, gameState);
                    }
                    catch(SEHException exception)
                    {
                        LogError("Caught an exception while cutting demo '{0}': {1}", Path.GetFileNameWithoutExtension(inFilePath), exception.Message);
                    }

                    if(_cancelJobValue != 0)
                    {
                        break;
                    }

                    progress += progressScale * duration;
                    SetProgressThreadSafe(progress);

                    timer.Stop();
                }
            }

            EnableUiThreadSafe();
        }

        private void MergeRanges(List<Tuple<int, int>> ranges, List<Tuple<int, int>> result)
        {
            if(ranges.Count == 0)
            {
                return;
            }

            int idx = 0;
            var current = ranges[idx++];


            for(; idx < ranges.Count; ++idx)
            {
                if(current.Item2 >= ranges[idx].Item1)
                {
                    int item2 = Math.Max(current.Item2, ranges[idx].Item2);
                    current = new Tuple<int, int>(current.Item1, item2);
                }
                else
                {
                    result.Add(current);
                    current = ranges[idx];
                }
            }

            result.Add(current);
        }

        private bool ChatMessageContains(ChatRule chatRule, string message)
        {
            if(chatRule.CaseSensitive)
            {
                return message.Contains(chatRule.Value);
            }

            return message.ToLower().Contains(chatRule.Value.ToLower());
        }

        private bool ChatMessageStartsWith(ChatRule chatRule, string message)
        {
            if(chatRule.CaseSensitive)
            {
                return message.StartsWith(chatRule.Value);
            }

            return message.ToLower().StartsWith(chatRule.Value.ToLower());
        }

        private bool ChatMessageEndsWith(ChatRule chatRule, string message)
        {
            if(chatRule.CaseSensitive)
            {
                return message.EndsWith(chatRule.Value);
            }

            return message.ToLower().EndsWith(chatRule.Value.ToLower());
        }

        private bool ChatMessageMatchesRegEx(ChatRule chatRule, string message)
        {
            try
            {
                var regEx = new Regex(chatRule.Value, chatRule.CaseSensitive ? RegexOptions.None : RegexOptions.IgnoreCase);
                return regEx.IsMatch(message);
            }
            catch(Exception exception)
            {
                LogError("Caught an exception: " + exception.Message);
            }

            return false;
        }

        private bool IsChatMessageMatching(string message)
        {
            var chatRules = _config.ChatRules;
            foreach(var chatRule in chatRules)
            {
                var msg = message;
                if(chatRule.IgnoreColors)
                {
                    msg = DemoInfo.ColorCodeRegEx.Replace(message, "");
                }

                if(chatRule.Operator.ToLower() == "contains")
                {
                    if(ChatMessageContains(chatRule, msg))
                    {
                        return true;
                    }
                }
                else if(chatRule.Operator.ToLower() == "startswith")
                {
                    if(ChatMessageStartsWith(chatRule, msg))
                    {
                        return true;
                    }
                }
                else if(chatRule.Operator.ToLower() == "endswith")
                {
                    if(ChatMessageEndsWith(chatRule, msg))
                    {
                        return true;
                    }
                }
                else if(chatRule.Operator.ToLower() == "matchesregex")
                {
                    if(ChatMessageMatchesRegEx(chatRule, msg))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        private void OnAddChatRuleClicked()
        {
            var rule = new ChatRule();
            if(!ShowChatRuleEditDialog(rule))
            {
                return;
            }

            _config.ChatRules.Add(rule);
            _chatRulesListView.Items.Add(new ChatRuleDisplayInfo(rule));
        }

        private void OnEditChatRuleClicked()
        {
            int idx = _chatRulesListView.SelectedIndex;
            if(idx < 0 || idx >= _chatRulesListView.Items.Count)
            {
                LogWarning("No rule was selected. Editing nothing.");
                return;
            }

            var rule = _config.ChatRules[idx];
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
                LogWarning("No rule was selected. Removed nothing.");
                return;
            }

            _chatRulesListView.Items.RemoveAt(idx);
            _config.ChatRules.RemoveAt(idx);
        }

        private bool ShowChatRuleEditDialog(ChatRule chatRule)
        {
            var ruleNamesDic = new List<Tuple<string, string>>();
            ruleNamesDic.Add(new Tuple<string, string>("Contains", "Contains"));
            ruleNamesDic.Add(new Tuple<string, string>("Starts With", "StartsWith"));
            ruleNamesDic.Add(new Tuple<string, string>("Ends With", "EndsWith"));
            ruleNamesDic.Add(new Tuple<string, string>("Matches RegEx", "MatchesRegEx"));

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
            panelList.Add(CreateTuple("Operator", operatorComboBox));
            panelList.Add(CreateTuple("Value", valueEditBox));
            panelList.Add(CreateTuple("Case Sensitive?", caseCheckBox));
            panelList.Add(CreateTuple("Ignore Colors?", colorsCheckBox));
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

            window.WindowStyle = WindowStyle.ToolWindow;
            window.AllowsTransparency = false;
            window.Background = new SolidColorBrush(System.Windows.SystemColors.ControlColor);
            window.ShowInTaskbar = false;
            window.Title = "Chat Rule Editor";
            window.Content = rootPanel;
            window.Width = 420;
            window.Height = 280;
            window.Left = _window.Left + (_window.Width - window.Width) / 2;
            window.Top = _window.Top + (_window.Height - window.Height) / 2;
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