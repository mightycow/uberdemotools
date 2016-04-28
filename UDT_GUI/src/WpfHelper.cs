using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Threading;


namespace Uber
{
    public static class WpfExtensionMethods
    {
        private static Action EmptyDelegate = delegate() { };

        public static void Refresh(this UIElement uiElement)
        {
            uiElement.Dispatcher.Invoke(DispatcherPriority.Render, EmptyDelegate);
        }
    }

    public static class WpfHelper
    {
        public class WrapPanelNewLine : FrameworkElement
        {
            public WrapPanelNewLine()
            {
                Height = 0;
                var binding = new Binding
                {
                    RelativeSource = new RelativeSource(RelativeSourceMode.FindAncestor, typeof(WrapPanel), 1),
                    Path = new PropertyPath("ActualWidth")
                };

                BindingOperations.SetBinding(this, WidthProperty, binding);
            }
        }

        public static StackPanel CreateDualColumnPanel(IEnumerable<Tuple<FrameworkElement, FrameworkElement>> elementPairs, int width, int dy = 2)
        {
            var rootPanel = new StackPanel();
            rootPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
            rootPanel.VerticalAlignment = VerticalAlignment.Stretch;
            rootPanel.Margin = new Thickness(5);
            rootPanel.Orientation = Orientation.Vertical;

            foreach(var elementPair in elementPairs)
            {
                var a = elementPair.Item1;
                var b = elementPair.Item2;
                a.Width = width;

                var pairPanel = new StackPanel();
                pairPanel.HorizontalAlignment = HorizontalAlignment.Stretch;
                pairPanel.VerticalAlignment = VerticalAlignment.Top;
                pairPanel.Margin = new Thickness(5, dy, 5, dy);
                pairPanel.Orientation = Orientation.Horizontal;
                pairPanel.Children.Add(a);
                pairPanel.Children.Add(b);

                rootPanel.Children.Add(pairPanel);
            }
            
            return rootPanel;
        }
    }
}