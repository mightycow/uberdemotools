#include "parser.hpp"
#include "shared.hpp"
#include "stack_trace.hpp"
#include "utils.hpp"
#include "path.hpp"
#include "memory_stream.hpp"
#include "file_stream.hpp"
#include "json_writer.hpp"

#include <stdio.h>
#include <stdlib.h>


#define UDT_PLAYER_STATS_ITEM(Enum, Desc) Desc,
static const char* PlayerStatsFieldNames[udtPlayerStatsField::Count + 1]
{
	UDT_PLAYER_STATS_LIST(UDT_PLAYER_STATS_ITEM)
	""
};
#undef UDT_PLAYER_STATS_ITEM

#define UDT_TEAM_STATS_ITEM(Enum, Desc) Desc,
static const char* TeamStatsFieldNames[udtPlayerStatsField::Count + 1]
{
	UDT_TEAM_STATS_LIST(UDT_TEAM_STATS_ITEM)
	""
};
#undef UDT_TEAM_STATS_ITEM


static void CrashHandler(const char* message)
{
	fprintf(stderr, "\n");
	fprintf(stderr, message);
	fprintf(stderr, "\n");

	PrintStackTrace(3, "UDT_json");

	exit(666);
}

static void PrintHelp()
{
	printf("\n");
	printf("UDT_json demo_path [json_path]\n");
	printf("If the output path isn't provided, the .json file will be output in the same directory as the input file with the same name.\n");
}

template<int N>
static bool IsSomeBitSet(const u8 (&flags)[N])
{
	for(int i = 0; i < N; ++i)
	{
		if(flags[i] != 0)
		{
			return true;
		}
	}

	return false;
}

static bool HasValidTeamStats(const udtParseDataStats& stats)
{
	for(s32 i = 0; i < 2; ++i)
	{
		if(IsSomeBitSet(stats.TeamStats[i].Flags))
		{
			return true;
		}
	}

	return false;
}

static bool HasValidPlayerStats(const udtParseDataStats& stats)
{
	for(s32 i = 0; i < 64; ++i)
	{
		if(IsSomeBitSet(stats.PlayerStats[i].Flags))
		{
			return true;
		}
	}

	return false;
}

static const char* GetUDTStringForValue(udtStringArray::Id stringId, u32 value)
{
	const char** strings = NULL;
	u32 stringCount = 0;
	if(udtGetStringArray(stringId, &strings, &stringCount) != (s32)udtErrorCode::None)
	{
		return NULL;
	}

	if(value >= stringCount)
	{
		return NULL;
	}

	return strings[value];
}

static void WriteUDTTeamIndex(udtJSONWriter& writer, s32 udtTeamIndex, const char* keyName = "team")
{
	if(keyName == NULL)
	{
		return;
	}

	writer.WriteStringValue(keyName, GetUDTStringForValue(udtStringArray::Teams, (u32)udtTeamIndex));
}

static void WriteUDTGameTypeShort(udtJSONWriter& writer, u32 udtGameType)
{
	writer.WriteStringValue("game type short", GetUDTStringForValue(udtStringArray::ShortGameTypes, udtGameType));
}

static void WriteUDTGameTypeLong(udtJSONWriter& writer, u32 udtGameType)
{
	writer.WriteStringValue("game type", GetUDTStringForValue(udtStringArray::GameTypes, udtGameType));
}

static void WriteUDTMod(udtJSONWriter& writer, u32 udtMod)
{
	writer.WriteStringValue("mod", GetUDTStringForValue(udtStringArray::ModNames, udtMod));
}

static void WriteUDTGamePlayShort(udtJSONWriter& writer, u32 udtGamePlay)
{
	writer.WriteStringValue("gameplay short", GetUDTStringForValue(udtStringArray::ShortGamePlayNames, udtGamePlay));
}

static void WriteUDTGamePlayLong(udtJSONWriter& writer, u32 udtGamePlay)
{
	writer.WriteStringValue("gameplay", GetUDTStringForValue(udtStringArray::GamePlayNames, udtGamePlay));
}

