#include "parser.hpp"
#include "file_stream.hpp"
#include "utils.hpp"
#include "shared.hpp"
#include "file_system.hpp"
#include "parser_context.hpp"
#include "timer.hpp"
#include "stack_trace.hpp"
#include "path.hpp"
#include "batch_runner.hpp"
#include "scoped_stack_allocator.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <float.h>


#define CUTTER_BATCH_SIZE 256


struct ProgramOptions
{
	const char* ConfigFilePath;
	const char* OutputFolderPath;
	u32 MaxThreadCount;
	u32 GameStateIndex;
	s32 StartTimeSec;
	s32 EndTimeSec;
	s32 StartOffsetSec;
	s32 EndOffsetSec;
	s32 PlayerIndex;
	bool Recursive;
	udtMultiRailPatternArg MultiRail;
	udtFlickRailPatternArg FlickRail;
	udtMidAirPatternArg MidAir;
	s32 AllowRL;
	s32 AllowBFG;
	s32 AllowGL;
	s32 AllowPF;
	udtFragRunPatternArg FragRun;
	s32 AllowSelfKills;
	s32 AllowTeamKills;
	s32 AllowDeaths;
	udtFlagCapturePatternArg FlagCapture;
};

static ProgramOptions options;

struct OptionType
{
	enum Id
	{
		Integer,
		Boolean,
		String,
		Seconds,
		AngleRad
	};
};

struct CmdLineOption
{
	const char* Name;
	const char* Desc;
	bool Mandatory;
	void* Data;
	const char* CommandsInc;
	const char* CommandsEx;
	OptionType::Id Type;
	const char* Default;
	const char* Min;
	const char* Max;
};

static const CmdLineOption cmdLineOptions[] =
{
	// general (mostly)
	{ "q", "quiet mode: no logging to stdout", false, NULL, NULL, NULL, OptionType::Boolean },
	{ "o", "output folder path", false, &options.OutputFolderPath, NULL, "g", OptionType::String },
	{ "g", "game state index (0-based)", false, &options.GameStateIndex, "t", NULL, OptionType::Integer, "0", "0" },
	{ "s", "start server time (minutes:seconds)", true, &options.StartTimeSec, "t", NULL, OptionType::Seconds, "-2147483648" },
	{ "e", "end server time (minutes:seconds)", true, &options.EndTimeSec, "t", NULL, OptionType::Seconds, "-2147483648" },
	{ "s", "start time offset (minutes:seconds)", false, &options.StartOffsetSec, NULL, "tg", OptionType::Seconds, "10" },
	{ "e", "end time offset (minutes:seconds)", false, &options.EndOffsetSec, NULL, "tg", OptionType::Seconds, "10" },
	{ "t", "maximum thread count", false, &options.MaxThreadCount, NULL, "tg", OptionType::Integer, "1", "1", "16" },
	{ "r", "enable recursive demo file search", false, &options.Recursive, NULL, "tg", OptionType::Boolean },
	{ "c", "cut by chat config file path", false, &options.ConfigFilePath, "cg", NULL, OptionType::String },
	{ "p",
		"player tracking mode\n"
		"            0-63: player id\n"
		"            -1  : demo taker\n"
		"            -2  : followed player",
		false, &options.PlayerIndex, "rfas", NULL, OptionType::Integer, "-2", "-2", "63" },
	// flick rails
	{ "angle", "min. angle delta, in degrees", false, &options.FlickRail.MinAngleDelta, "f", NULL, OptionType::AngleRad, "30", "0", NULL },
	{ "angle_snaps", "min. angle delta snapshot count", false, &options.FlickRail.MinAngleDeltaSnapshotCount, "f", NULL, OptionType::Integer, "2", "2", "4" },
	{ "speed", "min. angular speed, in degrees per second", false, &options.FlickRail.MinSpeed, "f", NULL, OptionType::AngleRad, "600", "0", NULL },
	{ "speed_snaps", "min. angular speed snapshot count", false, &options.FlickRail.MinSpeedSnapshotCount, "f", NULL, OptionType::Integer, "4", "2", "4" },
	// frag runs
	{ "suicides", "allow self kills", false, &options.AllowSelfKills, "s", NULL, OptionType::Integer, "1", "0", "1" },
	{ "team_kills", "allow team kills", false, &options.AllowTeamKills, "s", NULL, OptionType::Integer, "1", "0", "1" },
	{ "deaths", "allow deaths", false, &options.AllowDeaths, "s", NULL, OptionType::Integer, "1", "0", "1" },
	{ "frags", "min. frag count", false, &options.FragRun.MinFragCount, "s", NULL, OptionType::Integer, "4", "0", NULL },
	{ "duration", "max. time interval between consecutive frags, in seconds", false, &options.FragRun.TimeBetweenFragsSec, "s", NULL, OptionType::Integer, "4", "1", NULL },
	// mid air
	{ "RL", "allow rocket launcher", false, &options.AllowRL, "a", NULL, OptionType::Integer, "1", "0", "1" },
	{ "BFG", "allow big fucking gun", false, &options.AllowBFG, "a", NULL, OptionType::Integer, "1", "0", "1" },
	{ "GL", "allow grenade launcher", false, &options.AllowGL, "a", NULL, OptionType::Integer, "1", "0", "1" },
	{ "PF", "allow Panzerfaust", false, &options.AllowPF, "a", NULL, OptionType::Integer, "1", "0", "1" },
	{ "distance", "min. distance traveled", false, &options.MidAir.MinDistance, "a", NULL, OptionType::Integer, "500", "0", NULL },
	{ "duration", "min. time the victim was airborne prior to the hit, in milliseconds", false, &options.MidAir.MinAirTimeMs, "a", NULL, OptionType::Integer, "250", "0", NULL },
	// multi rail
	{ "frags", "min. amount of frags with a single rail shot", false, &options.MultiRail.MinKillCount, "r", NULL, OptionType::Integer, "2", "2", NULL },
	// flag capture
	{ "min_duration", "min. flag carry time (minutes:seconds)", false, &options.FlagCapture.MinCarryTimeMs, "p", NULL, OptionType::Seconds, "0", "0", NULL },
	{ "max_duration", "max. flag carry time (minutes:seconds)", false, &options.FlagCapture.MaxCarryTimeMs, "p", NULL, OptionType::Seconds, "1200", "1", NULL },
	{ "base", "allow pick-ups from the original flag spot", false, &options.FlagCapture.AllowBaseToBase, "p", NULL, OptionType::Integer, "1", "0", "1" },
	{ "non_base", "allow pick-ups *not* from the original flag spot", false, &options.FlagCapture.AllowMissingToBase, "p", NULL, OptionType::Integer, "1", "0", "1" }
};

