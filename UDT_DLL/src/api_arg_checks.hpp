#pragma once


#include "api.h"
#include "file_system.hpp"


static bool IsValid(const udtCutByChatArg& arg)
{
	if(arg.Rules == NULL || arg.RuleCount == 0 || arg.StartOffsetSec == 0 || arg.EndOffsetSec == 0)
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
	if(arg.TimeBetweenFragsSec == 0 || arg.StartOffsetSec == 0 || arg.EndOffsetSec == 0 || 
	   arg.MinFragCount < 2 || arg.AllowedMeansOfDeaths == 0)
	{
		return false;
	}

	return true;
}

static bool IsValid(const udtCutByAwardArg& arg)
{
	if(arg.StartOffsetSec == 0 || arg.EndOffsetSec == 0 || arg.AllowedAwards == 0)
	{
		return false;
	}

	return true;
}

static bool IsValid(const udtMultiParseArg& arg)
{
	if(arg.FileCount == 0 || arg.FilePaths == NULL || arg.OutputErrorCodes == NULL)
	{
		return false;
	}

	return true;
}

static bool HasValidOutputOption(const udtParseArg& arg)
{
	if(arg.OutputFolderPath != NULL && !IsValidDirectory(arg.OutputFolderPath))
	{
		return false;
	}

	return true;
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
