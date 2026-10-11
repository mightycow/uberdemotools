#include "tests.hpp"
#include "uberdemotools.h"
extern "C"
{
#include "json.h"
}
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

UDT_TEST_FILE("CLI");

static void Run(const char* exeName, const char* argsFormat, ...)
{
	char args[1024];
	char cmd[1024];
	va_list argList;
	va_start(argList, argsFormat);
	vsprintf(args, argsFormat, argList);
	va_end(argList);
#if defined(_WIN32)
	sprintf(cmd, "%s.exe %s", exeName, args);
#else
	sprintf(cmd, "./%s %s", exeName, args);
#endif
	printf("> %s\n", cmd);
	system(cmd);
}

static bool ValidateCutGameStateRange(const char* jsonString, int startSec, int endSec)
{
	const json gsArrayNode = json_get(jsonString, "gameStates");
	UDT_ENSURE(json_type(gsArrayNode) == JSON_ARRAY);
	UDT_ENSURE(json_array_count(gsArrayNode) == 1);
	const json gs0Node = json_array_get(gsArrayNode, 0);
	UDT_ENSURE(json_type(gs0Node) == JSON_OBJECT);
	const json startNode = json_object_get(gs0Node, "startTime");
	UDT_ENSURE(json_type(startNode) == JSON_NUMBER);
	const json endNode = json_object_get(gs0Node, "endTime");
	UDT_ENSURE(json_type(startNode) == JSON_NUMBER);
	const int start = json_int(startNode) / 1000;
	const int end = json_int(endNode) / 1000;
	UDT_ENSURE(start == startSec);
	UDT_ENSURE(end == endSec || end == endSec - 1);

	return true;
}

static bool ValidatePlayerNames(const char* jsonString, const char** cleanPlayerNames)
{
	const json gsArrayNode = json_get(jsonString, "gameStates");
	UDT_ENSURE(json_type(gsArrayNode) == JSON_ARRAY);
	UDT_ENSURE(json_array_count(gsArrayNode) == 1);
	const json gs0Node = json_array_get(gsArrayNode, 0);
	UDT_ENSURE(json_type(gs0Node) == JSON_OBJECT);
	const json playerArrayNode = json_object_get(gs0Node, "players");
	UDT_ENSURE(json_type(playerArrayNode) == JSON_ARRAY);
	json playerNode = json_first(playerArrayNode);
	while(json_type(playerNode) == JSON_OBJECT)
	{
		const json idNode = json_object_get(playerNode, "clientNumber");
		UDT_ENSURE(json_type(idNode) == JSON_NUMBER);
		const int id = json_int(idNode);
		UDT_ENSURE(id >= 0 && id < ID_MAX_CLIENTS);
		UDT_ENSURE(cleanPlayerNames[id] != nullptr);

		const json nameNode = json_object_get(playerNode, "cleanName");
		UDT_ENSURE(json_type(nameNode) == JSON_STRING);
		char name[64];
		json_string_copy(nameNode, name, sizeof(name));
		UDT_ENSURE(json_string_compare(nameNode, cleanPlayerNames[id]) == 0);

		playerNode = json_next(playerNode);
	}

	return true;
}

UDT_TEST("cut/time/wrong_cut_time_42_653.dm_91")
{
	Run("UDT_cutter", "t -g=0 -s=42 -e=653 \"-o=%s\" \"%s/wrong_cut_time_42_653.dm_91\"", ctx.OutTempDir.Path, ctx.DemoDir);
	ctx.OutTempDir.ListFiles();
	const auto& newFiles = ctx.OutTempDir.GetNewFiles();
	UDT_ENSURE(newFiles.GetSize() == 1);
	const char* const jsonString = RunAndCaptureOutput("UDT_json", "-c -a=g \"%s\"", newFiles[0].Path.GetPtr());
	UDT_ENSURE(jsonString != nullptr);
	UDT_ENSURE(json_valid(jsonString));
	json_parse(jsonString);
	UDT_ENSURE(ValidateCutGameStateRange(jsonString, 42, 653));

	return true;
}

