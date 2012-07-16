using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;


namespace Uber
{
    public static class WpfHelper
    {
        public static FrameworkElement CreateDualColumnPanel(IEnumerable<Tuple<FrameworkElement, FrameworkElement>> elementPairs, int width, int dy = 2)
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