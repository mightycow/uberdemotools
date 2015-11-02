using System;
using System.Threading;


namespace Uber.DemoTools.Updater
{
    public static class UpdaterHelper
    {
        private const string MutexName = "UDT_GUI_Updater";

        public static string NewFileExtension = ".newupdaterfile";
        public static string OldFileExtension = ".oldupdaterfile";
        public static string ExeFileName = "UDT_GUI_Updater.exe";

        public static Mutex TryOpenNamedMutex()
        {
            try
            {
                return Mutex.OpenExisting(MutexName);
            }
            catch(Exception)
            {
            }

            return null;
        }

        public static Mutex CreateNamedMutex()
        {
            try
            {
                return new Mutex(false, MutexName);
            }
            catch(Exception)
            {
            }

            return null;
        }
    }
}