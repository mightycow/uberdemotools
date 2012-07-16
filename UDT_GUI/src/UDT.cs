using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

using UdtHandle = System.IntPtr;


namespace Uber.DemoTools
{
    public class DemoCut
    {
        public DemoCut Clone()
        {
            var cut = new DemoCut();
            cut.FilePath = FilePath;
            cut.StartTimeMs = StartTimeMs;
            cut.EndTimeMs = EndTimeMs;

            return cut;
        }

        public string FilePath = "<invalid>";
        public int StartTimeMs = -1;
        public int EndTimeMs = -1;
    }

    public enum DemoProtocol
    {
        Invalid,
        Dm68,
        Dm73
    }

    public unsafe class Demo
    {
        [UnmanagedFunctionPointerAttribute(CallingConvention.Cdecl)]
        public delegate int ProgressCallback(float progress);

        [UnmanagedFunctionPointerAttribute(CallingConvention.Cdecl)]
        public delegate void MessageCallback(int logLevel, string message);

        private const string _dllPath = "UDT.dll";

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private IntPtr udtGetVersionString();

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private IntPtr udtCreate(int protocolId);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private void udtDestroy(UdtHandle lib);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtSetProgressCallback(UdtHandle lib, ProgressCallback callback);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtSetMessageCallback(UdtHandle lib, MessageCallback callback);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtSetFileStartOffset(UdtHandle lib, int fileStartOffset);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtSetServerTimeOffset(UdtHandle lib, int serverTimeOffset);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
	    extern static private int udtAddCut(UdtHandle lib, string filePath, int startTimeMs, int endTimeMs);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtParse(UdtHandle lib, string filePath, ProgressCallback progressCb, MessageCallback messageCb);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetWarmupEndTimeMs(UdtHandle lib, ref int warmupEndTimeMs);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetFirstSnapshotTimeMs(UdtHandle lib, ref int firstSnapshotTimeMs);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetGameStateCount(UdtHandle lib, ref int gameStateCount);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetGameStateFileOffset(UdtHandle lib, int gameStateIdx, ref int fileOffset);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetGameStateServerTimeOffset(UdtHandle lib, int gameStateIdx, ref int serverTimeOffset);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetGameStateFirstSnapshotTime(UdtHandle lib, int gameStateIdx, ref int firstSnapshotTime);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetGameStateLastSnapshotTime(UdtHandle lib, int gameStateIdx, ref int lastSnapshotTime);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetServerCommandCount(UdtHandle lib, ref int cmdCount);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetServerCommandSequence(UdtHandle lib, int cmdIndex, ref int seqNumber);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetServerCommandMessage(UdtHandle lib, int cmdIndex, byte* valueBuffer, int bufferLength);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetConfigStringCount(UdtHandle lib, ref int csCount);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetConfigStringValue(UdtHandle lib, int csIndex, byte* valueBuffer, int bufferLength);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetConfigStringIndex(UdtHandle lib, int udtCsIndex, ref int quakeCsIndex);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetChatStringCount(UdtHandle lib, ref int chatCount);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetChatStringMessage(UdtHandle lib, int chatIndex, byte* valueBuffer, int bufferLength);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetChatStringTime(UdtHandle lib, int chatIndex, ref int time);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetObituaryCount(UdtHandle lib, ref int obituaryCount);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetObituaryTime(UdtHandle lib, int obituaryIndex, ref int time);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetObituaryAttackerName(UdtHandle lib, int obituaryIndex, byte* nameBuffer, int bufferLength);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetObituaryTargetName(UdtHandle lib, int obituaryIndex, byte* nameBuffer, int bufferLength);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetObituaryMeanOfDeath(UdtHandle lib, int obituaryIndex, ref int mod);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetPuRunCount(UdtHandle lib, ref int puRunCount);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetPuRunTime(UdtHandle lib, int puRunIndex, ref int time);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetPuRunPlayerName(UdtHandle lib, int puRunIndex, byte* nameBuffer, int bufferLength);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetPuRunPu(UdtHandle lib, int puRunIndex, ref int pu);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetPuRunDuration(UdtHandle lib, int puRunIndex, ref int durationMs);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetPuRunKillCount(UdtHandle lib, int puRunIndex, ref int kills);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetPuRunTeamKillCount(UdtHandle lib, int puRunIndex, ref int teamKills);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtGetPuRunSelfKill(UdtHandle lib, int puRunIndex, ref int selfKill);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtCutDemoTime(string inFilePath, string outFilePath, int startTimeMs, int endTimeMs, ProgressCallback progressCb, MessageCallback messageCb);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private int udtCutDemoChat(string inFilePath, string outFilePath, string[] chatEntries, int chatEntryCount, int startOffsetSecs, int endOffsetSecs);

