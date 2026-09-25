#include "plug_in_pattern_search.hpp"
#include "utils.hpp"
#include "cut_section.hpp"
#include "analysis_pattern_chat.hpp"
#include "analysis_pattern_frag_run.hpp"
#include "analysis_pattern_mid_air.hpp"
#include "analysis_pattern_multi_rail.hpp"
#include "analysis_pattern_capture.hpp"
#include "analysis_pattern_flick_rail.hpp"
#include "analysis_pattern_match.hpp"

#include <stdlib.h>


#define UDT_PATTERN_ITEM(Enum, Desc, ArgType, AnalyzerType) sizeof(AnalyzerType) +
static const size_t SizeOfAllAnalyzers = UDT_PATTERN_LIST(UDT_PATTERN_ITEM) 0;
#undef UDT_PATTERN_ITEM


static int CompareCuts(const void* aPtr, const void* bPtr)
{
	const udtCutSection& a = *(udtCutSection*)aPtr;
	const udtCutSection& b = *(udtCutSection*)bPtr;

	const s32 g = a.GameStateIndex - b.GameStateIndex;
	if(g < 0) return -1;
	if(g > 0) return 1;

	const s32 s = a.StartTimeMs - b.StartTimeMs;
	if(s < 0) return -1;
	if(s > 0) return 1;

	const s32 e = a.EndTimeMs - b.EndTimeMs;
	if(e < 0) return -1;
	if(e > 0) return 1;

	const s32 p = (s32)a.PatternTypes - (s32)b.PatternTypes;
	if(p < 0) return -1;
	if(p > 0) return 1;

	return 0;
}

static bool MatchesRule(udtVMLinearAllocator& allocator, const udtString& configStringName, const udtStringMatchingRule& rule, udtProtocol::Id protocol)
{
	udtString name = udtString::NewCloneFromRef(allocator, configStringName);
	udtString value = udtString::NewClone(allocator, rule.Value);
	if((rule.Flags & (u32)udtStringMatchingRuleMask::CaseSensitive) == 0)
	{
		udtString::MakeLowerCase(name);
		udtString::MakeLowerCase(value);
	}
	if((rule.Flags & (u32)udtStringMatchingRuleMask::IgnoreColorCodes) != 0)
	{
		udtString::CleanUp(name, protocol);
		udtString::CleanUp(value, protocol);
	}

	switch((udtStringComparisonMode::Id)rule.ComparisonMode)
	{
		case udtStringComparisonMode::Equals: return udtString::Equals(name, value);
		case udtStringComparisonMode::Contains: return udtString::Contains(name, value);
		case udtStringComparisonMode::StartsWith: return udtString::StartsWith(name, value);
		case udtStringComparisonMode::EndsWith: return udtString::EndsWith(name, value);
		default: return false;
	}
}

static bool MatchesRules(udtVMLinearAllocator& allocator, const udtString& name, const udtStringMatchingRule* rules, u32 ruleCount, udtProtocol::Id protocol)
{
	for(u32 i = 0; i < ruleCount; ++i)
	{
		if(MatchesRule(allocator, name, rules[i], protocol))
		{
			return true;
		}
	}

	return false;
}


udtPatternSearchPlugIn::udtPatternSearchPlugIn()
	: _info(NULL)
	, _trackedPlayerIndex(UDT_S32_MIN)
{
	// @NOTE: This data can never be relocated.
	_analyzerAllocator.Init((uptr)SizeOfAllAnalyzers);

	_analyzerAllocatorScope.SetAllocator(_analyzerAllocator);
}

void udtPatternSearchPlugIn::InitAllocators(u32)
{
}

void udtPatternSearchPlugIn::InitAnalyzerAllocators(u32 demoCount)
{
	for(u32 i = 0, analyzerCount = _analyzers.GetSize(); i < analyzerCount; ++i)
	{
		_analyzers[i]->InitAllocators(demoCount);
	}
}