struct OptionRangeInt
{
	s32 Min;
	s32 Max;
};

struct OptionRangeFloat
{
	f32 Min;
	f32 Max;
};

union OptionRange
{
	OptionRangeInt Int;
	OptionRangeFloat Float;
};

struct CmdLineOptionMut
{
	OptionRange Range;
	bool Set;
};

static CmdLineOptionMut cmdLineOptionsMut[sizeof(cmdLineOptions) / sizeof(cmdLineOptions[0])];

struct Command
{
	char Letter;
	const char* Desc;
};

static const Command cmdLineCommands[] =
{
	{ 't', "cut by time" },
	{ 'c', "cut by chat" },
	{ 'm', "cut by matches" },
	{ 'r', "cut by multi-frag rails" },
	{ 'f', "cut by flick rails" },
	{ 'a', "cut by mid-airs" },
	{ 's', "cut by frag sequences" },
	{ 'p', "cut by flag captures" },
	{ 'g', "generate a cut by chat example config" }
};

static bool IsValidCommand(char command)
{
	for(u32 i = 0, count = (u32)UDT_COUNT_OF(cmdLineCommands); i < count; ++i)
	{
		if(command == cmdLineCommands[i].Letter)
		{
			return true;
		}
	}

	return false;
}

static bool IsValidOption(const CmdLineOption& option, char command)
{
	bool valid = false;
	if(option.CommandsInc != NULL)
	{
		for(const char* c = option.CommandsInc; *c != '\0'; ++c)
		{
			if(*c == command)
			{
				valid = true;
				break;
			}
		}
	}
	else
	{
		valid = true;
	}

	if(option.CommandsEx != NULL)
	{
		for(const char* c = option.CommandsEx; *c != '\0'; ++c)
		{
			if(*c == command)
			{
				valid = false;
				break;
			}
		}
	}

	return valid;
}

static void InitOptions()
{
	for(u32 i = 0; i < UDT_COUNT_OF(cmdLineOptions); ++i)
	{
		const CmdLineOption& option = cmdLineOptions[i];
		if(option.Data == NULL)
		{
			continue;
		}

		CmdLineOptionMut& optionMut = cmdLineOptionsMut[i];
		optionMut.Set = false;
		switch(option.Type)
		{
			case OptionType::Integer:
			case OptionType::Seconds:
				optionMut.Range.Int.Min = option.Min ? atoi(option.Min) : UDT_S32_MIN;
				optionMut.Range.Int.Max = option.Max ? atoi(option.Max) : UDT_S32_MAX;
				break;
			case OptionType::AngleRad:
				optionMut.Range.Float.Min = option.Min ? DegToRad((f32)atof(option.Min)) : -FLT_MAX;
				optionMut.Range.Float.Max = option.Max ? DegToRad((f32)atof(option.Max)) : FLT_MAX;
				break;
			default:
				break;
		}

		switch(option.Type)
		{
			case OptionType::Integer:
			case OptionType::Seconds:
				*(s32*)option.Data = option.Default ? atoi(option.Default) : 0;
				break;
			case OptionType::Boolean:
				*(bool*)option.Data = false;
				break;
			case OptionType::String:
				*(const char**)option.Data = option.Default;
				break;
			case OptionType::AngleRad:
				*(f32*)option.Data = option.Default ? DegToRad((float)atof(option.Default)) : 0.0f;
				break;
			default:
				assert(0);
		}
	}
}

