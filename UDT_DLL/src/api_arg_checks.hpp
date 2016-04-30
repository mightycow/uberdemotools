#pragma once


#include "uberdemotools.h"
#include "macros.hpp"
#include "file_system.hpp"


static bool IsValid(const udtCutByTimeArg& arg)
{
	return arg.CutCount > 0 && arg.Cuts != NULL;
}

static bool IsValid(const udtChatPatternArg& arg)
{
	if(arg.Rules == NULL || arg.RuleCount == 0)
	{
		return false;
	}

	for(u32 i = 0, count = arg.RuleCount; i < count; ++i)
	{
		if(arg.Rules[i].Pattern == NULL || arg.Rules[i].ChatOperator >= (u32)udtChatOperator::Count)
		{
			return false;
		}
	}

	return true;
}

static bool IsValid(const udtFragRunPatternArg& arg)
{
	return arg.TimeBetweenFragsSec > 0 && arg.MinFragCount >= 2 && arg.AllowedMeansOfDeaths > 0;
}

static bool IsValid(const udtMidAirPatternArg& arg)
{
	return arg.AllowedWeapons > 0;
}

static bool IsValid(const udtMultiRailPatternArg& arg)
{
	return arg.MinKillCount >= 2;
}

static bool IsValid(const udtFlagCapturePatternArg& arg)
{
	return arg.MaxCarryTimeMs > arg.MinCarryTimeMs && (arg.AllowBaseToBase | arg.AllowMissingToBase) != 0;
}

static bool IsValid(const udtFlickRailPatternArg& arg)
{
	return 
		arg.MinSpeed >= 0.0f && 
		arg.MinAngleDelta >= 0.0f && 
		(arg.MinSpeedSnapshotCount - 2) <= 2 && 
		(arg.MinAngleDeltaSnapshotCount - 2) <= 2;
}

static bool IsValid(const udtMatchPatternArg& /*arg*/)
{
	return true;
}

static bool IsValid(const udtPatternSearchArg& arg)
{
	if(arg.Patterns == NULL || arg.PatternCount == 0 || arg.StartOffsetSec == 0 || arg.EndOffsetSec == 0)
	{
		return false;
	}

	for(u32 i = 0, count = arg.PatternCount; i < count; ++i)
	{
		const udtPatternInfo info = arg.Patterns[i];
		if(info.TypeSpecificInfo == NULL || info.Type >= (u32)udtPatternType::Count)
		{
			return false;
		}

#define CASE(Enum, Desc, ArgType, AnalyzerType) case udtPatternType::Enum: if(!IsValid(*(ArgType*)info.TypeSpecificInfo)) return false; break;
		switch((udtPatternType::Id)info.Type)
		{
			UDT_PATTERN_LIST(CASE)
			default: return false;
		}
#undef CASE
	}

	if(arg.PlayerNameRules != NULL)
	{
		if(arg.PlayerNameRuleCount == 0)
		{
			return false;
		}

		for(u32 i = 0, count = arg.PlayerNameRuleCount; i < count; ++i)
		{
			const udtStringMatchingRule& rule = arg.PlayerNameRules[i];
			if(rule.Value == NULL || rule.ComparisonMode >= (u32)udtStringComparisonMode::Count)
			{
				return false;
			}
		}
	}

	return true;
}

static bool IsValid(const udtMultiParseArg& arg)
{
	return arg.FileCount > 0 && arg.FilePaths != NULL && arg.OutputErrorCodes != NULL;
}

static bool IsValid(const udtProtocolConversionArg& arg)
{
	return arg.OutputProtocol == (u32)udtProtocol::Dm68 || arg.OutputProtocol == (u32)udtProtocol::Dm91;
}

static bool HasValidOutputOption(const udtParseArg& arg)
{
	return arg.OutputFolderPath == NULL || IsValidDirectory(arg.OutputFolderPath);
}

static bool HasValidPlugInOptions(const udtParseArg& arg)
{
	if(arg.PlugInCount == 0 || arg.PlugIns == NULL)
	{
		return false;
	}

	for(u32 i = 0, count = arg.PlugInCount; i < count; ++i)
	{
		if(arg.PlugIns[i] >= (u32)udtParserPlugIn::Count)
		{
			return false;
		}
	}

	return true;
}
