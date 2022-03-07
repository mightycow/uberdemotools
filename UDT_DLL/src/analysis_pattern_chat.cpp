#include "analysis_pattern_chat.hpp"
#include "plug_in_pattern_search.hpp"
#include "utils.hpp"
#include "common.hpp"
#include "scoped_stack_allocator.hpp"
#include "cut_section.hpp"


static bool GetMessageAndType(udtString& message, bool& isTeamMessage, const idTokenizer& tokenizer, udtBaseParser& parser)
{
	bool hasCPMASyntax = false;

	const udtString command = tokenizer.GetArg(0);
	if(udtString::Equals(command, "chat"))
	{
		isTeamMessage = false;
	}
	else if(udtString::Equals(command, "tchat"))
	{
		isTeamMessage = true;
	}
	else if(udtString::Equals(command, "lchat"))
	{
		// @TODO: confirm that this is correct
		// it probably is since it displays with the same color as "chat"
		isTeamMessage = false;
	}
	else if(udtString::Equals(command, "mm2"))
	{
		isTeamMessage = true;
		hasCPMASyntax = true;
	}
	else
	{
		return false;
	}

	if(AreAllProtocolFlagsSet(parser._inProtocol, udtProtocolFlags::RTCW))
	{
		if(tokenizer.GetArgCount() >= 2)
		{
			udtString messageEmClean = udtString::NewCloneFromRef(parser._tempAllocator, tokenizer.GetArg(1));
			udtString::RemoveEmCharacter(messageEmClean);

			u32 colonIdx = 0;
			if(udtString::FindFirstCharacterMatch(colonIdx, messageEmClean, ':') &&
			   colonIdx + 4 < tokenizer.GetArgLength(1))
			{
				message = udtString::NewSubstringRef(messageEmClean, colonIdx + 4);
				return true;
			}
		}

		return false;
	}

	if(hasCPMASyntax && 
	   tokenizer.GetArgCount() == 4)
	{
		message = tokenizer.GetArg(3);
		return true;
	}

	u32 colonIdx = 0;
	if(!hasCPMASyntax && 
	   tokenizer.GetArgCount() == 2 &&
	   udtString::FindFirstCharacterMatch(colonIdx, tokenizer.GetArg(1), ':') &&
	   colonIdx + 2 < tokenizer.GetArgLength(1))
	{
		message = udtString::NewSubstringRef(tokenizer.GetArg(1), colonIdx + 2);
		return true;
	}

	return false;
}

udtChatPatternAnalyzer::udtChatPatternAnalyzer()
{
}

udtChatPatternAnalyzer::~udtChatPatternAnalyzer()
{
}

void udtChatPatternAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& /*commandInfo*/, udtBaseParser& parser)
{
	const idTokenizer& tokenizer = parser.GetTokenizer();
	if(tokenizer.GetArgCount() < 2)
	{
		return;
	}

	udtString message;
	bool isTeamMessage;
	udtVMScopedStackAllocator allocatorScopeGuard(parser._tempAllocator);
	if(!GetMessageAndType(message, isTeamMessage, tokenizer, parser))
	{
		return;
	}

	const udtChatPatternArg& extraInfo = GetExtraInfo<udtChatPatternArg>();
	bool match = false;
	for(u32 i = 0; i < extraInfo.RuleCount; ++i)
	{
		if(isTeamMessage && extraInfo.Rules[i].SearchTeamChat == 0)
		{
			continue;
		}

		udtVMScopedStackAllocator tempAllocatorScopeGuard(parser._tempAllocator);
		if(StringMatchesCutByChatRule(message, extraInfo.Rules[i], parser._tempAllocator, parser._inProtocol))
		{
			match = true;
			break;
		}
	}

	if(!match)
	{
		return;
	}

	const udtPatternSearchArg& patternInfo = PlugIn->GetInfo();
	const s32 startTimeMs = parser._inServerTime - (s32)patternInfo.StartOffsetSec * 1000;
	const s32 endTimeMs = parser._inServerTime + (s32)patternInfo.EndOffsetSec * 1000;

	udtCutSection cutSection;
	cutSection.VeryShortDesc = "chat";
	cutSection.GameStateIndex = parser._inGameStateIndex;
	cutSection.StartTimeMs = startTimeMs;
	cutSection.EndTimeMs = endTimeMs;
	cutSection.PatternTypes = UDT_BIT((u32)udtPatternType::Chat);
	_cutSections.Add(cutSection);
}

void udtChatPatternAnalyzer::StartAnalysis()
{
	_cutSections.Clear();
}

void udtChatPatternAnalyzer::FinishAnalysis()
{
	MergeRanges(CutSections, _cutSections);
}