        // -------------------------------------------------------------

        private UdtHandle _demo;
        private ASCIIEncoding _encoding = new ASCIIEncoding();
        private const int _stringBufferSize = 64 * 1024;

        public class ChatStringInfo
        {
            public string Message;
            public int Time;
        }

        public class ConfigStringInfo
        {
            public string Value;
            public int Index;
        }

        public class ServerCommandInfo
        {
            public string Command;
            public int Sequence;
        }

        public class GameStateInfo
        {
            public int FileOffset;
            public int ServerTimeOffset;
            public int FirstSnapshotTime;
            public int LastSnapshotTime;
        }

        public class FragInfo
        {
            public int Time;
            public int Mod;
            public string AttackerName;
            public string TargetName;
        }

        public class PuRunInfo
        {
            public int Time;
            public int Pu;
            public int Duration; // [ms]
            public int Kills;
            public int TeamKills;
            public int SelfKill; // Non-zero if the player kills himself with the PU :-)
            public string PlayerName;
        }

        public int WarmupEndTimeMs
        {
            get; private set;
        }

        public int FirstSnapshotTimeMs
        {
            get; private set;
        }

        public Demo(DemoProtocol protocol)
        {
            _demo = udtCreate((int)protocol);
        }

        public void Destroy()
        {
            udtDestroy(_demo);
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

        public bool Parse(string filePath, ProgressCallback progressCb, MessageCallback messageCb)
        {
            var result = udtParse(_demo, filePath, progressCb, messageCb);
            if(result != 0)
            {
                return false;
            }

            int warmupEndTimeMs = -1;
            udtGetWarmupEndTimeMs(_demo, ref warmupEndTimeMs);
            WarmupEndTimeMs = warmupEndTimeMs;

            int firstSnapshotTimeMs = -1;
            udtGetFirstSnapshotTimeMs(_demo, ref firstSnapshotTimeMs);
            FirstSnapshotTimeMs = firstSnapshotTimeMs;

            return true;
        }

        public static void CreateCutList(List<DemoCut> cuts, List<GameStateInfo> gameStates, ref List<List<DemoCut>> gsCutsList, ref List<int> gsParseTimes, ref int totalParseTime)
        {
            gsCutsList = new List<List<DemoCut>>();
            foreach(var gameState in gameStates)
            {
                var gsCuts = new List<DemoCut>();
                var matches = cuts.FindAll(c => c.StartTimeMs >= gameState.FirstSnapshotTime && c.StartTimeMs < gameState.LastSnapshotTime);
                foreach(var match in matches)
                {
                    var cut = match.Clone();
                    cut.EndTimeMs = Math.Min(cut.EndTimeMs, gameState.LastSnapshotTime - 40); // Make sure the cut doesn't cross game state boundaries.
                    if(cut.EndTimeMs > cut.StartTimeMs)
                    {
                        gsCuts.Add(cut);
                    }
                }

                gsCutsList.Add(gsCuts);
            }

            totalParseTime = 0;
            gsParseTimes = new List<int>();
            foreach(var gsCuts in gsCutsList)
            {
                int gsParseTime = 0;
                foreach(var cut in gsCuts)
                {
                    gsParseTime += cut.EndTimeMs - cut.StartTimeMs;
                }

                totalParseTime += gsParseTime;

                gsParseTimes.Add(gsParseTime);
            }
        }
        
        public static bool Cut(DemoProtocol protocol, string filePath, ProgressCallback progressCb, MessageCallback messageCb, List<DemoCut> cuts, GameStateInfo gameState)
        {
            if(cuts.Count == 0)
            {
                return true;
            }

            UdtHandle demo = udtCreate((int)protocol);
            if(demo == IntPtr.Zero)
            {
                return false;
            }

            udtSetFileStartOffset(demo, gameState.FileOffset);
            udtSetServerTimeOffset(demo, gameState.ServerTimeOffset);

            foreach(var cut in cuts)
            {
                udtAddCut(demo, cut.FilePath, cut.StartTimeMs, cut.EndTimeMs);
            }

            var result = udtParse(demo, filePath, progressCb, messageCb) != 0;

            udtDestroy(demo);

            return result;
        }
        
        private unsafe string GetChatString(int index)
        {
            var bufferArray = new byte[_stringBufferSize];
            fixed(byte* bufferPtr = bufferArray)
            {
                udtGetChatStringMessage(_demo, index, bufferPtr, _stringBufferSize);
            }

            return _encoding.GetString(bufferArray, 0, GetStringLength(bufferArray));
        }

        private int GetChatTime(int index)
        {
            int time = int.MinValue;
            udtGetChatStringTime(_demo, index, ref time);

            return time;
        }

        public List<ChatStringInfo> GetChatStrings()
        {
            var list = new List<ChatStringInfo>();

            int chatStringCount = 0;
            udtGetChatStringCount(_demo, ref chatStringCount);
            for(int i = 0; i < chatStringCount; ++i)
            {
                var info = new ChatStringInfo();
                info.Message = GetChatString(i);
                info.Time = GetChatTime(i);
                list.Add(info);
            }

            return list;
        }

        public List<GameStateInfo> GetGameStates()
        {
            var list = new List<GameStateInfo>();

            int gameStateCount = 0;
            udtGetGameStateCount(_demo, ref gameStateCount);
            for(int i = 0; i < gameStateCount; ++i)
            {
                var info = new GameStateInfo();
                udtGetGameStateFileOffset(_demo, i, ref info.FileOffset);
                udtGetGameStateServerTimeOffset(_demo, i, ref info.ServerTimeOffset);
                udtGetGameStateFirstSnapshotTime(_demo, i, ref info.FirstSnapshotTime);
                udtGetGameStateLastSnapshotTime(_demo, i, ref info.LastSnapshotTime);
                list.Add(info);
            }

            return list;
        }

        private unsafe string GetObituaryAttackerString(int index)
        {
            var bufferArray = new byte[_stringBufferSize];
            fixed(byte* bufferPtr = bufferArray)
            {
                udtGetObituaryAttackerName(_demo, index, bufferPtr, _stringBufferSize);
            }

            return _encoding.GetString(bufferArray, 0, GetStringLength(bufferArray));
        }

        private unsafe string GetObituaryTargetString(int index)
        {
            var bufferArray = new byte[_stringBufferSize];
            fixed(byte* bufferPtr = bufferArray)
            {
                udtGetObituaryTargetName(_demo, index, bufferPtr, _stringBufferSize);
            }

            return _encoding.GetString(bufferArray, 0, GetStringLength(bufferArray));
        }

        public List<FragInfo> GetFrags()
        {
            var list = new List<FragInfo>();

            int fragCount = 0;
            udtGetObituaryCount(_demo, ref fragCount);
            for(int i = 0; i < fragCount; ++i)
            {
                var info = new FragInfo();
                udtGetObituaryTime(_demo, i, ref info.Time);
                udtGetObituaryMeanOfDeath(_demo, i, ref info.Mod);
                info.AttackerName = GetObituaryAttackerString(i);
                info.TargetName = GetObituaryTargetString(i);
                list.Add(info);
            }

            return list;
        }

        private unsafe string GetPuRunPlayerName(int index)
        {
            var bufferArray = new byte[_stringBufferSize];
            fixed(byte* bufferPtr = bufferArray)
            {
                udtGetPuRunPlayerName(_demo, index, bufferPtr, _stringBufferSize);
            }

            return _encoding.GetString(bufferArray, 0, GetStringLength(bufferArray));
        }

        public List<PuRunInfo> GetPuRuns()
        {
            var list = new List<PuRunInfo>();

            int puRunCount = 0;
            udtGetPuRunCount(_demo, ref puRunCount);
            for(int i = 0; i < puRunCount; ++i)
            {
                var info = new PuRunInfo();
                info.PlayerName = GetPuRunPlayerName(i);
                udtGetPuRunTime(_demo, i, ref info.Time);
                udtGetPuRunPu(_demo, i, ref info.Pu);
                udtGetPuRunDuration(_demo, i, ref info.Duration);
                udtGetPuRunKillCount(_demo, i, ref info.Kills);
                udtGetPuRunTeamKillCount(_demo, i, ref info.TeamKills);
                udtGetPuRunSelfKill(_demo, i, ref info.SelfKill);
                list.Add(info);
            }

            return list;
        }

        private unsafe string GetServerCommandMessage(int index)
        {
            var bufferArray = new byte[_stringBufferSize];
            fixed(byte* bufferPtr = bufferArray)
            {
                udtGetServerCommandMessage(_demo, index, bufferPtr, _stringBufferSize);
            }

            return _encoding.GetString(bufferArray, 0, GetStringLength(bufferArray));
        }

        private int GetServerCommandSequence(int index)
        {
            int seqNumber = -1;
            udtGetServerCommandSequence(_demo, index, ref seqNumber);

            return seqNumber;
        }

        public List<ServerCommandInfo> GetServerCommands()
        {
            var list = new List<ServerCommandInfo>();

            int cmdCount = 0;
            udtGetServerCommandCount(_demo, ref cmdCount);
            for(int i = 0; i < cmdCount; ++i)
            {
                var info = new ServerCommandInfo();
                info.Command = GetServerCommandMessage(i);
                info.Sequence = GetServerCommandSequence(i);
                list.Add(info);
            }

            return list;
        }

        private unsafe string GetConfigStringValue(int index)
        {
            var bufferArray = new byte[_stringBufferSize];
            fixed(byte* bufferPtr = bufferArray)
            {
                udtGetConfigStringValue(_demo, index, bufferPtr, _stringBufferSize);
            }

            return _encoding.GetString(bufferArray, 0, GetStringLength(bufferArray));
        }

        private int GetConfigStringIndex(int index)
        {
            int quakeIndex = -1;
            udtGetConfigStringIndex(_demo, index, ref quakeIndex);

            return quakeIndex;
        }

        public List<ConfigStringInfo> GetConfigStrings()
        {
            var list = new List<ConfigStringInfo>();

            int csCount = 0;
            udtGetConfigStringCount(_demo, ref csCount);
            for(int i = 0; i < csCount; ++i)
            {
                var info = new ConfigStringInfo();
                info.Value = GetConfigStringValue(i);
                info.Index = GetConfigStringIndex(i);
                list.Add(info);
            }

            return list;
        }

        static public void CutDemoTime(string inFilePath, string outFilePath, int startTimeMs, int endTimeMs, ProgressCallback progressCb, MessageCallback messageCb)
        {
            udtCutDemoTime(inFilePath, outFilePath, startTimeMs, endTimeMs, progressCb, messageCb);
        }

        static public void CutDemoChat(string inFilePath, string outFilePath, List<string> chatEntries, int startOffsetSecs, int endOffsetSecs)
        {
            string[] entries = new string[chatEntries.Count];
            for(int i = 0; i < chatEntries.Count; ++i)
            {
                entries[i] = chatEntries[i];
            }

            udtCutDemoChat(inFilePath, outFilePath, entries, chatEntries.Count, startOffsetSecs, endOffsetSecs);
        }

        static private int GetStringLength(byte[] bufferArray)
        {
            int length = 0;
            for(int i = 0; i < bufferArray.Length; ++i)
            {
                if(bufferArray[i] == '\0')
                {
                    break;
                }

                ++length;
            }

            return length;
        }
    }
}