static void WriteUDTOverTimeType(udtJSONWriter& writer, u32 udtOverTimeType)
{
	writer.WriteStringValue("overtime type", GetUDTStringForValue(udtStringArray::OverTimeTypes, udtOverTimeType));
}

static void WriteTeamStats(udtJSONWriter& writer, const udtTeamStats& stats, const char* team)
{
	writer.StartObject();

	writer.WriteStringValue("team", team);
	for(s32 i = 0; i < (s32)udtTeamStatsField::Count; ++i)
	{
		const s32 byteIndex = i >> 3;
		const s32 bitIndex = i & 7;
		if((stats.Flags[byteIndex] & ((u8)1 << (u8)bitIndex)) != 0)
		{
			writer.WriteIntValue(TeamStatsFieldNames[i], stats.Fields[i]);
		}
	}

	writer.EndObject();
}

static void WritePlayerStats(udtJSONWriter& writer, const udtPlayerStats& stats, s32 clientNumber)
{
	writer.StartObject();

	writer.WriteIntValue("client number", clientNumber);
	writer.WriteStringValue("name", stats.Name);
	writer.WriteStringValue("clean name", stats.CleanName);
	WriteUDTTeamIndex(writer, stats.TeamIndex);

	const char** weaponNames = NULL;
	u32 weaponCount = 0;
	for(s32 i = 0; i < (s32)udtPlayerStatsField::Count; ++i)
	{
		const s32 byteIndex = i >> 3;
		const s32 bitIndex = i & 7;
		if((stats.Flags[byteIndex] & ((u8)1 << (u8)bitIndex)) != 0)
		{
			const s32 field = stats.Fields[i];
			if(i == (s32)udtPlayerStatsField::BestWeapon && 
			   udtGetStringArray(udtStringArray::Weapons, &weaponNames, &weaponCount) == (s32)udtErrorCode::None &&
			   field >= 0 && field < (s32)weaponCount)
			{
				writer.WriteStringValue(PlayerStatsFieldNames[i], weaponNames[field]);
			}
			else
			{
				writer.WriteIntValue(PlayerStatsFieldNames[i], field);
			}
		}
	}

	writer.EndObject();
}

static void WriteStats(udtJSONWriter& writer, const udtParseDataStats& stats)
{
	const bool hasTeamStats = HasValidTeamStats(stats);
	const bool hasPlayerStats = HasValidPlayerStats(stats);
	if(!hasTeamStats && !hasPlayerStats)
	{
		return;
	}

	const s32 MaxMatchDurationDeltaMs = 1000;
	s32 durationMs = (s32)stats.MatchDurationMs;
	const s32 durationMinuteModuloMs = durationMs % 60000;
	const s32 absMinuteDiffMs = udt_min(durationMinuteModuloMs, 60000 - durationMinuteModuloMs);
	if((stats.OverTimeCount == 0 || stats.OverTimeType == udtOvertimeType::Timed) && 
	   stats.Forfeited == 0 &&
	   absMinuteDiffMs < MaxMatchDurationDeltaMs)
	{
		s32 minutes = (durationMs + 60000 - 1) / 60000;
		if(durationMinuteModuloMs < MaxMatchDurationDeltaMs)
		{
			--minutes;
		}
		durationMs = 60000 * minutes;
	}

	writer.StartObject("game stats");

	WriteUDTMod(writer, stats.Mod);
	writer.WriteStringValue("mod version", stats.ModVersion);
	WriteUDTGameTypeShort(writer, stats.GameType);
	WriteUDTGameTypeLong(writer, stats.GameType);
	writer.WriteStringValue("map", stats.Map);
	writer.WriteIntValue("duration", (s32)durationMs);
	writer.WriteIntValue("overtime count", (s32)stats.OverTimeCount);
	if(stats.OverTimeCount > 0)
	{
		WriteUDTOverTimeType(writer, stats.OverTimeType);
	}
	writer.WriteBoolValue("forfeited", stats.Forfeited != 0);
	WriteUDTGamePlayShort(writer, stats.GamePlay);
	WriteUDTGamePlayLong(writer, stats.GamePlay);
	writer.WriteIntValue("time-out count", (s32)stats.TimeOutCount);
	if(stats.TimeOutCount > 0)
	{
		writer.WriteIntValue("total time-out duration", (s32)stats.TotalTimeOutDurationMs);
	}

	if(hasTeamStats)
	{
		writer.StartObject("team stats");
		for(s32 i = 0; i < 2; ++i)
		{
			const udtTeamStats& teamStats = stats.TeamStats[i];
			if(IsSomeBitSet(teamStats.Flags))
			{
				WriteTeamStats(writer, teamStats, i == 0 ? "red" : "blue");
			}
		}
		writer.EndObject();
	}
	
	if(hasPlayerStats)
	{
		writer.StartObject("player stats");
		for(s32 i = 0; i < 64; ++i)
		{
			const udtPlayerStats& playerStats = stats.PlayerStats[i];
			if(IsSomeBitSet(playerStats.Flags))
			{
				WritePlayerStats(writer, playerStats, i);
			}
		}
		writer.EndObject();
	}

	writer.EndObject();
}