static void ParseOptions(int firstOption, int afterLastOption, char** argv, char command)
{
	for(int i = firstOption; i < afterLastOption; ++i)
	{
		bool found = false;
		bool valid = false;
		u32 optionIndex = 0;

		const udtString arg = udtString::NewConstRef(argv[i]);
		for(u32 j = 0; j < UDT_COUNT_OF(cmdLineOptions); ++j)
		{
			const CmdLineOption& option = cmdLineOptions[j];
			if(option.Data == NULL)
			{
				continue;
			}

			if(!IsValidOption(option, command))
			{
				continue;
			}

			char optionStart[64];
			sprintf(optionStart, "-%s", option.Name);

			if(option.Type == OptionType::Boolean)
			{
				if(udtString::EqualsNoCase(arg, optionStart))
				{
					*(bool*)option.Data = true;
					found = true;
					optionIndex = j;
					valid = true;
				}
				else
				{
					continue;
				}
			}

			strcat(optionStart, "=");
			if(!udtString::StartsWithNoCase(arg, optionStart))
			{
				continue;
			}

			const u32 startLength = (u32)strlen(optionStart);
			if(arg.GetLength() < startLength + 1)
			{
				continue;
			}

			found = true;
			optionIndex = j;

			CmdLineOptionMut& optionMut = cmdLineOptionsMut[j];
			const char* const value = argv[i] + startLength;
			s32 localInt = 0;
			f32 localFloat = 0.0f;
			switch(option.Type)
			{
				case OptionType::Integer:
					if(StringParseInt(localInt, value) &&
					   localInt >= optionMut.Range.Int.Min &&
					   localInt <= optionMut.Range.Int.Max)
					{
						*(s32*)option.Data = localInt;
						optionMut.Set = true;
						valid = true;
					}
					break;
				case OptionType::Seconds:
					if(StringParseSeconds(localInt, value) &&
					   localInt >= optionMut.Range.Int.Min &&
					   localInt <= optionMut.Range.Int.Max)
					{
						*(s32*)option.Data = localInt;
						optionMut.Set = true;
						valid = true;
					}
					break;
				case OptionType::String:
					*(const char**)option.Data = value;
					optionMut.Set = true;
					valid = true;
					break;
				case OptionType::AngleRad:
					if(StringParseFloat(localFloat, value) &&
					   localInt >= optionMut.Range.Float.Min &&
					   localInt <= optionMut.Range.Float.Max)
					{
						*(f32*)option.Data = DegToRad(localFloat);
						optionMut.Set = true;
						valid = true;
					}
					break;
				case OptionType::Boolean:
					optionMut.Set = true;
					valid = true;
					break;
				default:
					assert(0);
			}
		}

		if(found && !valid)
		{
			fprintf(stderr, "Command-line option '-%s' specified but invalid.\n", cmdLineOptions[optionIndex].Name);
		}
		else if(!found)
		{
			fprintf(stderr, "Command-line option '%s': unrecognized name/syntax.\n", argv[i]);
		}
	}
}

static const char* GetCamelCaseString(udtVMLinearAllocator& alloc, const char* str)
{
	return udtString::NewCamelCaseClone(alloc, udtString::NewConstRef(str)).GetPtr();
}

static u64 GetMeanOfDeathBitMask(udtVMLinearAllocator& alloc, const char* modRaw)
{
	const char** mods;
	u32 modCount;
	udtGetStringArray(udtStringArray::MeansOfDeath, &mods, &modCount);
	const udtString mod = udtString::NewConstRef(modRaw);
	for(u32 i = 0; i < modCount; ++i)
	{
		if(udtString::EqualsNoCase(mod, GetCamelCaseString(alloc, mods[i])))
		{
			return (u64)1 << (u64)i;
		}
	}

	return 0;
}

static void ParseCommands(int firstOption, int afterLastOption, char** argv, char command)
{
	if(command != 's')
	{
		return;
	}

	const char* const cmdAdd = "mod_add=";
	const char* const cmdRem = "mod_rem=";

	udtVMLinearAllocator alloc("ParseCommands::Temp");
	udtVMScopedStackAllocator scope(alloc);

	for(int i = firstOption; i < afterLastOption; ++i)
	{
		const udtString arg = udtString::NewConstRef(argv[i]);
		if(udtString::EqualsNoCase(arg, "mod_all"))
		{
			options.FragRun.AllowedMeansOfDeaths = u64(~0);
		}
		else if(udtString::EqualsNoCase(arg, "mod_none"))
		{
			options.FragRun.AllowedMeansOfDeaths = 0;
		}
		else if(udtString::StartsWithNoCase(arg, cmdAdd) && arg.GetLength() > strlen(cmdAdd))
		{
			options.FragRun.AllowedMeansOfDeaths |= GetMeanOfDeathBitMask(alloc, argv[i] + strlen(cmdAdd));
		}
		else if(udtString::StartsWithNoCase(arg, cmdRem) && arg.GetLength() > strlen(cmdRem))
		{
			options.FragRun.AllowedMeansOfDeaths &= ~GetMeanOfDeathBitMask(alloc, argv[i] + strlen(cmdRem));
		}
	}
}

