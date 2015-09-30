#include "analysis_cut_by_match.hpp"
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


udtCutByMatchAnalyzer::udtCutByMatchAnalyzer()
{
}

udtCutByMatchAnalyzer::~udtCutByMatchAnalyzer()
{
}

void udtCutByMatchAnalyzer::InitAllocators(u32 /*demoCount*/)
{
	_tempAllocator.Init(1 << 16, "CutByMatchAnalyzer::TempAllocator");
	_statsAnalyzer.Init(1, _tempAllocator);
}

void udtCutByMatchAnalyzer::StartAnalysis()
{
	_statsAnalyzer.ClearMatchList();
	_statsAnalyzer.StartProcessingDemo();
}

void udtCutByMatchAnalyzer::FinishAnalysis()
{
	_statsAnalyzer.FinishProcessingDemo();

	const udtParseDataStats* const matches = (const udtParseDataStats*)_statsAnalyzer.GetFirstElementAddress(0);
	if(matches == NULL)
	{
		return;
	}

	const udtCutByMatchArg& info = GetExtraInfo<udtCutByMatchArg>();

	const u32 matchCount = udt_min(_statsAnalyzer.GetElementCount(0), (u32)UDT_COUNT_OF(ShortDescriptions));
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
		CutSections.Add(cut);
	}
}

void udtCutByMatchAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{ 
	_statsAnalyzer.ProcessGamestateMessage(arg, parser);
}

void udtCutByMatchAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	_statsAnalyzer.ProcessSnapshotMessage(arg, parser);
}

void udtCutByMatchAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	_statsAnalyzer.ProcessCommandMessage(arg, parser);
}
