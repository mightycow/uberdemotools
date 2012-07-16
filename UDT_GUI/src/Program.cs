using System;


namespace Uber.DemoTools
{
    public class Program
    {
        [STAThread]
        public static void Main(string[] args)
        {
            EntryPoint.Main = MainImpl;
            EntryPoint.Run(args);
        }

        private static void MainImpl(string[] args)
        {
            new App(args);
        }
    }
}
