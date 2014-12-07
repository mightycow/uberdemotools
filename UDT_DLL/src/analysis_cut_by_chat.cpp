#include "analysis_cut_by_chat.hpp"
#include "utils.hpp"
#include "common.hpp"
#include "scoped_stack_allocator.hpp"


static void MergeRanges(udtCutByChatAnalyzer::CutSectionVector& result, const udtCutByChatAnalyzer::CutSectionVector& ranges)
{
	if(ranges.IsEmpty())
	{
		return;
	}

	result.Clear();

	udtCutByChatAnalyzer::CutSection current = ranges[0];
	for(u32 i = 1, count = ranges.GetSize(); i < count; ++i)
	{
		const udtCutByChatAnalyzer::CutSection it = ranges[i];
		if(current.EndTimeMs >= it.StartTimeMs && current.GameStateIndex == it.GameStateIndex)
		{
			current.EndTimeMs = udt_max(current.EndTimeMs, it.EndTimeMs);
		}
		else
		{
			result.Add(current);
			current = it;
		}
	}

	result.Add(current);
}


void udtCutByChatAnalyzer::ProcessOriginalCommandMessage(udtContext& context, const char* commandMessage, s32 serverTimeMs, s32 gsIndex)
{
	CommandLineTokenizer& tokenizer = context.Tokenizer;
	tokenizer.Tokenize(commandMessage);
	if(strcmp(tokenizer.argv(0), "chat") != 0 || tokenizer.argc() != 2)
	{
		return;
	}

	bool match = false;
	for(u32 i = 0; i < _info.RuleCount; ++i)
	{
		udtVMScopedStackAllocator tempAllocatorScopeGuard(context.TempAllocator);
		if(StringMatchesCutByChatRule(tokenizer.argv(1), _info.Rules[i], context.TempAllocator))
		{
			match = true;
			break;
		}
	}

	if(!match)
	{
		return;
	}

	const s32 startTimeMs = serverTimeMs - (s32)_info.StartOffsetSec * 1000;
	const s32 endTimeMs = serverTimeMs + (s32)_info.EndOffsetSec * 1000;

	CutSection cutSection;
	cutSection.GameStateIndex = gsIndex;
	cutSection.StartTimeMs = startTimeMs;
	cutSection.EndTimeMs = endTimeMs;
	_cutSections.Add(cutSection);
}

void udtCutByChatAnalyzer::MergeCutSections()
{
	MergeRanges(MergedCutSections, _cutSections);
}
