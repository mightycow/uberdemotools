using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

using udtParserContextRef = System.IntPtr;
using udtParserContextGroupRef = System.IntPtr;
using udtPatternSearchContextRef = System.IntPtr;


namespace Uber.DemoTools
{
    // @TODO: Move this...
    public static class StringHelper
    {
        public static string Capitalize(this string s)
        {
            if(s.Length == 0)
            {
                return s;
            }

            if(s.Length == 1)
            {
                return s.ToUpper();
            }

            return s.Substring(0, 1).ToUpper() + s.Substring(1);
        }
    }

    // @TODO: Move this...
    public class MarshalHelper
    {
        public static T[] PtrToStructureArray<T>(IntPtr native, int count)
        {
            var typeOfT = typeof(T);
            var sizeOfT = Marshal.SizeOf(typeOfT);
            var array = new T[count];
            if(native != IntPtr.Zero)
            {
                for(var i = 0; i < count; ++i)
                {
                    var address = new IntPtr(native.ToInt64() + i * sizeOfT);
                    array[i] = (T)Marshal.PtrToStructure(address, typeOfT);
                }
            }

            return array;
        }

        public static byte[] PtrToByteArray(IntPtr native, int byteCount)
        {
            var array = new byte[byteCount];
            if(native != IntPtr.Zero)
            {
                Marshal.Copy(native, array, 0, byteCount);
            }

            return array;
        }

        public static int[] PtrToIntArray(IntPtr native, int intCount)
        {
            var array = new int[intCount];
            if(native != IntPtr.Zero)
            {
                Marshal.Copy(native, array, 0, intCount);
            }

            return array;
        }
    }

    // @TODO: Move this...
    public class ArgumentResources
    {
        public readonly List<PinnedObject> PinnedObjects = new List<PinnedObject>();
        public readonly List<IntPtr> GlobalAllocationHandles = new List<IntPtr>();

        public void Free()
        {
            foreach(var pinnedObject in PinnedObjects)
            {
                pinnedObject.Free();
            }
            PinnedObjects.Clear();

            foreach(var address in GlobalAllocationHandles)
            {
                Marshal.FreeHGlobal(address);
            }
            GlobalAllocationHandles.Clear();
        }
    }

    // @TODO: Move this...
    public class BatchJobRunner
    {
        private class BatchInfo
        {
            public long ByteCount;
            public int FirstFileIndex;
            public int FileCount;
        }

        public BatchJobRunner(UDT_DLL.udtParseArg parseArg, List<string> filePaths, List<int> fileIndices, int maxBatchSize)
        {
            var fileCount = filePaths.Count;

            _filePaths = filePaths;
            _fileIndices = fileIndices;
            _maxBatchSize = maxBatchSize;
            _oldProgressCb = parseArg.ProgressCb;
            _newParseArg = parseArg;

            long totalByteCount = 0;
            if(fileCount <= maxBatchSize)
            {
                for(var i = 0; i < fileCount; ++i)
                {
                    var byteCount = new FileInfo(filePaths[i]).Length;
                    totalByteCount += byteCount;
                }
                _totalByteCount = totalByteCount;

                var info = new BatchInfo();
                info.ByteCount = totalByteCount;
                info.FileCount = fileCount;
                info.FirstFileIndex = 0;
                _batches.Add(info);

                return;
            }

            _newParseArg.ProgressCb = delegate(float progress, IntPtr userData)
            {
                var realProgress = (float)(_progressBase + _progressRange * (double)progress);
                _oldProgressCb(realProgress, userData);
            };

            var batchCount = (fileCount + _maxBatchSize - 1) / _maxBatchSize;
            var filesPerBatch = fileCount / batchCount;
            var fileOffset = 0;
            for(var i = 0; i < batchCount; ++i)
            {
                var batchFileCount = (i == batchCount - 1) ? (fileCount - fileOffset) : filesPerBatch;

                long batchByteCount = 0;
                for(var j = fileOffset; j < fileOffset + batchFileCount; ++j)
                {
                    var byteCount = new FileInfo(filePaths[j]).Length;
                    totalByteCount += byteCount;
                    batchByteCount += byteCount;
                }

                var info = new BatchInfo();
                info.ByteCount = batchByteCount;
                info.FileCount = batchFileCount;
                info.FirstFileIndex = fileOffset;
                _batches.Add(info);

                fileOffset += batchFileCount;
            }

            _totalByteCount = totalByteCount;
        }

        public void PrepareNextBatch()
        {
            ++_batchIndex;
            if(_filePaths.Count <= _maxBatchSize || _batchIndex >= BatchCount)
            {
                return;
            }

            _progressBase = (double)_processedByteCount / (double)_totalByteCount;
            _progressRange = (double)_batches[_batchIndex].ByteCount / (double)_totalByteCount;
            _processedByteCount += _batches[_batchIndex].ByteCount;
        }

        public List<string> GetFileList()
        {
            var batch = _batches[_batchIndex];

            return _filePaths.GetRange(batch.FirstFileIndex, batch.FileCount);
        }

        public List<int> GetIndexList()
        {
            var batch = _batches[_batchIndex];

            return _fileIndices.GetRange(batch.FirstFileIndex, batch.FileCount);
        }

        public bool IsCanceled(IntPtr cancelValueAddress)
        {
            return Marshal.ReadInt32(cancelValueAddress) != 0;
        }

        public int FileIndex
        {
            get { return _batches[_batchIndex].FirstFileIndex; }
        }

        public int FileCount
        {
            get { return _batches[_batchIndex].FileCount; }
        }

        public int BatchCount
        {
            get { return _batches.Count; }
        }

        public UDT_DLL.udtParseArg NewParseArg
        {
            get { return _newParseArg; }
        }

        private List<string> _filePaths;
        private List<int> _fileIndices;
        private readonly List<BatchInfo> _batches = new List<BatchInfo>();
        private long _processedByteCount = 0;
        private long _totalByteCount = 0;
        private double _progressBase = 0.0;
        private double _progressRange = 0.0;
        private int _maxBatchSize = 512;
        private int _batchIndex = -1;
        private UDT_DLL.udtProgressCallback _oldProgressCb;
        private UDT_DLL.udtParseArg _newParseArg;
    }

    // @TODO: Move this...
    public class DemoThreadAllocator
    {
        public DemoThreadAllocator(List<string> filePaths, int maxThreadCount)
        {
            FinalThreadCount = 1;

            var fileCount = filePaths.Count;
            if(maxThreadCount <= 1 || fileCount <= 1)
            {
                return;
            }

            var files = new List<FileDesc>();
            long totalByteCount = 0;
            for(var i = 0; i < fileCount; ++i)
            {
                var byteCount = new FileInfo(filePaths[i]).Length;
                var file = new FileDesc();
                file.Path = filePaths[i];
                file.ByteCount = byteCount;
                file.ThreadIdx = -1;
                file.InputIdx = i;
                files.Add(file);
                totalByteCount += byteCount;
            }

            if(totalByteCount < 2 * MinByteCountPerThread)
            {
                return;
            }

            // Prepare the final thread list.
            var processorCoreCount = UDT_DLL.GetProcessorCoreCount();
            maxThreadCount = Math.Min(maxThreadCount, MaxThreadCount);
            maxThreadCount = Math.Min(maxThreadCount, processorCoreCount);
            maxThreadCount = Math.Min(maxThreadCount, fileCount);
            var finalThreadCount = Math.Min(maxThreadCount, totalByteCount / MinByteCountPerThread);
            if(finalThreadCount <= 1)
            {
                return;
            }

            // Sort files by size, descending.
            files.Sort((a, b) => a.ByteCount.CompareTo(b.ByteCount));

            // Assign files to threads.
            var threads = new ThreadDesc[finalThreadCount];
            for(var i = 0; i < finalThreadCount; ++i)
            {
                threads[i] = new ThreadDesc();
            }
            for(var i = 0; i < fileCount; ++i)
            {
                var threadIdx = 0;
                long smallestThreadByteCount = long.MaxValue;
                for(var j = 0; j < finalThreadCount; ++j)
                {
                    if(threads[j].TotalByteCount < smallestThreadByteCount)
                    {
                        smallestThreadByteCount = threads[j].TotalByteCount;
                        threadIdx = j;
                    }
                }

                threads[threadIdx].TotalByteCount += files[i].ByteCount;
                files[i].ThreadIdx = threadIdx;
            }

            // Sort files by thread index.
            files.Sort((a, b) => a.ThreadIdx.CompareTo(b.ThreadIdx));

            // Build and finalize the arrays.
            var threadIdx2 = 0;
            var firstFileIdx = 0;
            var filePathsArray = new string[fileCount];
            var fileSizes = new long[fileCount];
            var inputIndices = new int[fileCount];
            for(var i = 0; i < fileCount; ++i)
            {
                if(files[i].ThreadIdx != threadIdx2)
                {
                    threads[threadIdx2].FirstFileIndex = firstFileIdx;
                    threads[threadIdx2].FileCount = i - firstFileIdx;
                    firstFileIdx = i;
                    ++threadIdx2;
                }

                filePathsArray[i] = files[i].Path;
                fileSizes[i] = files[i].ByteCount;
                inputIndices[i] = files[i].InputIdx;
            }
            threads[threadIdx2].FirstFileIndex = firstFileIdx;
            threads[threadIdx2].FileCount = fileCount - firstFileIdx;

            // Store the results.
            FinalThreadCount = (int)finalThreadCount;
            FilePaths = filePathsArray;
            FileSizes = fileSizes;
            InputIndices = inputIndices;
            Threads = threads;
        }

        public int FinalThreadCount { get; private set; }
        public string[] FilePaths { get; private set; }
        public long[] FileSizes { get; private set; }
        public int[] InputIndices { get; private set; }
        public ThreadDesc[] Threads { get; private set; }

        public class FileDesc
        {
            public string Path;
            public long ByteCount;
            public int ThreadIdx;
            public int InputIdx;
        }

        public class ThreadDesc
        {
            public long TotalByteCount = 0;
            public int FirstFileIndex = 0;
            public int FileCount = 0;
        }

        private const int MinByteCountPerThread = 6 << 20; // 6 MB
        private const int MaxThreadCount = 16;
    }

    public class APIPerfStats
    {
        public APIPerfStats(ArgumentResources resources)
        {
            _stats = Marshal.AllocHGlobal(StatsByteCount);
            UDT_DLL.MemSet(_stats, 0, StatsByteCount);
            resources.GlobalAllocationHandles.Add(_stats);
        }

        public void Copy(IntPtr source)
        {
            UDT_DLL.MemCpy(_stats, source, (UIntPtr)StatsByteCount);
        }

        public void Merge(IntPtr source, int index)
        {
            if(index == 0)
            {
                Copy(source);
            }
            else
            {
                UDT_DLL.MergeBatchPerfStats(_stats, source);
            }
        }

        public void Add(IntPtr source, int index)
        {
            if(index == 0)
            {
                Copy(source);
            }
            else
            {
                UDT_DLL.AddThreadPerfStats(_stats, source);
            }
        }

        public IntPtr Address { get { return _stats; } }

        private IntPtr _stats;

        private static int StatsByteCount = UDT_DLL.StatsConstants.PerfFieldCount * 8;
    }

    public class APIArgument
    {
        public APIArgument()
        {
            _resources = new ArgumentResources();
            _cancelOperation = Marshal.AllocHGlobal(4);
            Marshal.WriteInt32(_cancelOperation, 0);
            _perfStats = new APIPerfStats(_resources);
            var outputFolder = App.Instance.GetOutputFolder();
            _resources.GlobalAllocationHandles.Add(_cancelOperation);

            Argument.CancelOperation = _cancelOperation;
            Argument.PerformanceStats = _perfStats.Address;
            Argument.MessageCb = (logLevel, message) => App.Instance.DemoLoggingCallback(logLevel, message);
            Argument.ProgressCb = (logLevel, message) => App.Instance.DemoProgressCallback(logLevel, message);
            Argument.ProgressContext = IntPtr.Zero;
            Argument.FileOffset = 0;
            Argument.GameStateIndex = 0;
            Argument.OutputFolderPath = IntPtr.Zero;
            Argument.PlugInCount = 0;
            Argument.PlugIns = IntPtr.Zero;
            Argument.Flags = 0;
            Argument.MinProgressTimeMs = 100;
            if(outputFolder != null)
            {
                Argument.OutputFolderPath = UDT_DLL.StringToHGlobalUTF8(outputFolder);
                _resources.GlobalAllocationHandles.Add(Argument.OutputFolderPath);
            }
        }

        public void CancelOperation()
        {
            Marshal.WriteInt32(_cancelOperation, 1);
        }

        public void FreeResources()
        {
            _resources.Free();
        }

        public UDT_DLL.udtParseArg Argument;

        private ArgumentResources _resources;
        private IntPtr _cancelOperation;
        private APIPerfStats _perfStats;
    }

    public class MultithreadedJob
    {
        public delegate void JobExecuter(ArgumentResources resources, ref UDT_DLL.udtParseArg parseArg, List<string> files, List<int> fileIndices);

        public bool Process(ref UDT_DLL.udtParseArg parseArg, List<string> filePaths, int maxThreadCount, int maxBatchSize, JobExecuter executeJob)
        {
            _filePaths = filePaths;
            _maxBatchSize = maxBatchSize;
            _executeJob = executeJob;
            _parseArg = parseArg;
            _threadAllocator = new DemoThreadAllocator(filePaths, maxThreadCount);
            if(_threadAllocator.FinalThreadCount > 1)
            {
                _multiThreaded = true;
                return ProcessMultipleThreads(parseArg);
            }
            else
            {
                return ProcessSingleThread(ref parseArg);
            }
        }

        public void Cancel()
        {
            if(_multiThreaded)
            {
                if(_arguments != null)
                {
                    foreach(var argument in _arguments)
                    {
                        argument.CancelOperation();
                    }
                }
            }
            else
            {
                Marshal.WriteInt32(_parseArg.CancelOperation, 1);
            }
        }

        public void FreeResources()
        {
            if(_arguments == null)
            {
                return;
            }

            foreach(var argument in _arguments)
            {
                argument.FreeResources();
            }
            _arguments = null;
        }

        private bool ProcessSingleThread(ref UDT_DLL.udtParseArg parseArg)
        {
            var timer = new Stopwatch();
            timer.Start();

            var fileCount = _filePaths.Count;
            var fileIndices = new List<int>();
            fileIndices.Capacity = fileCount;
            for(var i = 0; i < fileCount; ++i)
            {
                fileIndices.Add(i);
            }

            var resources = new ArgumentResources();
            var perfStats = new APIPerfStats(resources);
            var tempPerfStats = new APIPerfStats(resources);
            var runner = new BatchJobRunner(parseArg, _filePaths, fileIndices, _maxBatchSize);
            var newParseArg = runner.NewParseArg;
            var batchCount = runner.BatchCount;
            newParseArg.PerformanceStats = tempPerfStats.Address;

            for(var i = 0; i < batchCount; ++i)
            {
                runner.PrepareNextBatch();
                _executeJob(resources, ref newParseArg, runner.GetFileList(), runner.GetIndexList());
                perfStats.Merge(tempPerfStats.Address, i);
                if(runner.IsCanceled(parseArg.CancelOperation))
                {
                    return false;
                }
            }

            parseArg.ProgressCb(1.0f, parseArg.ProgressContext);

            UDT_DLL.PrintPerfStats(perfStats.Address, timer, _filePaths.Count);
            resources.Free();

            return true;
        }