static void WriteChatEvents(udtJSONWriter& writer, const udtParseDataChat* chatEvents, u32 count) 
{
	if(count == 0)
	{ 
		return;
	}

	writer.StartObject("global chat");

	for(u32 i = 0; i < count; ++i)
	{
		const udtParseDataChat& info = chatEvents[i];

		writer.StartObject();

		writer.WriteIntValue("game state number", info.GameStateIndex + 1);
		writer.WriteIntValue("server time", info.ServerTimeMs);
		if(info.PlayerIndex >= 0 && info.PlayerIndex < 64)
		{
			writer.WriteIntValue("client number", info.PlayerIndex);
		}
		writer.WriteStringValue("player name", info.Strings[0].PlayerName);
		writer.WriteStringValue("player clan name", info.Strings[0].ClanName);
		writer.WriteStringValue("message", info.Strings[0].Message);
		writer.WriteStringValue("clean player name", info.Strings[1].PlayerName);
		writer.WriteStringValue("clean player clan name", info.Strings[1].ClanName);
		writer.WriteStringValue("clean message", info.Strings[1].Message);

		writer.EndObject();
	}

	writer.EndObject();
}

static void WriteDeathEvents(udtJSONWriter& writer, const udtParseDataObituary* deathEvents, u32 count)
{
	if(count == 0)
	{
		return;
	}

	writer.StartObject("obituaries");

	for(u32 i = 0; i < count; ++i)
	{
		const udtParseDataObituary& info = deathEvents[i];

		writer.StartObject();

		writer.WriteIntValue("game state number", info.GameStateIndex + 1);
		writer.WriteIntValue("server time", info.ServerTimeMs);
		if(info.AttackerIdx >= 0 && info.AttackerIdx < 64)
		{
			writer.WriteIntValue("attacker client number", info.AttackerIdx);
			writer.WriteStringValue("attacker clean name", info.AttackerName);
			WriteUDTTeamIndex(writer, info.AttackerTeamIdx, "attacker team");
		}
		if(info.TargetIdx >= 0 && info.TargetIdx < 64)
		{
			writer.WriteIntValue("target client number", info.TargetIdx);
			writer.WriteStringValue("target clean name", info.TargetName);
			WriteUDTTeamIndex(writer, info.TargetTeamIdx, "target team");
		}
		writer.WriteStringValue("cause of death", info.MeanOfDeathName);
		
		writer.EndObject();
	}

	writer.EndObject();
}

