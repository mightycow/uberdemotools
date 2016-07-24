using System;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Threading;


namespace Uber.Builder
{
    public static class ProcessLauncher
    {
        private class ThreadData
        {
            public byte[] Buffer = new byte[4096]; // 4 KB.
            public Process Process;
            public OnTextReadDelegate TextHandler;
            public bool ReadStdError;
        }

        public delegate void OnTextReadDelegate(string text);

        public static ProcessStartInfo CreateProcessStartInfo(string workingDir, string filePath, string arguments)
        {
            workingDir = Path.GetFullPath(workingDir);

            // @MSDN:
            // You must set UseShellExecute to false if you want to set RedirectStandardOutput to true.
            // Otherwise, reading from the StandardOutput stream throws an exception.
            var info = new ProcessStartInfo();
            info.Arguments = arguments;
            info.CreateNoWindow = true;
            info.ErrorDialog = false;
            info.FileName = filePath;
            info.RedirectStandardOutput = true;
            info.RedirectStandardError = true;
            info.UseShellExecute = false;
            info.WindowStyle = ProcessWindowStyle.Hidden;
            info.WorkingDirectory = workingDir;

            return info;
        }

        public static bool ReadProcessOutputUntilDone(Process process, OnTextReadDelegate stdOutHandler, OnTextReadDelegate stdErrHandler)
        {
            var stdOutputReaderThread = CreateReaderThread(process, stdOutHandler, false);
            if(stdOutputReaderThread == null)
            {
                return false;
            }

            var stdErrorReaderThread = CreateReaderThread(process, stdErrHandler, true);
            if(stdErrorReaderThread == null)
            {
                return false;
            }

            Thread.Sleep(1000);
            process.WaitForExit();
            stdOutputReaderThread.Join();
            stdErrorReaderThread.Join();

            return process.ExitCode == 0;
        }

        private static Thread CreateReaderThread(Process process, OnTextReadDelegate textHandler, bool readStdError)
        {
            var threadData = new ThreadData();
            threadData.Process = process;
            threadData.TextHandler = textHandler;
            threadData.ReadStdError = readStdError;

            var readerThread = new Thread(ProcessReadingThread);
            readerThread.Start(threadData);

            return readerThread;
        }

        private static void ProcessReadingThread(object arg)
        {
            var threadData = arg as ThreadData;
            if(threadData == null)
            {
                return;
            }

            try
            {
                ProcessReadingThreadImpl(threadData);
            }
            catch(Exception exception)
            {
                EntryPoint.RaiseException(exception);
            }
        }

        private static void ProcessReadingThreadImpl(ThreadData threadData)
        {
            var process = threadData.Process;
            var buffer = threadData.Buffer;
            var stream = threadData.ReadStdError ? process.StandardError : process.StandardOutput;
            stream.BaseStream.BeginRead(buffer, 0, buffer.Length, AsyncReadCallback, threadData);
        }

        private static void AsyncReadCallback(IAsyncResult result)
        {
            var data = result.AsyncState as ThreadData;
            if(data == null || data.Process == null)
            {
                return;
            }

            var process = data.Process;
            var stream = data.ReadStdError ? process.StandardError.BaseStream : process.StandardOutput.BaseStream;
            var byteCount = stream.EndRead(result);
            var text = Encoding.ASCII.GetString(data.Buffer, 0, byteCount);
            if(!string.IsNullOrWhiteSpace(text))
            {
                data.TextHandler(text);
            }

            Thread.Sleep(20);

            try
            {
                stream.BeginRead(data.Buffer, 0, data.Buffer.Length, AsyncReadCallback, data);
            }
            catch(Exception)
            {
            }
        }
    }
}