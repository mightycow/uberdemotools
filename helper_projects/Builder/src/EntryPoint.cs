using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Windows;


namespace Uber
{
    public static class EntryPoint
    {
        private static Thread _mainThread;

        public delegate void MainFunction(string[] args);
        public delegate void ExceptionHandler(Exception exception);

        public static MainFunction Main;
        public static ExceptionHandler OnException;

        public static void Run(string[] args)
        {
            Directory.SetCurrentDirectory(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location));

            _mainThread = Thread.CurrentThread;
            AppDomain.CurrentDomain.UnhandledException += (obj, arg) => HandleUnhandledException(arg.ExceptionObject as Exception);

            var run = Debugger.IsAttached ? new MainFunction(RunDebugger) : new MainFunction(RunNormal);
            run(args);
        }

        public static void RaiseException(Exception exception)
        {
            _mainThread.Abort(exception);
        }

        private static void RunNormal(string[] args)
        {
            try
            {
                Main(args);
            }
            catch(Exception exception)
            {
                var abortException = exception as ThreadAbortException;
                var realException = abortException != null ? (abortException.ExceptionState as Exception) : exception;

                HandleException(realException ?? exception);
            }
        }

        private static void RunDebugger(string[] args)
        {
            Main(args);
        }

        private static void HandleException(Exception exception)
        {
            var handleException = OnException ?? DefaultExceptionHandler;
            handleException(exception);
        }

        private static void HandleUnhandledException(Exception exception)
        {
            _mainThread.Abort(exception);
        }

        private static void DefaultExceptionHandler(Exception exception)
        {
            try
            {
                DefaultExceptionHandlerImpl(exception);
            }
            catch(Exception)
            {
            }

            Process.GetCurrentProcess().Kill();
        }

        private static void DefaultExceptionHandlerImpl(Exception exception)
        {
            var stringBuilder = new StringBuilder();
            stringBuilder.Append("Exception caught: ");
            stringBuilder.Append(exception.Message);
            stringBuilder.AppendLine();
            stringBuilder.AppendLine(exception.ToString());
            if(exception.InnerException != null)
            {
                stringBuilder.AppendLine();
                stringBuilder.AppendLine();
                stringBuilder.Append("Inner exception: ");
                stringBuilder.Append(exception.InnerException.Message);
                stringBuilder.AppendLine();
                stringBuilder.AppendLine(exception.InnerException.ToString());
            }

            var time = DateTime.Now;
            var Y = time.Year.ToString("00");
            var M = time.Month.ToString("00");
            var D = time.Day.ToString("00");
            var h = time.Hour.ToString("00");
            var m = time.Minute.ToString("00");
            var s = time.Second.ToString("00");
            string filePath = string.Format("exception_{0}.{1}.{2}_{3}.{4}.{5}.txt", Y, M, D, h, m, s);

            File.WriteAllText(filePath, stringBuilder.ToString(), Encoding.UTF8);

            Process.Start(filePath);
        }
    }
}