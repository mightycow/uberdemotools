#include "analysis_cut_by_pattern.hpp"
#include "utils.hpp"
#include "cut_section.hpp"
#include "analysis_cut_by_chat.hpp"
#include "analysis_cut_by_frag.hpp"
#include "analysis_cut_by_mid_air.hpp"
#include "analysis_cut_by_multi_rail.hpp"
#include "analysis_cut_by_flag.hpp"
#include "analysis_cut_by_flick_rail.hpp"

#include <stdlib.h>


struct CutSection : public udtCutSection
{
	// qsort isn't guaranteed to be stable, so we work around that.
	int Order;
};

static int SortByStartTimeAscending(const void* aPtr, const void* bPtr)
{
	const u64 a = ((CutSection*)aPtr)->StartTimeMs;
	const u64 b = ((CutSection*)bPtr)->StartTimeMs;

	return (int)(a - b);
}

static int StableSortByGameStateIndexAscending(const void* aPtr, const void* bPtr)
{
	const CutSection& a = *(CutSection*)aPtr;
	const CutSection& b = *(CutSection*)bPtr;

	const int byGameState = a.GameStateIndex - b.GameStateIndex;
	const int byPreviousOrder = a.Order - b.Order;

	return byGameState != 0 ? byGameState : byPreviousOrder;
}

static void AppendCutSections(udtVMArray<udtCutSection>& dest, udtVMArray<CutSection>& source)
{
	for(u32 i = 0, cutCount = source.GetSize(); i < cutCount; ++i)
	{
		const CutSection cut = source[i];
		udtCutSection newCut;
		newCut.GameStateIndex = cut.GameStateIndex;
		newCut.StartTimeMs = cut.StartTimeMs;
		newCut.EndTimeMs = cut.EndTimeMs;
		newCut.VeryShortDesc = cut.VeryShortDesc;
		dest.Add(newCut);
	}
}


udtCutByPatternAnalyzerBase::udtCutByPatternAnalyzerBase() 
	: CutSections((uptr)(1 << 16))
{
}

udtCutByPatternPlugIn::udtCutByPatternPlugIn()
	: _info(NULL)
	, _trackedPlayerIndex(S32_MIN)
{
	_analyzers.Init(1 << 12);
	_analyzerTypes.Init(1 << 12);
	_analyzerAllocator.Init(1 << 16);
	_analyzerAllocatorScope.SetAllocator(_analyzerAllocator);
}

void udtCutByPatternPlugIn::InitAllocators(u32 demoCount)
{
	FinalAllocator.Init((uptr)(1 << 16) * (uptr)demoCount);
	CutSections.SetAllocator(FinalAllocator);
}

void udtCutByPatternPlugIn::InitAnalyzerAllocators(u32 demoCount)
{
	for(u32 i = 0, analyzerCount = _analyzers.GetSize(); i < analyzerCount; ++i)
	{
		_analyzers[i]->InitAllocators(demoCount);
	}
}

udtCutByPatternAnalyzerBase* udtCutByPatternPlugIn::CreateAndAddAnalyzer(udtPatternType::Id patternType, const void* extraInfo)
{
	if(extraInfo == NULL)
	{
		return NULL;
	}

#define UDT_CUT_PATTERN_ITEM(Enum, Desc, ArgType, AnalyzerType) case udtPatternType::Enum: analyzer = _analyzerAllocatorScope.NewObject<AnalyzerType>(); break;
	udtCutByPatternAnalyzerBase* analyzer = NULL;
	switch(patternType)
	{
		UDT_CUT_PATTERN_LIST(UDT_CUT_PATTERN_ITEM)
		default: return NULL;
	}
#undef UDT_CUT_PATTERN_ITEM

	if(analyzer != NULL)
	{
		analyzer->PlugIn = this;
		analyzer->ExtraInfo = extraInfo;
		_analyzers.Add(analyzer);
		_analyzerTypes.Add(patternType);
	}

	return analyzer;
}

udtCutByPatternAnalyzerBase* udtCutByPatternPlugIn::GetAnalyzer(udtPatternType::Id patternType)
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

void udtCutByPatternPlugIn::ProcessGamestateMessage(const udtGamestateCallbackArg& info, udtBaseParser& parser)
{
	const udtCutByPatternArg& pi = GetInfo();

	_trackedPlayerIndex = S32_MIN;
	if(pi.PlayerIndex >= 0 && pi.PlayerIndex < 64)
	{
		_trackedPlayerIndex = pi.PlayerIndex;
	}
	else if(pi.PlayerIndex == (s32)udtPlayerIndex::DemoTaker)
	{
		_trackedPlayerIndex = info.ClientNum;
	}
	else if(!udtString::IsNullOrEmpty(pi.PlayerName))
	{
		const s32 firstPlayerCsIdx = idConfigStringIndex::FirstPlayer(parser._inProtocol);
		for(s32 i = 0; i < MAX_CLIENTS; ++i)
		{
			udtString playerName;
			if(GetTempPlayerName(playerName, parser, firstPlayerCsIdx + i) &&
			   !udtString::IsNullOrEmpty(playerName) && 
			   udtString::Equals(playerName, pi.PlayerName))
			{
				_trackedPlayerIndex = i;
				break;
			}
		}
	}

	for(u32 i = 0, count = _analyzers.GetSize(); i < count; ++i)
	{
		_analyzers[i]->ProcessGamestateMessage(info, parser);
	}
}

