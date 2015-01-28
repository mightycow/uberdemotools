#pragma once


#include "api.h"
#include "file_system.hpp"


static bool IsValid(const udtCutByTimeArg& arg)
{
	return arg.CutCount > 0 && arg.Cuts != NULL;
}

static bool IsValid(const udtCutByChatArg& arg)
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

static bool IsValid(const udtCutByFragArg& arg)
{
	return arg.TimeBetweenFragsSec > 0 && arg.MinFragCount >= 2 && arg.AllowedMeansOfDeaths > 0;
}

static bool IsValid(const udtCutByMidAirArg& arg)
{
	return arg.AllowedWeapons > 0;
}

static bool IsValid(const udtCutByMultiRailArg& arg)
{
	return arg.MinKillCount >= 2;
}

static bool IsValid(const udtCutByPatternArg& arg)
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

#define UDT_CUT_PATTERN_ITEM(Enum, Desc, ArgType, AnalyzerType) case udtPatternType::Enum: if(!IsValid(*(ArgType*)info.TypeSpecificInfo)) return false; break;
		switch((udtPatternType::Id)info.Type)
		{
			UDT_CUT_PATTERN_LIST(UDT_CUT_PATTERN_ITEM)
			default: return false;
		}
#undef UDT_CUT_PATTERN_ITEM
	}

	return true;
}

static bool IsValid(const udtMultiParseArg& arg)
{
	return arg.FileCount > 0 && arg.FilePaths != NULL && arg.OutputErrorCodes != NULL;
}

static bool IsValid(const udtProtocolConversionArg& arg)
{
	return udtIsValidProtocol((udtProtocol::Id)arg.OutputProtocol) != 0;
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