static void WriteGameStates(udtJSONWriter& writer, const udtParseDataGameState* gameStates, u32 count)
{
	if(count == 0)
	{
		return;
	}

	writer.StartArray("game states");

	for(u32 i = 0; i < count; ++i)
	{
		const udtParseDataGameState& info = gameStates[i];

		writer.StartObject();

		if(info.DemoTakerPlayerIndex >= 0 && info.DemoTakerPlayerIndex < 64)
		{
			writer.WriteIntValue("demo taker client number", info.DemoTakerPlayerIndex);
			writer.WriteStringValue("demo taker clean name", info.DemoTakerName);
		}

		writer.WriteIntValue("file offset", (s32)info.FileOffset);
		writer.WriteIntValue("start time", info.FirstSnapshotTimeMs);
		writer.WriteIntValue("end time", info.LastSnapshotTimeMs);

		writer.StartArray("matches");
		for(u32 j = 0; j < info.MatchCount; ++j)
		{
			writer.StartObject();
			writer.WriteIntValue("start time", info.Matches[j].MatchStartTimeMs);
			writer.WriteIntValue("end time", info.Matches[j].MatchEndTimeMs);
			writer.EndObject();
		}
		writer.EndArray();

		writer.StartObject("players");
		for(u32 j = 0; j < info.PlayerCount; ++j)
		{
			writer.StartObject();
			writer.WriteIntValue("client number", info.Players[j].Index);
			writer.WriteStringValue("clean name", info.Players[j].FirstName);
			WriteUDTTeamIndex(writer, info.Players[j].FirstTeam);
			writer.WriteIntValue("start time", info.Players[j].FirstSnapshotTimeMs);
			writer.WriteIntValue("end time", info.Players[j].LastSnapshotTimeMs);
			writer.EndObject();
		}
		writer.EndObject();
		
		writer.StartObject("config string values");
		for(u32 j = 0; j < info.KeyValuePairCount; ++j)
		{
			writer.WriteStringValue(info.KeyValuePairs[j].Name, info.KeyValuePairs[j].Value);
		}
		writer.EndObject();

		writer.EndObject();
	}

	writer.EndArray();
}

