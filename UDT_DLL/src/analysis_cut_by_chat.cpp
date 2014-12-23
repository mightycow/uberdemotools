#include "analysis_cut_by_chat.hpp"
#include "utils.hpp"
#include "common.hpp"
#include "scoped_stack_allocator.hpp"
#include "cut_section.hpp"

#include <assert.h>


void udtCutByChatAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser)
{
	assert(_info != NULL);
	assert(_extraInfo != NULL);

	udtContext& context = *parser._context;
	CommandLineTokenizer& tokenizer = context.Tokenizer;
	tokenizer.Tokenize(info.String);
	if(strcmp(tokenizer.argv(0), "chat") != 0 || tokenizer.argc() != 2)
	{
		return;
	}

	const udtCutByChatArg* const extraInfo = (const udtCutByChatArg*)_extraInfo;

	bool match = false;
	for(u32 i = 0; i < extraInfo->RuleCount; ++i)
	{
		udtVMScopedStackAllocator tempAllocatorScopeGuard(context.TempAllocator);
		if(StringMatchesCutByChatRule(tokenizer.argv(1), extraInfo->Rules[i], context.TempAllocator))
		{
			match = true;
			break;
		}
	}

	if(!match)
	{
		return;
	}

	const s32 startTimeMs = parser._inServerTime - (s32)_info->StartOffsetSec * 1000;
	const s32 endTimeMs = parser._inServerTime + (s32)_info->EndOffsetSec * 1000;

	udtCutSection cutSection;
	cutSection.GameStateIndex = parser._inGameStateIndex;
	cutSection.StartTimeMs = startTimeMs;
	cutSection.EndTimeMs = endTimeMs;
	_cutSections.Add(cutSection);
}

void udtCutByChatAnalyzer::FinishAnalysis()
{
	MergeRanges(CutSections, _cutSections);
	_cutSections.Clear();
}