struct CutByChatConfig
{
	udtVMArray<udtChatPatternRule> ChatRules { "CutByChatConfig::ChatRulesArray" };
	udtVMLinearAllocator StringAllocator { "CutByChatConfig::String" };
	// these are parsed from the config but can be overridden from the command-line
	int StartOffsetSec = 10;
	int EndOffsetSec = 10;
};

static const char* exampleConfig =
"// Generated by UDT_cutter, feel free to modify :)\n"
"// ChatOperator values: Contains, StartsWith, EndsWith\n"
"\n"
"// These can be overridden from the command-line\n"
"StartOffset 10\n"
"EndOffset 10\n"
"\n"
"[ChatRule]\n"
"ChatOperator Contains\n"
"Pattern WAXEDDD\n"
"CaseSensitive 1\n"
"IgnoreColorCodes 0\n"
"[/ChatRule]\n"
"\n"
"[ChatRule]\n"
"ChatOperator Contains\n"
"Pattern \"fragmovie frag\"\n"
"CaseSensitive 0\n"
"IgnoreColorCodes 1\n"
"[/ChatRule]\n";

static void PrintCommandSyntax(char cmd)
{
	printf("UDT_cutter %c", cmd);

	u32 optional = 0;
	for(u32 o = 0; o < UDT_COUNT_OF(cmdLineOptions); ++o)
	{
		const CmdLineOption& option = cmdLineOptions[o];
		if(IsValidOption(option, cmd) && !option.Mandatory)
		{
			if(optional == 0)
			{
				printf(" [-%s", option.Name);
			}
			else
			{
				printf("|%s", option.Name);
			}
			++optional;
		}
	}
	if(optional > 0)
	{
		printf("]");
	}

	for(u32 o = 0; o < UDT_COUNT_OF(cmdLineOptions); ++o)
	{
		const CmdLineOption& option = cmdLineOptions[o];
		if(IsValidOption(option, cmd) && option.Mandatory)
		{
			printf(" -%s", option.Name);
		}
	}

	switch(cmd)
	{
		case 'g':
			printf("\n");
			break;
		case 't':
			printf(" inputfile\n");
			break;
		default:
			printf(" inputfile|inputfolder\n");
			break;
	}
}

static void PrintCommandHelp(char cmd)
{
	PrintCommandSyntax(cmd);
	for(u32 o = 0; o < UDT_COUNT_OF(cmdLineOptions); ++o)
	{
		const CmdLineOption& option = cmdLineOptions[o];
		if(!IsValidOption(option, cmd))
		{
			continue;
		}

		CmdLineOptionMut& optionMut = cmdLineOptionsMut[o];
		printf("    -%s    %s\n", option.Name, option.Desc);
		if(option.Default != NULL)
		{
			printf("            default: %s\n", option.Default);
		}
		else if(option.Type == OptionType::Boolean)
		{
			printf("            default: off\n");
		}
		if(option.Min != NULL)
		{
			if(option.Type == OptionType::Integer || option.Type == OptionType::Seconds)
			{
				printf("            minimum: %d\n", optionMut.Range.Int.Min);
			}
			else if(option.Type == OptionType::AngleRad)
			{
				printf("            minimum: %f\n", optionMut.Range.Float.Min);
			}
		}
		if(option.Max != NULL)
		{
			if(option.Type == OptionType::Integer || option.Type == OptionType::Seconds)
			{
				printf("            maximum: %d\n", optionMut.Range.Int.Max);
			}
			else if(option.Type == OptionType::AngleRad)
			{
				printf("            maximum: %f\n", optionMut.Range.Float.Max);
			}
		}
	}

	if(cmd == 's')
	{
		udtVMLinearAllocator alloc("PrintCommandHelp::Temp");
		udtVMScopedStackAllocator scope(alloc);

		const char** mods;
		u32 modCount;
		udtGetStringArray(udtStringArray::MeansOfDeath, &mods, &modCount);
		printf("\n");
		printf("Means of death commands:\n");
		printf("    mod_all : enable  all means of death\n");
		printf("    mod_none: disable all means of death\n");
		printf("    mod_add : enable  the specified mean of death\n");
		printf("    mod_rem : disable the specified mean of death\n");
		printf("\n");
		printf("Means of death names\n");
		for(u32 i = 0; i < modCount; ++i)
		{
			printf("    %s\n", GetCamelCaseString(alloc, mods[i]));
		}
		printf("\n");
		printf("Example usage:\n");
		printf("- only allow lightning gun deaths:\n");
		printf("  UDT_cutter s mod_none mod_add=lightning -duration=5 -frags=3 -r $inputfolder\n");
		printf("- allow all death causes except the lightning gun:\n");
		printf("  UDT_cutter s mod_all mod_rem=lightning -duration=5 -frags=3 -r $inputfolder\n");
	}
}

