#include "analysis_pattern_match.hpp"
#include "utils.hpp"


static const char* ShortDescriptions[16] =
{
	"match_1",
	"match_2",
	"match_3",
	"match_4",
	"match_5",
	"match_6",
	"match_7",
	"match_8",
	"match_9",
	"match_10",
	"match_11",
	"match_12",
	"match_13",
	"match_14",
	"match_15",
	"match_16"
};


udtMatchPatternAnalyzer::udtMatchPatternAnalyzer()
{
}

udtMatchPatternAnalyzer::~udtMatchPatternAnalyzer()
{
}

void udtMatchPatternAnalyzer::InitAllocators(u32 /*demoCount*/)
{
	_statsAnalyzer.Init(1, _tempAllocator);
}

void udtMatchPatternAnalyzer::StartAnalysis()
{
	_statsAnalyzer.ClearMatchList();
	_statsAnalyzer.StartProcessingDemo();
}

void udtMatchPatternAnalyzer::FinishAnalysis()
{
	_statsAnalyzer.FinishProcessingDemo();

	udtParseDataStatsBuffers statsBuffers;
	_statsAnalyzer.UpdateBufferStruct();
	_statsAnalyzer.CopyBuffersStruct(&statsBuffers);
	const udtParseDataStats* const matches = statsBuffers.MatchStats;
	if(matches == NULL)
	{
		return;
	}

	const udtMatchPatternArg& info = GetExtraInfo<udtMatchPatternArg>();

	const u32 matchCount = udt_min(statsBuffers.MatchCount, (u32)UDT_COUNT_OF(ShortDescriptions));
	for(u32 i = 0; i < matchCount; ++i)
	{
		const s32 startTime = matches[i].StartTimeMs;
		const s32 endTime = matches[i].EndTimeMs;
		const s32 countDownStartTime = matches[i].CountDownStartTimeMs;
		const s32 intermissionEndTime = matches[i].IntermissionEndTimeMs;

		udtCutSection cut;
		cut.GameStateIndex = matches[i].GameStateIndex;
		cut.StartTimeMs = (countDownStartTime < startTime) ? countDownStartTime : (startTime - (s32)info.MatchStartOffsetMs);
		cut.EndTimeMs = (intermissionEndTime > endTime) ? intermissionEndTime : (endTime + (s32)info.MatchEndOffsetMs);
		cut.VeryShortDesc = ShortDescriptions[i];
		cut.PatternTypes = UDT_BIT((u32)udtPatternType::Matches);
		CutSections.Add(cut);
	}
}

void udtMatchPatternAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{ 
	_statsAnalyzer.ProcessGamestateMessage(arg, parser);
}

void udtMatchPatternAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	_statsAnalyzer.ProcessSnapshotMessage(arg, parser);
}

void udtMatchPatternAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	_statsAnalyzer.ProcessCommandMessage(arg, parser);
}
