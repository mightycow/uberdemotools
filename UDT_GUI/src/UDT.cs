using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

using udtParserContextRef = System.IntPtr;
using udtParserContextGroupRef = System.IntPtr;


namespace Uber.DemoTools
{
    public unsafe class UDT_DLL
    {
#if (UDT_X86)
        private const int MaxBatchSizeParsing = 128;
        private const int MaxBatchSizeCutting = 512;
        private const int MaxBatchSizeConverting = 512;
#else
        private const int MaxBatchSizeParsing = 512;
        private const int MaxBatchSizeCutting = 2048;
        private const int MaxBatchSizeConverting = 2048;
#endif

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
            MidAirs,
            Count
        }

        public enum udtWeaponBits : uint
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
            Count
        }

        public enum udtPatternType : uint
        {
            GlobalChat,
            FragSequences,
            MidAirFrags,
            MultiFragRails,
            FlagCaptures,
            FlickRails,
            Count
        }

        [Flags]
        public enum udtParseArgFlags : uint
        {
            PrintAllocStats = 1 << 0
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
            public UInt32 PlugInCount;
            public Int32 GameStateIndex;
            public UInt32 FileOffset;
            public UInt32 Flags;
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
        public struct udtPatternInfo
	    {
		    public IntPtr TypeSpecificInfo; // const void*
		    public UInt32 Type;
		    public Int32 Reserved1;
	    }

        public enum udtPlayerIndex : int
        {
            FirstPersonPlayer = -2,
            DemoTaker = -1
        };

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
	    public struct udtCutByPatternArg
	    {
		    public IntPtr Patterns; // const udtPatternInfo*
            public IntPtr PlayerName; // const char*
		    public UInt32 PatternCount;
		    public UInt32 StartOffsetSec;
		    public UInt32 EndOffsetSec;
		    public Int32 PlayerIndex;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtCutByTimeArg
        {
            public IntPtr Cuts; // const udtCut*
            public UInt32 CutCount;
            public Int32 Reserved1;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
	    public struct udtCutByChatRule
	    {
		    public IntPtr Pattern; // const char*
		    public UInt32 ChatOperator;
		    public UInt32 CaseSensitive;
		    public UInt32 IgnoreColorCodes;
            public Int32 Reserved1;
	    }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
	    public struct udtCutByChatArg
	    {
		    public IntPtr Rules; // const udtCutByChatRule*
		    public UInt32 RuleCount;
            public Int32 Reserved1;
	    }

        [Flags]
        public enum udtCutByFragArgFlags
        {
            AllowSelfKills = 1 << 0,
            AllowTeamKills = 1 << 1,
            AllowDeaths = 1 << 2
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtCutByFragArg
        {
            public UInt32 MinFragCount;
            public UInt32 TimeBetweenFragsSec;
            public UInt32 TimeMode; // 0=max, 1=avg
            public UInt32 Flags;
            public UInt32 AllowedMeansOfDeaths;
            public Int32 Reserved1;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtCutByMidAirArg
        {
            public UInt32 AllowedWeapons;
            public UInt32 MinDistance;
            public UInt32 MinAirTimeMs;
            public Int32 Reserved1;
        };

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtCutByMultiRailArg
        {
            public UInt32 MinKillCount;
            public Int32 Reserved1;
        };

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtCutByFlagCaptureArg
        {
            public UInt32 MinCarryTimeMs;
            public UInt32 MaxCarryTimeMs;
        };

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtCutByFlickRailArg
        {
            public float MinSpeed;
            public Int32 Reserved1;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtMapConversionRule
	    {
            public IntPtr InputName; // const char*
            public IntPtr OutputName; // const char*
		    public float PositionOffsetX;
            public float PositionOffsetY;
            public float PositionOffsetZ;
		    public Int32 Reserved1;
	    };

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtProtocolConversionArg
        {
            public IntPtr MapRules; // const udtMapConversionRule*
            public UInt32 MapRuleCount;
            public UInt32 OutputProtocol;
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
            public Int32 Reserved1;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtGameStateKeyValuePair
	    {
		    public IntPtr Name; // const char*
            public IntPtr Value; // const char*
	    };

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtGameStatePlayerInfo
	    {
            public IntPtr FirstName; // const char*
            public Int32 Index;
            public Int32 FirstSnapshotTimeMs;
            public Int32 LastSnapshotTimeMs;
            public UInt32 FirstTeam;
	    };

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct udtParseDataGameState
	    {

		    public IntPtr Matches; // const udtMatchInfo*
            public IntPtr KeyValuePairs; // const udtGameStateInfo*
            public IntPtr Players; // const udtGameStatePlayerInfo*
            public IntPtr DemoTakerName; // const char*
		    public UInt32 MatchCount;
            public UInt32 KeyValuePairCount;
            public UInt32 PlayerCount;
            public Int32 DemoTakerPlayerIndex;
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
        extern static private udtErrorCode udtCutDemoFilesByPattern(ref udtParseArg info, ref udtMultiParseArg extraInfo, ref udtCutByPatternArg patternInfo);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtConvertDemoFiles(ref udtParseArg info, ref udtMultiParseArg extraInfo, ref udtProtocolConversionArg conversionInfo);

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

        public static udtProtocol GetProtocolFromFilePath(string filePath)
        {
            return udtGetProtocolByFilePath(filePath);
        }

        public static udtCutByFragArg CreateCutByFragArg(UdtConfig config, UdtPrivateConfig privateConfig)
        {
            UInt32 flags = 0;
            if(config.FragCutAllowAnyDeath)
            {
                flags |= (UInt32)UDT_DLL.udtCutByFragArgFlags.AllowDeaths;
            }
            if(config.FragCutAllowSelfKills)
            {
                flags |= (UInt32)UDT_DLL.udtCutByFragArgFlags.AllowSelfKills;
            }
            if(config.FragCutAllowTeamKills)
            {
                flags |= (UInt32)UDT_DLL.udtCutByFragArgFlags.AllowTeamKills;
            }

            var rules = new udtCutByFragArg();
            rules.MinFragCount = (UInt32)config.FragCutMinFragCount;
            rules.TimeBetweenFragsSec = (UInt32)config.FragCutTimeBetweenFrags;
            rules.TimeMode = 0; // @TODO:
            rules.Flags = flags;
            rules.AllowedMeansOfDeaths = privateConfig.FragCutAllowedMeansOfDeaths;

            return rules;
        }

        public static udtCutByMidAirArg CreateCutByMidAirArg(UdtConfig config)
        {
            UInt32 weaponFlags = 0;
            if(config.MidAirCutAllowRocket)
            {
                weaponFlags |= (UInt32)udtWeaponBits.Rocket;
            }
            if(config.MidAirCutAllowGrenade)
            {
                weaponFlags |= (UInt32)udtWeaponBits.Grenade;
            }
            if(config.MidAirCutAllowBFG)
            {
                weaponFlags |= (UInt32)udtWeaponBits.BFG;
            }

            var rules = new udtCutByMidAirArg();
            rules.AllowedWeapons = weaponFlags;
            rules.MinDistance = (UInt32)Math.Max(0, config.MidAirCutMinDistance);
            rules.MinAirTimeMs = (UInt32)Math.Max(0, config.MidAirCutMinAirTimeMs);

            return rules;
        }

        public static udtCutByMultiRailArg CreateCutByMultiRailArg(UdtConfig config)
        {
            var rules = new udtCutByMultiRailArg();
            rules.MinKillCount = (UInt32)config.MultiRailCutMinFragCount;

            return rules;
        }

        public static udtCutByFlagCaptureArg CreateCutByFlagCaptureArg(UdtConfig config)
        {
            var rules = new udtCutByFlagCaptureArg();
            rules.MinCarryTimeMs = (UInt32)config.FlagCaptureMinCarryTimeMs;
            rules.MaxCarryTimeMs = (UInt32)config.FlagCaptureMaxCarryTimeMs;

            return rules;
        }

        public static udtCutByFlickRailArg CreateCutByFlickRailArg(UdtConfig config)
        {
            var rules = new udtCutByFlickRailArg();
            rules.MinSpeed = (config.FlickRailMinSpeed / 180.0f) * (float)Math.PI;

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

        private static UInt32 GetOperatorFromString(string op)
        {
            udtChatOperator result;
            if(!Enum.TryParse<udtChatOperator>(op, false, out result))
            {
                return (UInt32)udtChatOperator.Contains;
            }

            return (UInt32)result;
        }

        // @TODO: Use in all cases, not just in CutByPattern.
        public class ArgumentResources
        {
            public List<PinnedObject> PinnedObjects = new List<PinnedObject>();
            public List<IntPtr> GlobalAllocationHandles = new List<IntPtr>();

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

        public static bool CreateChatPatternInfo(ref udtPatternInfo pattern, ArgumentResources resources, List<ChatRule> rules)
        {
            if(rules.Count == 0)
            {
                return false;
            }

            var rulesArray = new UDT_DLL.udtCutByChatRule[rules.Count];
            for(var i = 0; i < rules.Count; ++i)
            {
                rulesArray[i].CaseSensitive = (UInt32)(rules[i].CaseSensitive ? 1 : 0);
                rulesArray[i].ChatOperator = GetOperatorFromString(rules[i].Operator);
                rulesArray[i].IgnoreColorCodes = (UInt32)(rules[i].IgnoreColors ? 1 : 0);
                rulesArray[i].Pattern = Marshal.StringToHGlobalAnsi(rules[i].Value);
                resources.GlobalAllocationHandles.Add(rulesArray[i].Pattern);
            }
            var pinnedRulesArray = new PinnedObject(rulesArray);

            var cutByChatArg = new udtCutByChatArg();
            cutByChatArg.Rules = pinnedRulesArray.Address;
            cutByChatArg.RuleCount = (UInt32)rulesArray.Length;
            var pinnedRules = new PinnedObject(cutByChatArg);

            resources.PinnedObjects.Add(pinnedRulesArray);
            resources.PinnedObjects.Add(pinnedRules);

            pattern.Type = (UInt32)udtPatternType.GlobalChat;
            pattern.TypeSpecificInfo = pinnedRules.Address;

            return true;
        }

        public static bool CreateFragPatternInfo(ref udtPatternInfo pattern, ArgumentResources resources, udtCutByFragArg rules)
        {
            var pinnedRules = new PinnedObject(rules);
            resources.PinnedObjects.Add(pinnedRules);

            pattern.Type = (UInt32)udtPatternType.FragSequences;
            pattern.TypeSpecificInfo = pinnedRules.Address;

            return true;
        }

        public static bool CreateMidAirPatternInfo(ref udtPatternInfo pattern, ArgumentResources resources, udtCutByMidAirArg rules)
        {
            var pinnedRules = new PinnedObject(rules);
            resources.PinnedObjects.Add(pinnedRules);

            pattern.Type = (UInt32)udtPatternType.MidAirFrags;
            pattern.TypeSpecificInfo = pinnedRules.Address;

            return true;
        }

        public static bool CreateMultiRailPatternInfo(ref udtPatternInfo pattern, ArgumentResources resources, udtCutByMultiRailArg rules)
        {
            var pinnedRules = new PinnedObject(rules);
            resources.PinnedObjects.Add(pinnedRules);

            pattern.Type = (UInt32)udtPatternType.MultiFragRails;
            pattern.TypeSpecificInfo = pinnedRules.Address;

            return true;
        }

        public static bool CreateFlagCapturePatternInfo(ref udtPatternInfo pattern, ArgumentResources resources, udtCutByFlagCaptureArg rules)
        {
            var pinnedRules = new PinnedObject(rules);
            resources.PinnedObjects.Add(pinnedRules);

            pattern.Type = (UInt32)udtPatternType.FlagCaptures;
            pattern.TypeSpecificInfo = pinnedRules.Address;

            return true;
        }

        public static bool CreateFlickRailPatternInfo(ref udtPatternInfo pattern, ArgumentResources resources, udtCutByFlickRailArg rules)
        {
            var pinnedRules = new PinnedObject(rules);
            resources.PinnedObjects.Add(pinnedRules);

            pattern.Type = (UInt32)udtPatternType.FlickRails;
            pattern.TypeSpecificInfo = pinnedRules.Address;

            return true;
        }

        public class CutByPatternOptions
        {
            public int StartOffset;
            public int EndOffset;
            public int MaxThreadCount;
            public int PlayerIndex;
            public string PlayerName;

            public CutByPatternOptions(int startOffset, int endOffset, int maxThreadCount, int playerIndex, string playerName)
            {
                StartOffset = startOffset;
                EndOffset = endOffset;
                MaxThreadCount = maxThreadCount;
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

        public static bool CutDemosByFrag(ref udtParseArg parseArg, List<string> filePaths, udtCutByFragArg rules, CutByPatternOptions options)
        {
            var resources = new ArgumentResources();
            var patterns = new udtPatternInfo[1];
            if(!CreateFragPatternInfo(ref patterns[0], resources, rules))
            {
                return false;
            }

            return CutDemosByPattern(resources, ref parseArg, filePaths, patterns, options);
        }

        public static bool CutDemosByMidAir(ref udtParseArg parseArg, List<string> filePaths, udtCutByMidAirArg rules, CutByPatternOptions options)
        {
            var resources = new ArgumentResources();
            var patterns = new udtPatternInfo[1];
            if(!CreateMidAirPatternInfo(ref patterns[0], resources, rules))
            {
                return false;
            }

            return CutDemosByPattern(resources, ref parseArg, filePaths, patterns, options);
        }

        public static bool CutDemosByMultiRail(ref udtParseArg parseArg, List<string> filePaths, udtCutByMultiRailArg rules, CutByPatternOptions options)
        {
            var resources = new ArgumentResources();
            var patterns = new udtPatternInfo[1];
            if(!CreateMultiRailPatternInfo(ref patterns[0], resources, rules))
            {
                return false;
            }

            return CutDemosByPattern(resources, ref parseArg, filePaths, patterns, options);
        }

        public static bool CutDemosByFlagCapture(ref udtParseArg parseArg, List<string> filePaths, udtCutByFlagCaptureArg rules, CutByPatternOptions options)
        {
            var resources = new ArgumentResources();
            var patterns = new udtPatternInfo[1];
            if(!CreateFlagCapturePatternInfo(ref patterns[0], resources, rules))
            {
                return false;
            }

            return CutDemosByPattern(resources, ref parseArg, filePaths, patterns, options);
        }

        public static bool CutDemosByFlickRail(ref udtParseArg parseArg, List<string> filePaths, udtCutByFlickRailArg rules, CutByPatternOptions options)
        {
            var resources = new ArgumentResources();
            var patterns = new udtPatternInfo[1];
            if(!CreateFlickRailPatternInfo(ref patterns[0], resources, rules))
            {
                return false;
            }

            return CutDemosByPattern(resources, ref parseArg, filePaths, patterns, options);
        }

        public static bool CutDemosByPattern(ArgumentResources resources, ref udtParseArg parseArg, List<string> filePaths, udtPatternInfo[] patterns, CutByPatternOptions options)
        {
            var timer = new Stopwatch();
            timer.Start();

            var fileCount = filePaths.Count;
            if(fileCount <= MaxBatchSizeCutting)
            {
                var result = CutDemosByPatternImpl(resources, ref parseArg, filePaths, patterns, options);
                PrintExecutionTime(timer);
                return result;
            }

            var oldProgressCb = parseArg.ProgressCb;
            var progressBase = 0.0f;
            var progressRange = 0.0f;
            var fileIndex = 0;

            var newParseArg = parseArg;
            newParseArg.ProgressCb = delegate(float progress, IntPtr userData)
            {
                var realProgress = progressBase + progressRange * progress;
                oldProgressCb(realProgress, userData);
            };

            var demos = new List<DemoInfo>();
            var batchCount = (fileCount + MaxBatchSizeCutting - 1) / MaxBatchSizeCutting;
            var filesPerBatch = fileCount / batchCount;
            var success = true;
            for(int i = 0; i < batchCount; ++i)
            {
                progressBase = (float)fileIndex / (float)fileCount;
                var currentFileCount = (i == batchCount - 1) ? (fileCount - fileIndex) : filesPerBatch;
                var currentFiles = filePaths.GetRange(fileIndex, currentFileCount);
                progressRange = (float)currentFileCount / (float)fileCount;

                var newResources = new ArgumentResources();
                CutDemosByPatternImpl(newResources, ref newParseArg, currentFiles, patterns, options);

                fileIndex += currentFileCount;

                if(Marshal.ReadInt32(parseArg.CancelOperation) != 0)
                {
                    success = false;
                    break;
                }
            }

            resources.Free();

            PrintExecutionTime(timer);

            return success;
        }

        private static bool CutDemosByPatternImpl(ArgumentResources resources, ref udtParseArg parseArg, List<string> filePaths, udtPatternInfo[] patterns, CutByPatternOptions options)
        {
            var errorCodeArray = new Int32[filePaths.Count];
            var filePathArray = new IntPtr[filePaths.Count];
            for(var i = 0; i < filePaths.Count; ++i)
            {
                filePathArray[i] = Marshal.StringToHGlobalAnsi(Path.GetFullPath(filePaths[i]));
                resources.GlobalAllocationHandles.Add(filePathArray[i]);
            }

            parseArg.PlugInCount = 0;
            parseArg.PlugIns = IntPtr.Zero;

            var pinnedFilePaths = new PinnedObject(filePathArray);
            var pinnedErrorCodes = new PinnedObject(errorCodeArray);
            var multiParseArg = new udtMultiParseArg();
            multiParseArg.FileCount = (UInt32)filePathArray.Length;
            multiParseArg.FilePaths = pinnedFilePaths.Address;
            multiParseArg.OutputErrorCodes = pinnedErrorCodes.Address;
            multiParseArg.MaxThreadCount = (UInt32)options.MaxThreadCount;

            var playerNameUnmanaged = IntPtr.Zero;
            if(!string.IsNullOrEmpty(options.PlayerName))
            {
                playerNameUnmanaged = Marshal.StringToHGlobalAnsi(options.PlayerName);
                resources.GlobalAllocationHandles.Add(playerNameUnmanaged);
            }

            var pinnedPatterns = new PinnedObject(patterns);
            var cutByPatternArg = new udtCutByPatternArg();
            cutByPatternArg.StartOffsetSec = (UInt32)options.StartOffset;
            cutByPatternArg.EndOffsetSec = (UInt32)options.EndOffset;
            cutByPatternArg.Patterns = pinnedPatterns.Address;
            cutByPatternArg.PatternCount = (UInt32)patterns.Length;
            cutByPatternArg.PlayerIndex = options.PlayerIndex;
            cutByPatternArg.PlayerName = playerNameUnmanaged;

            resources.PinnedObjects.Add(pinnedPatterns);
            resources.PinnedObjects.Add(pinnedFilePaths);
            resources.PinnedObjects.Add(pinnedErrorCodes);

            udtErrorCode result = udtErrorCode.OperationFailed;
            try
            {
                result = udtCutDemoFilesByPattern(ref parseArg, ref multiParseArg, ref cutByPatternArg);
            }
            finally
            {
                resources.Free();
            }            

            return result == udtErrorCode.None;
        }

        public static bool ConvertDemos(ref udtParseArg parseArg, udtProtocol outProtocol, List<MapConversionRule> mapRules, List<string> filePaths, int maxThreadCount)
        {
            var timer = new Stopwatch();
            timer.Start();

            var fileCount = filePaths.Count;
            if(fileCount <= MaxBatchSizeConverting)
            {
                var result = ConvertDemosImpl(ref parseArg, outProtocol, mapRules, filePaths, maxThreadCount);
                PrintExecutionTime(timer);
                return result;
            }

            var oldProgressCb = parseArg.ProgressCb;
            var progressBase = 0.0f;
            var progressRange = 0.0f;
            var fileIndex = 0;

            var newParseArg = parseArg;
            newParseArg.ProgressCb = delegate(float progress, IntPtr userData)
            {
                var realProgress = progressBase + progressRange * progress;
                oldProgressCb(realProgress, userData);
            };

            var batchCount = (fileCount + MaxBatchSizeConverting - 1) / MaxBatchSizeConverting;
            var filesPerBatch = fileCount / batchCount;
            for(int i = 0; i < batchCount; ++i)
            {
                progressBase = (float)fileIndex / (float)fileCount;
                var currentFileCount = (i == batchCount - 1) ? (fileCount - fileIndex) : filesPerBatch;
                var currentFiles = filePaths.GetRange(fileIndex, currentFileCount);
                progressRange = (float)currentFileCount / (float)fileCount;
                fileIndex += currentFileCount;

                ConvertDemosImpl(ref newParseArg, outProtocol, mapRules, currentFiles, maxThreadCount);

                if(Marshal.ReadInt32(parseArg.CancelOperation) != 0)
                {
                    break;
                }
            }

            PrintExecutionTime(timer);

            return true;
        }

        private static bool ConvertDemosImpl(ref udtParseArg parseArg, udtProtocol outProtocol, List<MapConversionRule> mapRules, List<string> filePaths, int maxThreadCount)
        {
            var resources = new ArgumentResources();
            var errorCodeArray = new Int32[filePaths.Count];
            var filePathArray = new IntPtr[filePaths.Count];
            for(var i = 0; i < filePaths.Count; ++i)
            {
                var filePath = Marshal.StringToHGlobalAnsi(Path.GetFullPath(filePaths[i]));
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
            multiParseArg.MaxThreadCount = (UInt32)maxThreadCount;

            var conversionArg = new udtProtocolConversionArg();
            conversionArg.OutputProtocol = (UInt32)outProtocol;
            conversionArg.MapRules = IntPtr.Zero;
            conversionArg.MapRuleCount = 0;
            if(mapRules.Count > 0)
            {
                var mapRuleArray = new udtMapConversionRule[mapRules.Count];
                for(var i = 0; i < mapRules.Count; ++i)
                {
                    var inputName = Marshal.StringToHGlobalAnsi(mapRules[i].InputName);
                    var outputName = Marshal.StringToHGlobalAnsi(mapRules[i].OutputName);
                    mapRuleArray[i].InputName = inputName;
                    mapRuleArray[i].OutputName = outputName;
                    mapRuleArray[i].PositionOffsetX = mapRules[i].OffsetX;
                    mapRuleArray[i].PositionOffsetY = mapRules[i].OffsetY;
                    mapRuleArray[i].PositionOffsetZ = mapRules[i].OffsetZ;
                    resources.GlobalAllocationHandles.Add(inputName);
                    resources.GlobalAllocationHandles.Add(outputName);
                }
                var pinnedMapRules = new PinnedObject(mapRuleArray);
                resources.PinnedObjects.Add(pinnedMapRules);
                conversionArg.MapRules = pinnedMapRules.Address;
                conversionArg.MapRuleCount = (UInt32)mapRuleArray.Length;
            }

            var result = udtErrorCode.OperationFailed;
            try
            {
                result = udtConvertDemoFiles(ref parseArg, ref multiParseArg, ref conversionArg);
            }
            finally
            {
                resources.Free();
            }

            return result != udtErrorCode.None;
        }

        public static List<DemoInfo> ParseDemos(ref udtParseArg parseArg, List<string> filePaths, int maxThreadCount)
        {
            var timer = new Stopwatch();
            timer.Start();

            var fileCount = filePaths.Count;
            if(fileCount <= MaxBatchSizeParsing)
            {
                var result = ParseDemosImpl(ref parseArg, filePaths, maxThreadCount, 0);
                PrintExecutionTime(timer);
                return result;
            }

            var oldProgressCb = parseArg.ProgressCb;
            var progressBase = 0.0f;
            var progressRange = 0.0f;
            var fileIndex = 0;

            var newParseArg = parseArg;
            newParseArg.ProgressCb = delegate(float progress, IntPtr userData)
            {
                var realProgress = progressBase + progressRange * progress;
                oldProgressCb(realProgress, userData);
            };

            var demos = new List<DemoInfo>();
            var batchCount = (fileCount + MaxBatchSizeParsing - 1) / MaxBatchSizeParsing;
            var filesPerBatch = fileCount / batchCount;
            for(int i = 0; i < batchCount; ++i)
            {
                progressBase = (float)fileIndex / (float)fileCount;
                var currentFileCount = (i == batchCount - 1) ? (fileCount - fileIndex) : filesPerBatch;
                var currentFiles = filePaths.GetRange(fileIndex, currentFileCount);
                progressRange = (float)currentFileCount / (float)fileCount;

                var currentResults = ParseDemosImpl(ref newParseArg, currentFiles, maxThreadCount, fileIndex);
                demos.AddRange(currentResults);

                fileIndex += currentFileCount;

                if(Marshal.ReadInt32(parseArg.CancelOperation) != 0)
                {
                    break;
                }
            }

            PrintExecutionTime(timer);

            return demos;
        }

        private static List<DemoInfo> ParseDemosImpl(ref udtParseArg parseArg, List<string> filePaths, int maxThreadCount, int inputIndexBase)
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
                    info.Analyzed = true;
                    info.InputIndex = inputIndexBase + (int)inputIdx;
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
                App.GlobalLogError("Calling udtGetDemoDataInfo for chat messages failed");
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

        private static string SafeGetString(IntPtr address, string onError)
        {
            return Marshal.PtrToStringAnsi(address) ?? onError;
        }

        private static string SafeGetString(IntPtr address)
        {
            return Marshal.PtrToStringAnsi(address) ?? "";
        }

        private static string FormatDemoTaker(udtParseDataGameState info)
        {
            var name = SafeGetString(info.DemoTakerName, "N/A");

            return string.Format("{0} (player index {1})", name, info.DemoTakerPlayerIndex);
        }

        private static void AddKeyValuePairs(DemoInfo info, udtParseDataGameState data, string space)
        {
            for(uint i = 0; i < data.KeyValuePairCount; ++i)
            {
                var address = new IntPtr(data.KeyValuePairs.ToInt64() + i * sizeof(udtGameStateKeyValuePair));
                var kvPair = (udtGameStateKeyValuePair)Marshal.PtrToStructure(address, typeof(udtGameStateKeyValuePair));

                var key = SafeGetString(kvPair.Name, "N/A");
                var value = SafeGetString(kvPair.Value, "N/A");
                info.Generic.Add(Tuple.Create(space + key, value));
            }
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

        private static void AddPlayers(DemoInfo info, udtParseDataGameState data, string space)
        {
            for(uint i = 0; i < data.PlayerCount; ++i)
            {
                var address = new IntPtr(data.Players.ToInt64() + i * sizeof(udtGameStatePlayerInfo));
                var player = (udtGameStatePlayerInfo)Marshal.PtrToStructure(address, typeof(udtGameStatePlayerInfo));

                var desc = space + "Client Number " + player.Index.ToString();
                var startTime = FormatMinutesSecondsFromMs(player.FirstSnapshotTimeMs);
                var endTime = FormatMinutesSecondsFromMs(player.LastSnapshotTimeMs);
                var time = startTime + " - " + endTime;
                var name = SafeGetString(player.FirstName, "N/A");
                var value = string.Format("{0}, {1}, team {2}", name, time, GetTeamName(player.FirstTeam));

                info.Generic.Add(Tuple.Create(desc, value));
            }
        }

        private static void AddMatches(DemoInfo info, udtParseDataGameState data, string space)
        {
            var matchCount = data.MatchCount;
            if(matchCount == 0)
            {
                return;
            }

            info.Generic.Add(Tuple.Create(space + "Matches", data.MatchCount.ToString()));
            for(uint i = 0; i < matchCount; ++i)
            {
                var matchAddress = new IntPtr(data.Matches.ToInt64() + i * sizeof(udtMatchInfo));
                var matchData = (udtMatchInfo)Marshal.PtrToStructure(matchAddress, typeof(udtMatchInfo));

                var desc = space + "Match #" + (i + 1).ToString();
                var start = FormatMinutesSecondsFromMs(matchData.MatchStartTimeMs);
                var end = FormatMinutesSecondsFromMs(matchData.MatchEndTimeMs);
                var val = start + " - " + end;
                info.Generic.Add(Tuple.Create(desc, val));
            }
        }

        private static void ExtractGameStateEvents(udtParserContextRef context, uint demoIdx, ref DemoInfo info)
        {
            uint gsEventCount = 0;
            IntPtr gsEvents = IntPtr.Zero;
            if(udtGetDemoDataInfo(context, demoIdx, udtParserPlugIn.GameState, ref gsEvents, ref gsEventCount) != udtErrorCode.None)
            {
                App.GlobalLogError("Calling udtGetDemoDataInfo for game states failed");
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
                info.GameStateSnapshotTimesMs.Add(Tuple.Create(data.FirstSnapshotTimeMs, data.LastSnapshotTimeMs));
                info.Generic.Add(Tuple.Create("GameState #" + (i + 1).ToString(), ""));
                info.Generic.Add(Tuple.Create(space + "File Offset", FormatBytes(data.FileOffset)));
                info.Generic.Add(Tuple.Create(space + "Server Time Range", firstSnapTime + " - " + lastSnapTime));
                info.Generic.Add(Tuple.Create(space + "Demo Taker", FormatDemoTaker(data)));
                AddMatches(info, data, space);
                AddPlayers(info, data, space);
                AddKeyValuePairs(info, data, space);
            }
        }

        private static void ExtractObituaries(udtParserContextRef context, uint demoIdx, ref DemoInfo info)
        {
            uint obituaryEventCount = 0;
            IntPtr obituaryEvents = IntPtr.Zero;
            if(udtGetDemoDataInfo(context, demoIdx, udtParserPlugIn.Obituaries, ref obituaryEvents, ref obituaryEventCount) != udtErrorCode.None)
            {
                App.GlobalLogError("Calling udtGetDemoDataInfo for obituaries failed");
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

        private static void PrintExecutionTime(Stopwatch timer)
        {
            if(!App.Instance.Config.PrintExecutionTime)
            {
                return;
            }

            timer.Stop();
            App.GlobalLogInfo("Job execution time: " + App.FormatPerformanceTime(timer));
        }
    }
}