udtPatternSearchAnalyzerBase* udtPatternSearchPlugIn::CreateAndAddAnalyzer(udtPatternType::Id patternType, const void* extraInfo)
{
	if(extraInfo == NULL)
	{
		return NULL;
	}

#define UDT_PATTERN_ITEM(Enum, Desc, ArgType, AnalyzerType) case udtPatternType::Enum: analyzer = (udtPatternSearchAnalyzerBase*)_analyzerAllocator.GetAddressAt(_analyzerAllocatorScope.NewObject<AnalyzerType>()); break;
	udtPatternSearchAnalyzerBase* analyzer = NULL;
	switch(patternType)
	{
		UDT_PATTERN_LIST(UDT_PATTERN_ITEM)
		default: return NULL;
	}
#undef UDT_PATTERN_ITEM

	if(analyzer != NULL)
	{
		analyzer->PlugIn = this;
		analyzer->ExtraInfo = extraInfo;
		_analyzers.Add(analyzer);
		_analyzerTypes.Add(patternType);
	}

	return analyzer;
}

udtPatternSearchAnalyzerBase* udtPatternSearchPlugIn::GetAnalyzer(udtPatternType::Id patternType)
{
	for(u32 i = 0, count = _analyzers.GetSize(); i < count; ++i)
	{
		if(_analyzerTypes[i] == patternType)
		{
			return _analyzers[i];
		}
	}

	return NULL;
}

void udtPatternSearchPlugIn::ProcessGamestateMessage(const udtGamestateCallbackArg& info, udtBaseParser& parser)
{
	const udtPatternSearchArg& pi = GetInfo();

	_trackedPlayerIndex = UDT_S32_MIN;
	if(pi.PlayerNameRules != NULL)
	{
		FindPlayerInConfigStrings(parser);
	}
	else if(pi.PlayerIndex >= 0 && pi.PlayerIndex < 64)
	{
		_trackedPlayerIndex = pi.PlayerIndex;
	}
	else if(pi.PlayerIndex == (s32)udtPlayerIndex::DemoTaker)
	{
		_trackedPlayerIndex = info.ClientNum;
	}

	for(u32 i = 0, count = _analyzers.GetSize(); i < count; ++i)
	{
		_analyzers[i]->ProcessGamestateMessage(info, parser);
	}
}

void udtPatternSearchPlugIn::ProcessSnapshotMessage(const udtSnapshotCallbackArg& info, udtBaseParser& parser)
{
	const udtPatternSearchArg pi = GetInfo();

	if(pi.PlayerIndex == (s32)udtPlayerIndex::FirstPersonPlayer)
	{
		idPlayerStateBase* const ps = GetPlayerState(info.Snapshot, parser._inProtocol);
		if(ps != NULL)
		{
			_trackedPlayerIndex = ps->clientNum;
		}
	}
	else if(pi.PlayerIndex == UDT_S32_MIN &&
			pi.PlayerNameRules != NULL)
	{
		FindPlayerInConfigStrings(parser);
	}

	for(u32 i = 0, count = _analyzers.GetSize(); i < count; ++i)
	{
		_analyzers[i]->ProcessSnapshotMessage(info, parser);
	}
}

void udtPatternSearchPlugIn::FindPlayerInConfigStrings(udtBaseParser& parser)
{
	const udtPatternSearchArg pi = GetInfo();

	const s32 firstPlayerCsIdx = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, parser._inProtocol);
	for(s32 i = 0; i < ID_MAX_CLIENTS; ++i)
	{
		udtVMScopedStackAllocator allocatorScope(*TempAllocator);

		udtString playerName;
		if(GetPlayerName(playerName, *TempAllocator, parser, firstPlayerCsIdx + i) &&
		   MatchesRules(*TempAllocator, playerName, pi.PlayerNameRules, pi.PlayerNameRuleCount, parser._inProtocol))
		{
			_trackedPlayerIndex = i;
			break;
		}
	}
}

