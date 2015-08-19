using System;
using System.Collections.Generic;
using System.Linq;


namespace Uber
{
    public static class ListHelper
    {
        public static void StableSort<T>(this List<T> list) where T : IComparable<T>
        {
            var sortedList = list.OrderBy(x => x, new ComparableComparer<T>()).ToList();
            list.Clear();
            list.AddRange(sortedList);
        }

        public static void StableSort<T>(this List<T> list, IComparer<T> comparer) where T : IComparable<T>
        {
            var sortedList = list.OrderBy(x => x, comparer).ToList();
            list.Clear();
            list.AddRange(sortedList);
        }

        public static void StableSort<T>(this List<T> list, Comparison<T> comparison)
        {
            var sortedList = list.OrderBy(x => x, new ComparisonComparer<T>(comparison)).ToList();
            list.Clear();
            list.AddRange(sortedList);
        }

        private class ComparableComparer<T> : IComparer<T> where T : IComparable<T>
        {
            public int Compare(T x, T y)
            {
                return x.CompareTo(y);
            }
        }

        private class ComparisonComparer<T> : IComparer<T>
        {
            private Comparison<T> _comparison;

            public ComparisonComparer(Comparison<T> comparison)
            {
                _comparison = comparison;
            }

            public int Compare(T x, T y)
            {
                return _comparison(x, y);
            }
        }
    }
}