static int ProcessDemo(const char* demoPath, const char* jsonPath)
{
	s32 cancel = 0;
	s32 errorCode = 0;

	const u32 plugInIds[] = 
	{
		(u32)udtParserPlugIn::Stats,
		(u32)udtParserPlugIn::Chat,
		(u32)udtParserPlugIn::Obituaries,
		(u32)udtParserPlugIn::GameState
	};

	udtParseArg info;
	memset(&info, 0, sizeof(info));
	info.CancelOperation = &cancel;
	info.MessageCb = &CallbackConsoleMessage;
	info.ProgressCb = &CallbackConsoleProgress;
	info.PlugIns = plugInIds;
	info.PlugInCount = (u32)UDT_COUNT_OF(plugInIds);

	udtMultiParseArg extraInfo;
	memset(&extraInfo, 0, sizeof(extraInfo));
	extraInfo.FileCount = 1;
	extraInfo.FilePaths = &demoPath;
	extraInfo.MaxThreadCount = 1;
	extraInfo.OutputErrorCodes = &errorCode;

	udtParserContextGroup* contextGroup = NULL;
	if(udtParseDemoFiles(&contextGroup, &info, &extraInfo) != (s32)udtErrorCode::None)
	{
		return __LINE__;
	}

	u32 contextCount = 0;
	if(udtGetContextCountFromGroup(contextGroup, &contextCount) != (s32)udtErrorCode::None)
	{
		return __LINE__;
	}

	if(contextCount != 1)
	{
		return __LINE__;
	}

	udtParserContext* context = NULL;
	if(udtGetContextFromGroup(contextGroup, 0, &context) != (s32)udtErrorCode::None)
	{
		return __LINE__;
	}

	u32 demoCount = 0;
	if(udtGetDemoCountFromContext(context, &demoCount) != (s32)udtErrorCode::None)
	{
		return __LINE__;
	}

	if(demoCount != 1)
	{
		return __LINE__;
	}

	void* statsPointer = NULL;
	u32 statsCount = 0;
	if(udtGetDemoDataInfo(context, 0, (u32)udtParserPlugIn::Stats, &statsPointer, &statsCount) != (s32)udtErrorCode::None ||
	   statsPointer == NULL)
	{
		return __LINE__;
	}

	void* chatEventPointer = NULL;
	u32 chatEventCount = 0;
	if(udtGetDemoDataInfo(context, 0, (u32)udtParserPlugIn::Chat, &chatEventPointer, &chatEventCount) != (s32)udtErrorCode::None ||
	   chatEventPointer == NULL)
	{
		return __LINE__;
	}

	void* deathEventPointer = NULL;
	u32 deathEventCount = 0;
	if(udtGetDemoDataInfo(context, 0, (u32)udtParserPlugIn::Obituaries, &deathEventPointer, &deathEventCount) != (s32)udtErrorCode::None ||
	   deathEventPointer == NULL)
	{
		return __LINE__;
	}

	void* gameStatesPointer = NULL;
	u32 gameStateCount = 0;
	if(udtGetDemoDataInfo(context, 0, (u32)udtParserPlugIn::GameState, &gameStatesPointer, &gameStateCount) != (s32)udtErrorCode::None ||
	   gameStatesPointer == NULL)
	{
		return __LINE__;
	}

	udtVMLinearAllocator outputPathAllocator;
	if(jsonPath == NULL)
	{
		if(!outputPathAllocator.Init(1 << 16))
		{
			return __LINE__;
		}

		udtString filePathNoExt;
		if(!udtPath::GetFilePathWithoutExtension(filePathNoExt, outputPathAllocator, udtString::NewConstRef(demoPath)))
		{
			return __LINE__;
		}

		const udtString filePath = udtString::NewFromConcatenating(outputPathAllocator, filePathNoExt, udtString::NewConstRef(".json"));
		jsonPath = filePath.String;
	}

	udtVMMemoryStream memoryStream;
	if(!memoryStream.Open(1 << 20))
	{
		return __LINE__;
	}

	const udtParseDataStats* const stats = (const udtParseDataStats*)statsPointer;
	const udtParseDataChat* const chatEvents = (const udtParseDataChat*)chatEventPointer;
	const udtParseDataObituary* const deathEvents = (const udtParseDataObituary*)deathEventPointer;
	const udtParseDataGameState* const gameStates = (const udtParseDataGameState*)gameStatesPointer;
	udtJSONWriter jsonWriter;
	jsonWriter.SetOutputStream(&memoryStream);
	jsonWriter.StartFile();
	for(u32 i = 0; i < statsCount; ++i)
	{
		WriteStats(jsonWriter, stats[i]);
	}
	WriteChatEvents(jsonWriter, chatEvents, chatEventCount);
	WriteDeathEvents(jsonWriter, deathEvents, deathEventCount);
	WriteGameStates(jsonWriter, gameStates, gameStateCount);
	jsonWriter.EndFile();

	udtFileStream jsonFile;
	if(!jsonFile.Open(jsonPath, udtFileOpenMode::Write))
	{
		fprintf(stderr, "ERROR: Failed to open file '%s' for writing.\n", jsonPath);
		return __LINE__;
	}

	if(jsonFile.Write(memoryStream.GetBuffer(), (u32)memoryStream.Length(), 1) != 1)
	{
		fprintf(stderr, "ERROR: Failed to write to file '%s'.\n", jsonPath);
		return __LINE__;
	}

	fprintf(stdout, "Done writing to file '%s'.\n", jsonPath);

	return 0;
}

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		PrintHelp();
		return 1;
	}

	printf("UDT library version: %s\n", udtGetVersionString());

	ResetCurrentDirectory(argv[0]);
	udtSetCrashHandler(&CrashHandler);

	return ProcessDemo(argv[1], argv[2]);
}
