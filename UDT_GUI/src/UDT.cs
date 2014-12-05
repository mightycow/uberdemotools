using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

using udtParserContextRef = System.IntPtr;
using udtParserContextGroupRef = System.IntPtr;
using System.IO;


namespace Uber.DemoTools
{
    public unsafe class UDT_DLL
    {
        private const string _dllPath = "UDT.dll";

        [UnmanagedFunctionPointerAttribute(CallingConvention.Cdecl)]
        public delegate void udtProgressCallback(float progress, IntPtr userData);

        [UnmanagedFunctionPointerAttribute(CallingConvention.Cdecl)]
        public delegate void udtMessageCallback(int logLevel, string message);

        [UnmanagedFunctionPointerAttribute(CallingConvention.Cdecl)]
        public delegate void udtCrashCallback(string message);

        public enum udtProtocol
        {
            Invalid,
            Dm68,
            Dm73,
            Dm90
        }

        public enum udtErrorCode : int
        {
            None,
            InvalidArgument,
            OperationFailed
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
            Count
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct udtParseArg
        {
            public IntPtr PlugIns; // u32*
            public IntPtr OutputFolderPath; // const char*
            public udtMessageCallback MessageCb;
            public udtProgressCallback ProgressCb;
            public IntPtr ProgressContext; // void*
            public IntPtr CancelOperation; // s32*
            public UInt32 PlugInCount;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct udtMultiParseArg
	    {
		    public IntPtr FilePaths; // const char**
		    public UInt32 FileCount;
		    public UInt32 MaxThreadCount;
	    }
	
        [StructLayout(LayoutKind.Sequential)]
	    public struct udtCut
	    {
		    public Int32 StartTimeMs;
		    public Int32 EndTimeMs;
	    }

        [StructLayout(LayoutKind.Sequential)]
	    public struct udtCutByTimeArg
	    {
            public IntPtr Cuts; // const udtCut*
		    public UInt32 CutCount;
	    }

        [StructLayout(LayoutKind.Sequential)]
	    public struct udtCutByChatRule
	    {
		    public IntPtr Pattern; // const char*
		    public UInt32 ChatOperator;
		    public UInt32 CaseSensitive;
		    public UInt32 IgnoreColorCodes;
	    }

        [StructLayout(LayoutKind.Sequential)]
	    public struct udtCutByChatArg
	    {
		    public IntPtr Rules; // const udtCutByChatRule*
		    public UInt32 RuleCount;
		    public UInt32 StartOffsetSec;
		    public UInt32 EndOffsetSec;
	    }

	    [StructLayout(LayoutKind.Sequential)]
        public struct udtParseDataChat
	    {
            public IntPtr OriginalCommand; // const char*
            public IntPtr ClanName; // const char*
            public IntPtr PlayerName; // const char*
            public IntPtr Message; // const char*
            public IntPtr OriginalCommandNoCol; // const char*
            public IntPtr ClanNameNoCol; // const char*
            public IntPtr PlayerNameNoCol; // const char*
            public IntPtr MessageNoCol; // const char*
		    public Int32 ServerTimeMs;
		    public Int32 PlayerIndex;
	    }

        [StructLayout(LayoutKind.Sequential)]
        public struct udtMatchInfo
        {
            public Int32 WarmUpEndTimeMs; 
            public Int32 MatchStartTimeMs;
            public Int32 MatchEndTimeMs;
        }

	    [StructLayout(LayoutKind.Sequential)]
        public struct udtParseDataGameState
	    {
		    public IntPtr Matches; // const udtMatchInfo*
		    public UInt32 MatchCount;
		    public UInt32 FileOffset;
		    public Int32 FirstSnapshotTimeMs;
		    public Int32 LastSnapshotTimeMs;
	    }

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private IntPtr udtGetVersionString();

	    [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private Int32 udtIsValidProtocol(udtProtocol protocol);

	    [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private UInt32 udtGetSizeOfIdEntityState(udtProtocol protocol);

	    [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private UInt32 udtGetSizeOfidClientSnapshot(udtProtocol protocol);

	    [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private IntPtr udtGetFileExtensionByProtocol(udtProtocol protocol);

	    [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private udtProtocol udtGetProtocolByFilePath(string filePath);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtCrash(udtCrashType crashType);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtSetCrashHandler(IntPtr crashHandler);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private udtParserContextRef udtCreateContext();

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private udtErrorCode udtDestroyContext(udtParserContextRef context);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private udtErrorCode udtSplitDemoFile(udtParserContextRef context, ref udtParseArg info, string demoFilePath);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtCutDemoFileByTime(udtParserContextRef context, ref udtParseArg info, ref udtCutByTimeArg cutInfo, string demoFilePath);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtCutDemoFileByChat(udtParserContextRef context, ref udtParseArg info, ref udtCutByChatArg chatInfo, string demoFilePath);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtParseDemoFile(udtParserContextRef context, ref udtParseArg info, string demoFilePath);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtGetDemoDataInfo(udtParserContextRef context, UInt32 demoIdx, udtParserPlugIn plugInId, ref IntPtr buffer, ref UInt32 count);

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
        extern static private udtErrorCode udtCutDemoFilesByChat(ref udtParseArg info, ref udtMultiParseArg extraInfo, ref udtCutByChatArg chatInfo);

        private static UInt32[] PlugInArray = new UInt32[] { (UInt32)udtParserPlugIn.Chat, (UInt32)udtParserPlugIn.GameState };

        public static string GetVersion()
        {
            var version = udtGetVersionString();
            if(version == IntPtr.Zero)
            {
                return "N/A";
            }

            return Marshal.PtrToStringAnsi(version) ?? "N/A";
        }

        public static bool Crash(udtCrashType crashType)
        {
            return udtCrash(crashType) == udtErrorCode.None;
        }

        public static bool SetFatalErrorHandler(udtCrashCallback handler)
        {
            GCHandle.Alloc(handler);
            var address = Marshal.GetFunctionPointerForDelegate(handler);

            return udtSetCrashHandler(address) == udtErrorCode.None;
        }

        public static IntPtr CreateContext()
        {
            return udtCreateContext();
        }

        public static bool CutByTime(udtParserContextRef context, ref udtParseArg parseArg, string filePath, int startTimeSec, int endTimeSec)
        {
            if(context == IntPtr.Zero)
            {
                return false;
            }

            parseArg.PlugInCount = 0;
            parseArg.PlugIns = IntPtr.Zero;

            var cut = new udtCut();
            cut.StartTimeMs = startTimeSec * 1000;
            cut.EndTimeMs = endTimeSec * 1000;
            var pinnedCut = new PinnedObject(cut);
            var cutInfo = new udtCutByTimeArg();
            cutInfo.Cuts = pinnedCut.Address;
            cutInfo.CutCount = 1;

            var success = udtCutDemoFileByTime(context, ref parseArg, ref cutInfo, filePath) == udtErrorCode.None;
            pinnedCut.Free();

            return success;
        }

        public static DemoInfo ParseDemo(udtParserContextRef context, ref udtParseArg parseArg, string filePath)
        {
            if(context == IntPtr.Zero)
            {
                return null;
            }

            filePath = Path.GetFullPath(filePath);
            var protocol = udtGetProtocolByFilePath(filePath);
            if(protocol == udtProtocol.Invalid)
            {
                return null;
            }

            var pinnedPlugIns = new PinnedObject(PlugInArray);
            parseArg.PlugInCount = (UInt32)PlugInArray.Length;
            parseArg.PlugIns = pinnedPlugIns.Address;
            var result = udtParseDemoFile(context, ref parseArg, filePath);
            pinnedPlugIns.Free();
            if(result != udtErrorCode.None)
            {
                return null;
            }

            var info = new DemoInfo();
            info.FilePath = filePath;
            info.Protocol = protocol.ToString();
            ExtractDemoInfo(context, 0, ref info);

            return info;
        }

        public static List<DemoInfo> ParseDemos(ref udtParseArg parseArg, List<string> filePaths, int maxThreadCount)
        {
            var filePathArray = new IntPtr[filePaths.Count];
            for(var i = 0; i < filePaths.Count; ++i)
            {
                filePathArray[i] = Marshal.StringToHGlobalAnsi(Path.GetFullPath(filePaths[i]));
            }

            var pinnedPlugIns = new PinnedObject(PlugInArray);
            parseArg.PlugInCount = (UInt32)PlugInArray.Length;
            parseArg.PlugIns = pinnedPlugIns.Address;

            var pinnedFilePaths = new PinnedObject(filePathArray);
            var multiParseArg = new udtMultiParseArg();
            multiParseArg.FileCount = (UInt32)filePathArray.Length;
            multiParseArg.FilePaths = pinnedFilePaths.Address;
            multiParseArg.MaxThreadCount = (UInt32)maxThreadCount;

            udtParserContextGroupRef contextGroup = IntPtr.Zero;
            var result = udtParseDemoFiles(ref contextGroup, ref parseArg, ref multiParseArg);
            pinnedPlugIns.Free();
            pinnedFilePaths.Free();
            for(var i = 0; i < filePathArray.Length; ++i)
            {
                Marshal.FreeHGlobal(filePathArray[i]);
            }

            if(result != udtErrorCode.None)
            {
                udtDestroyContextGroup(contextGroup);
                return null;
            }

            uint contextCount = 0;
            if(udtGetContextCountFromGroup(contextGroup, ref contextCount) != udtErrorCode.None)
            {
                udtDestroyContextGroup(contextGroup);
                return null;
            }

            var infoList = new List<DemoInfo>();
            for(uint i = 0; i < contextCount; ++i)
            {
                udtParserContextRef context = IntPtr.Zero;
                if(udtGetContextFromGroup(contextGroup, i, ref context) != udtErrorCode.None)
                {
                    udtDestroyContextGroup(contextGroup);
                    return null;
                }

                uint demoCount = 0;
                if(udtGetDemoCountFromContext(context, ref demoCount) != udtErrorCode.None)
                {
                    udtDestroyContextGroup(contextGroup);
                    return null;
                }

                for(uint j = 0; j < demoCount; ++j)
                {
                    var info = new DemoInfo();
                    info.InputIndex = 0;
                    info.FilePath = "?";
                    info.Protocol = "?";

                    uint inputIdx = 0;
                    if(udtGetDemoInputIndex(context, j, ref inputIdx) == udtErrorCode.None)
                    {
                        info.InputIndex = (int)inputIdx;
                        info.FilePath = Path.GetFullPath(filePaths[(int)inputIdx]);
                        info.Protocol = udtGetProtocolByFilePath(filePaths[(int)inputIdx]).ToString();
                    }
                    
                    ExtractDemoInfo(context, j, ref info);
                    infoList.Add(info);
                }
            }

            udtDestroyContextGroup(contextGroup);

            // Keep the original input order.
            infoList.Sort((a, b) => a.InputIndex - b.InputIndex);

            return infoList;
        }

        private static void ExtractDemoInfo(udtParserContextRef context, uint demoIdx, ref DemoInfo info)
        {
            ExtractChatEvents(context, demoIdx, ref info);
        }

        private static void ExtractChatEvents(udtParserContextRef context, uint demoIdx, ref DemoInfo info)
        {
            uint chatEventCount = 0;
            IntPtr chatEvents = IntPtr.Zero;
            if(udtGetDemoDataInfo(context, demoIdx, udtParserPlugIn.Chat, ref chatEvents, ref chatEventCount) != udtErrorCode.None)
            {
                return;
            }

            for(uint i = 0; i < chatEventCount; ++i)
            {
                var address = new IntPtr(chatEvents.ToInt64() + i * sizeof(udtParseDataChat));
                var data = (udtParseDataChat)Marshal.PtrToStructure(address, typeof(udtParseDataChat));

                int totalSeconds = data.ServerTimeMs / 1000;
                int minutes = totalSeconds / 60;
                int seconds = totalSeconds % 60;
                var time = string.Format("{0}:{1}", minutes, seconds.ToString("00"));
                var player = Marshal.PtrToStringAnsi(data.PlayerNameNoCol);
                var message = Marshal.PtrToStringAnsi(data.MessageNoCol);
                var item = new ChatEventDisplayInfo(time, player ?? "N/A", message ?? "N/A");
                info.ChatEvents.Add(item);
            }
        }
    }
}