void PrintHelp()
{
	printf("Cuts demos by (t)ime, (c)hat, (m)atches, multi-kill (r)ails, (f)lick rails, mid-(a)irs, frag (s)equences, flag ca(p)tures\n");
	printf("\n");
	for(u32 c = 0; c < UDT_COUNT_OF(cmdLineCommands); ++c)
	{
		PrintCommandSyntax(cmdLineCommands[c].Letter);
	}
	printf("UDT_cutter h ");
	for(u32 c = 0; c < UDT_COUNT_OF(cmdLineCommands); ++c)
	{
		if(c > 0)
		{
			printf("|");
		}
		printf("%c", cmdLineCommands[c].Letter);
	}
	printf("\n");
	printf("\n");
	for(u32 c = 0; c < UDT_COUNT_OF(cmdLineCommands); ++c)
	{
		printf("%c    %s\n", cmdLineCommands[c].Letter, cmdLineCommands[c].Desc);
	}
	printf("h    print detailed help for the given command and its options\n");
	printf("\n");
	printf("Start and end times/offsets (-s and -e) can be formatted as:\n");
	printf("- 'seconds'          (example: 192)\n");
	printf("- 'minutes:seconds'  (example: 3:12)\n");
	printf("\n");
	printf("In cut by matches...\n");
	printf("- the start offset is only applied if no pre-match countdown is found\n");
	printf("- the end offset is only applied if no post-match intermission is found\n");
}

static void InitRule(udtChatPatternRule& rule)
{
	memset(&rule, 0, sizeof(rule));
	rule.CaseSensitive = 1;
	rule.ChatOperator = (u32)udtChatOperator::Contains;
	rule.IgnoreColorCodes = 1;
	rule.Pattern = "WAXEDDD";
	rule.SearchTeamChat = 0;
}

static bool CreateConfig(const char* filePath)
{
	udtFileStream file;
	if(!file.Open(filePath, udtFileOpenMode::Write))
	{
		return false;
	}

	return file.Write(exampleConfig, (u32)strlen(exampleConfig), 1) == 1;
}

static bool ReadConfig(CutByChatConfig& config, udtContext& context, udtVMLinearAllocator& fileAllocator, const char* filePath)
{
	bool definingRule = false;
	udtChatPatternRule rule;
	InitRule(rule);
	s32 tempInt;

	udtFileStream file;
	if(!file.Open(filePath, udtFileOpenMode::Read))
	{
		return false;
	}

	udtString fileString = file.ReadAllAsString(fileAllocator);
	if(fileString.GetLength() == 0 || !fileString.IsValid())
	{
		return false;
	}
	file.Close();

	udtVMArray<udtString> lines("ReadConfig::LinesArray");
	if(!StringSplitLines(lines, fileString))
	{
		return false;
	}

	idTokenizer& tokenizer = context.Tokenizer;
	for(u32 i = 0, count = lines.GetSize(); i < count; ++i)
	{
		const udtString line = lines[i];
		if(udtString::IsNullOrEmpty(line) || udtString::StartsWith(line, "//"))
		{
			continue;
		}

		if(udtString::Equals(line, "[ChatRule]"))
		{
			definingRule = true;
			InitRule(rule);
			continue;
		}

		if(udtString::Equals(line, "[/ChatRule]"))
		{
			definingRule = false;
			config.ChatRules.Add(rule);
			continue;
		}

		tokenizer.Tokenize(line.GetPtr());
		if(tokenizer.GetArgCount() != 2)
		{
			continue;
		}

		if(definingRule)
		{
			if(udtString::Equals(tokenizer.GetArg(0), "ChatOperator"))
			{
				if(udtString::Equals(tokenizer.GetArg(1), "Contains")) rule.ChatOperator = (u32)udtChatOperator::Contains;
				else if(udtString::Equals(tokenizer.GetArg(1), "StartsWith")) rule.ChatOperator = (u32)udtChatOperator::StartsWith;
				else if(udtString::Equals(tokenizer.GetArg(1), "EndsWith")) rule.ChatOperator = (u32)udtChatOperator::EndsWith;
			}
			else if(udtString::Equals(tokenizer.GetArg(0), "Pattern"))
			{
				// We temporarily save an offset because the data might get relocated.
				rule.Pattern = (const char*)(uintptr_t)udtString::NewClone(config.StringAllocator, tokenizer.GetArgString(1)).GetOffset();
			}
			else if(udtString::Equals(tokenizer.GetArg(0), "CaseSensitive"))
			{
				if(udtString::Equals(tokenizer.GetArg(1), "1")) rule.CaseSensitive = 1;
				else if(udtString::Equals(tokenizer.GetArg(1), "0")) rule.CaseSensitive = 0;
			}
			else if(udtString::Equals(tokenizer.GetArg(0), "IgnoreColorCodes"))
			{
				if(udtString::Equals(tokenizer.GetArg(1), "1")) rule.IgnoreColorCodes = 1;
				else if(udtString::Equals(tokenizer.GetArg(1), "0")) rule.IgnoreColorCodes = 0;
			}
		}
		else
		{
			if(udtString::Equals(tokenizer.GetArg(0), "StartOffset"))
			{
				if(StringParseInt(tempInt, tokenizer.GetArgString(1)) && tempInt > 0) config.StartOffsetSec = tempInt;
			}
			else if(udtString::Equals(tokenizer.GetArg(0), "EndOffset"))
			{
				if(StringParseInt(tempInt, tokenizer.GetArgString(1)) && tempInt > 0) config.EndOffsetSec = tempInt;
			}
		}
	}

	// Fix up the pattern pointers.
	for(u32 i = 0, count = config.ChatRules.GetSize(); i < count; ++i)
	{
		const u32 offset = (u32)(uintptr_t)config.ChatRules[i].Pattern;
		config.ChatRules[i].Pattern = config.StringAllocator.GetStringAt(offset);
	}

	return true;
}