        private bool ProcessMultipleThreads(UDT_DLL.udtParseArg parseArg)
        {
            var timer = new Stopwatch();
            timer.Start();
            var resources = new ArgumentResources();
            var perfStats = new APIPerfStats(resources);

            var threads = _threadAllocator.Threads;
            var threadCount = threads.Length;
            _arguments = new List<APIArgument>();
            for(var i = 0; i < threadCount; ++i)
            {
                _arguments.Add(new APIArgument());
            }

            var threadDataList = new List<ThreadInfo>();
            for(var i = 0; i < threadCount; ++i)
            {
                var threadData = new ThreadInfo();
                threadData.Argument = _arguments[i];
                threadData.FilePaths = new List<string>(_threadAllocator.FilePaths).GetRange(threads[i].FirstFileIndex, threads[i].FileCount);
                threadData.FileIndices = new List<int>(_threadAllocator.InputIndices).GetRange(threads[i].FirstFileIndex, threads[i].FileCount);
                threadData.MaxBatchSize = _maxBatchSize / threadCount;
                threadData.PerfStats = new APIPerfStats(resources);
                threadDataList.Add(threadData);
            }

            _arguments[0].Argument.ProgressCb = (progress, userData) => 
            {
                threadDataList[0].Progress = progress;
                var realProgress = 1.0f;
                foreach(var data in threadDataList)
                {
                    realProgress = Math.Min(realProgress, data.Progress);
                }
                parseArg.ProgressCb(realProgress, parseArg.ProgressContext);
            };

            for(var i = 1; i < threadCount; ++i)
            {
                var iCopy = i; // Make sure we capture a local copy in the lambda.
                _arguments[i].Argument.ProgressCb = (progress, userData) => { threadDataList[iCopy].Progress = progress; };
            }

            var extraThreads = new List<Thread>();
            for(var i = 1; i < threads.Length; ++i)
            {
                var thread = new Thread(ThreadEntryPoint);
                thread.Start(threadDataList[i]);
                extraThreads.Add(thread);
            }

            ProcessThread(threadDataList[0]);

            foreach(var thread in extraThreads)
            {
                thread.Join();
            }

            parseArg.ProgressCb(1.0f, parseArg.ProgressContext);

            for(var i = 0; i < threadCount; ++i)
            {
                perfStats.Add(threadDataList[i].PerfStats.Address, i);
            }

            UDT_DLL.PrintPerfStats(perfStats.Address, timer, _filePaths.Count);
            resources.Free();

            return true;
        }

        private void ProcessThread(ThreadInfo info)
        {
            var resources = new ArgumentResources();
            var runner = new BatchJobRunner(info.Argument.Argument, info.FilePaths, info.FileIndices, info.MaxBatchSize);
            var newParseArg = runner.NewParseArg;
            var batchCount = runner.BatchCount;
            for(var i = 0; i < batchCount; ++i)
            {
                runner.PrepareNextBatch();
                _executeJob(resources, ref newParseArg, runner.GetFileList(), runner.GetIndexList());
                info.PerfStats.Merge(info.Argument.Argument.PerformanceStats, i);
                if(runner.IsCanceled(info.Argument.Argument.CancelOperation))
                {
                    return;
                }
            }

            resources.Free();
        }

        private void ThreadEntryPoint(object arg)
        {
            var info = arg as ThreadInfo;
            if(info == null)
            {
                return;
            }

            if(Debugger.IsAttached)
            {
                ProcessThread(info);
                return;
            }

            try
            {
                ProcessThread(info);
            }
            catch(Exception exception)
            {
                EntryPoint.RaiseException(exception);
            }
        }

        private List<string> _filePaths;
        private int _maxBatchSize;
        private JobExecuter _executeJob;
        private UDT_DLL.udtParseArg _parseArg;
        private DemoThreadAllocator _threadAllocator;
        private List<APIArgument> _arguments;
        private bool _multiThreaded;
  
        private class ThreadInfo
        {
            public APIArgument Argument;
            public List<string> FilePaths;
            public List<int> FileIndices;
            public int MaxBatchSize = 0;
            public float Progress = 0.0f;
            public APIPerfStats PerfStats;
        }
    }

    public unsafe class UDT_DLL
    {
#if UDT_X64
        // All x64 numbers are selected to keep memory usage below 100 MB with 4 threads.
        private const int MaxBatchSizeParsing = 512;
        private const int MaxBatchSizeJSONExport = 256;
        private const int MaxBatchSizeCutting = 2048;
        private const int MaxBatchSizeConverting = 2048;
        private const int MaxBatchSizeTimeShifting = 2048;
#else
        private const int MaxBatchSizeParsing = 256;
        private const int MaxBatchSizeJSONExport = 128;
        private const int MaxBatchSizeCutting = 1024;
        private const int MaxBatchSizeConverting = 1024;
        private const int MaxBatchSizeTimeShifting = 1024;
#endif

        private const string _dllPath = "UDT.dll";

        [UnmanagedFunctionPointerAttribute(CallingConvention.Cdecl)]
        public delegate void udtProgressCallback(float progress, IntPtr userData);

        [UnmanagedFunctionPointerAttribute(CallingConvention.Cdecl)]
        public delegate void udtMessageCallback(int logLevel, string message);

        [UnmanagedFunctionPointerAttribute(CallingConvention.Cdecl)]
        public delegate void udtCrashCallback(string message);

        public enum udtProtocol : uint
        {
            Dm3,
            Dm48,
            Dm66,
            Dm67,
            Dm68,
            Dm73,
            Dm90,
            Dm91,
            Count,
            Invalid
        }

        public enum udtErrorCode : int
        {
            None,
            InvalidArgument,
            OperationFailed,
            OperationCanceled,
            Unprocessed
        }

        public enum udtChatOperator : int
        {
            Contains,
            StartsWith,
            EndsWith,
            Count
        }

        public enum udtCrashType : uint
        {
            FatalError,
		    ReadAccess,
		    WriteAccess,
		    Count
        }

        public enum udtParserPlugIn : uint
        {
            Chat,
            GameState,
            Obituaries,
            Stats,
            RawCommands,
            RawConfigStrings,
            Captures,
            Scores,
            Count
        }

        public enum udtWeaponMask : uint
        {
            Gauntlet = 1 << 0,
            MachineGun = 1 << 1,
            Shotgun = 1 << 2,
            Grenade = 1 << 3,
            Rocket = 1 << 4,
            Plasma = 1 << 5,
            Railgun = 1 << 6,
            LightningGun = 1 << 7,
            BFG = 1 << 8,
            NailGun = 1 << 9,
            ChainGun = 1 << 10,
            ProximityMineLauncher = 1 << 11,
            HeavyMachineGun = 1 << 12,
            AfterLast
        }

        public enum udtStringArray : uint
        {
            Weapons,
            PowerUps,
            MeansOfDeath,
            PlayerMeansOfDeath,
            Teams,
            CutPatterns,
            GameTypes,
            ShortGameTypes,
            ModNames,
            GamePlayNames,
            ShortGamePlayNames,
            OverTimeTypes,
            TeamStatsNames,
            PlayerStatsNames,
            PlugInNames,
            PerfStatsNames,
            Count
        }

        public enum udtByteArray : uint
        {
            TeamStatsCompModes,
            PlayerStatsCompModes,
            TeamStatsDataTypes,
            PlayerStatsDataTypes,
            PerfStatsDataTypes,
            GameTypeFlags,
            Count
        }

        public enum udtPatternType : uint
        {
            Chat,
            FragSequences,
            MidAirFrags,
            MultiFragRails,
            FlagCaptures,
            FlickRails,
            Matches,
            Count
        }

        public enum udtStatsCompMode : uint
        {
            NeitherWins,
            BiggerWins,
            SmallerWins,
            Count
        }

        public enum udtMatchStatsDataType : uint
        {
            Generic,
            Team,
            Minutes,
            Seconds,
            Percentage,
            Weapon,
            Ping,
            Positive,
            Boolean,
            Count
        }

        enum udtGameTypeFlags : byte
        {
            None = 0,
            Team = 1 << 0,
            RoundBased = 1 << 1,
            HasCaptureLimit = 1 << 2,
            HasFragLimit = 1 << 3,
            HasScoreLimit = 1 << 4,
            HasRoundLimit = 1 << 5
        }

        private enum udtGameType : uint
        {
            SP,
            FFA,
            Duel,
            Race,
            HM,
            RedRover,
            TDM,
            CBTDM,
            CA,
            CTF,
            OneFlagCTF,
            Obelisk,
            Harvester,
            Domination,
            CTFS,
            NTF,
            TwoVsTwo,
            FT,
            Count
        }

        private enum udtOvertimeType : uint
        {
            None,
            Timed,
            SuddenDeath
        }

        private enum udtPerfStatsDataType : uint
        {
            Generic,
            Bytes,
            Throughput,
            Duration,
            Percentage,
            Count
        }

