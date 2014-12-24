#include "analysis_cut_by_pattern.hpp"
#include "utils.hpp"
#include "cut_section.hpp"
#include "analysis_cut_by_chat.hpp"
#include "analysis_cut_by_frag.hpp"
#include "analysis_cut_by_mid_air.hpp"

// @TODO:
//#include "analysis_cut_by_multi_rail.hpp"
struct udtCutByMultiRailAnalyzer : public udtCutByPatternAnalyzerBase
{
};


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


udtCutByPatternPlugIn::udtCutByPatternPlugIn(udtVMLinearAllocator& analyzerAllocator) 
	: _analyzerAllocatorScope(analyzerAllocator)
{
}

udtCutByPatternAnalyzerBase* udtCutByPatternPlugIn::CreateAndAddAnalyzer(udtPatternType::Id patternType, const udtCutByPatternArg* info, const void* extraInfo)
{
	if(info == NULL || extraInfo == NULL)
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
		analyzer->_info = info;
		analyzer->_extraInfo = extraInfo;
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
	for(u32 i = 0, count = _analyzers.GetSize(); i < count; ++i)
	{
		_analyzers[i]->ProcessGamestateMessage(info, parser);
	}
}

void udtCutByPatternPlugIn::ProcessSnapshotMessage(const udtSnapshotCallbackArg& info, udtBaseParser& parser)
{
	for(u32 i = 0, count = _analyzers.GetSize(); i < count; ++i)
	{
		_analyzers[i]->ProcessSnapshotMessage(info, parser);
	}
}

void udtCutByPatternPlugIn::ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser)
{
	for(u32 i = 0, count = _analyzers.GetSize(); i < count; ++i)
	{
		_analyzers[i]->ProcessCommandMessage(info, parser);
	}
}

void udtCutByPatternPlugIn::FinishAnalysis()
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
	udtVMArray<CutSection> tempCutSections;
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
	// and merge the sections.
	//
	udtVMArray<udtCutSection> cutSections;
	for(u32 i = 0; i < cutCount; ++i)
	{
		const CutSection cut = tempCutSections[i];
		udtCutSection newCut;
		newCut.GameStateIndex = cut.GameStateIndex;
		newCut.StartTimeMs = cut.StartTimeMs;
		newCut.EndTimeMs = cut.EndTimeMs;
		cutSections.Add(newCut);
	}

	MergeRanges(CutSections, cutSections);
}