void udtCutByPatternPlugIn::ProcessSnapshotMessage(const udtSnapshotCallbackArg& info, udtBaseParser& parser)
{
	if(_info->PlayerIndex == (s32)udtPlayerIndex::FirstPersonPlayer)
	{
		idPlayerStateBase* const ps = GetPlayerState(info.Snapshot, parser._inProtocol);
		if(ps != NULL)
		{
			_trackedPlayerIndex = ps->clientNum;
		}
	}

	for(u32 i = 0, count = _analyzers.GetSize(); i < count; ++i)
	{
		_analyzers[i]->ProcessSnapshotMessage(info, parser);
	}
}

void udtCutByPatternPlugIn::TrackPlayerFromCommandMessage(udtBaseParser& parser)
{
	const udtString playerName = udtString::NewConstRef(_info->PlayerName);
	if(_trackedPlayerIndex != S32_MIN || udtString::IsNullOrEmpty(playerName))
	{
		return;
	}

	CommandLineTokenizer& tokenizer = parser._context->Tokenizer;
	const int tokenCount = tokenizer.GetArgCount();
	if(tokenCount != 3 || !udtString::Equals(tokenizer.GetArg(0), "cs"))
	{
		return;
	}

	s32 csIndex = -1;
	if(!StringParseInt(csIndex, tokenizer.GetArgString(1)))
	{
		return;
	}

	const s32 firstPlayerCsIdx = idConfigStringIndex::FirstPlayer(parser._inProtocol);
	const s32 playerIndex = csIndex - firstPlayerCsIdx;
	if(playerIndex < 0 || playerIndex >= MAX_CLIENTS)
	{
		return;
	}

	udtString extractedPlayerName;
	GetTempPlayerName(extractedPlayerName, parser, csIndex);
	if(!udtString::IsNullOrEmpty(extractedPlayerName) && udtString::Equals(extractedPlayerName, playerName))
	{
		_trackedPlayerIndex = playerIndex;
	}
}

void udtCutByPatternPlugIn::ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser)
{
	TrackPlayerFromCommandMessage(parser);

	for(u32 i = 0, count = _analyzers.GetSize(); i < count; ++i)
	{
		_analyzers[i]->ProcessCommandMessage(info, parser);
	}
}

void udtCutByPatternPlugIn::StartDemoAnalysis()
{
	CutSections.Clear();

	for(u32 i = 0, analyzerCount = _analyzers.GetSize(); i < analyzerCount; ++i)
	{
		_analyzers[i]->CutSections.Clear();
		_analyzers[i]->StartAnalysis();
	}
}

void udtCutByPatternPlugIn::FinishDemoAnalysis()
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
	TempAllocator->Clear();
	udtVMScopedStackAllocator tempAllocScope(*TempAllocator);
	udtVMArray<CutSection> tempCutSections;
	tempCutSections.SetAllocator(*TempAllocator);
	for(u32 i = 0, analyzerCount = _analyzers.GetSize(); i < analyzerCount; ++i)
	{
		udtCutByPatternAnalyzerBase* const analyzer = _analyzers[i];
		for(u32 j = 0, cutCount = analyzer->CutSections.GetSize(); j < cutCount; ++j)
		{
			const udtCutSection cut = _analyzers[i]->CutSections[j];
			CutSection newCut;
			newCut.GameStateIndex = cut.GameStateIndex;
			newCut.StartTimeMs = cut.StartTimeMs;
			newCut.EndTimeMs = cut.EndTimeMs;
			newCut.VeryShortDesc = cut.VeryShortDesc;
			tempCutSections.Add(newCut);
		}
	}

	//
	// Apply sorting pass #1.
	//
	const u32 cutCount = tempCutSections.GetSize();
	qsort(tempCutSections.GetStartAddress(), (size_t)cutCount, sizeof(CutSection), &SortByStartTimeAscending);

	//
	// Apply sorting pass #2, which must be stable with respect to 
	// the sorting of pass #1.
	//
	for(u32 i = 0; i < cutCount; ++i)
	{
		tempCutSections[i].Order = (int)i;
	}
	qsort(tempCutSections.GetStartAddress(), (size_t)cutCount, sizeof(CutSection), &StableSortByGameStateIndexAscending);

	//
	// Create a new list with the sorted data using the final data format
	// and merge the sections if asked for it.
	//
	if((GetInfo().Flags & (u32)udtCutByPatternArgFlags::MergeCutSections) != 0)
	{
		udtVMArrayWithAlloc<udtCutSection> cutSections(1 << 16);
		AppendCutSections(cutSections, tempCutSections);
		MergeRanges(CutSections, cutSections);
	}
	else
	{
		CutSections.Clear();
		AppendCutSections(CutSections, tempCutSections);
	}
}

s32 udtCutByPatternPlugIn::GetTrackedPlayerIndex() const
{
	return _trackedPlayerIndex;
}

bool udtCutByPatternPlugIn::GetTempPlayerName(udtString& playerName, udtBaseParser& parser, s32 csIdx)
{
	udtVMScopedStackAllocator scopedTempAllocator(*TempAllocator);

	udtString clan;
	bool hasClan;
	if(!GetClanAndPlayerName(clan, playerName, hasClan, *TempAllocator, parser._inProtocol, parser._inConfigStrings[csIdx].String))
	{
		playerName = udtString::NewEmptyConstant();
		return false;
	}

	udtString::CleanUp(playerName, parser._inProtocol);
	udtString::MakeLowerCase(playerName);

	return true;
}
