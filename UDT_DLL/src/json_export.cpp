#include "json_export.hpp"
#include "parser.hpp"
#include "shared.hpp"
#include "stack_trace.hpp"
#include "utils.hpp"
#include "path.hpp"
#include "memory_stream.hpp"
#include "file_stream.hpp"
#include "json_writer.hpp"
#include "parser_context.hpp"
#include "scoped_stack_allocator.hpp"

#include <ctype.h>
#include <time.h>


struct udtJSONExporter
{
public:
	udtJSONExporter(udtJSONWriter& writer, udtVMLinearAllocator& tempAllocator)
		: Writer(writer)
		, TempAllocator(tempAllocator)
		, StringBuffer(NULL)
		, StringBufferSize(0)
	{
	}

	void SetStringBuffer(const u8* stringBuffer, u32 stringBufferSize)
	{
		StringBuffer = stringBuffer;
		StringBufferSize = stringBufferSize;
	}

	void StartObject()
	{
		Writer.StartObject();
	}

	void StartObject(const char* name)
	{
		udtVMScopedStackAllocator allocatorScope(TempAllocator);
		Writer.StartObject(GetFixedName(name).GetPtr());
	}

	void EndObject()
	{
		Writer.EndObject();
	}

	void StartArray()
	{
		Writer.StartArray();
	}

	void StartArray(const char* name)
	{
		udtVMScopedStackAllocator allocatorScope(TempAllocator);
		Writer.StartArray(GetFixedName(name).GetPtr());
	}

	void EndArray()
	{
		Writer.EndArray();
	}

	void WriteBoolValue(const char* name, bool value)
	{
		udtVMScopedStackAllocator allocatorScope(TempAllocator);
		Writer.WriteBoolValue(GetFixedName(name).GetPtr(), value);
	}

	void WriteIntValue(const char* name, s32 number)
	{
		udtVMScopedStackAllocator allocatorScope(TempAllocator);
		Writer.WriteIntValue(GetFixedName(name).GetPtr(), number);
	}

	void WriteStringValue(const char* name, const char* string)
	{
		udtVMScopedStackAllocator allocatorScope(TempAllocator);
		Writer.WriteStringValue(GetFixedName(name).GetPtr(), string);
	}

	void WriteStringValue(const char* name, u32 stringOffset)
	{
		if(stringOffset >= StringBufferSize)
		{
			return;
		}

		const char* const string = (const char*)StringBuffer + stringOffset;

		udtVMScopedStackAllocator allocatorScope(TempAllocator);
		Writer.WriteStringValue(GetFixedName(name).GetPtr(), string);
	}

	void WriteStringValue(u32 nameOffset, u32 stringOffset)
	{
		if(nameOffset >= StringBufferSize ||
		   stringOffset >= StringBufferSize)
		{
			return;
		}

		const char* const name = (const char*)StringBuffer + nameOffset;
		const char* const string = (const char*)StringBuffer + stringOffset;

		udtVMScopedStackAllocator allocatorScope(TempAllocator);
		Writer.WriteStringValue(GetFixedName(name).GetPtr(), string);
	}

private:
	UDT_NO_COPY_SEMANTICS(udtJSONExporter);

private:
	udtString GetFixedName(const char* name)
	{
		return udtString::NewCamelCaseClone(TempAllocator, udtString::NewConstRef(name));
	}

	udtJSONWriter& Writer;
	udtVMLinearAllocator& TempAllocator;
	const u8* StringBuffer;
	u32 StringBufferSize;
};


static bool HasValidTeamStats(const udtParseDataStats& stats)
{
	return stats.ValidTeams != 0;
}