UDT_TEST("cut/time/cuts_entire_demo_2526_2536.dm_68")
{
	const int startSec = ServerTimeSec(25, 26);
	const int endSec = ServerTimeSec(25, 36);
	const int realEndSec = ServerTimeSec(25, 32);
	Run("UDT_cutter", "t -g=0 -s=%d -e=%d \"-o=%s\" \"%s/%s\"",
		startSec, endSec, ctx.OutTempDir.Path, ctx.DemoDir, "cuts_entire_demo_2526_2536.dm_68");
	ctx.OutTempDir.ListFiles();
	const auto& newFiles = ctx.OutTempDir.GetNewFiles();
	UDT_ENSURE(newFiles.GetSize() == 1);
	const char* const jsonString = RunAndCaptureOutput("UDT_json", "-c -a=g \"%s\"", newFiles[0].Path.GetPtr());
	UDT_ENSURE(jsonString != nullptr);
	UDT_ENSURE(json_valid(jsonString));
	json_parse(jsonString);
	UDT_ENSURE(ValidateCutGameStateRange(jsonString, startSec, realEndSec));

	return true;
}

UDT_TEST("cut/time/enemy_has_no_name_512_537.dm_68")
{
	const int startSec = ServerTimeSec(5, 12);
	const int endSec = ServerTimeSec(5, 37);
	Run("UDT_cutter", "t -g=0 -s=%d -e=%d \"-o=%s\" \"%s/%s\"",
		startSec, endSec, ctx.OutTempDir.Path, ctx.DemoDir, "enemy_has_no_name_512_537.dm_68");
	ctx.OutTempDir.ListFiles();
	const auto& newFiles = ctx.OutTempDir.GetNewFiles();
	UDT_ENSURE(newFiles.GetSize() == 1);
	const char* const jsonString = RunAndCaptureOutput("UDT_json", "-c -a=g \"%s\"", newFiles[0].Path.GetPtr());
	UDT_ENSURE(jsonString != nullptr);
	UDT_ENSURE(json_valid(jsonString));
	json_parse(jsonString);
	UDT_ENSURE(ValidateCutGameStateRange(jsonString, startSec, endSec));
	const char* playerNames[ID_MAX_CLIENTS] =
	{
		"#ugq",
		"sim the sppoky ghost", // <-- "sim" with the old QL clan parsing enabled
		"venim",
		"oranje",
		"rush",
		"Sooint",
		"Benroads",
		"ofnlut_"
	};
	UDT_ENSURE(ValidatePlayerNames(jsonString, playerNames));

	return true;
}

#if 0 // @TODO: these fail!

// @TODO: similar: edawn_1_map_2_matches.dm_68
UDT_TEST("json/matches/baseq3_1_map_2_matches.dm_68")
{
	const char* const jsonString = RunAndCaptureOutput("UDT_json", "-c -a=g \"%s/%s\"",
		ctx.DemoDir,
		"baseq3_1_map_2_matches.dm_68");
	UDT_ENSURE(jsonString != nullptr);
	UDT_ENSURE(json_valid(jsonString));
	json_parse(jsonString);
	const json gsArrayNode = json_get(jsonString, "gameStates");
	UDT_ENSURE(json_type(gsArrayNode) == JSON_ARRAY);
	UDT_ENSURE(json_array_count(gsArrayNode) == 1);
	const json gs0Node = json_array_get(gsArrayNode, 0);
	UDT_ENSURE(json_type(gs0Node) == JSON_OBJECT);
	const json matchNode = json_object_get(gs0Node, "matches");
	UDT_ENSURE(json_type(matchNode) == JSON_ARRAY);
	UDT_ENSURE(json_array_count(matchNode) == 2);
	// @TODO: check start and end times too...
}

#endif
