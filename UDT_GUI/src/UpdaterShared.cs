using System;
using System.Threading;


namespace Uber.DemoTools.Updater
{
    public static class UpdaterHelper
    {
        private const string MutexName = "UDT_GUI_Updater";

        public const string NewFileExtension = ".newupdaterfile";
        public const string OldFileExtension = ".oldupdaterfile";
        public const string ExeFileName = "UDT_GUI_Updater.exe";
        public const string NoMessageBoxIfCurrentArg = "silentifcurrent";

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

        public static Mutex TryCreateNamedMutex()
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