using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;


namespace Uber
{
    public class AlternatingListBoxBackground
    {
        private Style _listBoxStyle;

        public Style Style
        {
            get { return _listBoxStyle; }
        }

        public AlternatingListBoxBackground(Color color1, Color color2)
        {
            var setter = new Setter();
            setter.Property = ListBoxItem.BackgroundProperty;
            setter.Value = new SolidColorBrush(color1);

            var trigger = new Trigger();
            trigger.Property = ItemsControl.AlternationIndexProperty;
            trigger.Value = 0;
            trigger.Setters.Add(setter);

            var setter2 = new Setter();
            setter2.Property = ListBoxItem.BackgroundProperty;
            setter2.Value = new SolidColorBrush(color2);

            var trigger2 = new Trigger();
            trigger2.Property = ItemsControl.AlternationIndexProperty;
            trigger2.Value = 1;
            trigger2.Setters.Add(setter2);

            var listBoxStyle = new Style(typeof(ListBoxItem));
            listBoxStyle.Triggers.Add(trigger);
            listBoxStyle.Triggers.Add(trigger2);

            _listBoxStyle = listBoxStyle;
        }

        public void ApplyTo(ListBox listBox)
        {
            listBox.ItemContainerStyle = _listBoxStyle;
            listBox.AlternationCount = 2;
        }
    }

    public class ListBoxHotColor
    {
        private Style _listBoxStyle;
        
        public Style Style
        {
            get { return _listBoxStyle; }
        }

        public ListBoxHotColor(Color hotColor)
        {
            var setter = new Setter();
            setter.Property = ListBoxItem.BackgroundProperty;
            setter.Value = new SolidColorBrush(hotColor);

            var trigger = new MultiTrigger();
            trigger.Conditions.Add(new Condition(ListBoxItem.IsMouseOverProperty, true));
            trigger.Conditions.Add(new Condition(ListBoxItem.IsSelectedProperty, false));
            trigger.Setters.Add(setter);
            
            var listBoxStyle = new Style(typeof(ListBoxItem));
            listBoxStyle.Triggers.Add(trigger);

            _listBoxStyle = listBoxStyle;
        }

        public void ApplyTo(ListBox listBox)
        {
            listBox.ItemContainerStyle = _listBoxStyle;
        }

        public void AddTriggersTo(ListBox listBox)
        {
            foreach(var trigger in _listBoxStyle.Triggers)
            {
                listBox.Triggers.Add(trigger);
            }
        }
    }
}
