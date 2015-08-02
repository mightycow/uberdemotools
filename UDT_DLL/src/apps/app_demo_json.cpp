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

static void WriteUDTTeamIndex(udtJSONWriter& writer, s32 udtTeamIndex)
{
	const char** teamNames = NULL;
	u32 teamCount = 0;
	if(udtGetStringArray(udtStringArray::Teams, &teamNames, &teamCount) != (s32)udtErrorCode::None)
	{
		return;
	}

	if(udtTeamIndex < 0 || udtTeamIndex >= (s32)teamCount)
	{
		return;
	}

	writer.WriteStringValue("team", teamNames[udtTeamIndex]);
}

static void WriteUDTGameType(udtJSONWriter& writer, u32 udtGameType)
{
	const char** gameTypeNames = NULL;
	u32 gameTypeCount = 0;
	if(udtGetStringArray(udtStringArray::GameTypes, &gameTypeNames, &gameTypeCount) != (s32)udtErrorCode::None)
	{
		return;
	}

	if(udtGameType >= gameTypeCount)
	{
		return;
	}

	writer.WriteStringValue("game type", gameTypeNames[udtGameType]);
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

	if(stats.Name != NULL)
	{
		writer.WriteStringValue("name", stats.Name);
	}

	if(stats.CleanName != NULL)
	{
		writer.WriteStringValue("clean name", stats.CleanName);
	}

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

	writer.StartObject("game stats");

	WriteUDTGameType(writer, stats.GameType);

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

static int ProcessDemo(const char* demoPath, const char* jsonPath)
{
	s32 cancel = 0;
	s32 errorCode = 0;
	const u32 statsPlugIn = (u32)udtParserPlugIn::Stats;

	udtParseArg info;
	memset(&info, 0, sizeof(info));
	info.CancelOperation = &cancel;
	info.MessageCb = &CallbackConsoleMessage;
	info.ProgressCb = &CallbackConsoleProgress;
	info.PlugIns = &statsPlugIn;
	info.PlugInCount = 1;

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
	if(udtGetDemoDataInfo(context, 0, (u32)udtParserPlugIn::Stats, &statsPointer, &statsCount) != (s32)udtErrorCode::None)
	{
		return __LINE__;
	}

	if(statsPointer == NULL)
	{
		return __LINE__;
	}

	udtVMMemoryStream memoryStream;
	if(!memoryStream.Open(1 << 20))
	{
		return __LINE__;
	}

	const udtParseDataStats* const stats = (const udtParseDataStats*)statsPointer;
	udtJSONWriter jsonWriter;
	jsonWriter.SetOutputStream(&memoryStream);
	jsonWriter.StartFile();
	for(u32 i = 0; i < statsCount; ++i)
	{
		WriteStats(jsonWriter, stats[i]);
	}
	jsonWriter.EndFile();

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
