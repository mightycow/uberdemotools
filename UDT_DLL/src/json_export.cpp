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


static udtString NewCamelCaseString(udtVMLinearAllocator& allocator, const udtString& name)
{
	if(udtString::IsNullOrEmpty(name))
	{
		return name;
	}

	udtString fixed = udtString::NewCloneFromRef(allocator, name);
	char* dest = fixed.String;
	const char* src = fixed.String;
	*dest++ = (char)::tolower((int)*src++);
	while(*src)
	{
		if(src[0] != ' ')
		{
			*dest++ = *src++;
			continue;
		}

		if(src[1] == '\0')
		{
			break;
		}

		*dest++ = (char)::toupper((int)src[1]);
		src += 2;
	}

	*dest = '\0';
	fixed.Length = (u32)(dest - name.String);

	return fixed;
}

struct udtJSONExporter
{
public:
	udtJSONExporter(udtJSONWriter& writer, udtVMLinearAllocator& tempAllocator)
		: Writer(writer)
		, TempAllocator(tempAllocator)
	{
	}

	void StartObject()
	{
		Writer.StartObject();
	}

	void StartObject(const char* name)
	{
		udtVMScopedStackAllocator allocatorScope(TempAllocator);
		Writer.StartObject(GetFixedName(name).String);
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
		Writer.StartArray(GetFixedName(name).String);
	}

	void EndArray()
	{
		Writer.EndArray();
	}

	void WriteBoolValue(const char* name, bool value)
	{
		udtVMScopedStackAllocator allocatorScope(TempAllocator);
		Writer.WriteBoolValue(GetFixedName(name).String, value);
	}

	void WriteIntValue(const char* name, s32 number)
	{
		udtVMScopedStackAllocator allocatorScope(TempAllocator);
		Writer.WriteIntValue(GetFixedName(name).String, number);
	}

	void WriteStringValue(const char* name, const char* string)
	{
		udtVMScopedStackAllocator allocatorScope(TempAllocator);
		Writer.WriteStringValue(GetFixedName(name).String, string);
	}

private:
	UDT_NO_COPY_SEMANTICS(udtJSONExporter);

private:
	udtString GetFixedName(const char* name)
	{
		return NewCamelCaseString(TempAllocator, udtString::NewConstRef(name));
	}