static bool HasValidPlayerStats(const udtParseDataStats& stats)
{
	return stats.ValidPlayers != 0;
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

static void WriteStartDate(udtJSONExporter& writer, u32 date)
{
	if(date == 0)
	{
		return;
	}

	time_t timeStamp = (time_t)date;
	tm* const t = gmtime(&timeStamp);
	if(t == NULL)
	{
		return;
	}

	char dateString[256];
	sprintf(dateString, "%04d.%02d.%02d %02d:%02d:%02d UTC", 
			t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

	writer.WriteStringValue("start date", dateString);
	writer.WriteIntValue("start date unix", (s32)date);
}

static void WriteUDTWeapon(udtJSONExporter& writer, s32 udtWeaponIndex, const char* keyName = "weapon")
{
	if(keyName == NULL)
	{
		return;
	}

	writer.WriteStringValue(keyName, GetUDTStringForValue(udtStringArray::Weapons, (u32)udtWeaponIndex));
}

static void WriteUDTTeamIndex(udtJSONExporter& writer, s32 udtTeamIndex, const char* keyName = "team")
{
	if(keyName == NULL)
	{
		return;
	}

	writer.WriteStringValue(keyName, GetUDTStringForValue(udtStringArray::Teams, (u32)udtTeamIndex));
}

static void WriteUDTGameTypeShort(udtJSONExporter& writer, u32 udtGameType)
{
	writer.WriteStringValue("game type short", GetUDTStringForValue(udtStringArray::ShortGameTypes, udtGameType));
}

static void WriteUDTGameTypeLong(udtJSONExporter& writer, u32 udtGameType)
{
	writer.WriteStringValue("game type", GetUDTStringForValue(udtStringArray::GameTypes, udtGameType));
}

static void WriteUDTMod(udtJSONExporter& writer, u32 udtMod)
{
	writer.WriteStringValue("mod", GetUDTStringForValue(udtStringArray::ModNames, udtMod));
}

static void WriteUDTGamePlayShort(udtJSONExporter& writer, u32 udtGamePlay)
{
	writer.WriteStringValue("gameplay short", GetUDTStringForValue(udtStringArray::ShortGamePlayNames, udtGamePlay));
}

static void WriteUDTGamePlayLong(udtJSONExporter& writer, u32 udtGamePlay)
{
	writer.WriteStringValue("gameplay", GetUDTStringForValue(udtStringArray::GamePlayNames, udtGamePlay));
}

static void WriteUDTOverTimeType(udtJSONExporter& writer, u32 udtOverTimeType)
{
	writer.WriteStringValue("overtime type", GetUDTStringForValue(udtStringArray::OverTimeTypes, udtOverTimeType));
}

static void WriteTeamStats(s32& fieldsRead, udtJSONExporter& writer, const u8* flags, const s32* fields, s32 teamIndex, const char** fieldNames)
{
	writer.StartObject();

	writer.WriteStringValue("team", teamIndex == (s32)udtTeam::Red ? "red" : "blue");

	s32 fieldIdx = 0;
	for(s32 i = 0; i < (s32)udtTeamStatsField::Count; ++i)
	{
		if(IsBitSet(flags, (u32)i))
		{
			writer.WriteIntValue(fieldNames[i], fields[fieldIdx++]);
		}
	}

	fieldsRead = fieldIdx;

	writer.EndObject();
}

static void WritePlayerStats(s32& fieldsRead, udtJSONExporter& writer, const udtPlayerStats& stats, const u8* flags, const s32* fields, s32 clientNumber, const char** fieldNames)
{
	writer.StartObject();

	writer.WriteIntValue("client number", clientNumber);
	writer.WriteStringValue("name", stats.Name);
	writer.WriteStringValue("clean name", stats.CleanName);

	s32 fieldIdx = 0;
	for(s32 i = 0; i < (s32)udtPlayerStatsField::Count; ++i)
	{
		if(IsBitSet(flags, (u32)i))
		{
			const s32 field = fields[fieldIdx++];
			switch((udtPlayerStatsField::Id)i)
			{
				case udtPlayerStatsField::BestWeapon:
					WriteUDTWeapon(writer, field, "best weapon");
					break;

				case udtPlayerStatsField::TeamIndex:
					WriteUDTTeamIndex(writer, field);
					break;

				default:
					writer.WriteIntValue(fieldNames[i], field);
					break;
			}
		}
	}

	fieldsRead = fieldIdx;

	writer.EndObject();
}

static void WriteStats(udtJSONExporter& writer, const udtParseDataStatsBuffers& statsBuffers, u32 demoIndex, const char** playerStatsFieldNames, const char** teamStatsFieldNames)
{
	if(playerStatsFieldNames == NULL ||
	   teamStatsFieldNames == NULL)
	{
		return;
	}

	const udtParseDataBufferRange range = statsBuffers.MatchStatsRanges[demoIndex];
	const u32 first = range.FirstIndex;
	const u32 count = range.Count;
	if(count == 0)
	{
		return;
	}

	writer.StartArray("match stats");
	writer.SetStringBuffer(statsBuffers.StringBuffer, statsBuffers.StringBufferSize);

	for(u32 matchIdx = first, end = first + count; matchIdx < end; ++matchIdx)
	{
		const udtParseDataStats& stats = statsBuffers.MatchStats[matchIdx];

		const bool hasTeamStats = HasValidTeamStats(stats);
		const bool hasPlayerStats = HasValidPlayerStats(stats);
		if(!hasTeamStats && !hasPlayerStats)
		{
			continue;
		}

		writer.StartObject();

		writer.WriteIntValue("game state index", stats.GameStateIndex);
		writer.WriteIntValue("start time", stats.StartTimeMs);
		writer.WriteIntValue("end time", stats.EndTimeMs);
		if(stats.CountDownStartTimeMs < stats.StartTimeMs)
		{
			writer.WriteIntValue("count down start time", stats.CountDownStartTimeMs);
		}
		if(stats.IntermissionEndTimeMs > stats.EndTimeMs)
		{
			writer.WriteIntValue("intermission end time", stats.IntermissionEndTimeMs);
		}
		writer.WriteStringValue("winner", stats.SecondPlaceWon ? stats.SecondPlaceName : stats.FirstPlaceName);
		writer.WriteStringValue("first place name", stats.FirstPlaceName);
		writer.WriteStringValue("second place name", stats.SecondPlaceName);
		writer.WriteIntValue("first place score", (s32)stats.FirstPlaceScore);
		writer.WriteIntValue("second place score", (s32)stats.SecondPlaceScore);
		WriteStartDate(writer, stats.StartDateEpoch);
		writer.WriteStringValue("custom red name", stats.CustomRedName);
		writer.WriteStringValue("custom blue name", stats.CustomBlueName);
		WriteUDTMod(writer, stats.Mod);
		writer.WriteStringValue("mod version", stats.ModVersion);
		WriteUDTGameTypeShort(writer, stats.GameType);
		WriteUDTGameTypeLong(writer, stats.GameType);
		writer.WriteStringValue("map", stats.MapName);
		if(stats.TimeLimit != 0)
		{
			writer.WriteIntValue("time limit", (s32)stats.TimeLimit);
		}
		if(stats.ScoreLimit != 0)
		{
			writer.WriteIntValue("score limit", (s32)stats.ScoreLimit);
		}
		if(stats.FragLimit != 0)
		{
			writer.WriteIntValue("frag limit", (s32)stats.FragLimit);
		}
		if(stats.CaptureLimit != 0)
		{
			writer.WriteIntValue("capture limit", (s32)stats.CaptureLimit);
		}
		if(stats.RoundLimit != 0)
		{
			writer.WriteIntValue("round limit", (s32)stats.RoundLimit);
		}
		writer.WriteIntValue("duration", (s32)stats.MatchDurationMs);
		writer.WriteIntValue("overtime count", (s32)stats.OverTimeCount);
		if(stats.OverTimeCount > 0)
		{
			WriteUDTOverTimeType(writer, stats.OverTimeType);
		}
		writer.WriteBoolValue("forfeited", stats.Forfeited != 0);
		writer.WriteBoolValue("leader forfeited", stats.SecondPlaceWon != 0);
		writer.WriteBoolValue("mercy limited", stats.MercyLimited != 0);
		WriteUDTGamePlayShort(writer, stats.GamePlay);
		WriteUDTGamePlayLong(writer, stats.GamePlay);
		writer.WriteIntValue("time out count", (s32)stats.TimeOutCount);
		if(stats.TimeOutCount > 0)
		{
			writer.WriteIntValue("total time out duration", (s32)stats.TotalTimeOutDurationMs);
			writer.StartArray("time outs");

			const u32 firstTo = stats.FirstTimeOutRangeIndex;
			const u32 countTo = stats.TimeOutCount;
			for(u32 i = firstTo, endTo = firstTo + countTo; i < endTo; ++i)
			{
				writer.StartObject();
				writer.WriteIntValue("start time", statsBuffers.TimeOutStartAndEndTimes[2*i]);
				writer.WriteIntValue("end time", statsBuffers.TimeOutStartAndEndTimes[2*i + 1]);
				writer.EndObject();
			}

			writer.EndArray();
		}

		if(hasTeamStats)
		{
			const u8* flags = statsBuffers.TeamFlags + stats.FirstTeamFlagIndex;
			const s32* fields = statsBuffers.TeamFields + stats.FirstTeamFieldIndex;

			writer.StartArray("team stats");

			for(s32 i = 0; i < 2; ++i)
			{
				if((stats.ValidTeams & ((u64)1 << (u64)i)) != 0)
				{
					s32 fieldsRead;
					WriteTeamStats(fieldsRead, writer, flags, fields, i, teamStatsFieldNames);
					flags += UDT_TEAM_STATS_MASK_BYTE_COUNT;
					fields += fieldsRead;
				}
			}

			writer.EndArray();
		}

		if(hasPlayerStats)
		{
			const u8* flags = statsBuffers.PlayerFlags + stats.FirstPlayerFlagIndex;
			const s32* fields = statsBuffers.PlayerFields + stats.FirstPlayerFieldIndex;
			const udtPlayerStats* extraStats = statsBuffers.PlayerStats + stats.FirstPlayerStatsIndex;

			writer.StartArray("player stats");

			for(s32 i = 0; i < 64; ++i)
			{
				if((stats.ValidPlayers & ((u64)1 << (u64)i)) != 0)
				{
					s32 fieldsRead;
					WritePlayerStats(fieldsRead, writer, *extraStats, flags, fields, i, playerStatsFieldNames);
					flags += UDT_PLAYER_STATS_MASK_BYTE_COUNT;
					fields += fieldsRead;
					extraStats += 1;
				}
			}

			writer.EndArray();
		}

		writer.EndObject();
	}

	writer.EndArray();
}

static void WriteChatEvents(udtJSONExporter& writer, const udtParseDataChatBuffers& chatBuffers, u32 demoIndex)
{
	const udtParseDataBufferRange range = chatBuffers.ChatMessageRanges[demoIndex];
	const u32 first = range.FirstIndex;
	const u32 count = range.Count;
	if(count == 0)
	{
		return;
	}

	writer.StartArray("chat");
	writer.SetStringBuffer(chatBuffers.StringBuffer, chatBuffers.StringBufferSize);

	for(u32 i = first, end = first + count; i < end; ++i)
	{
		const udtParseDataChat& info = chatBuffers.ChatMessages[i];

		writer.StartObject();

		writer.WriteIntValue("game state number", info.GameStateIndex + 1);
		writer.WriteIntValue("server time", info.ServerTimeMs);
		writer.WriteBoolValue("team chat", info.TeamMessage != 0);
		if(info.PlayerIndex >= 0 && info.PlayerIndex < 64)
		{
			writer.WriteIntValue("client number", info.PlayerIndex);
		}
		writer.WriteStringValue("player name", info.Strings[0].PlayerName);
		writer.WriteStringValue("player clan name", info.Strings[0].ClanName);
		writer.WriteStringValue("message", info.Strings[0].Message);
		writer.WriteStringValue("location", info.Strings[0].Location);
		writer.WriteStringValue("command", info.Strings[0].OriginalCommand);
		writer.WriteStringValue("clean player name", info.Strings[1].PlayerName);
		writer.WriteStringValue("clean player clan name", info.Strings[1].ClanName);
		writer.WriteStringValue("clean message", info.Strings[1].Message);
		writer.WriteStringValue("clean location", info.Strings[1].Location);
		writer.WriteStringValue("clean command", info.Strings[1].OriginalCommand);

		writer.EndObject();
	}

	writer.EndArray();
}

static void WriteDeathEvents(udtJSONExporter& writer, const udtParseDataObituaryBuffers& deathBuffers, u32 demoIndex)
{
	const udtParseDataBufferRange range = deathBuffers.ObituaryRanges[demoIndex];
	const u32 first = range.FirstIndex;
	const u32 count = range.Count;
	if(count == 0)
	{
		return;
	}

	writer.StartArray("obituaries");
	writer.SetStringBuffer(deathBuffers.StringBuffer, deathBuffers.StringBufferSize);

	for(u32 i = first, end = first + count; i < end; ++i)
	{
		const udtParseDataObituary& info = deathBuffers.Obituaries[i];

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

	writer.EndArray();
}

static void WriteRawCommands(udtJSONExporter& writer, const udtParseDataRawCommandBuffers& commandBuffers, u32 demoIndex)
{
	const udtParseDataBufferRange range = commandBuffers.CommandRanges[demoIndex];
	const u32 first = range.FirstIndex;
	const u32 count = range.Count;
	if(count == 0)
	{
		return;
	}

	writer.StartArray("raw commands");
	writer.SetStringBuffer(commandBuffers.StringBuffer, commandBuffers.StringBufferSize);

	for(u32 i = first, end = first + count; i < end; ++i)
	{
		const udtParseDataRawCommand& info = commandBuffers.Commands[i];

		writer.StartObject();

		writer.WriteIntValue("game state number", info.GameStateIndex + 1);
		writer.WriteIntValue("server time", info.ServerTimeMs);
		writer.WriteStringValue("raw command", info.RawCommand);

		writer.EndObject();
	}

	writer.EndArray();
}

static void WriteRawConfigStrings(udtJSONExporter& writer, const udtParseDataRawConfigStringBuffers& configStringBuffers, u32 demoIndex)
{
	const udtParseDataBufferRange range = configStringBuffers.ConfigStringRanges[demoIndex];
	const u32 first = range.FirstIndex;
	const u32 count = range.Count;
	if(count == 0)
	{
		return;
	}

	writer.StartArray("raw config strings");
	writer.SetStringBuffer(configStringBuffers.StringBuffer, configStringBuffers.StringBufferSize);

	for(u32 i = first, end = first + count; i < end; ++i)
	{
		const udtParseDataRawConfigString& info = configStringBuffers.ConfigStrings[i];

		writer.StartObject();

		writer.WriteIntValue("game state number", info.GameStateIndex + 1);
		writer.WriteIntValue("index", info.ConfigStringIndex);
		writer.WriteStringValue("raw string", info.RawConfigString);

		writer.EndObject();
	}

	writer.EndArray();
}

static void WriteGameStates(udtJSONExporter& writer, const udtParseDataGameStateBuffers& gameStateBuffers, u32 demoIndex)
{
	const udtParseDataBufferRange range = gameStateBuffers.GameStateRanges[demoIndex];
	const u32 first = range.FirstIndex;
	const u32 count = range.Count;
	if(count == 0)
	{
		return;
	}

	writer.StartArray("game states");
	writer.SetStringBuffer(gameStateBuffers.StringBuffer, gameStateBuffers.StringBufferSize);

	for(u32 i = first, end = first + count; i < end; ++i)
	{
		const udtParseDataGameState& info = gameStateBuffers.GameStates[i];

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
		for(u32 j = info.FirstMatchIndex, endM = j + info.MatchCount; j < endM; ++j)
		{
			writer.StartObject();
			writer.WriteIntValue("start time", gameStateBuffers.Matches[j].MatchStartTimeMs);
			writer.WriteIntValue("end time", gameStateBuffers.Matches[j].MatchEndTimeMs);
			writer.EndObject();
		}
		writer.EndArray();

		writer.StartArray("players");
		for(u32 j = info.FirstPlayerIndex, endP = j + info.PlayerCount; j < endP; ++j)
		{
			writer.StartObject();
			writer.WriteIntValue("client number", gameStateBuffers.Players[j].Index);
			writer.WriteStringValue("clean name", gameStateBuffers.Players[j].FirstName);
			WriteUDTTeamIndex(writer, gameStateBuffers.Players[j].FirstTeam);
			writer.WriteIntValue("start time", gameStateBuffers.Players[j].FirstSnapshotTimeMs);
			writer.WriteIntValue("end time", gameStateBuffers.Players[j].LastSnapshotTimeMs);
			writer.EndObject();
		}
		writer.EndArray();

		writer.StartObject("config string values");
		for(u32 j = info.FirstKeyValuePairIndex, endKvp = j + info.KeyValuePairCount; j < endKvp; ++j)
		{
			writer.WriteStringValue(gameStateBuffers.KeyValuePairs[j].Name, gameStateBuffers.KeyValuePairs[j].Value);
		}
		writer.EndObject();

		writer.EndObject();
	}

	writer.EndArray();
}

static void WriteCaptures(udtJSONExporter& writer, const udtParseDataCaptureBuffers& captureBuffers, u32 demoIndex)
{
	const udtParseDataBufferRange range = captureBuffers.CaptureRanges[demoIndex];
	const u32 first = range.FirstIndex;
	const u32 count = range.Count;
	if(count == 0)
	{
		return;
	}

	writer.StartArray("captures");
	writer.SetStringBuffer(captureBuffers.StringBuffer, captureBuffers.StringBufferSize);

	for(u32 i = first, end = first + count; i < end; ++i)
	{
		const udtParseDataCapture& info = captureBuffers.Captures[i];
		const bool playerNameValid = (info.Flags & (u32)udtParseDataCaptureMask::PlayerNameValid) != 0;
		const bool playerIndexValid = (info.Flags & (u32)udtParseDataCaptureMask::PlayerIndexValid) != 0;
		const bool distanceValid = (info.Flags & (u32)udtParseDataCaptureMask::DistanceValid) != 0;

		writer.StartObject();

		writer.WriteStringValue("map", info.MapName);
		if(playerNameValid)  writer.WriteStringValue("player name", info.PlayerName);
		if(playerIndexValid) writer.WriteIntValue("player index", info.PlayerIndex);
		writer.WriteIntValue("game state index", info.GameStateIndex);
		writer.WriteIntValue("pick up time", info.PickUpTimeMs);
		writer.WriteIntValue("capture time", info.CaptureTimeMs);
		if(distanceValid) writer.WriteIntValue("distance", (s32)info.Distance);
		writer.WriteBoolValue("base to base", (info.Flags & (u32)udtParseDataCaptureMask::BaseToBase) != 0);
		writer.WriteBoolValue("demo taker", (info.Flags & (u32)udtParseDataCaptureMask::DemoTaker) != 0);
		writer.WriteBoolValue("spectated player", (info.Flags & (u32)udtParseDataCaptureMask::FirstPersonPlayer) != 0);

		writer.EndObject();
	}

	writer.EndArray();
}

static void WriteScores(udtJSONExporter& writer, const udtParseDataScoreBuffers& scoreBuffers, u32 demoIndex)
{
	const udtParseDataBufferRange range = scoreBuffers.ScoreRanges[demoIndex];
	const u32 first = range.FirstIndex;
	const u32 count = range.Count;
	if(count == 0)
	{
		return;
	}

	writer.StartArray("scores");
	writer.SetStringBuffer(scoreBuffers.StringBuffer, scoreBuffers.StringBufferSize);

	for(u32 i = first, end = first + count; i < end; ++i)
	{
		const udtParseDataScore& info = scoreBuffers.Scores[i];
		const bool teamMode = (info.Flags & (u32)udtParseDataScoreMask::TeamBased) != 0;

		writer.StartObject();

		writer.WriteIntValue("game state index", info.GameStateIndex);
		writer.WriteIntValue("server time", info.ServerTimeMs);
		writer.WriteBoolValue("team mode", teamMode);
		writer.WriteIntValue("score 1", info.Score1);
		writer.WriteIntValue("score 2", info.Score2);
		if(!teamMode)
		{
			if(info.Id1 < 64) writer.WriteIntValue("client 1", (s32)info.Id1);
			if(info.Id2 < 64) writer.WriteIntValue("client 2", (s32)info.Id2);
			if(info.Name1 != UDT_U32_MAX) writer.WriteStringValue("name 1", info.Name1);
			if(info.Name2 != UDT_U32_MAX) writer.WriteStringValue("name 2", info.Name2);
			if(info.CleanName1 != UDT_U32_MAX) writer.WriteStringValue("clean name 1", info.CleanName1);
			if(info.CleanName2 != UDT_U32_MAX) writer.WriteStringValue("clean name 2", info.CleanName2);
		}

		writer.EndObject();
	}

	writer.EndArray();
}

bool ExportPlugInsDataToJSON(udtParserContext* context, u32 demoIndex, const char* jsonPath)
{
	udtFileStream jsonFile;
	if(jsonPath != NULL)
	{
		if(!jsonFile.Open(jsonPath, udtFileOpenMode::Write))
		{
			context->Context.LogError("Failed to open file '%s' for writing", jsonPath);
			return false;
		}
	}

	context->JSONWriterContext.ResetForNextDemo();
	udtJSONWriter& writer = context->JSONWriterContext.Writer;
	udtVMLinearAllocator& tempAllocator = context->Parser._tempAllocator;
	udtJSONExporter jsonWriter(writer, tempAllocator);

	writer.StartFile();

	{
		udtParseDataGameStateBuffers gameStateBuffers;
		if(udtGetContextPlugInBuffers(context, (u32)udtParserPlugIn::GameState, &gameStateBuffers) == (s32)udtErrorCode::None)
		{
			WriteGameStates(jsonWriter, gameStateBuffers, demoIndex);
		}
	}

	{
		udtParseDataChatBuffers chatBuffers;
		if(udtGetContextPlugInBuffers(context, (u32)udtParserPlugIn::Chat, &chatBuffers) == (s32)udtErrorCode::None)
		{
			WriteChatEvents(jsonWriter, chatBuffers, demoIndex);
		}
	}

	{
		udtParseDataObituaryBuffers deathBuffers;
		if(udtGetContextPlugInBuffers(context, (u32)udtParserPlugIn::Obituaries, &deathBuffers) == (s32)udtErrorCode::None)
		{
			WriteDeathEvents(jsonWriter, deathBuffers, demoIndex);
		}
	}

	{
		udtParseDataStatsBuffers statsBuffers;
		if(udtGetContextPlugInBuffers(context, (u32)udtParserPlugIn::Stats, &statsBuffers) == (s32)udtErrorCode::None)
		{
			u32 dummy = 0;
			const char** playerStatsFieldNames = NULL;
			const char** teamStatsFieldNames = NULL;
			udtGetStringArray(udtStringArray::PlayerStatsNames, &playerStatsFieldNames, &dummy);
			udtGetStringArray(udtStringArray::TeamStatsNames, &teamStatsFieldNames, &dummy);
			WriteStats(jsonWriter, statsBuffers, demoIndex, playerStatsFieldNames, teamStatsFieldNames);
		}
	}

	{
		udtParseDataRawCommandBuffers commandBuffers;
		if(udtGetContextPlugInBuffers(context, (u32)udtParserPlugIn::RawCommands, &commandBuffers) == (s32)udtErrorCode::None)
		{
			WriteRawCommands(jsonWriter, commandBuffers, demoIndex);
		}
	}

	{
		udtParseDataRawConfigStringBuffers configStringBuffers;
		if(udtGetContextPlugInBuffers(context, (u32)udtParserPlugIn::RawConfigStrings, &configStringBuffers) == (s32)udtErrorCode::None)
		{
			WriteRawConfigStrings(jsonWriter, configStringBuffers, demoIndex);
		}
	}

	{
		udtParseDataCaptureBuffers captureBuffers;
		if(udtGetContextPlugInBuffers(context, (u32)udtParserPlugIn::Captures, &captureBuffers) == (s32)udtErrorCode::None)
		{
			WriteCaptures(jsonWriter, captureBuffers, demoIndex);
		}
	}

	{
		udtParseDataScoreBuffers scoreBuffers;
		if(udtGetContextPlugInBuffers(context, (u32)udtParserPlugIn::Scores, &scoreBuffers) == (s32)udtErrorCode::None)
		{
			WriteScores(jsonWriter, scoreBuffers, demoIndex);
		}
	}

	writer.EndFile();

	udtVMMemoryStream& memoryStream = context->JSONWriterContext.MemoryStream;
	if(jsonPath == NULL)
	{
		return fwrite(memoryStream.GetBuffer(), (size_t)memoryStream.Length(), 1, stdout) == 1;
	}

	if(jsonFile.Write(memoryStream.GetBuffer(), (u32)memoryStream.Length(), 1) != 1)
	{
		context->Context.LogError("Failed to write to JSON file '%s'", jsonPath);
		return false;
	}

	context->Context.LogInfo("Successfully wrote to file '%s'.", jsonPath);

	return true;
}