void udtPatternSearchPlugIn::FindPlayerInServerCommand(const udtCommandCallbackArg& info, udtBaseParser& parser)
{
	const udtPatternSearchArg pi = GetInfo();
	if(pi.PlayerNameRules == NULL || 
	   !info.IsConfigString)
	{
		return;
	}

	const s32 firstPlayerCsIdx = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, parser._inProtocol);
	const s32 playerIndex = info.ConfigStringIndex - firstPlayerCsIdx;
	if(playerIndex < 0 || playerIndex >= ID_MAX_CLIENTS)
	{
		return;
	}

	if(playerIndex == _trackedPlayerIndex && 
	   info.IsEmptyConfigString)
	{
		// The player we had selected just left!
		// We'll try to find the right player to track next snapshot.
		_trackedPlayerIndex = UDT_S32_MIN;
		return;
	}

	if(_trackedPlayerIndex != UDT_S32_MIN)
	{
		// Avoid switching to another player if the current one is still around.
		return;
	}

	udtVMScopedStackAllocator allocatorScope(*TempAllocator);

	udtString playerName;
	if(GetPlayerName(playerName, *TempAllocator, parser, info.ConfigStringIndex) &&
	   MatchesRules(*TempAllocator, playerName, pi.PlayerNameRules, pi.PlayerNameRuleCount, parser._inProtocol))
	{
		_trackedPlayerIndex = playerIndex;
	}
}

void udtPatternSearchPlugIn::ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser)
{
	FindPlayerInServerCommand(info, parser);

	for(u32 i = 0, count = _analyzers.GetSize(); i < count; ++i)
	{
		_analyzers[i]->ProcessCommandMessage(info, parser);
	}
}

void udtPatternSearchPlugIn::StartDemoAnalysis()
{
	CutSections.Clear();

	for(u32 i = 0, analyzerCount = _analyzers.GetSize(); i < analyzerCount; ++i)
	{
		_analyzers[i]->CutSections.Clear();
		_analyzers[i]->StartAnalysis();
	}
}

void udtPatternSearchPlugIn::FinishDemoAnalysis()
{
	if(_analyzers.GetSize() == 0)
	{
		return;
	}

	for(u32 i = 0, analyzerCount = _analyzers.GetSize(); i < analyzerCount; ++i)
	{
		_analyzers[i]->FinishAnalysis();
	}

	// If we only have 1 analyzer, we don't need to do any sorting.
	if(_analyzers.GetSize() == 1)
	{
		MergeRanges(CutSections, _analyzers[0]->CutSections);
		return;
	}

	//
	// Create a list with all the cut sections.
	//
	_tempCutSections.Clear();
	for(u32 i = 0, analyzerCount = _analyzers.GetSize(); i < analyzerCount; ++i)
	{
		udtPatternSearchAnalyzerBase* const analyzer = _analyzers[i];
		for(u32 j = 0, cutCount = analyzer->CutSections.GetSize(); j < cutCount; ++j)
		{
			_tempCutSections.Add(_analyzers[i]->CutSections[j]);
		}
	}

	//
	// Sort cuts by increasing order: gamestate index -> start time -> end time -> pattern mask
	//
	qsort(_tempCutSections.GetStartAddress(), (size_t)_tempCutSections.GetSize(), sizeof(udtCutSection), &CompareCuts);

	//
	// Merge the sections if asked for it.
	//
	CutSections.Clear();
	if((GetInfo().Flags & (u32)udtPatternSearchArgMask::MergeCutSections) != 0)
	{
		MergeRanges(CutSections, _tempCutSections);
	}
	else
	{
		for(u32 i = 0, count = _tempCutSections.GetSize(); i < count; ++i)
		{
			CutSections.Add(_tempCutSections[i]);
		}
	}
}

s32 udtPatternSearchPlugIn::GetTrackedPlayerIndex() const
{
	return _trackedPlayerIndex;
}

bool udtPatternSearchPlugIn::GetPlayerName(udtString& playerName, udtVMLinearAllocator& allocator, udtBaseParser& parser, s32 csIdx)
{
	udtString clan;
	bool hasClan;
	if(!GetClanAndPlayerName(clan, playerName, hasClan, allocator, parser._inProtocol, parser._inConfigStrings[csIdx].GetPtr()))
	{
		playerName = udtString::NewEmptyConstant();
		return false;
	}

	return true;
}
