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

    public unsafe class UDT_DLL
    {
#if (UDT_X86)
        private const int MaxBatchSizeParsing = 128;
        private const int MaxBatchSizeJSONExport = 128;
        private const int MaxBatchSizeCutting = 512;
        private const int MaxBatchSizeConverting = 512;
        private const int MaxBatchSizeTimeShifting = 512;
#else
        private const int MaxBatchSizeParsing = 512;
        private const int MaxBatchSizeJSONExport = 512;
        private const int MaxBatchSizeCutting = 2048;
        private const int MaxBatchSizeConverting = 2048;
        private const int MaxBatchSizeTimeShifting = 2048;
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
            Dm3,
            Dm48,
            Dm66,
            Dm67,
            Dm68,
            Dm73,
            Dm90,
            Dm91
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
            GameTypes,
            ShortGameTypes,
            ModNames,
            GamePlayNames,
            ShortGamePlayNames,
            OverTimeTypes,
            TeamStatsNames,
            PlayerStatsNames,
            PlugInNames,
            Count
        }

        public enum udtByteArray : uint
        {
            TeamStatsCompModes,
            PlayerStatsCompModes,
            TeamStatsDataTypes,
            PlayerStatsDataTypes,
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
            Count
        }

        public enum udtStatsCompMode : uint
        {
            NeitherWins,
            BiggerWins,
            SmallerWins,
            Count
        };

        public enum udtStatsDataType : uint
        {
            Generic,
            Team,
            Minutes,
            Seconds,
            Percentage,
            Weapon,
            Ping,
            Count
        };

        private enum udtGameType : uint
        {
            SP,
            FFA,
            Duel,
            Race,
            HM,
            TDM,
            CA,
            CTF,
            OneFlagCTF,
            Obelisk,
            Harvester,
            Domination,
            CTFS,
            RedRover,
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

        [Flags]
        public enum udtCutByPatternArgFlags
        {
            MergeCutSections = 1 << 0
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
	    public struct udtCutByPatternArg
	    {
		    public IntPtr Patterns; // const udtPatternInfo*
            public IntPtr PlayerName; // const char*
		    public UInt32 PatternCount;
		    public UInt32 StartOffsetSec;
		    public UInt32 EndOffsetSec;
		    public Int32 PlayerIndex;
            public UInt32 Flags;
            public Int32 Reserved1;
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
            public UInt32 SearchTeamChat;
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
            public UInt32 MinSpeedSnapshotCount;
            public float MinAngleDelta;
            public UInt32 MinAngleDeltaSnapshotCount;
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
            public IntPtr Location; // const char*
            public IntPtr Reserved1; // const char*
            public IntPtr OriginalCommandNoCol; // const char*
            public IntPtr ClanNameNoCol; // const char*
            public IntPtr PlayerNameNoCol; // const char*
            public IntPtr MessageNoCol; // const char*
            public IntPtr LocationNoCol; // const char*
            public IntPtr Reserved2; // const char*
		    public Int32 ServerTimeMs;
		    public Int32 PlayerIndex;
            public Int32 GameStateIndex;
            public UInt32 TeamMessage;
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

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        struct udtPlayerStats
	    {
		    public IntPtr Name; // const char*
		    public IntPtr CleanName; // const char*
	    };

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
	    struct udtParseDataStats
	    {
		    public UInt64 ValidTeams;
		    public UInt64 ValidPlayers;
		    public IntPtr TeamFlags; // const u8*
		    public IntPtr PlayerFlags; // const u8*
		    public IntPtr TeamFields; // const s32*
		    public IntPtr PlayerFields; // const s32*
		    public IntPtr PlayerStats; // const udtPlayerStats*
		    public IntPtr ModVersion; // const char*
		    public IntPtr Map; // const char*
            public IntPtr FirstPlaceName; // const char*
            public IntPtr SecondPlaceName; // const char*
            public IntPtr CustomRedName; // const char*
            public IntPtr CustomBlueName; // const char*
            public IntPtr Reserved1;
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
	    };

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        struct udtTimeShiftArg
        {
            public Int32 SnapshotCount;
            public Int32 Reserved1;
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
        extern static private udtErrorCode udtGetByteArray(udtByteArray arrayId, ref IntPtr array, ref UInt32 elementCount);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtGetStatsConstants(ref UInt32 playerMaskByteCount, ref UInt32 teamMaskByteCount, ref UInt32 playerFieldCount, ref UInt32 teamFieldCount);

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
        extern static private udtErrorCode udtMergeDemoFiles(ref udtParseArg info, IntPtr filePaths, UInt32 fileCount);

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

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtTimeShiftDemoFiles(ref udtParseArg info, ref udtMultiParseArg extraInfo, ref udtTimeShiftArg timeShiftArg);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private udtErrorCode udtSaveDemoFilesAnalysisDataToJSON(ref udtParseArg info, ref udtMultiParseArg extraInfo);

        public class StatsConstantsGrabber
        {
            public StatsConstantsGrabber()
            {
                UInt32 playerMaskByteCount = 0;
                UInt32 teamMaskByteCount = 0;
                UInt32 playerFieldCount = 0;
                UInt32 teamFieldCount = 0;
                udtGetStatsConstants(ref playerMaskByteCount, ref teamMaskByteCount, ref playerFieldCount, ref teamFieldCount);
                PlayerMaskByteCount = (int)playerMaskByteCount;
                TeamMaskByteCount = (int)teamMaskByteCount;
                PlayerFieldCount = (int)playerFieldCount;
                TeamFieldCount = (int)teamFieldCount;
            }

            public int PlayerMaskByteCount { private set; get; }
            public int TeamMaskByteCount { private set; get; }
            public int PlayerFieldCount { private set; get; }
            public int TeamFieldCount { private set; get; }
        }

        public static readonly StatsConstantsGrabber StatsConstants = new StatsConstantsGrabber();

        // The list of plug-ins activated when loading demos.
        private static UInt32[] PlugInArray = new UInt32[] 
        { 
            (UInt32)udtParserPlugIn.Chat, 
            (UInt32)udtParserPlugIn.GameState,
            (UInt32)udtParserPlugIn.Obituaries,
            (UInt32)udtParserPlugIn.Stats
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

        public static bool MergeDemos(ref udtParseArg parseArg, List<string> filePaths)
        {
            var resources = new ArgumentResources();
            var filePathsArray = new IntPtr[filePaths.Count];
            for(var i = 0; i < filePaths.Count; ++i)
            {
                filePathsArray[i] = Marshal.StringToHGlobalAnsi(filePaths[i]);
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
                rulesArray[i].SearchTeamChat = (UInt32)(rules[i].SearchTeamMessages ? 1 : 0);
                resources.GlobalAllocationHandles.Add(rulesArray[i].Pattern);
            }
            var pinnedRulesArray = new PinnedObject(rulesArray);

            var cutByChatArg = new udtCutByChatArg();
            cutByChatArg.Rules = pinnedRulesArray.Address;
            cutByChatArg.RuleCount = (UInt32)rulesArray.Length;
            var pinnedRules = new PinnedObject(cutByChatArg);

            resources.PinnedObjects.Add(pinnedRulesArray);
            resources.PinnedObjects.Add(pinnedRules);

            pattern.Type = (UInt32)udtPatternType.Chat;
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
            var runner = new BatchJobRunner(parseArg, filePaths, MaxBatchSizeCutting);
            var newParseArg = runner.NewParseArg;

            var batchCount = runner.BatchCount;
            for(var i = 0; i < batchCount; ++i)
            {
                CutDemosByPatternImpl(resources, ref newParseArg, runner.GetNextFiles(i), patterns, options);
                resources.Free();
                if(runner.IsCanceled(parseArg.CancelOperation))
                {
                    break;
                }
            }

            PrintExecutionTime(runner.Timer);

            return true;
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
            cutByPatternArg.Flags = 0;
            if(options.MergeCutSections)
            {
                cutByPatternArg.Flags |= (UInt32)udtCutByPatternArgFlags.MergeCutSections;
            }

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
            var runner = new BatchJobRunner(parseArg, filePaths, MaxBatchSizeConverting);
            var newParseArg = runner.NewParseArg;

            var batchCount = runner.BatchCount;
            for(var i = 0; i < batchCount; ++i)
            {
                ConvertDemosImpl(ref newParseArg, outProtocol, mapRules, runner.GetNextFiles(i), maxThreadCount);
                if(runner.IsCanceled(parseArg.CancelOperation))
                {
                    break;
                }
            }

            PrintExecutionTime(runner.Timer);

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

        public static bool TimeShiftDemos(ref udtParseArg parseArg, List<string> filePaths, int maxThreadCount, int snapshotCount)
        {
            var runner = new BatchJobRunner(parseArg, filePaths, MaxBatchSizeTimeShifting);
            var newParseArg = runner.NewParseArg;

            var batchCount = runner.BatchCount;
            for(var i = 0; i < batchCount; ++i)
            {
                TimeShiftDemosImpl(ref newParseArg, runner.GetNextFiles(i), maxThreadCount, snapshotCount);
                if(runner.IsCanceled(parseArg.CancelOperation))
                {
                    break;
                }
            }

            PrintExecutionTime(runner.Timer);

            return true;
        }

        private static bool TimeShiftDemosImpl(ref udtParseArg parseArg, List<string> filePaths, int maxThreadCount, int snapshotCount)
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

            var timeShiftArg = new udtTimeShiftArg();
            timeShiftArg.SnapshotCount = snapshotCount;

            var result = udtErrorCode.OperationFailed;
            try
            {
                result = udtTimeShiftDemoFiles(ref parseArg, ref multiParseArg, ref timeShiftArg);
            }
            finally
            {
                resources.Free();
            }

            return result != udtErrorCode.None;
        }

        public static bool ExportDemosDataToJSON(ref udtParseArg parseArg, List<string> filePaths, int maxThreadCount, UInt32[] plugIns)
        {
            var runner = new BatchJobRunner(parseArg, filePaths, MaxBatchSizeJSONExport);
            var newParseArg = runner.NewParseArg;

            var batchCount = runner.BatchCount;
            for(var i = 0; i < batchCount; ++i)
            {
                ExportDemosDataToJSONImpl(ref newParseArg, runner.GetNextFiles(i), maxThreadCount, plugIns);
                if(runner.IsCanceled(parseArg.CancelOperation))
                {
                    break;
                }
            }

            PrintExecutionTime(runner.Timer);

            return true;
        }

        private static bool ExportDemosDataToJSONImpl(ref udtParseArg parseArg, List<string> filePaths, int maxThreadCount, UInt32[] plugIns)
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

            var pinnedPlugIns = new PinnedObject(plugIns);
            parseArg.PlugInCount = (UInt32)plugIns.Length;
            parseArg.PlugIns = pinnedPlugIns.Address;

            var pinnedFilePaths = new PinnedObject(filePathArray);
            var pinnedErrorCodes = new PinnedObject(errorCodeArray);
            resources.PinnedObjects.Add(pinnedPlugIns);
            resources.PinnedObjects.Add(pinnedFilePaths);
            resources.PinnedObjects.Add(pinnedErrorCodes);
            var multiParseArg = new udtMultiParseArg();
            multiParseArg.FileCount = (UInt32)filePathArray.Length;
            multiParseArg.FilePaths = pinnedFilePaths.Address;
            multiParseArg.OutputErrorCodes = pinnedErrorCodes.Address;
            multiParseArg.MaxThreadCount = (UInt32)maxThreadCount;

            var result = udtErrorCode.OperationFailed;
            try
            {
                result = udtSaveDemoFilesAnalysisDataToJSON(ref parseArg, ref multiParseArg);
            }
            finally
            {
                resources.Free();
            }

            return result != udtErrorCode.None;
        }

        public static List<DemoInfo> ParseDemos(ref udtParseArg parseArg, List<string> filePaths, int maxThreadCount)
        {
            var runner = new BatchJobRunner(parseArg, filePaths, MaxBatchSizeParsing);
            var newParseArg = runner.NewParseArg;

            var demos = new List<DemoInfo>();
            var batchCount = runner.BatchCount;
            for(var i = 0; i < batchCount; ++i)
            {
                var fileIndex = runner.FileIndex;
                var files = runner.GetNextFiles(i);
                var currentResults = ParseDemosImpl(ref newParseArg, files, maxThreadCount, fileIndex);
                demos.AddRange(currentResults);
                if(runner.IsCanceled(parseArg.CancelOperation))
                {
                    break;
                }
            }

            PrintExecutionTime(runner.Timer);

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
                    info.ProtocolNumber = UDT_DLL.udtGetProtocolByFilePath(info.FilePath);
                    
                    ExtractDemoInfo(context, j, info);
                    infoList.Add(info);
                }
            }

            udtDestroyContextGroup(contextGroup);

            // Keep the original input order.
            infoList.Sort((a, b) => a.InputIndex - b.InputIndex);

            return infoList;
        }

        private static void ExtractDemoInfo(udtParserContextRef context, uint demoIdx, DemoInfo info)
        {
            ExtractChatEvents(context, demoIdx, info);
            ExtractGameStateEvents(context, demoIdx, info);
            ExtractObituaries(context, demoIdx, info);
            ExtractStats(context, demoIdx, info);
        }

        private static void ExtractChatEvents(udtParserContextRef context, uint demoIdx, DemoInfo info)
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
                var player = SafeGetUTF8String(data.PlayerNameNoCol, "N/A");
                var message = SafeGetUTF8String(data.MessageNoCol, "N/A");
                var item = new ChatEventDisplayInfo(data.GameStateIndex, time, player, message, data.TeamMessage != 0);
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

        private static string FormatDemoTaker(udtParseDataGameState info)
        {
            var name = SafeGetUTF8String(info.DemoTakerName, "N/A");

            return string.Format("{0} (player index {1})", name, info.DemoTakerPlayerIndex);
        }

        private static void AddKeyValuePairs(DemoInfo info, udtParseDataGameState data, string space)
        {
            for(uint i = 0; i < data.KeyValuePairCount; ++i)
            {
                var address = new IntPtr(data.KeyValuePairs.ToInt64() + i * sizeof(udtGameStateKeyValuePair));
                var kvPair = (udtGameStateKeyValuePair)Marshal.PtrToStructure(address, typeof(udtGameStateKeyValuePair));

                var key = SafeGetUTF8String(kvPair.Name, "N/A");
                var value = SafeGetUTF8String(kvPair.Value, "N/A");
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
                var name = SafeGetUTF8String(player.FirstName, "N/A");
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

        private static void ExtractGameStateEvents(udtParserContextRef context, uint demoIdx, DemoInfo info)
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

        private static void ExtractObituaries(udtParserContextRef context, uint demoIdx, DemoInfo info)
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
                var attacker = SafeGetUTF8String(data.AttackerName, "N/A");
                var target = SafeGetUTF8String(data.TargetName, "N/A");
                var mod = SafeGetUTF8String(data.MeanOfDeathName, "N/A");
                var item = new FragEventDisplayInfo(data.GameStateIndex, time, attacker, target, mod);
                info.FragEvents.Add(item);
            }
        }

        private static void ExtractStats(udtParserContextRef context, uint demoIdx, DemoInfo info)
        {
            uint statsCount = 0;
            IntPtr stats = IntPtr.Zero;
            if(udtGetDemoDataInfo(context, demoIdx, udtParserPlugIn.Stats, ref stats, ref statsCount) != udtErrorCode.None)
            {
                App.GlobalLogError("Calling udtGetDemoDataInfo for stats failed");
                return;
            }

            for(uint i = 0; i < statsCount; ++i)
            {
                var address = new IntPtr(stats.ToInt64() + i * sizeof(udtParseDataStats));
                var data = (udtParseDataStats)Marshal.PtrToStructure(address, typeof(udtParseDataStats));
                ExtractStatsSingleMatch(data, info);
            }
        }
        
        private static void ExtractStatsSingleMatch(udtParseDataStats data, DemoInfo info)
        {
            var stats = new DemoStatsInfo();

            var name1 = SafeGetUTF8String(data.FirstPlaceName);
            var name2 = SafeGetUTF8String(data.SecondPlaceName);
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

            var redTeamName = data.CustomRedName != IntPtr.Zero ? Marshal.PtrToStringAnsi(data.CustomRedName) : null;
            var blueTeamName = data.CustomBlueName != IntPtr.Zero ? Marshal.PtrToStringAnsi(data.CustomBlueName) : null;
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
                stats.AddGenericField("Mod version", data.ModVersion);
            }
            stats.AddGenericField("Game type", GetUDTStringForValueOrNull(udtStringArray.GameTypes, data.GameType));
            stats.AddGenericField("Game play", GetUDTStringForValueOrNull(udtStringArray.GamePlayNames, data.GamePlay));
            stats.AddGenericField("Map name", data.Map);
            stats.AddGenericField("Match duration", App.FormatMinutesSeconds((int)GetFixedMatchDuration(data) / 1000));
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

            ExtractTeamStats(data, info, ref stats);
            ExtractPlayerStats(data, info, ref stats);

            info.MatchStats.Add(stats);
        }

        private static void ExtractTeamStats(udtParseDataStats data, DemoInfo info, ref DemoStatsInfo stats)
        {
            // For the GUI, we'll be picky and only bother if both teams have stats.
            if((data.ValidTeams & (ulong)3) != (ulong)3)
            {
                return;
            }

            IntPtr fieldNames = IntPtr.Zero;
            UInt32 fieldNameCount = 0;
            if(udtGetStringArray(udtStringArray.TeamStatsNames, ref fieldNames, ref fieldNameCount) != udtErrorCode.None)
            {
                return;
            }
            var pointerSize = Marshal.SizeOf(typeof(IntPtr));

            IntPtr fieldCompModes = IntPtr.Zero;
            UInt32 fieldCompModeCount = 0;
            if(udtGetByteArray(udtByteArray.TeamStatsCompModes, ref fieldCompModes, ref fieldCompModeCount) != udtErrorCode.None)
            {
                return;
            }

            IntPtr fieldDataTypes = IntPtr.Zero;
            UInt32 fieldDataTypeCount = 0;
            if(udtGetByteArray(udtByteArray.TeamStatsDataTypes, ref fieldDataTypes, ref fieldDataTypeCount) != udtErrorCode.None)
            {
                return;
            }

            var fieldIdx = 0;
            var flagsByteOffset = 0;
            for(int i = 0; i < 2; ++i)
            {
                var teamStats = new StatsInfoGroup();
                teamStats.Name = i == 0 ? "RED" : "BLUE";

                var customTeamNameAddress = i == 0 ? data.CustomRedName : data.CustomBlueName;
                var customTeamName = customTeamNameAddress != IntPtr.Zero ? Marshal.PtrToStringAnsi(customTeamNameAddress) : null;
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
                    var byteValue = Marshal.ReadByte(data.TeamFlags, byteIndex + flagsByteOffset);
                    if((byteValue & (byte)(1 << bitIndex)) != 0)
                    {
                        var dataType = (udtStatsDataType)Marshal.ReadByte(fieldDataTypes, j);
                        var field = new DemoStatsField();
                        var fieldName = "";
                        var fieldValue = "";
                        var fieldIntegerValue = Marshal.ReadInt32(data.TeamFields, fieldIdx * 4);
                        var fieldNameAddress = Marshal.ReadIntPtr(fieldNames, j * pointerSize);
                        FormatStatsField(out fieldName, out fieldValue, fieldIntegerValue, dataType, fieldNameAddress);
                        field.Key = fieldName;
                        field.Value = fieldValue;
                        field.IntegerValue = fieldIntegerValue;
                        field.FieldBitIndex = j;
                        field.ComparisonMode = (udtStatsCompMode)Marshal.ReadByte(fieldCompModes, j);
                        teamStats.Fields.Add(field);
                        ++fieldIdx;
                    }
                }

                stats.TeamStats.Add(teamStats);

                flagsByteOffset += StatsConstants.TeamMaskByteCount;
            }
        }

        private static void ExtractPlayerStats(udtParseDataStats data, DemoInfo info, ref DemoStatsInfo stats)
        {
            IntPtr fieldNames = IntPtr.Zero;
            UInt32 fieldNameCount = 0;
            if(udtGetStringArray(udtStringArray.PlayerStatsNames, ref fieldNames, ref fieldNameCount) != udtErrorCode.None)
            {
                return;
            }
            var pointerSize = Marshal.SizeOf(typeof(IntPtr));
            
            IntPtr fieldCompModes = IntPtr.Zero;
            UInt32 fieldCompModeCount = 0;
            if(udtGetByteArray(udtByteArray.PlayerStatsCompModes, ref fieldCompModes, ref fieldCompModeCount) != udtErrorCode.None)
            {
                return;
            }

            IntPtr fieldDataTypes = IntPtr.Zero;
            UInt32 fieldDataTypeCount = 0;
            if(udtGetByteArray(udtByteArray.PlayerStatsDataTypes, ref fieldDataTypes, ref fieldDataTypeCount) != udtErrorCode.None)
            {
                return;
            }

            var extraInfoAddress = data.PlayerStats.ToInt64();
            var extraInfoItemSize = Marshal.SizeOf(typeof(udtPlayerStats));
            var fieldIdx = 0;
            var flagsByteOffset = 0;
            for(int i = 0; i < 64; ++i)
            {
                if((data.ValidPlayers & ((ulong)1 << i)) == 0)
                {
                    continue;
                }

                var playerStats = new StatsInfoGroup();
                var extraInfo = (udtPlayerStats)Marshal.PtrToStructure(new IntPtr(extraInfoAddress), typeof(udtPlayerStats));
                playerStats.Name = SafeGetUTF8String(extraInfo.CleanName);
                for(int j = 0; j < StatsConstants.PlayerFieldCount; ++j)
                {
                    var byteIndex = j / 8;
                    var bitIndex = j % 8;
                    var byteValue = Marshal.ReadByte(data.PlayerFlags, byteIndex + flagsByteOffset);
                    if((byteValue & (byte)(1 << bitIndex)) != 0)
                    {
                        var dataType = (udtStatsDataType)Marshal.ReadByte(fieldDataTypes, j);
                        var field = new DemoStatsField();
                        var fieldName = "";
                        var fieldValue = "";
                        var fieldIntegerValue = Marshal.ReadInt32(data.PlayerFields, fieldIdx * 4);
                        var fieldNameAddress = Marshal.ReadIntPtr(fieldNames, j * pointerSize);
                        FormatStatsField(out fieldName, out fieldValue, fieldIntegerValue, dataType, fieldNameAddress);
                        field.Key = fieldName;
                        field.Value = fieldValue;
                        field.IntegerValue = fieldIntegerValue;
                        field.FieldBitIndex = j;
                        field.ComparisonMode = (udtStatsCompMode)Marshal.ReadByte(fieldCompModes, j);
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

                extraInfoAddress += extraInfoItemSize;
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
            foreach(var player in stats.PlayerStats)
            {
                var teamIdx = player.Fields.FindIndex(f => f.FieldBitIndex == 0);
                if(teamIdx >= 0)
                {
                    player.Fields.RemoveAt(teamIdx);
                }
            }
        }

        private static void FormatStatsField(out string fieldName, out string fieldValue, int fieldIntegerValue, udtStatsDataType dataType, IntPtr fieldNameAddress)
        {
            fieldName = dataType == udtStatsDataType.Team ? "Team" : GetStatFieldNameFromAddress(fieldNameAddress);

            switch(dataType)
            {
                case udtStatsDataType.Team:
                    fieldValue = GetUDTStringForValueOrNull(udtStringArray.Teams, (uint)fieldIntegerValue) ?? "N/A";
                    break;

                case udtStatsDataType.Weapon:
                    fieldValue = GetUDTStringForValueOrNull(udtStringArray.Weapons, (uint)fieldIntegerValue) ?? "N/A";
                    break;

                case udtStatsDataType.Percentage:
                    fieldValue = fieldIntegerValue.ToString() + "%";
                    break;

                case udtStatsDataType.Minutes:
                    fieldValue = fieldIntegerValue.ToString() + (fieldIntegerValue > 1 ? " minutes" : "minute");
                    break;

                case udtStatsDataType.Seconds:
                    fieldValue = FormatStatsSeconds(fieldIntegerValue);
                    break;

                case udtStatsDataType.Ping:
                    fieldValue = fieldIntegerValue.ToString() + " ms";
                    break;

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

        private static string GetStatFieldNameFromAddress(IntPtr fieldNameAddress)
        {
            var fieldName = Marshal.PtrToStringAnsi(fieldNameAddress) ?? "???";

            return ProcessStatsFieldName(fieldName);
        }

        private static string ProcessStatsFieldName(string name)
        {
            return name.Replace("bfg", "BFG").Replace("possession", "poss.").Capitalize();
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

        private class BatchJobRunner
        {
            public BatchJobRunner(udtParseArg parseArg, List<string> filePaths, int maxBatchSize)
            {
                var fileCount = filePaths.Count;

                _filePaths = filePaths;
                _stopwatch = new Stopwatch();
                _maxBatchSize = maxBatchSize;
                _oldProgressCb = parseArg.ProgressCb;
                _batchCount = (fileCount + _maxBatchSize - 1) / _maxBatchSize;
                _filesPerBatch = fileCount / _batchCount;
                _newParseArg = parseArg;

                if(fileCount > maxBatchSize)
                {
                    _newParseArg.ProgressCb = delegate(float progress, IntPtr userData)
                    {
                        var realProgress = _progressBase + _progressRange * progress;
                        _oldProgressCb(realProgress, userData);
                    };
                }

                _stopwatch.Start();
            }

            public List<string> GetNextFiles(int batchIndex)
            {
                if(_batchCount <= 1)
                {
                    return _filePaths;
                }

                var fileCount = _filePaths.Count;
                _progressBase = (float)_fileIndex / (float)fileCount;
                var currentFileCount = (batchIndex == _batchCount - 1) ? (fileCount - _fileIndex) : _filesPerBatch;
                var currentFiles = _filePaths.GetRange(_fileIndex, currentFileCount);
                _progressRange = (float)currentFileCount / (float)fileCount;
                _fileIndex += currentFileCount;

                return currentFiles;
            }

            public bool IsCanceled(IntPtr cancelValueAddress)
            {
                return Marshal.ReadInt32(cancelValueAddress) != 0;
            }

            public int BatchCount
            {
                get { return _batchCount; }
            }

            public int FileIndex
            {
                get { return _fileIndex; }
            }

            public Stopwatch Timer
            {
                get { return _stopwatch; }
            }

            public udtParseArg NewParseArg
            {
                get { return _newParseArg; }
            }

            private List<string> _filePaths;
            private Stopwatch _stopwatch;
            private int _maxBatchSize = 512;
            private float _progressBase = 0.0f;
            private float _progressRange = 0.0f;
            private int _fileIndex = 0;
            private int _batchCount = 0;
            private int _filesPerBatch = 0;
            private udtProgressCallback _oldProgressCb;
            private udtParseArg _newParseArg;
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

        private static uint GetFixedMatchDuration(udtParseDataStats stats)
        {
            var MaxMatchDurationDeltaMs = 1000;
            var durationMs = (int)stats.MatchDurationMs;
            var durationMinuteModuloMs = durationMs % 60000;
            var absMinuteDiffMs = Math.Min(durationMinuteModuloMs, 60000 - durationMinuteModuloMs);
            if((stats.OverTimeCount == 0 || stats.OverTimeType == (uint)udtOvertimeType.Timed) &&
               stats.Forfeited == 0 &&
               absMinuteDiffMs < MaxMatchDurationDeltaMs)
            {
                var minutes = (durationMs + 60000 - 1) / 60000;
                if(durationMinuteModuloMs < MaxMatchDurationDeltaMs)
                {
                    --minutes;
                }

                return (uint)(60000 * minutes);
            }

            return stats.MatchDurationMs;
        }
    }
}