        [Flags]
        public enum udtParseArgFlags : uint
        {
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtParseArg
        {
            public IntPtr PlugIns; // u32*
            public IntPtr OutputFolderPath; // const char*
            public udtMessageCallback MessageCb;
            public udtProgressCallback ProgressCb;
            public IntPtr ProgressContext; // void*
            public IntPtr CancelOperation; // s32*
            public IntPtr PerformanceStats; // u64*
            public IntPtr Reserved1;
            public UInt32 PlugInCount;
            public Int32 GameStateIndex;
            public UInt32 FileOffset;
            public UInt32 Flags;
            public UInt32 MinProgressTimeMs;
            public Int32 Reserved2;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtMultiParseArg
	    {
		    public IntPtr FilePaths; // const char**
            public IntPtr OutputErrorCodes; // s32*
		    public UInt32 FileCount;
		    public UInt32 MaxThreadCount;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
	    public struct udtCut
	    {
            public IntPtr FilePath; // const char*
            public IntPtr Reserved1;
		    public Int32 StartTimeMs;
		    public Int32 EndTimeMs;
            public Int32 GameStateIndex;
            public Int32 Reserved2;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtPatternInfo
	    {
		    public IntPtr TypeSpecificInfo; // const void*
            public IntPtr Reserved1; // const void*
		    public UInt32 Type;
            public Int32 Reserved2;
	    }

        public enum udtPlayerIndex : int
        {
            FirstPersonPlayer = -2,
            DemoTaker = -1
        }

        [Flags]
        public enum udtCutByPatternArgFlags : uint
        {
            MergeCutSections = 1 << 0
        }

        public enum udtStringComparisonMode : uint
        {
            Equals,
            Contains,
            StartsWith,
            EndsWith,
            Count
        }

        [Flags]
        public enum udtStringMatchingRuleFlags : uint
        {
            CaseSensitive = 1 << 0,
            IgnoreColorCodes = 1 << 1
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        struct udtStringMatchingRule
        {
            public IntPtr Value; // const char*
            public IntPtr Reserved1;
            public UInt32 ComparisonMode;
            public UInt32 Flags;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtPatternSearchArg
        {
            public IntPtr Patterns; // const udtPatternInfo*
            public IntPtr PlayerNameRules; // const udtStringMatchingRule*
            public UInt32 PatternCount;
            public UInt32 PlayerNameRuleCount;
            public UInt32 StartOffsetSec;
            public UInt32 EndOffsetSec;
            public Int32 PlayerIndex;
            public UInt32 Flags;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtPatternMatch
        {
            public UInt32 DemoInputIndex;
            public UInt32 GameStateIndex;
            public Int32 StartTimeMs;
            public Int32 EndTimeMs;
            public UInt32 Patterns;
            public Int32 Reserved1;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtPatternSearchResults
        {
            public IntPtr Matches; // const udtPatternMatch*
            public IntPtr Reserved1;
            public UInt32 MatchCount;
            public Int32 Reserved2;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtCutByTimeArg
        {
            public IntPtr Cuts; // const udtCut*
            public IntPtr Reserved1;
            public UInt32 CutCount;
            public Int32 Reserved2;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
	    public struct udtChatPatternRule
	    {
		    public IntPtr Pattern; // const char*
            public IntPtr Reserved1;
		    public UInt32 ChatOperator;
		    public UInt32 CaseSensitive;
		    public UInt32 IgnoreColorCodes;
            public UInt32 SearchTeamChat;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
	    public struct udtChatPatternArg
	    {
		    public IntPtr Rules; // const udtCutByChatRule*
            public IntPtr Reserved1;
		    public UInt32 RuleCount;
            public Int32 Reserved2;
	    }

        [Flags]
        public enum udtFragRunPatternArgFlags : uint
        {
            AllowSelfKills = 1 << 0,
            AllowTeamKills = 1 << 1,
            AllowDeaths = 1 << 2
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtFragRunPatternArg
        {
            public UInt32 MinFragCount;
            public UInt32 TimeBetweenFragsSec;
            public UInt32 TimeMode; // 0=max, 1=avg
            public UInt32 Flags;
            public UInt32 AllowedMeansOfDeaths;
            public Int32 Reserved1;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtMidAirPatternArg
        {
            public UInt32 AllowedWeapons;
            public UInt32 MinDistance;
            public UInt32 MinAirTimeMs;
            public Int32 Reserved1;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtMultiRailPatternArg
        {
            public UInt32 MinKillCount;
            public Int32 Reserved1;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtFlagCapturePatternArg
        {
            public UInt32 MinCarryTimeMs;
            public UInt32 MaxCarryTimeMs;
            public UInt32 AllowBaseToBase;
            public UInt32 AllowMissingToBase;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtFlickRailPatternArg
        {
            public float MinSpeed;
            public UInt32 MinSpeedSnapshotCount;
            public float MinAngleDelta;
            public UInt32 MinAngleDeltaSnapshotCount;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtMatchPatternArg
        {
            public UInt32 MatchStartOffsetMs;
            public UInt32 MatchEndOffsetMs;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtProtocolConversionArg
        {
            public UInt32 OutputProtocol;
            public Int32 Reserved1;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtParseDataBufferRange
        {
            public UInt32 FirstIndex;
            public UInt32 Count;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtParseDataChat
	    {
            public UInt32 OriginalCommand; // string offset
            public UInt32 OriginalCommandLength;
            public UInt32 ClanName; // string offset
            public UInt32 ClanNameLength;
            public UInt32 PlayerName; // string offset
            public UInt32 PlayerNameLength;
            public UInt32 Message; // string offset
            public UInt32 MessageLength;
            public UInt32 Location; // string offset
            public UInt32 LocationLength;
            public UInt32 OriginalCommandNoCol; // string offset
            public UInt32 OriginalCommandNoColLength;
            public UInt32 ClanNameNoCol; // string offset
            public UInt32 ClanNameNoColLength;
            public UInt32 PlayerNameNoCol; // string offset
            public UInt32 PlayerNameNoColLength;
            public UInt32 MessageNoCol; // string offset
            public UInt32 MessageNoColLength;
            public UInt32 LocationNoCol; // string offset
            public UInt32 LocationNoColLength;
		    public Int32 ServerTimeMs;
		    public Int32 PlayerIndex;
            public Int32 GameStateIndex;
            public UInt32 TeamMessage;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        struct udtParseDataChatBuffers
        {
            public IntPtr ChatMessages; // const udtParseDataChat*
            public IntPtr ChatMessageRanges; // const udtParseDataBufferRange*
            public IntPtr StringBuffer; // const char*
            public IntPtr Reserved1;
            public UInt32 ChatMessageCount;
            public UInt32 StringBufferSize;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtMatchInfo
        {
            public Int32 WarmUpEndTimeMs; 
            public Int32 MatchStartTimeMs;
            public Int32 MatchEndTimeMs;
            public Int32 Reserved1;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtGameStateKeyValuePair
	    {
            public UInt32 Name; // string offset
            public UInt32 NameLength;
            public UInt32 Value; // string offset
            public UInt32 ValueLength;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtGameStatePlayerInfo
	    {
            public UInt32 FirstName; // string offset
            public UInt32 FirstNameLength;
            public Int32 Index;
            public Int32 FirstSnapshotTimeMs;
            public Int32 LastSnapshotTimeMs;
            public UInt32 FirstTeam;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtParseDataGameState
	    {
            public UInt32 DemoTakerName; // string offset
            public UInt32 DemoTakerNameLength;
            public UInt32 FirstMatchIndex;
            public UInt32 MatchCount;
            public UInt32 FirstKeyValuePairIndex;
            public UInt32 KeyValuePairCount;
            public UInt32 FirstPlayerIndex;
            public UInt32 PlayerCount;
            public Int32 DemoTakerPlayerIndex;
            public UInt32 FileOffset;
            public Int32 FirstSnapshotTimeMs;
            public Int32 LastSnapshotTimeMs;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        struct udtParseDataGameStateBuffers
        {
            public IntPtr GameStates; // const udtParseDataGameState*
            public IntPtr GameStateRanges; // const udtParseDataBufferRange*
            public IntPtr Matches; // const udtMatchInfo*
            public IntPtr KeyValuePairs; // const udtGameStateKeyValuePair*
            public IntPtr Players; // const udtGameStatePlayerInfo*
            public IntPtr StringBuffer; // const char*
            public UInt32 GameStateCount;
            public UInt32 MatchCount;
            public UInt32 KeyValuePairCount;
            public UInt32 PlayerCount;
            public UInt32 StringBufferSize;
            public Int32 Reserved1;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        struct udtParseDataObituary
	    {
            public UInt32 AttackerName; // string offset
            public UInt32 AttackerNameLength;
            public UInt32 TargetName; // string offset
            public UInt32 TargetNameLength;
            public UInt32 MeanOfDeathName; // string offset
            public UInt32 MeanOfDeathNameLength;
            public Int32 GameStateIndex;
            public Int32 ServerTimeMs;
            public Int32 AttackerIdx;
            public Int32 TargetIdx;
            public Int32 MeanOfDeath;
            public Int32 AttackerTeamIdx;
            public Int32 TargetTeamIdx;
            public Int32 Reserved1;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        struct udtParseDataObituaryBuffers
        {
            public IntPtr Obituaries; // const udtParseDataObituary*
            public IntPtr ObituaryRanges; // const udtParseDataBufferRange*
            public IntPtr StringBuffer; // const char*
            public IntPtr Reserved1;
            public UInt32 ObituaryCount;
            public UInt32 StringBufferSize;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        struct udtPlayerStats
	    {
            public UInt32 Name; // string offset
            public UInt32 NameLength;
            public UInt32 CleanName; // string offset
            public UInt32 CleanNameLength;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
	    struct udtParseDataStats
	    {
            public UInt64 ValidTeams;
            public UInt64 ValidPlayers;
            public UInt32 ModVersion; // string offset
            public UInt32 ModVersionLength;
            public UInt32 MapName; // string offset
            public UInt32 MapNameLength;
            public UInt32 FirstPlaceName; // string offset
            public UInt32 FirstPlaceNameLength;
            public UInt32 SecondPlaceName; // string offset
            public UInt32 SecondPlaceNameLength;
            public UInt32 CustomRedName; // string offset
            public UInt32 CustomRedNameLength;
            public UInt32 CustomBlueName; // string offset
            public UInt32 CustomBlueNameLength;
            public UInt32 FirstTimeOutRangeIndex; // Index into the s32 integers pair array, not the s32 integers array.
            public UInt32 FirstTeamFlagIndex;
            public UInt32 FirstPlayerFlagIndex;
            public UInt32 FirstTeamFieldIndex;
            public UInt32 FirstPlayerFieldIndex;
            public UInt32 FirstPlayerStatsIndex;
            public UInt32 GameType;
            public UInt32 MatchDurationMs;
            public UInt32 Mod;
            public UInt32 GamePlay;
            public UInt32 OverTimeType;
            public UInt32 OverTimeCount;
            public UInt32 Forfeited;
            public UInt32 TimeOutCount;
            public UInt32 TotalTimeOutDurationMs;
            public UInt32 MercyLimited;
            public Int32 FirstPlaceScore;
            public Int32 SecondPlaceScore;
            public UInt32 SecondPlaceWon;
            public UInt32 TeamMode;
            public UInt32 StartDateEpoch;
            public UInt32 TimeLimit;
            public UInt32 ScoreLimit;
            public UInt32 FragLimit;
            public UInt32 CaptureLimit;
            public UInt32 RoundLimit;
            public Int32 StartTimeMs;
            public Int32 EndTimeMs;
            public UInt32 GameStateIndex;
            public Int32 CountDownStartTimeMs;
            public Int32 IntermissionEndTimeMs;
            public Int32 Reserved1;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        struct udtParseDataStatsBuffers
        {
            public IntPtr MatchStats; // const udtParseDataStats*
            public IntPtr MatchStatsRanges; // const udtParseDataBufferRange*
            public IntPtr TimeOutStartAndEndTimes; // const s32*
            public IntPtr TeamFlags; // const u8*
            public IntPtr PlayerFlags; // const u8*
            public IntPtr TeamFields; // const s32*
            public IntPtr PlayerFields; // const s32*
            public IntPtr PlayerStats; // const udtPlayerStats*
            public IntPtr StringBuffer; // const char*
            public IntPtr Reserved1;
            public UInt32 MatchCount;
            public UInt32 TimeOutRangeCount; // The number of ranges (i.e. the number of s32 values over 2).
            public UInt32 TeamFlagCount;
            public UInt32 PlayerFlagCount;
            public UInt32 TeamFieldCount;
            public UInt32 PlayerFieldCount;
            public UInt32 PlayerStatsCount;
            public UInt32 StringBufferSize;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        struct udtParseDataRawCommand
        {
            public UInt32 RawCommand; // string offset
            public UInt32 RawCommandLength;
            public Int32 ServerTimeMs;
            public Int32 GameStateIndex;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        struct udtParseDataRawCommandBuffers
        {
            public IntPtr Commands; // const udtParseDataRawCommand*
            public IntPtr CommandRanges; // const udtParseDataBufferRange*
            public IntPtr StringBuffer; // const char*
            public IntPtr Reserved1;
            public UInt32 CommandCount;
            public UInt32 StringBufferSize;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        struct udtParseDataRawConfigString
        {
            public UInt32 RawConfigString; // string offset
            public UInt32 RawConfigStringLength;
            public UInt32 ConfigStringIndex;
            public Int32 GameStateIndex;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        struct udtParseDataRawConfigStringBuffers
        {
            public IntPtr ConfigStrings; // const udtParseDataRawConfigString*
            public IntPtr ConfigStringRanges; // const udtParseDataBufferRange*
            public IntPtr StringBuffer; // const char*
            public IntPtr Reserved1;
            public UInt32 ConfigStringCount;
            public UInt32 StringBufferSize;
        }

        [Flags]
        enum udtParseDataCaptureFlags : uint
	    {
		    BaseToBase = 1 << 0,
			DemoTaker = 1 << 1,
			FirstPersonPlayer = 1 << 2,
            PlayerIndexValid = 1 << 3,
			PlayerNameValid = 1 << 4,
			DistanceValid = 1 << 5
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        struct udtParseDataCapture
        {
            public UInt32 MapName; // string offset
            public UInt32 MapNameLength;
            public UInt32 PlayerName; // string offset
            public UInt32 PlayerNameLength;
            public Int32 GameStateIndex;
            public Int32 PickUpTimeMs;
            public Int32 CaptureTimeMs;
            public float Distance;
            public UInt32 Flags;
            public Int32 PlayerIndex;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        struct udtParseDataCaptureBuffers
        {
            public IntPtr Captures; // const udtParseDataCapture*
            public IntPtr CaptureRanges; // const udtParseDataBufferRange*
            public IntPtr StringBuffer; // const char*
            public IntPtr Reserved1;
            public UInt32 CaptureCount;
            public UInt32 StringBufferSize;
        }

        [Flags]
        enum udtParseDataScoreMask : uint
		{
			TeamBased = 1 << 0
		};

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
	    struct udtParseDataScore
	    {
		    public Int32 GameStateIndex;
		    public Int32 ServerTimeMs;
		    public Int32 Score1; 
		    public Int32 Score2;
		    public UInt32 Id1;
		    public UInt32 Id2;
		    public UInt32 Flags;
		    public UInt32 Name1;
		    public UInt32 Name1Length;
		    public UInt32 Name2;
		    public UInt32 Name2Length;
            public UInt32 CleanName1;
            public UInt32 CleanName1Length;
            public UInt32 CleanName2;
            public UInt32 CleanName2Length;
		    public Int32 Reserved1;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
	    struct udtParseDataScoreBuffers
	    {
		    public IntPtr Scores; // const udtParseDataScore*
            public IntPtr ScoreRanges; // const udtParseDataBufferRange*
		    public IntPtr StringBuffer; // const u8*
		    public IntPtr Reserved1; // const void*
		    public UInt32 ScoreCount;
		    public UInt32 StringBufferSize;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        struct udtTimeShiftArg
        {
            public Int32 SnapshotCount;
            public Int32 Reserved1;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        struct udtJSONArg
        {
            public UInt32 ConsoleOutput;
            public Int32 Reserved1;
        }

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private IntPtr udtGetVersionString();

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private IntPtr udtGetVersionNumbers(ref UInt32 major, ref UInt32 minor, ref UInt32 revision);

	    [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private Int32 udtIsValidProtocol(udtProtocol protocol);

	    [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private UInt32 udtGetSizeOfIdEntityState(udtProtocol protocol);

	    [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private UInt32 udtGetSizeOfidClientSnapshot(udtProtocol protocol);

	    [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private IntPtr udtGetFileExtensionByProtocol(udtProtocol protocol);

	    [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private udtProtocol udtGetProtocolByFilePath(IntPtr filePath);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtCrash(udtCrashType crashType);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtGetStringArray(udtStringArray arrayId, ref IntPtr array, ref UInt32 elementCount);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtGetByteArray(udtByteArray arrayId, ref IntPtr array, ref UInt32 elementCount);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtGetStatsConstants(ref UInt32 playerMaskByteCount, ref UInt32 teamMaskByteCount, ref UInt32 playerFieldCount, ref UInt32 teamFieldCount, ref UInt32 perfFieldCount);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtMergeBatchPerfStats(IntPtr destPerfStats, IntPtr sourcePerfStats);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtAddThreadPerfStats(IntPtr destPerfStats, IntPtr sourcePerfStats);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private udtErrorCode udtGetProcessorCoreCount(ref UInt32 cpuCoreCount);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtInitLibrary();

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtShutDownLibrary();

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtSetCrashHandler(IntPtr crashHandler);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private udtParserContextRef udtCreateContext();

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private udtErrorCode udtDestroyContext(udtParserContextRef context);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtSplitDemoFile(udtParserContextRef context, ref udtParseArg info, IntPtr demoFilePath);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtCutDemoFileByTime(udtParserContextRef context, ref udtParseArg info, ref udtCutByTimeArg cutInfo, IntPtr demoFilePath);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtMergeDemoFiles(ref udtParseArg info, IntPtr filePaths, UInt32 fileCount);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtGetContextPlugInBuffers(udtParserContextRef context, udtParserPlugIn plugInId, IntPtr buffersStruct);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl, EntryPoint = "udtGetContextPlugInBuffers")]
        extern static private udtErrorCode udtGetContextPlugInRawCommandBuffers(udtParserContextRef context, udtParserPlugIn plugInId, ref udtParseDataRawCommandBuffers buffersStruct);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl, EntryPoint = "udtGetContextPlugInBuffers")]
        extern static private udtErrorCode udtGetContextPlugInRawConfigStringBuffers(udtParserContextRef context, udtParserPlugIn plugInId, ref udtParseDataRawConfigStringBuffers buffersStruct);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl, EntryPoint = "udtGetContextPlugInBuffers")]
        extern static private udtErrorCode udtGetContextPlugInChatBuffers(udtParserContextRef context, udtParserPlugIn plugInId, ref udtParseDataChatBuffers buffersStruct);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl, EntryPoint = "udtGetContextPlugInBuffers")]
        extern static private udtErrorCode udtGetContextPlugInDeathBuffers(udtParserContextRef context, udtParserPlugIn plugInId, ref udtParseDataObituaryBuffers buffersStruct);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl, EntryPoint = "udtGetContextPlugInBuffers")]
        extern static private udtErrorCode udtGetContextPlugInCaptureBuffers(udtParserContextRef context, udtParserPlugIn plugInId, ref udtParseDataCaptureBuffers buffersStruct);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl, EntryPoint = "udtGetContextPlugInBuffers")]
        extern static private udtErrorCode udtGetContextPlugInScoreBuffers(udtParserContextRef context, udtParserPlugIn plugInId, ref udtParseDataScoreBuffers buffersStruct);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl, EntryPoint = "udtGetContextPlugInBuffers")]
        extern static private udtErrorCode udtGetContextPlugInGameStateBuffers(udtParserContextRef context, udtParserPlugIn plugInId, ref udtParseDataGameStateBuffers buffersStruct);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl, EntryPoint = "udtGetContextPlugInBuffers")]
        extern static private udtErrorCode udtGetContextPlugInStatsBuffers(udtParserContextRef context, udtParserPlugIn plugInId, ref udtParseDataStatsBuffers buffersStruct);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtParseDemoFiles(ref udtParserContextGroupRef contextGroup, ref udtParseArg info, ref udtMultiParseArg extraInfo);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtGetContextCountFromGroup(udtParserContextGroupRef contextGroup, ref UInt32 count);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtGetContextFromGroup(udtParserContextGroupRef contextGroup, UInt32 contextIdx, ref udtParserContextRef context);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtGetDemoCountFromGroup(udtParserContextGroupRef contextGroup, ref UInt32 count);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtGetDemoCountFromContext(udtParserContextRef context, ref UInt32 count);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtGetDemoInputIndex(udtParserContextRef context, UInt32 demoIdx, ref UInt32 demoInputIdx);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtDestroyContextGroup(udtParserContextGroupRef contextGroup);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtCutDemoFilesByPattern(ref udtParseArg info, ref udtMultiParseArg extraInfo, ref udtPatternSearchArg patternInfo);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtFindPatternsInDemoFiles(ref udtPatternSearchContextRef context, ref udtParseArg info, ref udtMultiParseArg extraInfo, ref udtPatternSearchArg patternInfo);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private udtErrorCode udtGetSearchResults(udtPatternSearchContextRef context, ref udtPatternSearchResults results);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private udtErrorCode udtDestroySearchContext(udtPatternSearchContextRef context);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtConvertDemoFiles(ref udtParseArg info, ref udtMultiParseArg extraInfo, ref udtProtocolConversionArg conversionInfo);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtTimeShiftDemoFiles(ref udtParseArg info, ref udtMultiParseArg extraInfo, ref udtTimeShiftArg timeShiftArg);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtSaveDemoFilesAnalysisDataToJSON(ref udtParseArg info, ref udtMultiParseArg extraInfo, ref udtJSONArg jsonInfo);

        public class StatsConstantsGrabber
        {
            public StatsConstantsGrabber()
            {
                UInt32 playerMaskByteCount = 0;
                UInt32 teamMaskByteCount = 0;
                UInt32 playerFieldCount = 0;
                UInt32 teamFieldCount = 0;
                UInt32 perfFieldCount = 0;
                udtGetStatsConstants(ref playerMaskByteCount, ref teamMaskByteCount, ref playerFieldCount, ref teamFieldCount, ref perfFieldCount);
                PlayerMaskByteCount = (int)playerMaskByteCount;
                TeamMaskByteCount = (int)teamMaskByteCount;
                PlayerFieldCount = (int)playerFieldCount;
                TeamFieldCount = (int)teamFieldCount;
                PerfFieldCount = (int)perfFieldCount;
            }

            public int PlayerMaskByteCount { private set; get; }
            public int TeamMaskByteCount { private set; get; }
            public int PlayerFieldCount { private set; get; }
            public int TeamFieldCount { private set; get; }
            public int PerfFieldCount { private set; get; }
        }

        public static readonly StatsConstantsGrabber StatsConstants = new StatsConstantsGrabber();

        public class LibraryArraysGrabber
        {
            public LibraryArraysGrabber()
            {
                _playerFieldNames = GetStringArray(udtStringArray.PlayerStatsNames);

                IntPtr playerCompModes = IntPtr.Zero;
                UInt32 playerCompModeCount = 0;
                udtGetByteArray(udtByteArray.PlayerStatsCompModes, ref playerCompModes, ref playerCompModeCount);
                _playerCompModes = MarshalHelper.PtrToByteArray(playerCompModes, (int)playerCompModeCount);

                IntPtr playerDataTypes = IntPtr.Zero;
                UInt32 playerDataTypeCount = 0;
                udtGetByteArray(udtByteArray.PlayerStatsDataTypes, ref playerDataTypes, ref playerDataTypeCount);
                _playerDataTypes = MarshalHelper.PtrToByteArray(playerDataTypes, (int)playerDataTypeCount);

                _teamFieldNames = GetStringArray(udtStringArray.TeamStatsNames);

                IntPtr teamFieldCompModes = IntPtr.Zero;
                UInt32 teamFieldCompModeCount = 0;
                udtGetByteArray(udtByteArray.TeamStatsCompModes, ref teamFieldCompModes, ref teamFieldCompModeCount);
                _teamCompModes = MarshalHelper.PtrToByteArray(playerCompModes, (int)playerCompModeCount);

                IntPtr teamFieldDataTypes = IntPtr.Zero;
                UInt32 teamFieldDataTypeCount = 0;
                udtGetByteArray(udtByteArray.TeamStatsDataTypes, ref teamFieldDataTypes, ref teamFieldDataTypeCount);
                _teamDataTypes = MarshalHelper.PtrToByteArray(teamFieldDataTypes, (int)teamFieldDataTypeCount);

                IntPtr gameTypeFlags = IntPtr.Zero;
                UInt32 gameTypeCount = 0;
                udtGetByteArray(udtByteArray.GameTypeFlags, ref gameTypeFlags, ref gameTypeCount);
                _gameTypes = MarshalHelper.PtrToByteArray(gameTypeFlags, (int)gameTypeCount);
            }

            public string GetPlayerFieldName(int index)
            {
                return _playerFieldNames[index];
            }

            public udtStatsCompMode GetPlayerFieldCompMode(int index)
            {
                return (udtStatsCompMode)_playerCompModes[index];
            }

            public udtMatchStatsDataType GetPlayerFieldDataType(int index)
            {
                return (udtMatchStatsDataType)_playerDataTypes[index];
            }

            public string GetTeamFieldName(int index)
            {
                return _teamFieldNames[index];
            }

            public udtStatsCompMode GetTeamFieldCompMode(int index)
            {
                return (udtStatsCompMode)_teamCompModes[index];
            }

            public udtMatchStatsDataType GetTeamFieldDataType(int index)
            {
                return (udtMatchStatsDataType)_teamDataTypes[index];
            }

            public bool IsModeRoundBased(int modeIndex)
            {
                return modeIndex >= 0 && modeIndex < _gameTypes.Length && (_gameTypes[modeIndex] & (byte)udtGameTypeFlags.RoundBased) != 0;
            }

            private List<string> _playerFieldNames;
            private byte[] _playerCompModes;
            private byte[] _playerDataTypes;
            private List<string> _teamFieldNames;
            private byte[] _teamCompModes;
            private byte[] _teamDataTypes;
            private byte[] _gameTypes;
        }

        public static readonly LibraryArraysGrabber LibraryArrays = new LibraryArraysGrabber();

        // The list of plug-ins activated when loading demos.
        private static UInt32[] PlugInArray = new UInt32[] 
        { 
            (UInt32)udtParserPlugIn.Chat, 
            (UInt32)udtParserPlugIn.GameState,
            (UInt32)udtParserPlugIn.Obituaries,
            (UInt32)udtParserPlugIn.Stats,
            (UInt32)udtParserPlugIn.RawCommands,
            (UInt32)udtParserPlugIn.RawConfigStrings,
            (UInt32)udtParserPlugIn.Captures,
            (UInt32)udtParserPlugIn.Scores
        };

        public static List<string> GetStringArray(udtStringArray array)
        {
            IntPtr elements = IntPtr.Zero;
            UInt32 elementCount = 0;
            if(udtGetStringArray(array, ref elements, ref elementCount) != udtErrorCode.None)
            {
                return null;
            }

            int elementSize = Marshal.SizeOf(typeof(IntPtr));

            var list = new List<string>();
            for(UInt32 i = 0; i < elementCount; ++i)
            {
                var address = Marshal.ReadIntPtr(elements, (int)i * elementSize);
                var element = Marshal.PtrToStringAnsi(address);
                list.Add(element ?? "N/A");
            }

            return list;
        }

        public static string GetVersion()
        {
            var version = udtGetVersionString();
            if(version == IntPtr.Zero)
            {
                return "N/A";
            }

            return Marshal.PtrToStringAnsi(version) ?? "N/A";
        }

        public class Version
        {
            public Version(uint maj, uint min, uint rev)
            {
                Major = maj;
                Minor = min;
                Revision = rev;
            }

            public uint AsSingleNumber()
            {
                return Major * 100 + Minor * 10 + Revision;
            }

            public int CompareTo(Version other)
            {
                return AsSingleNumber().CompareTo(other.AsSingleNumber());
            }

            public override string ToString()
            {
                return string.Format("{0}.{1}.{2}", Major, Minor, Revision);
            }

            public uint Major { get; private set; }
            public uint Minor { get; private set; }
            public uint Revision { get; private set; }
        }

        public static Version GetVersionNumbers()
        {
            uint maj = 0;
            uint min = 0;
            uint rev = 0;
            udtGetVersionNumbers(ref maj, ref min, ref rev);

            return new Version(maj, min, rev);
        }

        public static bool Crash(udtCrashType crashType)
        {
            return udtCrash(crashType) == udtErrorCode.None;
        }

        public static void InitLibrary()
        {
            udtInitLibrary();
        }

        public static void ShutDownLibrary()
        {
            udtShutDownLibrary();
        }

        public static bool SetFatalErrorHandler(udtCrashCallback handler)
        {
            GCHandle.Alloc(handler);
            var address = Marshal.GetFunctionPointerForDelegate(handler);

            return udtSetCrashHandler(address) == udtErrorCode.None;
        }

        public static string GetErrorCodeString(udtErrorCode errorCode)
        {
            switch(errorCode)
            {
                case udtErrorCode.None: return "no error";
                case udtErrorCode.InvalidArgument: return "invalid argument";
                case udtErrorCode.OperationFailed: return "operation failed";
                case udtErrorCode.Unprocessed: return "unprocessed";
            }

            return "invalid error code";
        }

        public static udtProtocol GetProtocolFromFilePath(string filePath)
        {
            var filePathPtr = StringToHGlobalUTF8(filePath);
            var protocol = udtGetProtocolByFilePath(filePathPtr);
            Marshal.FreeHGlobal(filePathPtr);

            return protocol;
        }

        public static udtFragRunPatternArg CreateCutByFragArg(UdtConfig config, UdtPrivateConfig privateConfig)
        {
            UInt32 flags = 0;
            if(config.FragCutAllowAnyDeath)
            {
                flags |= (UInt32)UDT_DLL.udtFragRunPatternArgFlags.AllowDeaths;
            }
            if(config.FragCutAllowSelfKills)
            {
                flags |= (UInt32)UDT_DLL.udtFragRunPatternArgFlags.AllowSelfKills;
            }
            if(config.FragCutAllowTeamKills)
            {
                flags |= (UInt32)UDT_DLL.udtFragRunPatternArgFlags.AllowTeamKills;
            }

            var rules = new udtFragRunPatternArg();
            rules.MinFragCount = (UInt32)config.FragCutMinFragCount;
            rules.TimeBetweenFragsSec = (UInt32)config.FragCutTimeBetweenFrags;
            rules.TimeMode = 0; // @TODO:
            rules.Flags = flags;
            rules.AllowedMeansOfDeaths = privateConfig.FragCutAllowedMeansOfDeaths;

            return rules;
        }

        public static udtMidAirPatternArg CreateCutByMidAirArg(UdtConfig config)
        {
            UInt32 weaponFlags = 0;
            if(config.MidAirCutAllowRocket)
            {
                weaponFlags |= (UInt32)udtWeaponMask.Rocket;
            }
            if(config.MidAirCutAllowBFG)
            {
                weaponFlags |= (UInt32)udtWeaponMask.BFG;
            }

            var rules = new udtMidAirPatternArg();
            rules.AllowedWeapons = weaponFlags;
            rules.MinDistance = (UInt32)Math.Max(0, config.MidAirCutMinDistance);
            rules.MinAirTimeMs = (UInt32)Math.Max(0, config.MidAirCutMinAirTimeMs);

            return rules;
        }

        public static udtMultiRailPatternArg CreateCutByMultiRailArg(UdtConfig config)
        {
            var rules = new udtMultiRailPatternArg();
            rules.MinKillCount = (UInt32)config.MultiRailCutMinFragCount;

            return rules;
        }

        public static udtFlagCapturePatternArg CreateCutByFlagCaptureArg(UdtConfig config)
        {
            var rules = new udtFlagCapturePatternArg();
            rules.MinCarryTimeMs = (UInt32)config.FlagCaptureMinCarryTimeMs;
            rules.MaxCarryTimeMs = (UInt32)config.FlagCaptureMaxCarryTimeMs;
            rules.AllowBaseToBase = (UInt32)(config.FlagCaptureAllowBaseToBase ? 1 : 0);
            rules.AllowMissingToBase = (UInt32)(config.FlagCaptureAllowMissingToBase ? 1 : 0);

            return rules;
        }

        public static udtMatchPatternArg CreateCutByMatchArg(UdtConfig config)
        {
            var rules = new udtMatchPatternArg();
            rules.MatchStartOffsetMs = (UInt32)config.MatchCutStartTimeOffsetMs;
            rules.MatchEndOffsetMs = (UInt32)config.MatchCutEndTimeOffsetMs;

            return rules;
        }

        public static udtFlickRailPatternArg CreateCutByFlickRailArg(UdtConfig config)
        {
            var rules = new udtFlickRailPatternArg();
            rules.MinSpeed = (config.FlickRailMinSpeed / 180.0f) * (float)Math.PI;
            rules.MinAngleDelta = (config.FlickRailMinAngleDelta / 180.0f) * (float)Math.PI;
            rules.MinSpeedSnapshotCount = (UInt32)config.FlickRailMinSpeedSnaps;
            rules.MinAngleDeltaSnapshotCount = (UInt32)config.FlickRailMinAngleDeltaSnaps;

            return rules;
        }

        public static IntPtr CreateContext()
        {
            return udtCreateContext();
        }

        public static string GetProtocolAsString(udtProtocol protocol)
        {
            switch(protocol)
            {
                case udtProtocol.Dm3:  return "3 (Quake 3 1.11-1.17)";
                case udtProtocol.Dm48: return "48 (Quake 3 1.27)";
                case udtProtocol.Dm66: return "66 (Quake 3 1.29-1.30)";
                case udtProtocol.Dm67: return "67 (Quake 3 1.31)";
                case udtProtocol.Dm68: return "68 (Quake 3 1.32)";
                case udtProtocol.Dm73: return "73 (Quake Live)";
                case udtProtocol.Dm90: return "90 (Quake Live)";
                case udtProtocol.Dm91: return "91 (Quake Live)";
                default: return "?";
            }
        }

        public static bool SplitDemo(udtParserContextRef context, ref udtParseArg parseArg, string filePath)
        {
            if(context == IntPtr.Zero)
            {
                return false;
            }

            parseArg.PlugInCount = 0;
            parseArg.PlugIns = IntPtr.Zero;

            var filePathPtr = StringToHGlobalUTF8(filePath);
            var success = udtSplitDemoFile(context, ref parseArg, filePathPtr) == udtErrorCode.None;
            Marshal.FreeHGlobal(filePathPtr);

            return success;
        }

        public static bool CutDemoByTime(udtParserContextRef context, ref udtParseArg parseArg, string filePath, int startTimeSec, int endTimeSec)
        {
            if(context == IntPtr.Zero)
            {
                return false;
            }

            parseArg.PlugInCount = 0;
            parseArg.PlugIns = IntPtr.Zero;

            var cut = new udtCut();
            cut.FilePath = IntPtr.Zero;
            cut.GameStateIndex = parseArg.GameStateIndex;
            cut.StartTimeMs = startTimeSec * 1000;
            cut.EndTimeMs = endTimeSec * 1000;
            var pinnedCut = new PinnedObject(cut);
            var cutInfo = new udtCutByTimeArg();
            cutInfo.Cuts = pinnedCut.Address;
            cutInfo.CutCount = 1;

            var filePathPtr = StringToHGlobalUTF8(filePath);
            var success = udtCutDemoFileByTime(context, ref parseArg, ref cutInfo, filePathPtr) == udtErrorCode.None;
            Marshal.FreeHGlobal(filePathPtr);
            pinnedCut.Free();

            return success;
        }

        public class Cut
        {
            public int GameStateIndex;
            public int StartTimeMs;
            public int EndTimeMs;
        }

        public static bool CutDemoByTimes(udtParserContextRef context, ref udtParseArg parseArg, string filePath, List<Cut> newCuts)
        {
            if(context == IntPtr.Zero || newCuts.Count == 0)
            {
                return false;
            }

            parseArg.PlugInCount = 0;
            parseArg.PlugIns = IntPtr.Zero;

            var cutCount = newCuts.Count;
            var cuts = new udtCut[cutCount];
            for(var i = 0; i < cutCount; ++i)
            {
                var cut = new udtCut();
                cut.FilePath = IntPtr.Zero;
                cut.GameStateIndex = (Int32)newCuts[i].GameStateIndex;
                cut.StartTimeMs = newCuts[i].StartTimeMs;
                cut.EndTimeMs = newCuts[i].EndTimeMs;
                cuts[i] = cut;
            }

            var pinnedCuts = new PinnedObject(cuts);
            var cutInfo = new udtCutByTimeArg();
            cutInfo.Cuts = pinnedCuts.Address;
            cutInfo.CutCount = (UInt32)cutCount;

            var filePathPtr = StringToHGlobalUTF8(filePath);
            var success = udtCutDemoFileByTime(context, ref parseArg, ref cutInfo, filePathPtr) == udtErrorCode.None;
            Marshal.FreeHGlobal(filePathPtr);
            pinnedCuts.Free();

            return success;
        }

        public static bool MergeDemos(ref udtParseArg parseArg, List<string> filePaths)
        {
            var resources = new ArgumentResources();
            var filePathsArray = new IntPtr[filePaths.Count];
            for(var i = 0; i < filePaths.Count; ++i)
            {
                filePathsArray[i] = StringToHGlobalUTF8(filePaths[i]);
                resources.GlobalAllocationHandles.Add(filePathsArray[i]);
            }
            var pinnedFilePaths = new PinnedObject(filePathsArray);
            resources.PinnedObjects.Add(pinnedFilePaths);

            var success = udtMergeDemoFiles(ref parseArg, pinnedFilePaths.Address, (UInt32)filePaths.Count) == udtErrorCode.None;
            resources.Free();

            return success;
        }

        private static UInt32 GetOperatorFromString(string op)
        {
            udtChatOperator result;
            if(!Enum.TryParse<udtChatOperator>(op, false, out result))
            {
                return (UInt32)udtChatOperator.Contains;
            }

            return (UInt32)result;
        }

        public static bool CreateChatPatternInfo(ref udtPatternInfo pattern, ArgumentResources resources, List<ChatRule> rules)
        {
            if(rules.Count == 0)
            {
                return false;
            }

            var rulesArray = new UDT_DLL.udtChatPatternRule[rules.Count];
            for(var i = 0; i < rules.Count; ++i)
            {
                rulesArray[i].CaseSensitive = (UInt32)(rules[i].CaseSensitive ? 1 : 0);
                rulesArray[i].ChatOperator = GetOperatorFromString(rules[i].Operator);
                rulesArray[i].IgnoreColorCodes = (UInt32)(rules[i].IgnoreColors ? 1 : 0);
                rulesArray[i].Pattern = StringToHGlobalUTF8(rules[i].Value);
                rulesArray[i].SearchTeamChat = (UInt32)(rules[i].SearchTeamMessages ? 1 : 0);
                resources.GlobalAllocationHandles.Add(rulesArray[i].Pattern);
            }
            var pinnedRulesArray = new PinnedObject(rulesArray);

            var cutByChatArg = new udtChatPatternArg();
            cutByChatArg.Rules = pinnedRulesArray.Address;
            cutByChatArg.RuleCount = (UInt32)rulesArray.Length;
            var pinnedRules = new PinnedObject(cutByChatArg);

            resources.PinnedObjects.Add(pinnedRulesArray);
            resources.PinnedObjects.Add(pinnedRules);

            pattern.Type = (UInt32)udtPatternType.Chat;
            pattern.TypeSpecificInfo = pinnedRules.Address;

            return true;
        }

        public static bool CreateFragPatternInfo(ref udtPatternInfo pattern, ArgumentResources resources, udtFragRunPatternArg rules)
        {
            var pinnedRules = new PinnedObject(rules);
            resources.PinnedObjects.Add(pinnedRules);

            pattern.Type = (UInt32)udtPatternType.FragSequences;
            pattern.TypeSpecificInfo = pinnedRules.Address;

            return true;
        }

        public static bool CreateMidAirPatternInfo(ref udtPatternInfo pattern, ArgumentResources resources, udtMidAirPatternArg rules)
        {
            var pinnedRules = new PinnedObject(rules);
            resources.PinnedObjects.Add(pinnedRules);

            pattern.Type = (UInt32)udtPatternType.MidAirFrags;
            pattern.TypeSpecificInfo = pinnedRules.Address;

            return true;
        }

        public static bool CreateMultiRailPatternInfo(ref udtPatternInfo pattern, ArgumentResources resources, udtMultiRailPatternArg rules)
        {
            var pinnedRules = new PinnedObject(rules);
            resources.PinnedObjects.Add(pinnedRules);

            pattern.Type = (UInt32)udtPatternType.MultiFragRails;
            pattern.TypeSpecificInfo = pinnedRules.Address;

            return true;
        }

        public static bool CreateFlagCapturePatternInfo(ref udtPatternInfo pattern, ArgumentResources resources, udtFlagCapturePatternArg rules)
        {
            var pinnedRules = new PinnedObject(rules);
            resources.PinnedObjects.Add(pinnedRules);

            pattern.Type = (UInt32)udtPatternType.FlagCaptures;
            pattern.TypeSpecificInfo = pinnedRules.Address;

            return true;
        }

        public static bool CreateFlickRailPatternInfo(ref udtPatternInfo pattern, ArgumentResources resources, udtFlickRailPatternArg rules)
        {
            var pinnedRules = new PinnedObject(rules);
            resources.PinnedObjects.Add(pinnedRules);

            pattern.Type = (UInt32)udtPatternType.FlickRails;
            pattern.TypeSpecificInfo = pinnedRules.Address;

            return true;
        }

        public static bool CreateMatchPatternInfo(ref udtPatternInfo pattern, ArgumentResources resources, udtMatchPatternArg rules)
        {
            var pinnedRules = new PinnedObject(rules);
            resources.PinnedObjects.Add(pinnedRules);

            pattern.Type = (UInt32)udtPatternType.Matches;
            pattern.TypeSpecificInfo = pinnedRules.Address;

            return true;
        }

        public class CutByPatternOptions
        {
            public int StartOffset;
            public int EndOffset;
            public int MaxThreadCount;
            public bool MergeCutSections;
            public int PlayerIndex;
            public string PlayerName;

            public CutByPatternOptions(int startOffset, int endOffset, int maxThreadCount, bool mergeCutSections, int playerIndex, string playerName)
            {
                StartOffset = startOffset;
                EndOffset = endOffset;
                MaxThreadCount = maxThreadCount;
                MergeCutSections = mergeCutSections;
                PlayerIndex = playerIndex;
                PlayerName = playerName;
            }
        }

        public static CutByPatternOptions CreateCutByPatternOptions(UdtConfig config, UdtPrivateConfig privateConfig)
        {
            return new CutByPatternOptions(
                config.CutStartOffset,
                config.CutEndOffset,
                config.MaxThreadCount,
                config.MergeCutSectionsFromDifferentPatterns,
                privateConfig.PatternCutPlayerIndex,
                privateConfig.PatternCutPlayerName);
        }

        public static bool CutDemosByChat(ref udtParseArg parseArg, List<string> filePaths, List<ChatRule> rules, CutByPatternOptions options)
        {
            var resources = new ArgumentResources();
            var patterns = new udtPatternInfo[1];
            if(!CreateChatPatternInfo(ref patterns[0], resources, rules))
            {
                return false;
            }

            return CutDemosByPattern(resources, ref parseArg, filePaths, patterns, options);
        }

        public static bool CutDemosByFrag(ref udtParseArg parseArg, List<string> filePaths, udtFragRunPatternArg rules, CutByPatternOptions options)
        {
            var resources = new ArgumentResources();
            var patterns = new udtPatternInfo[1];
            if(!CreateFragPatternInfo(ref patterns[0], resources, rules))
            {
                return false;
            }

            return CutDemosByPattern(resources, ref parseArg, filePaths, patterns, options);
        }

        public static bool CutDemosByMidAir(ref udtParseArg parseArg, List<string> filePaths, udtMidAirPatternArg rules, CutByPatternOptions options)
        {
            var resources = new ArgumentResources();
            var patterns = new udtPatternInfo[1];
            if(!CreateMidAirPatternInfo(ref patterns[0], resources, rules))
            {
                return false;
            }

            return CutDemosByPattern(resources, ref parseArg, filePaths, patterns, options);
        }

        public static bool CutDemosByMultiRail(ref udtParseArg parseArg, List<string> filePaths, udtMultiRailPatternArg rules, CutByPatternOptions options)
        {
            var resources = new ArgumentResources();
            var patterns = new udtPatternInfo[1];
            if(!CreateMultiRailPatternInfo(ref patterns[0], resources, rules))
            {
                return false;
            }

            return CutDemosByPattern(resources, ref parseArg, filePaths, patterns, options);
        }

        public static bool CutDemosByFlagCapture(ref udtParseArg parseArg, List<string> filePaths, udtFlagCapturePatternArg rules, CutByPatternOptions options)
        {
            var resources = new ArgumentResources();
            var patterns = new udtPatternInfo[1];
            if(!CreateFlagCapturePatternInfo(ref patterns[0], resources, rules))
            {
                return false;
            }

            return CutDemosByPattern(resources, ref parseArg, filePaths, patterns, options);
        }

        public static bool CutDemosByFlickRail(ref udtParseArg parseArg, List<string> filePaths, udtFlickRailPatternArg rules, CutByPatternOptions options)
        {
            var resources = new ArgumentResources();
            var patterns = new udtPatternInfo[1];
            if(!CreateFlickRailPatternInfo(ref patterns[0], resources, rules))
            {
                return false;
            }

            return CutDemosByPattern(resources, ref parseArg, filePaths, patterns, options);
        }

        public static bool CutDemosByMatch(ref udtParseArg parseArg, List<string> filePaths, udtMatchPatternArg rules, CutByPatternOptions options)
        {
            var resources = new ArgumentResources();
            var patterns = new udtPatternInfo[1];
            if(!CreateMatchPatternInfo(ref patterns[0], resources, rules))
            {
                return false;
            }

            return CutDemosByPattern(resources, ref parseArg, filePaths, patterns, options);
        }

        public static bool CutDemosByPattern(ArgumentResources resources, ref udtParseArg parseArg, List<string> filePaths, udtPatternInfo[] patterns, CutByPatternOptions options)
        {
            MultithreadedJob.JobExecuter jobExecuter = delegate(ArgumentResources res, ref udtParseArg pa, List<string> files, List<int> fileIndices)
            {
                CutDemosByPatternImpl(res, ref pa, files, patterns, options);
            };

            var result = App.Instance.CreateAndProcessJob(ref parseArg, filePaths, options.MaxThreadCount, MaxBatchSizeCutting, jobExecuter);
            resources.Free();

            return result;
        }

        private class SearchJobArguments
        {
            public udtMultiParseArg MultiParseArg;
            public udtPatternSearchArg PatternSearchArg;
        }

        private static SearchJobArguments CreateSearchJobArguments(ArgumentResources resources, ref udtParseArg parseArg, List<string> filePaths, udtPatternInfo[] patterns, CutByPatternOptions options)
        {
            parseArg.PlugInCount = 0;
            parseArg.PlugIns = IntPtr.Zero;

            var multiParseArg = CreateMultiParseArg(resources, filePaths);

            var playerNameUnmanaged = IntPtr.Zero;
            var pinnedPatterns = new PinnedObject(patterns);
            resources.PinnedObjects.Add(pinnedPatterns);
            var patternSearchArg = new udtPatternSearchArg();
            patternSearchArg.StartOffsetSec = (UInt32)options.StartOffset;
            patternSearchArg.EndOffsetSec = (UInt32)options.EndOffset;
            patternSearchArg.Patterns = pinnedPatterns.Address;
            patternSearchArg.PatternCount = (UInt32)patterns.Length;
            patternSearchArg.PlayerIndex = options.PlayerIndex;
            patternSearchArg.Flags = 0;
            patternSearchArg.PlayerNameRules = IntPtr.Zero;
            patternSearchArg.PlayerNameRuleCount = 0;
            if(options.MergeCutSections)
            {
                patternSearchArg.Flags |= (UInt32)udtCutByPatternArgFlags.MergeCutSections;
            }
            if(!string.IsNullOrEmpty(options.PlayerName))
            {
                playerNameUnmanaged = StringToHGlobalUTF8(options.PlayerName);
                resources.GlobalAllocationHandles.Add(playerNameUnmanaged);

                var rule = new udtStringMatchingRule();
                rule.ComparisonMode = (UInt32)udtStringComparisonMode.Equals;
                rule.Flags = (UInt32)udtStringMatchingRuleFlags.IgnoreColorCodes;
                rule.Value = playerNameUnmanaged;

                var rules = new udtStringMatchingRule[] { rule };
                var pinnedRules = new PinnedObject(rules);
                resources.PinnedObjects.Add(pinnedRules);

                patternSearchArg.PlayerNameRules = pinnedRules.Address;
                patternSearchArg.PlayerNameRuleCount = 1;
            }

            var data = new SearchJobArguments();
            data.MultiParseArg = multiParseArg;
            data.PatternSearchArg = patternSearchArg;

            return data;
        }

        private static bool CutDemosByPatternImpl(ArgumentResources resources, ref udtParseArg parseArg, List<string> filePaths, udtPatternInfo[] patterns, CutByPatternOptions options)
        {
            var data = CreateSearchJobArguments(resources, ref parseArg, filePaths, patterns, options);
            var result = udtErrorCode.OperationFailed;
            try
            {
                result = udtCutDemoFilesByPattern(ref parseArg, ref data.MultiParseArg, ref data.PatternSearchArg);
            }
            finally
            {
            }

            return result != udtErrorCode.None;
        }

        public static List<udtPatternMatch> FindPatternsInDemos(ArgumentResources resources, ref udtParseArg parseArg, List<string> filePaths, udtPatternInfo[] patterns, CutByPatternOptions options)
        {
            var results = new List<udtPatternMatch>();
            MultithreadedJob.JobExecuter jobExecuter = delegate(ArgumentResources res, ref udtParseArg pa, List<string> files, List<int> fileIndices)
            {
                var localResults = FindPatternsInDemosImpl(res, ref pa, files, patterns, options);
                if(localResults != null)
                {
                    // Add results with fixed up indices.
                    foreach(var localResult in localResults)
                    {
                        var result = localResult;
                        result.DemoInputIndex = (uint)fileIndices[(int)result.DemoInputIndex];
                        results.Add(result);
                    }
                }
            };

            App.Instance.CreateAndProcessJob(ref parseArg, filePaths, options.MaxThreadCount, MaxBatchSizeCutting, jobExecuter);
            resources.Free();

            return results;
        }

        private static udtPatternMatch[] FindPatternsInDemosImpl(ArgumentResources resources, ref udtParseArg parseArg, List<string> filePaths, udtPatternInfo[] patterns, CutByPatternOptions options)
        {
            var data = CreateSearchJobArguments(resources, ref parseArg, filePaths, patterns, options);
            udtPatternSearchContextRef context = IntPtr.Zero;
            try
            {
                var errorCode = udtFindPatternsInDemoFiles(ref context, ref parseArg, ref data.MultiParseArg, ref data.PatternSearchArg);
                if(errorCode != udtErrorCode.None)
                {
                    return null;
                }

                var results = new udtPatternSearchResults();
                errorCode = udtGetSearchResults(context, ref results);
                if(errorCode != udtErrorCode.None)
                {
                    return null;
                }

                return MarshalHelper.PtrToStructureArray<udtPatternMatch>(results.Matches, (int)results.MatchCount);
            }
            finally
            {
                if(context != IntPtr.Zero)
                {
                    udtDestroySearchContext(context);
                }
            }
        }

        public static bool ConvertDemos(ref udtParseArg parseArg, udtProtocol outProtocol, List<string> filePaths, int maxThreadCount)
        {
            MultithreadedJob.JobExecuter jobExecuter = delegate(ArgumentResources res, ref udtParseArg pa, List<string> files, List<int> fileIndices)
            {
                ConvertDemosImpl(res, ref pa, outProtocol, files);
            };

            return App.Instance.CreateAndProcessJob(ref parseArg, filePaths, maxThreadCount, MaxBatchSizeConverting, jobExecuter);
        }

        private static bool ConvertDemosImpl(ArgumentResources resources, ref udtParseArg parseArg, udtProtocol outProtocol, List<string> filePaths)
        {
            var multiParseArg = CreateMultiParseArg(resources, filePaths);
            var conversionArg = new udtProtocolConversionArg();
            conversionArg.OutputProtocol = (UInt32)outProtocol;

            var result = udtErrorCode.OperationFailed;
            try
            {
                result = udtConvertDemoFiles(ref parseArg, ref multiParseArg, ref conversionArg);
            }
            finally
            {
            }

            return result != udtErrorCode.None;
        }

        private static udtMultiParseArg CreateMultiParseArg(ArgumentResources resources, List<string> filePaths)
        {
            var errorCodeArray = new Int32[filePaths.Count];
            var filePathArray = new IntPtr[filePaths.Count];
            for(var i = 0; i < filePaths.Count; ++i)
            {
                var filePath = StringToHGlobalUTF8(Path.GetFullPath(filePaths[i]));
                filePathArray[i] = filePath;
                resources.GlobalAllocationHandles.Add(filePath);
            }

            var pinnedFilePaths = new PinnedObject(filePathArray);
            var pinnedErrorCodes = new PinnedObject(errorCodeArray);
            resources.PinnedObjects.Add(pinnedFilePaths);
            resources.PinnedObjects.Add(pinnedErrorCodes);

            var multiParseArg = new udtMultiParseArg();
            multiParseArg.FileCount = (UInt32)filePathArray.Length;
            multiParseArg.FilePaths = pinnedFilePaths.Address;
            multiParseArg.OutputErrorCodes = pinnedErrorCodes.Address;
            multiParseArg.MaxThreadCount = 1;

            return multiParseArg;
        }

        private static void RegisterPlugIns(ArgumentResources resources, ref udtParseArg parseArg, UInt32[] plugIns)
        {
            var pinnedPlugIns = new PinnedObject(plugIns);
            parseArg.PlugInCount = (UInt32)plugIns.Length;
            parseArg.PlugIns = pinnedPlugIns.Address;
            resources.PinnedObjects.Add(pinnedPlugIns);
        }

        public static bool TimeShiftDemos(ref udtParseArg parseArg, List<string> filePaths, int maxThreadCount, int snapshotCount)
        {
            MultithreadedJob.JobExecuter jobExecuter = delegate(ArgumentResources res, ref udtParseArg pa, List<string> files, List<int> fileIndices)
            {
                TimeShiftDemosImpl(res, ref pa, files, snapshotCount);
            };

            return App.Instance.CreateAndProcessJob(ref parseArg, filePaths, maxThreadCount, MaxBatchSizeTimeShifting, jobExecuter);
        }

        private static bool TimeShiftDemosImpl(ArgumentResources resources, ref udtParseArg parseArg, List<string> filePaths, int snapshotCount)
        {
            var multiParseArg = CreateMultiParseArg(resources, filePaths);
            var timeShiftArg = new udtTimeShiftArg();
            timeShiftArg.SnapshotCount = snapshotCount;

            var result = udtErrorCode.OperationFailed;
            try
            {
                result = udtTimeShiftDemoFiles(ref parseArg, ref multiParseArg, ref timeShiftArg);
            }
            finally
            {
            }

            return result != udtErrorCode.None;
        }

        public static bool ExportDemosDataToJSON(ref udtParseArg parseArg, List<string> filePaths, int maxThreadCount, UInt32[] plugIns)
        {
            MultithreadedJob.JobExecuter jobExecuter = delegate(ArgumentResources res, ref udtParseArg pa, List<string> files, List<int> fileIndices)
            {
                ExportDemosDataToJSONImpl(res, ref pa, files, plugIns);
            };

            return App.Instance.CreateAndProcessJob(ref parseArg, filePaths, maxThreadCount, MaxBatchSizeJSONExport, jobExecuter);
        }

        private static bool ExportDemosDataToJSONImpl(ArgumentResources resources, ref udtParseArg parseArg, List<string> filePaths, UInt32[] plugIns)
        {
            RegisterPlugIns(resources, ref parseArg, plugIns);
            var multiParseArg = CreateMultiParseArg(resources, filePaths);
            var jsonArg = new udtJSONArg();
            jsonArg.ConsoleOutput = 0;

            var result = udtErrorCode.OperationFailed;
            try
            {
                result = udtSaveDemoFilesAnalysisDataToJSON(ref parseArg, ref multiParseArg, ref jsonArg);
            }
            finally
            {
            }

            return result != udtErrorCode.None;
        }

        private delegate void VoidDelegate();
        
        public static List<DemoInfo> ParseDemos(ref udtParseArg parseArg, List<string> filePaths, int maxThreadCount)
        {
            var demos = new List<DemoInfo>();
            MultithreadedJob.JobExecuter jobExecuter = delegate(ArgumentResources res, ref udtParseArg pa, List<string> files, List<int> fileIndices)
            {
                var currentResults = ParseDemosImpl(res, ref pa, files, fileIndices);

                VoidDelegate demoAdder = delegate
                {
                    demos.AddRange(currentResults);
                };
                App.Instance.MainWindow.Dispatcher.Invoke(demoAdder);
            };

            App.Instance.CreateAndProcessJob(ref parseArg, filePaths, maxThreadCount, MaxBatchSizeParsing, jobExecuter);

            return demos;
        }
        
        private class BuffersBase
        {
            public string GetString(uint offset, uint length)
            {
                if(offset == uint.MaxValue)
                {
                    return "";
                }

                return Encoding.UTF8.GetString(StringBuffer, (int)offset, (int)length) ?? "";
            }

            public string GetString(uint offset, uint length, string replacement)
            {
                if(offset == uint.MaxValue)
                {
                    return replacement;
                }

                return Encoding.UTF8.GetString(StringBuffer, (int)offset, (int)length) ?? replacement;
            }

            public byte[] StringBuffer;
        }

        private class RawCommandBuffers : BuffersBase
        {
            public RawCommandBuffers(udtParserContextRef context, uint demoCount)
            {                
                var buffers = new udtParseDataRawCommandBuffers();
                if(udtGetContextPlugInRawCommandBuffers(context, udtParserPlugIn.RawCommands, ref buffers) != udtErrorCode.None)
                {
                    return;
                }
                
                Commands = MarshalHelper.PtrToStructureArray<udtParseDataRawCommand>(buffers.Commands, (int)buffers.CommandCount);
                CommandRanges = MarshalHelper.PtrToStructureArray<udtParseDataBufferRange>(buffers.CommandRanges, (int)demoCount);
                StringBuffer = MarshalHelper.PtrToByteArray(buffers.StringBuffer, (int)buffers.StringBufferSize);
            }

            public udtParseDataRawCommand[] Commands;
            public udtParseDataBufferRange[] CommandRanges;
        }

        private class RawConfigStringBuffers : BuffersBase
        {
            public RawConfigStringBuffers(udtParserContextRef context, uint demoCount)
            {
                var buffers = new udtParseDataRawConfigStringBuffers();
                if(udtGetContextPlugInRawConfigStringBuffers(context, udtParserPlugIn.RawConfigStrings, ref buffers) != udtErrorCode.None)
                {
                    return;
                }

                ConfigStrings = MarshalHelper.PtrToStructureArray<udtParseDataRawConfigString>(buffers.ConfigStrings, (int)buffers.ConfigStringCount);
                ConfigStringRanges = MarshalHelper.PtrToStructureArray<udtParseDataBufferRange>(buffers.ConfigStringRanges, (int)demoCount);
                StringBuffer = MarshalHelper.PtrToByteArray(buffers.StringBuffer, (int)buffers.StringBufferSize);
            }

            public udtParseDataRawConfigString[] ConfigStrings;
            public udtParseDataBufferRange[] ConfigStringRanges;
        }

        private class ChatBuffers : BuffersBase
        {
            public ChatBuffers(udtParserContextRef context, uint demoCount)
            {
                var buffers = new udtParseDataChatBuffers();
                if(udtGetContextPlugInChatBuffers(context, udtParserPlugIn.Chat, ref buffers) != udtErrorCode.None)
                {
                    return;
                }

                ChatMessages = MarshalHelper.PtrToStructureArray<udtParseDataChat>(buffers.ChatMessages, (int)buffers.ChatMessageCount);
                ChatMessageRanges = MarshalHelper.PtrToStructureArray<udtParseDataBufferRange>(buffers.ChatMessageRanges, (int)demoCount);
                StringBuffer = MarshalHelper.PtrToByteArray(buffers.StringBuffer, (int)buffers.StringBufferSize);
            }

            public udtParseDataChat[] ChatMessages;
            public udtParseDataBufferRange[] ChatMessageRanges;
        }

        private class DeathBuffers : BuffersBase
        {
            public DeathBuffers(udtParserContextRef context, uint demoCount)
            {
                var buffers = new udtParseDataObituaryBuffers();
                if(udtGetContextPlugInDeathBuffers(context, udtParserPlugIn.Obituaries, ref buffers) != udtErrorCode.None)
                {
                    return;
                }

                Deaths = MarshalHelper.PtrToStructureArray<udtParseDataObituary>(buffers.Obituaries, (int)buffers.ObituaryCount);
                DeathRanges = MarshalHelper.PtrToStructureArray<udtParseDataBufferRange>(buffers.ObituaryRanges, (int)demoCount);
                StringBuffer = MarshalHelper.PtrToByteArray(buffers.StringBuffer, (int)buffers.StringBufferSize);
            }

            public udtParseDataObituary[] Deaths;
            public udtParseDataBufferRange[] DeathRanges;
        }

        private class CaptureBuffers : BuffersBase
        {
            public CaptureBuffers(udtParserContextRef context, uint demoCount)
            {
                var buffers = new udtParseDataCaptureBuffers();
                if(udtGetContextPlugInCaptureBuffers(context, udtParserPlugIn.Captures, ref buffers) != udtErrorCode.None)
                {
                    return;
                }

                Captures = MarshalHelper.PtrToStructureArray<udtParseDataCapture>(buffers.Captures, (int)buffers.CaptureCount);
                CaptureRanges = MarshalHelper.PtrToStructureArray<udtParseDataBufferRange>(buffers.CaptureRanges, (int)demoCount);
                StringBuffer = MarshalHelper.PtrToByteArray(buffers.StringBuffer, (int)buffers.StringBufferSize);
            }

            public udtParseDataCapture[] Captures;
            public udtParseDataBufferRange[] CaptureRanges;
        }

        private class ScoreBuffers : BuffersBase
        {
            public ScoreBuffers(udtParserContextRef context, uint demoCount)
            {
                var buffers = new udtParseDataScoreBuffers();
                if(udtGetContextPlugInScoreBuffers(context, udtParserPlugIn.Scores, ref buffers) != udtErrorCode.None)
                {
                    return;
                }

                Scores = MarshalHelper.PtrToStructureArray<udtParseDataScore>(buffers.Scores, (int)buffers.ScoreCount);
                ScoreRanges = MarshalHelper.PtrToStructureArray<udtParseDataBufferRange>(buffers.ScoreRanges, (int)demoCount);
                StringBuffer = MarshalHelper.PtrToByteArray(buffers.StringBuffer, (int)buffers.StringBufferSize);
            }

            public udtParseDataScore[] Scores;
            public udtParseDataBufferRange[] ScoreRanges;
        }

        private class GameStateBuffers : BuffersBase
        {
            public GameStateBuffers(udtParserContextRef context, uint demoCount)
            {
                var buffers = new udtParseDataGameStateBuffers();
                if(udtGetContextPlugInGameStateBuffers(context, udtParserPlugIn.GameState, ref buffers) != udtErrorCode.None)
                {
                    return;
                }

                GameStates = MarshalHelper.PtrToStructureArray<udtParseDataGameState>(buffers.GameStates, (int)buffers.GameStateCount);
                GameStateRanges = MarshalHelper.PtrToStructureArray<udtParseDataBufferRange>(buffers.GameStateRanges, (int)demoCount);
                StringBuffer = MarshalHelper.PtrToByteArray(buffers.StringBuffer, (int)buffers.StringBufferSize);
                KeyValuePairs = MarshalHelper.PtrToStructureArray<udtGameStateKeyValuePair>(buffers.KeyValuePairs, (int)buffers.KeyValuePairCount);
                Matches = MarshalHelper.PtrToStructureArray<udtMatchInfo>(buffers.Matches, (int)buffers.MatchCount);
                Players = MarshalHelper.PtrToStructureArray<udtGameStatePlayerInfo>(buffers.Players, (int)buffers.PlayerCount);
            }

            public udtParseDataGameState[] GameStates;
            public udtParseDataBufferRange[] GameStateRanges;
            public udtGameStateKeyValuePair[] KeyValuePairs;
            public udtMatchInfo[] Matches;
            public udtGameStatePlayerInfo[] Players;
        }

        private class StatsBuffers : BuffersBase
        {
            public StatsBuffers(udtParserContextRef context, uint demoCount)
            {
                var buffers = new udtParseDataStatsBuffers();
                if(udtGetContextPlugInStatsBuffers(context, udtParserPlugIn.Stats, ref buffers) != udtErrorCode.None)
                {
                    return;
                }

                MatchStats = MarshalHelper.PtrToStructureArray<udtParseDataStats>(buffers.MatchStats, (int)buffers.MatchCount);
                MatchStatsRanges = MarshalHelper.PtrToStructureArray<udtParseDataBufferRange>(buffers.MatchStatsRanges, (int)demoCount);
                StringBuffer = MarshalHelper.PtrToByteArray(buffers.StringBuffer, (int)buffers.StringBufferSize);
                TimeOutStartAndEndTimes = MarshalHelper.PtrToIntArray(buffers.TimeOutStartAndEndTimes, (int)buffers.TimeOutRangeCount * 2);
                TeamFlags = MarshalHelper.PtrToByteArray(buffers.TeamFlags, (int)buffers.TeamFlagCount);
                PlayerFlags = MarshalHelper.PtrToByteArray(buffers.PlayerFlags, (int)buffers.PlayerFlagCount);
                TeamFields = MarshalHelper.PtrToIntArray(buffers.TeamFields, (int)buffers.TeamFieldCount);
                PlayerFields = MarshalHelper.PtrToIntArray(buffers.PlayerFields, (int)buffers.PlayerFieldCount);
                PlayerStats = MarshalHelper.PtrToStructureArray<udtPlayerStats>(buffers.PlayerStats, (int)buffers.PlayerStatsCount);
            }

            public udtParseDataStats[] MatchStats;
            public udtParseDataBufferRange[] MatchStatsRanges;
            public Int32[] TimeOutStartAndEndTimes;
            public byte[] TeamFlags;
            public byte[] PlayerFlags;
            public Int32[] TeamFields;
            public Int32[] PlayerFields;
            public udtPlayerStats[] PlayerStats;
        }
        
        private static List<DemoInfo> ParseDemosImpl(ArgumentResources resources, ref udtParseArg parseArg, List<string> filePaths, List<int> fileIndices)
        {
            RegisterPlugIns(resources, ref parseArg, PlugInArray);
            var multiParseArg = CreateMultiParseArg(resources, filePaths);

            udtParserContextGroupRef contextGroup = IntPtr.Zero;
            var result = udtErrorCode.OperationFailed;
            try
            {
                result = udtParseDemoFiles(ref contextGroup, ref parseArg, ref multiParseArg);
            }
            finally
            {
            }

            if(result != udtErrorCode.None && result != udtErrorCode.OperationCanceled)
            {
                udtDestroyContextGroup(contextGroup);
                App.GlobalLogError("Failed to parse demos: " + GetErrorCodeString(result));
                return null;
            }

            uint contextCount = 0;
            if(udtGetContextCountFromGroup(contextGroup, ref contextCount) != udtErrorCode.None ||
                contextCount != 1)
            {
                udtDestroyContextGroup(contextGroup);
                return null;
            }

            uint totalDemoCount = 0;
            if(udtGetDemoCountFromGroup(contextGroup, ref totalDemoCount) != udtErrorCode.None ||
                totalDemoCount == 0)
            {
                udtDestroyContextGroup(contextGroup);
                return null;
            }

            udtParserContextRef context = IntPtr.Zero;
            if(udtGetContextFromGroup(contextGroup, 0, ref context) != udtErrorCode.None)
            {
                udtDestroyContextGroup(contextGroup);
                return null;
            }

            uint demoCount = 0;
            if(udtGetDemoCountFromContext(context, ref demoCount) != udtErrorCode.None ||
                demoCount != totalDemoCount)
            {
                udtDestroyContextGroup(contextGroup);
                return null;
            }

            var rawCommandBuffers = new RawCommandBuffers(context, demoCount);
            var rawConfigStringBuffers = new RawConfigStringBuffers(context, demoCount);
            var chatBuffers = new ChatBuffers(context, demoCount);
            var deathBuffers = new DeathBuffers(context, demoCount);
            var captureBuffers = new CaptureBuffers(context, demoCount);
            var gameStateBuffers = new GameStateBuffers(context, demoCount);
            var statsBuffers = new StatsBuffers(context, demoCount);
            var scoreBuffers = new ScoreBuffers(context, demoCount);

            var demoList = new List<DemoInfo>();
            demoList.Capacity = (int)demoCount;
            for(uint i = 0; i < demoCount; ++i)
            {
                Int32 errorCode = Marshal.ReadInt32(multiParseArg.OutputErrorCodes, 4 * (int)i);
                if(errorCode == (Int32)udtErrorCode.Unprocessed || 
                    errorCode == (Int32)udtErrorCode.OperationCanceled)
                {
                    continue;
                }

                if(errorCode != (Int32)udtErrorCode.None)
                {
                    var fileName = Path.GetFileName(filePaths[(int)i]);
                    var errorCodeString = GetErrorCodeString((udtErrorCode)errorCode);
                    App.GlobalLogError("Failed to parse demo file {0}: {1}", fileName, errorCodeString);
                    continue;
                }

                var filePath = filePaths[(int)i];
                var protocol = GetProtocolFromFilePath(filePath);
                var info = new DemoInfo();
                info.Analyzed = true;
                info.InputIndex = fileIndices[(int)i];
                info.FilePath = Path.GetFullPath(filePath);
                info.Protocol = UDT_DLL.GetProtocolAsString(protocol);
                info.ProtocolNumber = protocol;
                demoList.Add(info);

                ExtractCommands(context, i, info, rawCommandBuffers, rawConfigStringBuffers);
                ExtractChat(context, i, info, chatBuffers);
                ExtractDeaths(context, i, info, deathBuffers);
                ExtractCaptures(context, i, info, captureBuffers);
                ExtractGameStates(context, i, info, gameStateBuffers);
                ExtractStats(context, i, info, statsBuffers);
                ExtractScores(context, i, info, scoreBuffers);
            }

            udtDestroyContextGroup(contextGroup);

            return demoList;
        }
        
        private static void ExtractCommands(udtParserContextRef context, uint demoIdx, DemoInfo info, RawCommandBuffers cmdBuffers, RawConfigStringBuffers csBuffers)
        {
            var range = cmdBuffers.CommandRanges[demoIdx];
            var start = range.FirstIndex;
            var count = range.Count;
            var commandList = new List<CommandDisplayInfo>();
            for(uint i = start, end = start + count; i < end; ++i)
            {
                var data = cmdBuffers.Commands[i];
                var gs = data.GameStateIndex.ToString();
                var time = FormatMinutesSecondsFromMs(data.ServerTimeMs);
                var rawCmd = cmdBuffers.GetString(data.RawCommand, data.RawCommandLength).Replace("\n", "\\n");
                var cmd = "";
                var val = "";
                if(rawCmd.StartsWith("cs "))
                {
                    var firstQuote = rawCmd.IndexOf('"');
                    var lastQuote = rawCmd.LastIndexOf('"');
                    if(firstQuote > 0 && lastQuote > firstQuote)
                    {
                        cmd = rawCmd.Substring(0, firstQuote - 1);
                        val = rawCmd.Substring(firstQuote + 1, lastQuote - firstQuote - 1);
                    }
                    else
                    {
                        cmd = "cs";
                        val = rawCmd.Substring(3);
                    }
                }
                else
                {
                    var firstSpace = rawCmd.IndexOf(' ');
                    if(firstSpace < 0)
                    {
                        cmd = rawCmd;
                    }
                    else
                    {
                        cmd = rawCmd.Substring(0, firstSpace);
                        val = rawCmd.Substring(firstSpace + 1);
                    }
                }

                commandList.Add(new CommandDisplayInfo(data.GameStateIndex, gs, time, cmd, val));
            }

            range = csBuffers.ConfigStringRanges[demoIdx];
            start = range.FirstIndex;
            count = range.Count;
            var configStringList = new List<CommandDisplayInfo>();
            for(uint i = start, end = start + count; i < end; ++i)
            {
                var data = csBuffers.ConfigStrings[i];
                var gs = data.GameStateIndex.ToString();
                var cmd = "cs " + data.ConfigStringIndex.ToString();
                var val = csBuffers.GetString(data.RawConfigString, data.RawConfigStringLength).Replace("\n", "\\n");
                configStringList.Add(new CommandDisplayInfo(data.GameStateIndex, gs, "", cmd, val));
            }

            var lastGameStateIndex = configStringList.Count == 0 ? 0 : configStringList[configStringList.Count - 1].GameStateIndex;
            var gameStateCount = lastGameStateIndex + 1;
            for(var i = 0; i < gameStateCount; ++i)
            {
                info.Commands.AddRange(configStringList.FindAll(cs => cs.GameStateIndex == i));
                info.Commands.AddRange(commandList.FindAll(cmd => cmd.GameStateIndex == i));
            }
        }

        private static void ExtractChat(udtParserContextRef context, uint demoIdx, DemoInfo info, ChatBuffers buffers)
        {
            var range = buffers.ChatMessageRanges[demoIdx];
            var start = range.FirstIndex;
            var count = range.Count;
            for(uint i = start, end = start + count; i < end; ++i)
            {
                var data = buffers.ChatMessages[i];
                int totalSeconds = data.ServerTimeMs / 1000;
                int minutes = totalSeconds / 60;
                int seconds = totalSeconds % 60;
                var time = string.Format("{0}:{1}", minutes, seconds.ToString("00"));
                var player = buffers.GetString(data.PlayerNameNoCol, data.PlayerNameNoColLength, "N/A");
                var message = buffers.GetString(data.MessageNoCol, data.MessageNoColLength, "N/A");
                var item = new ChatEventDisplayInfo(data.GameStateIndex, time, player, message, data.TeamMessage != 0);
                info.ChatEvents.Add(item);
            }
        }

        private static void ExtractDeaths(udtParserContextRef context, uint demoIdx, DemoInfo info, DeathBuffers buffers)
        {
            var range = buffers.DeathRanges[demoIdx];
            var start = range.FirstIndex;
            var count = range.Count;
            for(uint i = start, end = start + count; i < end; ++i)
            {
                var data = buffers.Deaths[i];
                int totalSeconds = data.ServerTimeMs / 1000;
                int minutes = totalSeconds / 60;
                int seconds = totalSeconds % 60;
                var time = string.Format("{0}:{1}", minutes, seconds.ToString("00"));
                var attacker = buffers.GetString(data.AttackerName, data.AttackerNameLength, "N/A");
                var target = buffers.GetString(data.TargetName, data.TargetNameLength, "N/A");
                var mod = buffers.GetString(data.MeanOfDeathName, data.MeanOfDeathNameLength, "N/A");
                var item = new FragEventDisplayInfo(data.GameStateIndex, time, attacker, target, mod);
                info.FragEvents.Add(item);
            }
        }

        private static void ExtractCaptures(udtParserContextRef context, uint demoIdx, DemoInfo info, CaptureBuffers buffers)
        {
            var range = buffers.CaptureRanges[demoIdx];
            var start = range.FirstIndex;
            var count = range.Count;
            for(uint i = start, end = start + count; i < end; ++i)
            {
                var data = buffers.Captures[i];
                var playerNameValid = (data.Flags & (uint)udtParseDataCaptureFlags.PlayerNameValid) != 0;
                var gs = data.GameStateIndex;
                var startTime = data.PickUpTimeMs;
                var endTime = data.CaptureTimeMs;
                var dur = endTime - startTime;
                var b2b = (data.Flags & (uint)udtParseDataCaptureFlags.BaseToBase) != 0;
                var player = playerNameValid ? buffers.GetString(data.PlayerName, data.PlayerNameLength, "N/A") : "N/A";
                var map = buffers.GetString(data.MapName, data.MapNameLength, "N/A");
                var item = new FlagCaptureDisplayInfo(gs, startTime, endTime, dur, b2b, player, map);
                info.FlagCaptures.Add(item);
            }
        }

        private static void ExtractGameStates(udtParserContextRef context, uint demoIdx, DemoInfo info, GameStateBuffers buffers)
        {
            const string space = "   ";
            var range = buffers.GameStateRanges[demoIdx];
            var start = range.FirstIndex;
            var count = range.Count;
            for(uint i = start, end = start + count; i < end; ++i)
            {
                var data = buffers.GameStates[i];
                info.GameStateFileOffsets.Add(data.FileOffset);
                var firstSnapTime = App.FormatMinutesSeconds(data.FirstSnapshotTimeMs / 1000);
                var lastSnapTime = App.FormatMinutesSeconds(data.LastSnapshotTimeMs / 1000);
                info.GameStateSnapshotTimesMs.Add(Tuple.Create(data.FirstSnapshotTimeMs, data.LastSnapshotTimeMs));
                info.Generic.Add(Tuple.Create("GameState #" + (i + 1 - start).ToString(), ""));
                info.Generic.Add(Tuple.Create(space + "File Offset", FormatFileOffset(data.FileOffset)));
                info.Generic.Add(Tuple.Create(space + "Server Time Range", firstSnapTime + " - " + lastSnapTime));
                info.Generic.Add(Tuple.Create(space + "Demo Taker", data.DemoTakerName == uint.MaxValue ? "N/A" : FormatDemoTaker(buffers, data)));
                AddMatches(info, buffers, data, space);
                AddPlayers(info, buffers, data, space);
                AddKeyValuePairs(info, buffers, data, space);
            }
        }

        private static void ExtractScores(udtParserContextRef context, uint demoIdx, DemoInfo info, ScoreBuffers buffers)
        {
            var range = buffers.ScoreRanges[demoIdx];
            var start = range.FirstIndex;
            var count = range.Count;

            info.TeamGameType = false;
            if(count != 0 && (buffers.Scores[start].Flags & (uint)udtParseDataScoreMask.TeamBased) != 0)
            {
                info.TeamGameType = true;
            }

            if(info.TeamGameType)
            {
                for(uint i = start, end = start + count; i < end; ++i)
                {
                    var data = buffers.Scores[i];
                    var gs = data.GameStateIndex;
                    var time = App.FormatMinutesSeconds(data.ServerTimeMs / 1000);
                    var s1 = data.Score1;
                    var s2 = data.Score2;
                    var item = new TeamScoreDisplayInfo(gs, time, s1, s2);
                    info.TeamScores.Add(item);
                }
            }
            else
            {
                for(uint i = start, end = start + count; i < end; ++i)
                {
                    var data = buffers.Scores[i];
                    var gs = data.GameStateIndex;
                    var time = App.FormatMinutesSeconds(data.ServerTimeMs / 1000);
                    var s1 = data.Score1;
                    var s2 = data.Score2;
                    var n1 = buffers.GetString(data.CleanName1, data.CleanName1Length, "");
                    var n2 = buffers.GetString(data.CleanName2, data.CleanName2Length, "");
                    var item = new ScoreDisplayInfo(gs, time, s1, s2, n1, n2);
                    info.Scores.Add(item);
                }
            }
        }

        private static void ExtractStats(udtParserContextRef context, uint demoIdx, DemoInfo info, StatsBuffers buffers)
        {
            var range = buffers.MatchStatsRanges[demoIdx];
            var start = range.FirstIndex;
            var count = range.Count;
            for(uint i = start, end = start + count; i < end; ++i)
            {
                var data = buffers.MatchStats[i];
                ExtractStatsSingleMatch(buffers, data, info);
            }
        }

        private static void ExtractStatsSingleMatch(StatsBuffers buffers, udtParseDataStats data, DemoInfo info)
        {
            var stats = new DemoStatsInfo();
            var name1 = buffers.GetString(data.FirstPlaceName, data.FirstPlaceNameLength);
            var name2 = buffers.GetString(data.SecondPlaceName, data.SecondPlaceNameLength);
            var finalScore = string.Format("{0} {1} : {2} {3}", name1, data.FirstPlaceScore, data.SecondPlaceScore, name2);
            if(data.Forfeited != 0)
            {
                stats.AddGenericField("Score before forfeit", finalScore);
                stats.AddGenericField("Victor", data.SecondPlaceWon != 0 ? name2 : name1);
            }
            else
            {
                stats.AddGenericField("Final score", finalScore);
            }

            if(data.StartDateEpoch != 0)
            {
                var date = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);
                date = date.AddSeconds(data.StartDateEpoch);

                var dateString = new StringBuilder();
                dateString.Append(date.Year.ToString("0000"));
                dateString.Append(".");
                dateString.Append(date.Month.ToString("00"));
                dateString.Append(".");
                dateString.Append(date.Day.ToString("00"));
                dateString.Append(" ");
                dateString.Append(date.Hour.ToString("00"));
                dateString.Append(":");
                dateString.Append(date.Minute.ToString("00"));
                dateString.Append(":");
                dateString.Append(date.Second.ToString("00"));
                dateString.Append(" UTC");

                stats.AddGenericField("Start date", dateString.ToString());
            }

            var redTeamName = data.CustomRedName != uint.MaxValue ? buffers.GetString(data.CustomRedName, data.CustomRedNameLength) : null;
            var blueTeamName = data.CustomBlueName != uint.MaxValue ? buffers.GetString(data.CustomBlueName, data.CustomBlueNameLength) : null;
            if(redTeamName != null)
            {
                stats.AddGenericField("Red team name", redTeamName);
            }
            if(blueTeamName != null)
            {
                stats.AddGenericField("Blue team name", blueTeamName);
            }

            stats.AddGenericField("Mod", GetUDTStringForValueOrNull(udtStringArray.ModNames, data.Mod));
            if(data.Mod != 0)
            {
                stats.AddGenericField("Mod version", buffers.GetString(data.ModVersion, data.ModVersionLength));
            }
            stats.AddGenericField("Game type", GetUDTStringForValueOrNull(udtStringArray.GameTypes, data.GameType));
            stats.AddGenericField("Game play", GetUDTStringForValueOrNull(udtStringArray.GamePlayNames, data.GamePlay));
            stats.AddGenericField("Map name", buffers.GetString(data.MapName, data.MapNameLength));

            if(data.TimeLimit != 0)
            {
                stats.AddGenericField("Time limit", data.TimeLimit.ToString());
            }
            if(data.ScoreLimit != 0)
            {
                stats.AddGenericField("Score limit", data.ScoreLimit.ToString());
            }
            if(data.FragLimit != 0)
            {
                stats.AddGenericField("Frag limit", data.FragLimit.ToString());
            }
            if(data.CaptureLimit != 0)
            {
                stats.AddGenericField("Capture limit", data.CaptureLimit.ToString());
            }
            if(data.RoundLimit != 0)
            {
                stats.AddGenericField("Round limit", data.RoundLimit.ToString());
            }

            stats.AddGenericField("Match duration", App.FormatMinutesSeconds((int)data.MatchDurationMs / 1000));
            if(info.ProtocolNumber >= udtProtocol.Dm73 && data.GameType == (uint)udtGameType.TDM)
            {
                stats.AddGenericField("Mercy limit hit?", data.MercyLimited != 0 ? "yes" : "no");
            }
            stats.AddGenericField("Forfeited?", data.Forfeited != 0 ? "yes" : "no");
            stats.AddGenericField("Overtime count", data.OverTimeCount.ToString());
            if(data.OverTimeCount > 0)
            {
                stats.AddGenericField("Overtime type", GetUDTStringForValueOrNull(udtStringArray.OverTimeTypes, data.OverTimeType));
            }
            stats.AddGenericField("Time-out count", data.TimeOutCount.ToString());
            if(data.TimeOutCount > 0)
            {
                stats.AddGenericField("Total time-out duration", App.FormatMinutesSeconds((int)data.TotalTimeOutDurationMs / 1000));
            }

            ExtractTimeInfo(stats, buffers, data, info);
            
            stats.AddGenericField("Game state index", data.GameStateIndex.ToString());
            if(data.CountDownStartTimeMs < data.StartTimeMs)
            {
                stats.AddGenericField("Count-down start", App.FormatMinutesSeconds(data.CountDownStartTimeMs / 1000));
            }
            var startString = App.FormatMinutesSeconds(data.StartTimeMs / 1000);
            var endString = App.FormatMinutesSeconds(data.EndTimeMs / 1000);
            stats.AddGenericField("Match time range", startString + " - " + endString);
            if(data.IntermissionEndTimeMs > data.EndTimeMs)
            {
                stats.AddGenericField("Post-match intermission end", App.FormatMinutesSeconds(data.IntermissionEndTimeMs / 1000));
            }

            ExtractTeamStats(buffers, data, info, stats);
            ExtractPlayerStats(buffers, data, info, ref stats);

            info.MatchStats.Add(stats);
        }

        private static void ExtractPlayerStats(StatsBuffers buffers, udtParseDataStats data, DemoInfo info, ref DemoStatsInfo stats)
        {
            var playerInfoIndex = (int)data.FirstPlayerStatsIndex;
            var fieldIdx = (int)data.FirstPlayerFieldIndex;
            var flagsByteOffset = (int)data.FirstPlayerFlagIndex;
            for(int i = 0; i < 64; ++i)
            {
                if((data.ValidPlayers & ((ulong)1 << i)) == 0)
                {
                    continue;
                }

                var playerStats = new StatsInfoGroup();
                var extraInfo = buffers.PlayerStats[playerInfoIndex];
                playerStats.Name = buffers.GetString(extraInfo.CleanName, extraInfo.CleanNameLength);
                for(int j = 0; j < StatsConstants.PlayerFieldCount; ++j)
                {
                    var byteIndex = j / 8;
                    var bitIndex = j % 8;
                    var byteValue = buffers.PlayerFlags[byteIndex + flagsByteOffset];
                    if((byteValue & (byte)(1 << bitIndex)) != 0)
                    {
                        var dataType = LibraryArrays.GetPlayerFieldDataType(j);
                        var field = new DemoStatsField();
                        var fieldName = "";
                        var fieldValue = "";
                        var fieldIntegerValue = buffers.PlayerFields[fieldIdx];
                        FormatStatsField(out fieldName, out fieldValue, fieldIntegerValue, dataType, LibraryArrays.GetPlayerFieldName(j));
                        field.Key = fieldName;
                        field.Value = fieldValue;
                        field.IntegerValue = fieldIntegerValue;
                        field.FieldBitIndex = j;
                        field.ComparisonMode = LibraryArrays.GetPlayerFieldCompMode(j);
                        playerStats.Fields.Add(field);
                        ++fieldIdx;
                    }
                }

                var teamIndexField = playerStats.Fields.Find(f => f.FieldBitIndex == 0);
                var teamIndex = teamIndexField != null ? teamIndexField.IntegerValue : -1;
                playerStats.TeamIndex = teamIndex;

                // Get rid of spectators and players without a team.
                if(teamIndex >= 0 && teamIndex < 3)
                {
                    stats.PlayerStats.Add(playerStats);
                }

                ++playerInfoIndex;
                flagsByteOffset += StatsConstants.PlayerMaskByteCount;
            }

            var highestFieldCount = 0;
            var highestFieldCountIndex = -1;
            var highestFieldCountTeam = -1;
            for(var i = 0; i < stats.PlayerStats.Count; ++i)
            {
                var fieldCount = stats.PlayerStats[i].Fields.Count;
                if(fieldCount > highestFieldCount)
                {
                    var teamIndexField = stats.PlayerStats[i].Fields.Find(f => f.FieldBitIndex == 0);
                    highestFieldCount = fieldCount;
                    highestFieldCountIndex = i;
                    highestFieldCountTeam = teamIndexField != null ? teamIndexField.IntegerValue : -1;
                }
                else if(fieldCount == highestFieldCount)
                {
                    highestFieldCountIndex = -1;
                }
            }

            if(highestFieldCountIndex != -1 &&
                highestFieldCountTeam != -1 &&
                highestFieldCountIndex != 0)
            {
                var a = highestFieldCountIndex;
                var b = 0;
                var temp = stats.PlayerStats[a];
                stats.PlayerStats[a] = stats.PlayerStats[b];
                stats.PlayerStats[b] = temp;
            }

            // Sort the players by team index.
            // Red comes first unless a blue player has more stats than the others.
            if(highestFieldCountTeam == 2)
            {
                stats.PlayerStats.StableSort((a, b) => b.TeamIndex - a.TeamIndex);
            }
            else
            {
                stats.PlayerStats.StableSort((a, b) => a.TeamIndex - b.TeamIndex);
            }

            // Skip the team index field for non team game types.
            if(data.TeamMode == 0)
            {
                foreach(var player in stats.PlayerStats)
                {
                    var teamFieldIdx = player.Fields.FindIndex(f => f.FieldBitIndex == 0);
                    if(teamFieldIdx >= 0)
                    {
                        player.Fields.RemoveAt(teamFieldIdx);
                    }
                }
            }
        }

        private static void ExtractTeamStats(StatsBuffers buffers, udtParseDataStats data, DemoInfo info, DemoStatsInfo stats)
        {
            // For the GUI, we'll be picky and only bother if both teams have stats.
            if((data.ValidTeams & (ulong)3) != (ulong)3)
            {
                return;
            }

            var fieldIdx = (int)data.FirstTeamFieldIndex;
            var flagsByteOffset = (int)data.FirstTeamFlagIndex;
            for(int i = 0; i < 2; ++i)
            {
                var teamStats = new StatsInfoGroup();
                teamStats.Name = i == 0 ? "RED" : "BLUE";
                var customTeamNameOffset = i == 0 ? data.CustomRedName : data.CustomBlueName;
                var customTeamNameLength = i == 0 ? data.CustomRedNameLength : data.CustomBlueNameLength;
                var customTeamName = customTeamNameOffset != uint.MaxValue ? buffers.GetString(customTeamNameOffset, customTeamNameLength) : null;
                if(customTeamName != null)
                {
                    var customNameField = new DemoStatsField();
                    customNameField.Key = "Custom name";
                    customNameField.Value = customTeamName;
                    customNameField.FieldBitIndex = -1;
                    customNameField.IntegerValue = -1;
                    customNameField.ComparisonMode = udtStatsCompMode.NeitherWins;
                    teamStats.Fields.Add(customNameField);
                }

                for(int j = 0; j < StatsConstants.TeamFieldCount; ++j)
                {
                    var byteIndex = j / 8;
                    var bitIndex = j % 8;
                    var byteValue = buffers.TeamFlags[byteIndex + flagsByteOffset];
                    if((byteValue & (byte)(1 << bitIndex)) != 0)
                    {
                        var dataType = LibraryArrays.GetTeamFieldDataType(j);
                        var field = new DemoStatsField();
                        var fieldName = "";
                        var fieldValue = "";
                        var fieldIntegerValue = buffers.TeamFields[fieldIdx];
                        FormatStatsField(out fieldName, out fieldValue, fieldIntegerValue, dataType, LibraryArrays.GetTeamFieldName(j));
                        field.Key = fieldName;
                        field.Value = fieldValue;
                        field.IntegerValue = fieldIntegerValue;
                        field.FieldBitIndex = j;
                        field.ComparisonMode = LibraryArrays.GetTeamFieldCompMode(j);
                        teamStats.Fields.Add(field);
                        ++fieldIdx;
                    }
                }

                stats.TeamStats.Add(teamStats);

                flagsByteOffset += StatsConstants.TeamMaskByteCount;
            }
        }

        private static void ExtractTimeInfo(DemoStatsInfo stats, StatsBuffers buffers, udtParseDataStats data, DemoInfo info)
        {
            var match = new MatchTimeInfo();
            match.GameStateIndex = (int)data.GameStateIndex;
            match.StartTimeMs = data.StartTimeMs;
            match.EndTimeMs = data.EndTimeMs;
            match.TimeLimit = (int)data.TimeLimit;
            match.HadOvertime = data.OverTimeCount > 0;
            match.RoundBasedMode = LibraryArrays.IsModeRoundBased((int)data.GameType);

            var start = data.FirstTimeOutRangeIndex;
            var count = data.TimeOutCount;
            for(uint i = start, end = start + count; i < end; ++i)
            {
                var startTime = buffers.TimeOutStartAndEndTimes[2 * i];
                var endTime = buffers.TimeOutStartAndEndTimes[2 * i + 1];
                var startTimeString = App.FormatMinutesSeconds(startTime / 1000);
                var endTimeString = App.FormatMinutesSeconds(endTime / 1000);
                stats.AddGenericField("Time-out #" + (i + 1 - start).ToString(), startTimeString + " - " + endTimeString);
                match.TimeOuts.Add(new Timeout(startTime, endTime));
            }

            info.MatchTimes.Add(match);
        }

        private static void AddMatches(DemoInfo info, GameStateBuffers buffers, udtParseDataGameState data, string space)
        {
            var start = data.FirstMatchIndex;
            var count = data.MatchCount;
            info.Generic.Add(Tuple.Create(space + "Matches", data.MatchCount.ToString()));
            for(uint i = start, end = start + count; i < end; ++i)
            {
                var matchData = buffers.Matches[i];
                var desc = space + "Match #" + (i + 1 - start).ToString();
                var startTime = FormatMinutesSecondsFromMs(matchData.MatchStartTimeMs);
                var endTime = FormatMinutesSecondsFromMs(matchData.MatchEndTimeMs);
                var val = startTime + " - " + endTime;
                info.Generic.Add(Tuple.Create(desc, val));
            }
        }

        private static void AddPlayers(DemoInfo info, GameStateBuffers buffers, udtParseDataGameState data, string space)
        {
            var start = data.FirstPlayerIndex;
            var count = data.PlayerCount;
            for(uint i = start, end = start + count; i < end; ++i)
            {
                var player = buffers.Players[i];
                var desc = space + "Client Number " + player.Index.ToString();
                var startTime = FormatMinutesSecondsFromMs(player.FirstSnapshotTimeMs);
                var endTime = FormatMinutesSecondsFromMs(player.LastSnapshotTimeMs);
                var time = startTime + " - " + endTime;
                var name = buffers.GetString(player.FirstName, player.FirstNameLength, "N/A");
                var value = string.Format("{0}, {1}, team {2}", name, time, GetTeamName(player.FirstTeam));
                info.Generic.Add(Tuple.Create(desc, value));
            }
        }

        private static void AddKeyValuePairs(DemoInfo info, GameStateBuffers buffers, udtParseDataGameState data, string space)
        {
            var start = data.FirstKeyValuePairIndex;
            var count = data.KeyValuePairCount;
            for(uint i = start, end = start + count; i < end; ++i)
            {
                var kvPair = buffers.KeyValuePairs[i];
                var key = buffers.GetString(kvPair.Name, kvPair.NameLength, "N/A");
                var value = buffers.GetString(kvPair.Value, kvPair.ValueLength, "N/A");
                info.Generic.Add(Tuple.Create(space + key, value));
            }
        }
        
        private static string FormatMinutesSecondsFromMs(int totalMs)
        {
            return totalMs == Int32.MinValue ? "?" : App.FormatMinutesSeconds(totalMs / 1000);
        }

        private static string FormatFileOffset(uint bytes)
        {
            return bytes.ToString() + (bytes == 0 ? " byte" : " bytes");
        }

        private static string FormatBytes(ulong byteCount)
        {
            if(byteCount == 0)
	        {
		        return "0 byte";
	        }

	        var units = new[] { "bytes", "KB", "MB", "GB", "TB" };

	        var unitIndex = 0;
            ulong prev = 0;
            ulong temp = byteCount;
	        while(temp >= 1024)
	        {
		        ++unitIndex;
		        prev = temp;
		        temp >>= 10;
	        }

            var number = (double)prev / 1024.0;

            return number.ToString("F3") + " " + units[unitIndex];
        }

        private static string FormatThroughput(ulong bytesPerSecond)
        {
            if(bytesPerSecond == 0)
            {
                return "N/A";
            }

            return FormatBytes(bytesPerSecond) + "/s";
        }

        public static string SafeGetUTF8String(IntPtr address, string onError)
        {
            if(address == IntPtr.Zero)
            {
                return "N/A";
            }

            var length = 0;
            while(Marshal.ReadByte(address, length) != 0)
            {
                ++length;
            }

            if(length == 0)
            {
                return "";
            }

            var buffer = new byte[length];
            Marshal.Copy(address, buffer, 0, buffer.Length);

            return Encoding.UTF8.GetString(buffer);
        }

        public static string SafeGetUTF8String(IntPtr address)
        {
            return SafeGetUTF8String(address, "");
        }
        
        private static string FormatDemoTaker(GameStateBuffers buffers, udtParseDataGameState info)
        {
            var name = buffers.GetString(info.DemoTakerName, info.DemoTakerNameLength, "N/A");

            return string.Format("{0} (player index {1})", name, info.DemoTakerPlayerIndex);
        }

        private static List<string> _teamNames = new List<string>();

        private static string GetTeamName(uint teamIndex)
        {
            if(_teamNames.Count == 0)
            {
                _teamNames.AddRange(GetStringArray(udtStringArray.Teams));
            }

            if(teamIndex >= _teamNames.Count)
            {
                return "N/A";
            }

            return _teamNames[(int)teamIndex];
        }

        private static bool IsStatsValueValid(udtMatchStatsDataType type, int value)
        {
            switch(type)
            {
                case udtMatchStatsDataType.Team:
                    return value >= 0 && value <= 3;

                case udtMatchStatsDataType.Weapon:
                    return true;

                case udtMatchStatsDataType.Percentage:
                    return value >= 0 && value <= 100;

                case udtMatchStatsDataType.Minutes:
                case udtMatchStatsDataType.Seconds:
                case udtMatchStatsDataType.Ping:
                case udtMatchStatsDataType.Positive:
                    return value >= 0;

                case udtMatchStatsDataType.Boolean:
                    return value == 0 || value == 1;

                case udtMatchStatsDataType.Generic:
                default:
                    return value != int.MinValue;
            }
        }

        private static void FormatStatsField(out string fieldName, out string fieldValue, int fieldIntegerValue, udtMatchStatsDataType dataType, string inFieldName)
        {
            fieldName = dataType == udtMatchStatsDataType.Team ? "Team" : ProcessStatsFieldName(inFieldName);
            if(!IsStatsValueValid(dataType, fieldIntegerValue))
            {
                fieldValue = "<invalid>";
                return;
            }

            switch(dataType)
            {
                case udtMatchStatsDataType.Team:
                    fieldValue = GetUDTStringForValueOrNull(udtStringArray.Teams, (uint)fieldIntegerValue) ?? "N/A";
                    break;

                case udtMatchStatsDataType.Weapon:
                    fieldValue = GetUDTStringForValueOrNull(udtStringArray.Weapons, (uint)fieldIntegerValue) ?? "N/A";
                    break;

                case udtMatchStatsDataType.Percentage:
                    fieldValue = fieldIntegerValue.ToString() + "%";
                    break;

                case udtMatchStatsDataType.Minutes:
                    fieldValue = fieldIntegerValue.ToString() + (fieldIntegerValue > 1 ? " minutes" : " minute");
                    break;

                case udtMatchStatsDataType.Seconds:
                    fieldValue = FormatStatsSeconds(fieldIntegerValue);
                    break;

                case udtMatchStatsDataType.Ping:
                    fieldValue = fieldIntegerValue.ToString() + " ms";
                    break;

                case udtMatchStatsDataType.Boolean:
                    fieldValue = fieldIntegerValue == 0 ? "no" : "yes";
                    break;

                case udtMatchStatsDataType.Generic:
                case udtMatchStatsDataType.Positive:
                default:
                    fieldValue = fieldIntegerValue.ToString();
                    break;
            }
        }

        private static string FormatStatsSeconds(int seconds)
        {
            if(seconds <= 0)
            {
                return "0";
            }

            return FormatMinutesSecondsFromMs(seconds * 1000);
        }

        private static string ProcessStatsFieldName(string name)
        {
            return name.Replace("bfg", "BFG").Replace("possession", "poss.").Capitalize();
        }

        public static void PrintPerfStats(IntPtr stats, Stopwatch timer, int fileCount)
        {
            timer.Stop();

            var config = App.Instance.Config;
            var enabledStatsCount = BitManip.PopCnt(config.PerfStatsEnabled) + BitManip.PopCnt(config.CSharpPerfStatsEnabled);
            if(enabledStatsCount == 0)
            {
                return;
            }

            App.GlobalLogInfo("Performance stats:");
            PrintCSharpPerfStats(timer, fileCount);
            PrintLibraryPerfStats(stats);
        }

        private static void PrintLibraryPerfStats(IntPtr stats)
        {
            IntPtr fieldNames = IntPtr.Zero;
            UInt32 fieldNameCount = 0;
            if(udtGetStringArray(udtStringArray.PerfStatsNames, ref fieldNames, ref fieldNameCount) != udtErrorCode.None)
            {
                return;
            }

            IntPtr fieldTypes = IntPtr.Zero;
            UInt32 fieldTypeCount = 0;
            if(udtGetByteArray(udtByteArray.PerfStatsDataTypes, ref fieldTypes, ref fieldTypeCount) != udtErrorCode.None)
            {
                return;
            }

            if((int)fieldNameCount < StatsConstants.PerfFieldCount || (int)fieldTypeCount < StatsConstants.PerfFieldCount)
            {
                return;
            }

            for(var i = 0; i < StatsConstants.PerfFieldCount; ++i)
            {
                if(!BitManip.IsBitSet(App.Instance.Config.PerfStatsEnabled, i))
                {
                    continue;
                }

                var name = SafeGetUTF8String(Marshal.ReadIntPtr(fieldNames, i * IntPtr.Size));
                var type = (udtPerfStatsDataType)Marshal.ReadByte(fieldTypes, i);
                var value = (ulong)Marshal.ReadInt64(stats, i * 8);
                App.GlobalLogInfo("- {0}: {1}", name.Capitalize(), FormatPerfStatsField(type, value));
            }
        }

        private static void PrintCSharpPerfStats(Stopwatch timer, int fileCount)
        {
            var config = App.Instance.Config;
            if(BitManip.IsBitSet(config.CSharpPerfStatsEnabled, (int)CSharpPerfStats.FileCount))
            {
                App.GlobalLogInfo("- File count: " + fileCount);
            }

            if(BitManip.IsBitSet(config.CSharpPerfStatsEnabled, (int)CSharpPerfStats.Duration))
            {
                var microSeconds = 1000000.0 * ((double)timer.ElapsedTicks / (double)Stopwatch.Frequency);
                App.GlobalLogInfo("- Duration (C#): " + App.FormatPerformanceTimeUs((long)microSeconds));
            }
        }

        private static string FormatPerfStatsField(udtPerfStatsDataType type, ulong value)
        {
            switch(type)
            {
                case udtPerfStatsDataType.Duration:
                    return App.FormatPerformanceTimeUs((long)value);

                case udtPerfStatsDataType.Bytes:
                    return FormatBytes(value);

                case udtPerfStatsDataType.Throughput:
                    return FormatThroughput(value);

                case udtPerfStatsDataType.Percentage:
                    return ((float)value / 10.0f).ToString() + @"%";

                case udtPerfStatsDataType.Generic:
                default:
                    return value.ToString();
            }
        }

        private static string GetUDTStringForValueOrNull(udtStringArray stringId, uint value)
        {
            var strings = GetStringArray(stringId);
            if(value >= (uint)strings.Count)
            {
                return null;
            }

            return strings[(int)value];
        }

        public static IntPtr StringToHGlobalUTF8(string nativeString)
        {
            if(nativeString == null)
            {
                return IntPtr.Zero;
            }

            // @NOTE:
            // C# initializes arrays to 0, so the last byte of the byte array
            // we create already is a NULL terminating character.
            var byteCount = Encoding.UTF8.GetByteCount(nativeString) + 1;
            var bytes = new byte[byteCount]; 
            Encoding.UTF8.GetBytes(nativeString, 0, nativeString.Length, bytes, 0);
            var stringAddress = Marshal.AllocHGlobal(byteCount);
            Marshal.Copy(bytes, 0, stringAddress, byteCount);

            return stringAddress;
        }

        [DllImport("msvcrt.dll", EntryPoint = "memset", CallingConvention = CallingConvention.Cdecl, SetLastError = false)]
        public static extern IntPtr MemSet(IntPtr dest, int c, int count);

        [DllImport("msvcrt.dll", EntryPoint = "memcpy", CallingConvention = CallingConvention.Cdecl, SetLastError = false)]
        public static extern IntPtr MemCpy(IntPtr dest, IntPtr src, UIntPtr count);

        public static void MergeBatchPerfStats(IntPtr dest, IntPtr source)
        {
            udtMergeBatchPerfStats(dest, source);
        }

        public static void AddThreadPerfStats(IntPtr dest, IntPtr source)
        {
            udtAddThreadPerfStats(dest, source);
        }
        
        public static int GetProcessorCoreCount()
        {
            UInt32 count = 0;
            if(udtGetProcessorCoreCount(ref count) == udtErrorCode.None)
            {
                return (int)count;
            }

            return Environment.ProcessorCount;
        }
    }
}