static bool LoadConfig(CutByChatConfig& config, const char* configPath)
{
	udtParserContext* const context = udtCreateContext();
	if(context == NULL)
	{
		fprintf(stderr, "udtCreateContext failed.\n");
		return false;
	}

	udtVMLinearAllocator fileAllocator("LoadConfig::File");
	if(!ReadConfig(config, context->Context, fileAllocator, configPath))
	{
		fprintf(stderr, "Could not load the specified config file.\n");
		return false;
	}

	if(config.ChatRules.IsEmpty())
	{
		fprintf(stderr, "The specified config file doesn't define any chat rule.\n");
		return false;
	}

	return true;
}

static bool CutByTime(const char* filePath, const char* outputFolder, s32 startSec, s32 endSec)
{
	udtParseArg info;
	memset(&info, 0, sizeof(info));
	info.MessageCb = &CallbackConsoleMessage;
	info.ProgressCb = &CallbackConsoleProgress;
	info.OutputFolderPath = outputFolder;
	
	udtCut cut;
	memset(&cut, 0, sizeof(cut));
	cut.StartTimeMs = startSec * 1000;
	cut.EndTimeMs = endSec * 1000;

	udtCutByTimeArg cutInfo;
	memset(&cutInfo, 0, sizeof(cutInfo));
	cutInfo.CutCount = 1;
	cutInfo.Cuts = &cut;

	udtParserContext* const context = udtCreateContext();
	if(context == NULL)
	{
		return false;
	}

	const s32 result = udtCutDemoFileByTime(context, &info, &cutInfo, filePath);
	udtDestroyContext(context);
	if(result == udtErrorCode::None)
	{
		return true;
	}

	fprintf(stderr, "udtCutDemoFileByTime failed with error: %s\n", udtGetErrorCodeString(result));

	return false;
}

static bool CutFilesBySinglePatternBatch(udtParseArg& parseArg, const udtFileInfo* files, const u32 fileCount, const ProgramOptions& config, udtPatternType::Id patternType, const void* patternData)
{
	udtVMArray<const char*> filePaths("CutFilesBySinglePatternBatch::FilePathsArray");
	udtVMArray<s32> errorCodes("CutFilesBySinglePatternBatch::ErrorCodesArray");
	filePaths.Resize(fileCount);
	errorCodes.Resize(fileCount);
	for(u32 i = 0; i < fileCount; ++i)
	{
		filePaths[i] = files[i].Path.GetPtr();
	}

	udtMultiParseArg threadInfo;
	memset(&threadInfo, 0, sizeof(threadInfo));
	threadInfo.FilePaths = filePaths.GetStartAddress();
	threadInfo.OutputErrorCodes = errorCodes.GetStartAddress();
	threadInfo.FileCount = fileCount;
	threadInfo.MaxThreadCount = (u32)config.MaxThreadCount;

	udtPatternInfo patternInfo;
	memset(&patternInfo, 0, sizeof(patternInfo));
	patternInfo.Type = (u32)patternType;
	patternInfo.TypeSpecificInfo = patternData;

	udtPatternSearchArg patternArg;
	memset(&patternArg, 0, sizeof(patternArg));
	patternArg.StartOffsetSec = (u32)config.StartOffsetSec;
	patternArg.EndOffsetSec = (u32)config.EndOffsetSec;
	patternArg.PatternCount = 1;
	patternArg.Patterns = &patternInfo;
	patternArg.Flags = udtPatternSearchArgMask::MergeCutSections;
	patternArg.PlayerIndex = config.PlayerIndex;

	const s32 result = udtCutDemoFilesByPattern(&parseArg, &threadInfo, &patternArg);

	udtVMLinearAllocator tempAllocator("CutFilesBySinglePatternBatch::Temp");
	for(u32 i = 0; i < fileCount; ++i)
	{
		if(errorCodes[i] != (s32)udtErrorCode::None)
		{
			udtString fileName;
			tempAllocator.Clear();
			udtPath::GetFileName(fileName, tempAllocator, udtString::NewConstRef(filePaths[i]));

			fprintf(stderr, "Processing of file %s failed with error: %s\n", fileName.GetPtrSafe("?"), udtGetErrorCodeString(errorCodes[i]));
		}
	}

	if(result == udtErrorCode::None)
	{
		return true;
	}

	fprintf(stderr, "udtCutDemoFilesByPattern failed with error: %s\n", udtGetErrorCodeString(result));

	return false;
}

