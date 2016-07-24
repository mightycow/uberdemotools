#include "analysis_pattern_capture.hpp"
#include "plug_in_pattern_search.hpp"


udtFlagCapturePatternAnalyzer::udtFlagCapturePatternAnalyzer()
{
}

udtFlagCapturePatternAnalyzer::~udtFlagCapturePatternAnalyzer()
{
}

void udtFlagCapturePatternAnalyzer::InitAllocators(u32 demoCount)
{
	_analyzer.Init(demoCount, &PlugIn->GetTempAllocator());
}

void udtFlagCapturePatternAnalyzer::StartAnalysis()
{
	_analyzer.Clear();
	_analyzer.StartDemoAnalysis();
}

void udtFlagCapturePatternAnalyzer::FinishAnalysis()
{
	_analyzer.FinishDemoAnalysis();

	const udtPatternSearchArg& info = PlugIn->GetInfo();
	const udtFlagCapturePatternArg& extraInfo = GetExtraInfo<udtFlagCapturePatternArg>();
	const s32 trackedPlayerIndex = PlugIn->GetTrackedPlayerIndex();
	const bool allowBaseToBase = extraInfo.AllowBaseToBase != 0;
	const bool allowMissingToBase = extraInfo.AllowMissingToBase != 0;

	const u32 count = _analyzer.Captures.GetSize();
	for(u32 i = 0; i < count; ++i)
	{
		const udtParseDataCapture& capture = _analyzer.Captures[i];

		const bool playerIndexFound = (capture.Flags & (u32)udtParseDataCaptureMask::PlayerIndexValid) != 0;
		if(!playerIndexFound ||
		   capture.PlayerIndex != trackedPlayerIndex)
		{
			continue;
		}

		const bool baseToBase = (capture.Flags & (u32)udtParseDataCaptureMask::BaseToBase) != 0;
		const u32 durationMs = capture.CaptureTimeMs - capture.PickUpTimeMs;
		if(durationMs < extraInfo.MinCarryTimeMs ||
		   durationMs > extraInfo.MaxCarryTimeMs ||
		   (baseToBase && !allowBaseToBase) ||
		   (!baseToBase && !allowMissingToBase))
		{
			continue;
		}

		udtCutSection cut;
		cut.VeryShortDesc = "flag";
		cut.GameStateIndex = capture.GameStateIndex;
		cut.StartTimeMs = capture.PickUpTimeMs - (s32)(info.StartOffsetSec * 1000);
		cut.EndTimeMs = capture.CaptureTimeMs + (s32)(info.EndOffsetSec * 1000);
		cut.PatternTypes = UDT_BIT((u32)udtPatternType::FlagCaptures);
		CutSections.Add(cut);
	}
}

void udtFlagCapturePatternAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	_analyzer.ProcessGamestateMessage(arg, parser);
}

void udtFlagCapturePatternAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	_analyzer.ProcessSnapshotMessage(arg, parser);
}

void udtFlagCapturePatternAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	_analyzer.ProcessCommandMessage(arg, parser);
}
