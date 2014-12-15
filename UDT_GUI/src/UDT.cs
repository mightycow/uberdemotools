using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

using udtParserContextRef = System.IntPtr;
using udtParserContextGroupRef = System.IntPtr;


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
            Count
        }

        public enum udtStringArray : uint
        {
            Weapons,
            PowerUps,
            MeansOfDeath,
            PlayerMeansOfDeath,
            Teams,
            Count
        };

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtParseArg
        {
            public IntPtr PlugIns; // u32*
            public IntPtr OutputFolderPath; // const char*
            public udtMessageCallback MessageCb;
            public udtProgressCallback ProgressCb;
            public IntPtr ProgressContext; // void*
            public IntPtr CancelOperation; // s32*
            public UInt32 PlugInCount;
            public Int32 GameStateIndex;
            public UInt32 FileOffset;
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
		    public Int32 StartTimeMs;
		    public Int32 EndTimeMs;
            public Int32 GameStateIndex;
            public Int32 Reserved1;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
	    public struct udtCutByTimeArg
	    {
            public IntPtr Cuts; // const udtCut*
		    public UInt32 CutCount;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
	    public struct udtCutByChatRule
	    {
		    public IntPtr Pattern; // const char*
		    public UInt32 ChatOperator;
		    public UInt32 CaseSensitive;
		    public UInt32 IgnoreColorCodes;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
	    public struct udtCutByChatArg
	    {
		    public IntPtr Rules; // const udtCutByChatRule*
		    public UInt32 RuleCount;
		    public UInt32 StartOffsetSec;
		    public UInt32 EndOffsetSec;
	    }

        [Flags]
        public enum udtCutByFragArgFlags
        {
            AllowSelfKills = 1 << 0,
            AllowTeamKills = 1 << 1,
            AllowDeaths = 1 << 2
        };

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtCutByFragArg
        {
            public UInt32 MinFragCount;
            public UInt32 TimeBetweenFragsSec;
            public UInt32 TimeMode; // 0=max, 1=avg
            public UInt32 StartOffsetSec;
            public UInt32 EndOffsetSec;
            public Int32 PlayerIndex;
            public UInt32 Flags;
            public UInt32 AllowedMeansOfDeaths;
        };

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
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
            public Int32 GameStateIndex;
            public Int32 Reserved1;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtMatchInfo
        {
            public Int32 WarmUpEndTimeMs; 
            public Int32 MatchStartTimeMs;
            public Int32 MatchEndTimeMs;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtParseDataGameState
	    {
		    public IntPtr Matches; // const udtMatchInfo*
		    public UInt32 MatchCount;
		    public UInt32 FileOffset;
		    public Int32 FirstSnapshotTimeMs;
		    public Int32 LastSnapshotTimeMs;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        struct udtParseDataObituary
	    {
		    public IntPtr AttackerName; // const char*
            public IntPtr TargetName; // const char*
            public IntPtr MeanOfDeathName; // const char*
            public IntPtr Reserved1;
            public Int32 GameStateIndex;
            public Int32 ServerTimeMs;
            public Int32 AttackerIdx;
            public Int32 TargetIdx;
            public Int32 MeanOfDeath;
            public Int32 AttackerTeamIdx;
            public Int32 TargetTeamIdx;
            public Int32 Reserved2;
	    };

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
        extern static private udtErrorCode udtGetStringArray(udtStringArray arrayId, ref IntPtr array, ref UInt32 elementCount);

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
        extern static private udtErrorCode udtCutDemoFileByFrag(udtParserContextRef context, ref udtParseArg info, ref udtCutByFragArg fragInfo, string demoFilePath);

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

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtCutDemoFilesByFrag(ref udtParseArg info, ref udtMultiParseArg extraInfo, ref udtCutByFragArg fragInfo);

        // The list of plug-ins activated when loading demos.
        private static UInt32[] PlugInArray = new UInt32[] 
        { 
            (UInt32)udtParserPlugIn.Chat, 
            (UInt32)udtParserPlugIn.GameState,
            (UInt32)udtParserPlugIn.Obituaries
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

        public static IntPtr CreateContext()
        {
            return udtCreateContext();
        }

        public static string GetProtocolAsString(udtProtocol protocol)
        {
            switch(protocol)
            {
                case udtProtocol.Dm68: return "68 (Quake 3's last protocol)";
                case udtProtocol.Dm73: return "73 (Quake Live's old protocol)";
                case udtProtocol.Dm90: return "90 (Quake Live's current protocol)";
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

            return udtSplitDemoFile(context, ref parseArg, filePath) == udtErrorCode.None;
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
            cut.GameStateIndex = parseArg.GameStateIndex;
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

        private static UInt32 GetOperatorFromString(string op)
        {
            udtChatOperator result;
            if(!Enum.TryParse<udtChatOperator>(op, false, out result))
            {
                return (UInt32)udtChatOperator.Contains;
            }

            return (UInt32)result;
        }

        public static bool CutDemosByChat(ref udtParseArg parseArg, List<string> filePaths, List<ChatRule> rules, int startOffset, int endOffset, int maxThreadCount)
        {
            var errorCodeArray = new Int32[filePaths.Count];
            var filePathArray = new IntPtr[filePaths.Count];
            for(var i = 0; i < filePaths.Count; ++i)
            {
                filePathArray[i] = Marshal.StringToHGlobalAnsi(Path.GetFullPath(filePaths[i]));
            }

            var rulesArray = new UDT_DLL.udtCutByChatRule[rules.Count];
            for(var i = 0; i < rules.Count; ++i)
            {
                rulesArray[i].CaseSensitive = (UInt32)(rules[i].CaseSensitive ? 1 : 0);
                rulesArray[i].ChatOperator = GetOperatorFromString(rules[i].Operator);
                rulesArray[i].IgnoreColorCodes = (UInt32)(rules[i].IgnoreColors ? 1 : 0);
                rulesArray[i].Pattern = Marshal.StringToHGlobalAnsi(rules[i].Value);
            }

            parseArg.PlugInCount = 0;
            parseArg.PlugIns = IntPtr.Zero;

            var pinnedFilePaths = new PinnedObject(filePathArray);
            var pinnedErrorCodes = new PinnedObject(errorCodeArray);
            var multiParseArg = new udtMultiParseArg();
            multiParseArg.FileCount = (UInt32)filePathArray.Length;
            multiParseArg.FilePaths = pinnedFilePaths.Address;
            multiParseArg.OutputErrorCodes = pinnedErrorCodes.Address;
            multiParseArg.MaxThreadCount = (UInt32)maxThreadCount;

            var pinnedRules = new PinnedObject(rulesArray);
            var cutByChatArg = new udtCutByChatArg();
            cutByChatArg.StartOffsetSec = (UInt32)startOffset;
            cutByChatArg.EndOffsetSec = (UInt32)endOffset;
            cutByChatArg.RuleCount = (UInt32)rulesArray.Length;
            cutByChatArg.Rules = pinnedRules.Address;

            var result = udtCutDemoFilesByChat(ref parseArg, ref multiParseArg, ref cutByChatArg);
            pinnedFilePaths.Free();
            pinnedErrorCodes.Free();
            for(var i = 0; i < filePathArray.Length; ++i)
            {
                Marshal.FreeHGlobal(filePathArray[i]);
            }

            pinnedRules.Free();
            for(var i = 0; i < rulesArray.Length; ++i)
            {
                Marshal.FreeHGlobal(rulesArray[i].Pattern);
            }

            return result == udtErrorCode.None;
        }

        public static bool CutDemosByFrag(ref udtParseArg parseArg, List<string> filePaths, udtCutByFragArg rules, int maxThreadCount)
        {
            var errorCodeArray = new Int32[filePaths.Count];
            var filePathArray = new IntPtr[filePaths.Count];
            for(var i = 0; i < filePaths.Count; ++i)
            {
                filePathArray[i] = Marshal.StringToHGlobalAnsi(Path.GetFullPath(filePaths[i]));
            }

            parseArg.PlugInCount = 0;
            parseArg.PlugIns = IntPtr.Zero;

            var pinnedFilePaths = new PinnedObject(filePathArray);
            var pinnedErrorCodes = new PinnedObject(errorCodeArray);
            var multiParseArg = new udtMultiParseArg();
            multiParseArg.FileCount = (UInt32)filePathArray.Length;
            multiParseArg.FilePaths = pinnedFilePaths.Address;
            multiParseArg.OutputErrorCodes = pinnedErrorCodes.Address;
            multiParseArg.MaxThreadCount = (UInt32)maxThreadCount;

            var result = udtCutDemoFilesByFrag(ref parseArg, ref multiParseArg, ref rules);

            pinnedFilePaths.Free();
            pinnedErrorCodes.Free();
            for(var i = 0; i < filePathArray.Length; ++i)
            {
                Marshal.FreeHGlobal(filePathArray[i]);
            }

            return result == udtErrorCode.None;
        }

        public static List<DemoInfo> ParseDemos(ref udtParseArg parseArg, List<string> filePaths, int maxThreadCount)
        {
            var errorCodeArray = new Int32[filePaths.Count];
            var filePathArray = new IntPtr[filePaths.Count];
            for(var i = 0; i < filePaths.Count; ++i)
            {
                filePathArray[i] = Marshal.StringToHGlobalAnsi(Path.GetFullPath(filePaths[i]));
            }

            var pinnedPlugIns = new PinnedObject(PlugInArray);
            parseArg.PlugInCount = (UInt32)PlugInArray.Length;
            parseArg.PlugIns = pinnedPlugIns.Address;

            var pinnedFilePaths = new PinnedObject(filePathArray);
            var pinnedErrorCodes = new PinnedObject(errorCodeArray);
            var multiParseArg = new udtMultiParseArg();
            multiParseArg.FileCount = (UInt32)filePathArray.Length;
            multiParseArg.FilePaths = pinnedFilePaths.Address;
            multiParseArg.OutputErrorCodes = pinnedErrorCodes.Address;
            multiParseArg.MaxThreadCount = (UInt32)maxThreadCount;

            udtParserContextGroupRef contextGroup = IntPtr.Zero;
            var result = udtParseDemoFiles(ref contextGroup, ref parseArg, ref multiParseArg);
            pinnedPlugIns.Free();
            pinnedFilePaths.Free();
            pinnedErrorCodes.Free();
            for(var i = 0; i < filePathArray.Length; ++i)
            {
                Marshal.FreeHGlobal(filePathArray[i]);
            }

            if(result != udtErrorCode.None && result != udtErrorCode.OperationCanceled)
            {
                udtDestroyContextGroup(contextGroup);
                App.GlobalLogError("Failed to parse demos: " + GetErrorCodeString(result));
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
                    uint inputIdx = 0;
                    if(udtGetDemoInputIndex(context, j, ref inputIdx) != udtErrorCode.None)
                    {
                        continue;
                    }

                    var errorCode = errorCodeArray[(int)inputIdx];
                    if(errorCode != (Int32)udtErrorCode.None)
                    {
                        if(errorCode != (Int32)udtErrorCode.Unprocessed && errorCode != (Int32)udtErrorCode.OperationCanceled)
                        {
                            var fileName = Path.GetFileName(filePaths[(int)inputIdx]);
                            var errorCodeString = GetErrorCodeString((udtErrorCode)errorCode);
                            App.GlobalLogError("Failed to parse demo file {0}: {1}", fileName, errorCodeString);
                        }
                        continue;
                    }

                    var filePath = filePaths[(int)inputIdx];
                    var protocol = udtGetProtocolByFilePath(filePath);
                    var info = new DemoInfo();
                    info.InputIndex = (int)inputIdx;
                    info.FilePath = Path.GetFullPath(filePath);
                    info.Protocol = UDT_DLL.GetProtocolAsString(protocol);
                    
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
            ExtractGameStateEvents(context, demoIdx, ref info);
            ExtractObituaries(context, demoIdx, ref info);
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
                var player = Marshal.PtrToStringAnsi(data.PlayerNameNoCol) ?? "N/A";
                var message = Marshal.PtrToStringAnsi(data.MessageNoCol) ?? "N/A";
                var item = new ChatEventDisplayInfo(data.GameStateIndex, time, player, message);
                info.ChatEvents.Add(item);
            }
        }

        private static string FormatMinutesSecondsFromMs(int totalMs)
        {
            return totalMs == Int32.MinValue ? "?" : App.FormatMinutesSeconds(totalMs / 1000);
        }

        private static string FormatBytes(uint bytes)
        {
            return bytes.ToString() + (bytes == 0 ? " byte" : " bytes");
        }

        private static void ExtractGameStateEvents(udtParserContextRef context, uint demoIdx, ref DemoInfo info)
        {
            uint gsEventCount = 0;
            IntPtr gsEvents = IntPtr.Zero;
            if(udtGetDemoDataInfo(context, demoIdx, udtParserPlugIn.GameState, ref gsEvents, ref gsEventCount) != udtErrorCode.None)
            {
                return;
            }

            const string space = "   ";

            for(uint i = 0; i < gsEventCount; ++i)
            {
                var address = new IntPtr(gsEvents.ToInt64() + i * sizeof(udtParseDataGameState));
                var data = (udtParseDataGameState)Marshal.PtrToStructure(address, typeof(udtParseDataGameState));
                info.GameStateFileOffsets.Add(data.FileOffset);

                var firstSnapTime = App.FormatMinutesSeconds(data.FirstSnapshotTimeMs / 1000);
                var lastSnapTime = App.FormatMinutesSeconds(data.LastSnapshotTimeMs / 1000);
                info.Generic.Add(Tuple.Create("GameState #" + (i + 1).ToString(), ""));
                info.Generic.Add(Tuple.Create(space + "File Offset", FormatBytes(data.FileOffset)));
                info.Generic.Add(Tuple.Create(space + "Server Time Range", firstSnapTime + " - " + lastSnapTime));
                info.Generic.Add(Tuple.Create(space + "Matches", data.MatchCount.ToString()));
  
                var matchCount = data.MatchCount;
                for(uint j = 0; j < matchCount; ++j)
                {
                    var matchAddress = new IntPtr(data.Matches.ToInt64() + j * sizeof(udtMatchInfo));
                    var matchData = (udtMatchInfo)Marshal.PtrToStructure(matchAddress, typeof(udtMatchInfo));

                    var desc = space + "Match #" + (j + 1).ToString();
                    var start = FormatMinutesSecondsFromMs(matchData.MatchStartTimeMs);
                    var end = FormatMinutesSecondsFromMs(matchData.MatchEndTimeMs);
                    var val = start + " - " + end;
                    info.Generic.Add(Tuple.Create(desc, val));
                }
            }
        }

        private static void ExtractObituaries(udtParserContextRef context, uint demoIdx, ref DemoInfo info)
        {
            uint obituaryEventCount = 0;
            IntPtr obituaryEvents = IntPtr.Zero;
            if(udtGetDemoDataInfo(context, demoIdx, udtParserPlugIn.Obituaries, ref obituaryEvents, ref obituaryEventCount) != udtErrorCode.None)
            {
                App.GlobalLogError("udtGetDemoDataInfo for Obituaries FAILED");
                return;
            }

            for(uint i = 0; i < obituaryEventCount; ++i)
            {
                var address = new IntPtr(obituaryEvents.ToInt64() + i * sizeof(udtParseDataObituary));
                var data = (udtParseDataObituary)Marshal.PtrToStructure(address, typeof(udtParseDataObituary));

                int totalSeconds = data.ServerTimeMs / 1000;
                int minutes = totalSeconds / 60;
                int seconds = totalSeconds % 60;
                var time = string.Format("{0}:{1}", minutes, seconds.ToString("00"));
                var attacker = Marshal.PtrToStringAnsi(data.AttackerName) ?? "N/A";
                var target = Marshal.PtrToStringAnsi(data.TargetName) ?? "N/A";
                var mod = Marshal.PtrToStringAnsi(data.MeanOfDeathName) ?? "N/A";
                var item = new FragEventDisplayInfo(data.GameStateIndex, time, attacker, target, mod);
                info.FragEvents.Add(item);
            }
        }
    }
}