static bool CutFilesBySinglePattern(udtParseArg& parseArg, const udtFileInfo* files, const u32 fileCount, const ProgramOptions& config, udtPatternType::Id patternType, const void* patternData)
{
	parseArg.OutputFolderPath = config.OutputFolderPath;

	BatchRunner runner(parseArg, files, fileCount, CUTTER_BATCH_SIZE);
	const u32 batchCount = runner.GetBatchCount();
	for(u32 i = 0; i < batchCount; ++i)
	{
		runner.PrepareNextBatch();
		const BatchRunner::BatchInfo& info = runner.GetBatchInfo(i);
		if(!CutFilesBySinglePatternBatch(parseArg, files + info.FirstFileIndex, info.FileCount, config, patternType, patternData))
		{
			return false;
		}
	}

	return true;
}

static bool HasCuttableDemoFileExtension(const udtString& filePath)
{
	udtProtocolList list;
	udtGetProtocolList(&list);	

	for(u32 i = 0; i < (u32)udtProtocol::Count; ++i)
	{
		if((list.Flags[i] & udtProtocolFlags::ReadOnly) == 0 &&
		   udtString::EndsWithNoCase(filePath, list.Extensions[i]))
		{
			return true;
		}
	}

	return false;
}

static bool HasCuttableDemoFileExtension(const char* filePath)
{
	return HasCuttableDemoFileExtension(udtString::NewConstRef(filePath));
}

static bool KeepOnlyCuttableDemoFiles(const char* name, u64 /*size*/, void* /*userData*/)
{
	return HasCuttableDemoFileExtension(name);
}

static bool LoadChatConfig(CutByChatConfig& config, const ProgramOptions& options)
{
	if(!LoadConfig(config, options.ConfigFilePath))
	{
		fprintf(stderr, "Failed to load the config file.\n");
		return false;
	}

	if(options.StartOffsetSec > 0) config.StartOffsetSec = (int)options.StartOffsetSec;
	if(options.EndOffsetSec > 0) config.EndOffsetSec = (int)options.EndOffsetSec;

	return true;
}