	udtJSONWriter& Writer;
	udtVMLinearAllocator& TempAllocator;
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
		const s32 byteIndex = i >> 3;
		const s32 bitIndex = i & 7;
		if((flags[byteIndex] & ((u8)1 << (u8)bitIndex)) != 0)
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
		const s32 byteIndex = i >> 3;
		const s32 bitIndex = i & 7;
		if((flags[byteIndex] & ((u8)1 << (u8)bitIndex)) != 0)
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

static void WriteStats(udtJSONExporter& writer, const udtParseDataStats* statsArray, u32 count, const char** playerStatsFieldNames, const char** teamStatsFieldNames)
{
	if(count == 0 ||
	   playerStatsFieldNames == NULL ||
	   teamStatsFieldNames == NULL)
	{
		return;
	}

	writer.StartArray("match stats");

	for(u32 matchIdx = 0; matchIdx < count; ++matchIdx)
	{
		const udtParseDataStats& stats = statsArray[matchIdx];

		const bool hasTeamStats = HasValidTeamStats(stats);
		const bool hasPlayerStats = HasValidPlayerStats(stats);
		if(!hasTeamStats && !hasPlayerStats)
		{
			continue;
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

		writer.StartObject();

		writer.WriteStringValue("winner", stats.SecondPlaceWon ? stats.SecondPlaceName : stats.FirstPlaceName);
		writer.WriteStringValue("first place name", stats.FirstPlaceName);
		writer.WriteStringValue("second place name", stats.SecondPlaceName);
		writer.WriteIntValue("first place score", (s32)stats.FirstPlaceScore);
		writer.WriteIntValue("second place score", (s32)stats.SecondPlaceScore);
		writer.WriteStringValue("custom red name", stats.CustomRedName);
		writer.WriteStringValue("custom blue name", stats.CustomBlueName);
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
		writer.WriteBoolValue("leader forfeited", stats.SecondPlaceWon != 0);
		writer.WriteBoolValue("mercy limited", stats.MercyLimited != 0);
		WriteUDTGamePlayShort(writer, stats.GamePlay);
		WriteUDTGamePlayLong(writer, stats.GamePlay);
		writer.WriteIntValue("time out count", (s32)stats.TimeOutCount);
		if(stats.TimeOutCount > 0)
		{
			writer.WriteIntValue("total time out duration", (s32)stats.TotalTimeOutDurationMs);
		}

		if(stats.TimeOutCount > 0)
		{
			writer.StartArray("time outs");

			for(u32 i = 0; i < stats.TimeOutCount; ++i)
			{
				writer.StartObject();
				writer.WriteIntValue("start time", stats.TimeOutStartAndEndTimes[2 * i]);
				writer.WriteIntValue("end time", stats.TimeOutStartAndEndTimes[2 * i + 1]);
				writer.EndObject();
			}

			writer.EndArray();
		}

		if(hasTeamStats)
		{
			const u8* flags = stats.TeamFlags;
			const s32* fields = stats.TeamFields;

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
			const u8* flags = stats.PlayerFlags;
			const s32* fields = stats.PlayerFields;
			const udtPlayerStats* extraStats = stats.PlayerStats;

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

static void WriteChatEvents(udtJSONExporter& writer, const udtParseDataChat* chatEvents, u32 count)
{
	if(count == 0)
	{
		return;
	}

	writer.StartArray("chat");

	for(u32 i = 0; i < count; ++i)
	{
		const udtParseDataChat& info = chatEvents[i];

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

static void WriteDeathEvents(udtJSONExporter& writer, const udtParseDataObituary* deathEvents, u32 count)
{
	if(count == 0)
	{
		return;
	}

	writer.StartArray("obituaries");

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

	writer.EndArray();
}

static void WriteRawCommands(udtJSONExporter& writer, const udtParseDataRawCommand* commands, u32 count)
{
	if(count == 0)
	{
		return;
	}

	writer.StartArray("raw commands");

	for(u32 i = 0; i < count; ++i)
	{
		const udtParseDataRawCommand& info = commands[i];

		writer.StartObject();

		writer.WriteIntValue("game state number", info.GameStateIndex + 1);
		writer.WriteIntValue("server time", info.ServerTimeMs);
		writer.WriteStringValue("raw command", info.RawCommand);
		writer.WriteStringValue("clean command", info.CleanCommand);

		writer.EndObject();
	}

	writer.EndArray();
}

static void WriteRawConfigStrings(udtJSONExporter& writer, const udtParseDataRawConfigString* configString, u32 count)
{
	if(count == 0)
	{
		return;
	}

	writer.StartArray("raw config strings");

	for(u32 i = 0; i < count; ++i)
	{
		const udtParseDataRawConfigString& info = configString[i];

		writer.StartObject();

		writer.WriteIntValue("game state number", info.GameStateIndex + 1);
		writer.WriteIntValue("index", info.ConfigStringIndex);
		writer.WriteStringValue("raw string", info.RawConfigString);
		writer.WriteStringValue("clean string", info.CleanConfigString);

		writer.EndObject();
	}

	writer.EndArray();
}

static void WriteGameStates(udtJSONExporter& writer, const udtParseDataGameState* gameStates, u32 count)
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

		writer.StartArray("players");
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
		writer.EndArray();

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

bool ExportPlugInsDataToJSON(udtParserContext* context, u32 demoIndex, const char* jsonPath)
{
	udtFileStream jsonFile;
	if(!jsonFile.Open(jsonPath, udtFileOpenMode::Write))
	{
		context->Context.LogError("Failed to open file '%s' for writing", jsonPath);
		return false;
	}

	context->JSONWriterContext.ResetForNextDemo();
	udtJSONWriter& writer = context->JSONWriterContext.Writer;
	udtVMLinearAllocator& tempAllocator = context->Parser._tempAllocator;
	udtJSONExporter jsonWriter(writer, tempAllocator);

	writer.StartFile();

	void* gameStatesPointer = NULL;
	u32 gameStateCount = 0;
	if(udtGetDemoDataInfo(context, demoIndex, (u32)udtParserPlugIn::GameState, &gameStatesPointer, &gameStateCount) == (s32)udtErrorCode::None &&
	   gameStatesPointer != NULL)
	{
		WriteGameStates(jsonWriter, (const udtParseDataGameState*)gameStatesPointer, gameStateCount);
	}

	void* chatEventPointer = NULL;
	u32 chatEventCount = 0;
	if(udtGetDemoDataInfo(context, demoIndex, (u32)udtParserPlugIn::Chat, &chatEventPointer, &chatEventCount) == (s32)udtErrorCode::None &&
	   chatEventPointer != NULL)
	{
		WriteChatEvents(jsonWriter, (const udtParseDataChat*)chatEventPointer, chatEventCount);
	}

	void* deathEventPointer = NULL;
	u32 deathEventCount = 0;
	if(udtGetDemoDataInfo(context, demoIndex, (u32)udtParserPlugIn::Obituaries, &deathEventPointer, &deathEventCount) == (s32)udtErrorCode::None &&
	   deathEventPointer != NULL)
	{
		WriteDeathEvents(jsonWriter, (const udtParseDataObituary*)deathEventPointer, deathEventCount);
	}

	void* statsPointer = NULL;
	u32 statsCount = 0;
	if(udtGetDemoDataInfo(context, demoIndex, (u32)udtParserPlugIn::Stats, &statsPointer, &statsCount) == (s32)udtErrorCode::None &&
	   statsPointer != NULL)
	{
		u32 dummy = 0;
		const char** playerStatsFieldNames = NULL;
		const char** teamStatsFieldNames = NULL;
		udtGetStringArray(udtStringArray::PlayerStatsNames, &playerStatsFieldNames, &dummy);
		udtGetStringArray(udtStringArray::TeamStatsNames, &teamStatsFieldNames, &dummy);
		WriteStats(jsonWriter, (const udtParseDataStats*)statsPointer, statsCount, playerStatsFieldNames, teamStatsFieldNames);
	}

	void* rawEventsPointer = NULL;
	u32 rawEventCount = 0;
	if(udtGetDemoDataInfo(context, demoIndex, (u32)udtParserPlugIn::RawCommands, &rawEventsPointer, &rawEventCount) == (s32)udtErrorCode::None &&
	   rawEventsPointer != NULL)
	{
		WriteRawCommands(jsonWriter, (const udtParseDataRawCommand*)rawEventsPointer, rawEventCount);
	}

	void* rawConfigStringsPointer = NULL;
	u32 rawConfigStringCount = 0;
	if(udtGetDemoDataInfo(context, demoIndex, (u32)udtParserPlugIn::RawConfigStrings, &rawConfigStringsPointer, &rawConfigStringCount) == (s32)udtErrorCode::None &&
	   rawConfigStringsPointer != NULL)
	{
		WriteRawConfigStrings(jsonWriter, (const udtParseDataRawConfigString*)rawConfigStringsPointer, rawConfigStringCount);
	}

	writer.EndFile();

	udtVMMemoryStream& memoryStream = context->JSONWriterContext.MemoryStream;
	if(jsonFile.Write(memoryStream.GetBuffer(), (u32)memoryStream.Length(), 1) != 1)
	{
		context->Context.LogError("Failed to write to JSON file '%s'", jsonPath);
		return false;
	}

	context->Context.LogInfo("Successfully wrote to file '%s'.", jsonPath);

	return true;
}