int udt_main(int argc, char** argv)
{
	if(argc < 3)
	{
		PrintHelp();
		return 0;
	}

	const udtString commandString = udtString::NewConstRef(argv[1]);
	if(commandString.GetLength() != 1)
	{
		fprintf(stderr, "Invalid command.\n");
		return 1;
	}

	InitOptions();

	const char command = commandString.GetPtr()[0];
	if(command == 'h')
	{
		if(argc >= 3 && strlen(argv[2]) == 1 && IsValidCommand(*argv[2]))
		{
			PrintCommandHelp(*argv[2]);
		}
		else
		{
			PrintHelp();
		}
		return 0;
	}

	if(!IsValidCommand(command))
	{
		fprintf(stderr, "Invalid command.\n");
		return 1;
	}

	const int firstOption = 2;
	const int afterLastOption = command == 'g' ? argc : (argc - 1);

	options.FragRun.AllowedMeansOfDeaths = u64(~0);
	ParseOptions(firstOption, afterLastOption, argv, command);
	ParseCommands(firstOption, afterLastOption, argv, command);

	if(command == 'g')
	{
		if(options.ConfigFilePath == NULL)
		{
			fprintf(stderr, "The config file path was not specified.\n");
			return 1;
		}

		return CreateConfig(options.ConfigFilePath) ? 0 : 1;
	}

	const char* const inputPath = argv[argc - 1];
	bool fileMode = false;
	if(udtFileStream::Exists(inputPath) && HasCuttableDemoFileExtension(inputPath))
	{
		fileMode = true;
	}
	else if(!IsValidDirectory(inputPath))
	{
		fprintf(stderr, "Invalid file/folder path.\n");
		return 1;
	}

	if(command == 't')
	{
		if(!fileMode)
		{
			fprintf(stderr, "The input path must be a file, not a folder.\n");
			return 1;
		}

		if(options.StartTimeSec == UDT_S32_MIN)
		{
			fprintf(stderr, "The start time was not specified.\n");
			return 1;
		}

		if(options.EndTimeSec == UDT_S32_MIN)
		{
			fprintf(stderr, "The end time was not specified.\n");
			return 1;
		}

		return CutByTime(inputPath, options.OutputFolderPath, options.StartTimeSec, options.EndTimeSec) ? 0 : 1;
	}

	if(command == 'c' && options.ConfigFilePath == NULL)
	{
		fprintf(stderr, "The config file path was not specified.\n");
		return 1;
	}

	CmdLineParseArg parseArg;
	udtFileInfo fileInfo;
	udtFileListQuery query;
	const udtFileInfo* files = NULL;
	u32 fileCount = 0;
	if(fileMode)
	{
		fileInfo.Name = udtString::NewNull();
		fileInfo.Path = udtString::NewConstRef(inputPath);
		fileInfo.Size = 0;
		files = &fileInfo;
		fileCount = 1;
	}
	else
	{
		query.FileFilter = &KeepOnlyCuttableDemoFiles;
		query.FolderPath = udtString::NewConstRef(inputPath);
		query.Recursive = options.Recursive;
		query.UserData = NULL;
		GetDirectoryFileList(query);
		files = query.Files.GetStartAddress();
		fileCount = query.Files.GetSize();
	}

	if(command == 'c')
	{
		CutByChatConfig config;
		if(!LoadChatConfig(config, options))
		{
			return 1;
		}
		options.StartOffsetSec = config.StartOffsetSec;
		options.EndOffsetSec = config.EndOffsetSec;

		udtChatPatternArg pattern;
		memset(&pattern, 0, sizeof(pattern));
		pattern.Rules = config.ChatRules.GetStartAddress();
		pattern.RuleCount = config.ChatRules.GetSize();

		return CutFilesBySinglePattern(parseArg.ParseArg, files, fileCount, options, udtPatternType::Chat, &pattern) ? 0 : 1;
	}
	else if(command == 'm')
	{
		udtMatchPatternArg pattern;
		memset(&pattern, 0, sizeof(pattern));
		pattern.MatchStartOffsetMs = (u32)options.StartOffsetSec * 1000;
		pattern.MatchEndOffsetMs = (u32)options.EndOffsetSec * 1000;

		return CutFilesBySinglePattern(parseArg.ParseArg, files, fileCount, options, udtPatternType::Matches, &pattern) ? 0 : 1;
	}
	else if(command == 'r')
	{
		return CutFilesBySinglePattern(parseArg.ParseArg, files, fileCount, options, udtPatternType::MultiFragRails, &options.MultiRail) ? 0 : 1;
	}
	else if(command == 'f')
	{
		return CutFilesBySinglePattern(parseArg.ParseArg, files, fileCount, options, udtPatternType::FlickRailFrags, &options.FlickRail) ? 0 : 1;
	}
	else if(command == 'a')
	{
		options.MidAir.AllowedWeapons = 0;
		if(options.AllowRL)
		{
			options.MidAir.AllowedWeapons |= UDT_BIT(udtWeapon::RocketLauncher);
		}
		if(options.AllowBFG)
		{
			options.MidAir.AllowedWeapons |= UDT_BIT(udtWeapon::BFG);
		}
		if(options.AllowGL)
		{
			options.MidAir.AllowedWeapons |= UDT_BIT(udtWeapon::GrenadeLauncher);
		}
		if(options.AllowPF)
		{
			options.MidAir.AllowedWeapons |= UDT_BIT(udtWeapon::Panzerfaust);
		}
		return CutFilesBySinglePattern(parseArg.ParseArg, files, fileCount, options, udtPatternType::MidAirFrags, &options.MidAir) ? 0 : 1;
	}
	else if(command == 's')
	{
		options.FragRun.Flags = 0;
		if(options.AllowSelfKills)
		{
			options.FragRun.Flags |= udtFragRunPatternArgMask::AllowSelfKills;
		}
		if(options.AllowTeamKills)
		{
			options.FragRun.Flags |= udtFragRunPatternArgMask::AllowTeamKills;
		}
		if(options.AllowDeaths)
		{
			options.FragRun.Flags |= udtFragRunPatternArgMask::AllowDeaths;
		}
		return CutFilesBySinglePattern(parseArg.ParseArg, files, fileCount, options, udtPatternType::FragSequences, &options.FragRun) ? 0 : 1;
	}
	else if(command == 'p')
	{
		// our command-line options are in seconds
		options.FlagCapture.MinCarryTimeMs *= 1000;
		options.FlagCapture.MaxCarryTimeMs *= 1000;
		return CutFilesBySinglePattern(parseArg.ParseArg, files, fileCount, options, udtPatternType::FlagCaptures, &options.FlagCapture) ? 0 : 1;
	}

	return 0;
}
