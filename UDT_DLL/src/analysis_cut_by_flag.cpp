#include "analysis_cut_by_flag.hpp"


udtCutByFlagCaptureAnalyzer::udtCutByFlagCaptureAnalyzer()
{
}

udtCutByFlagCaptureAnalyzer::~udtCutByFlagCaptureAnalyzer()
{
}

void udtCutByFlagCaptureAnalyzer::InitAllocators(u32 demoCount)
{
	_analyzer.Init(demoCount, &PlugIn->GetTempAllocator());
}

void udtCutByFlagCaptureAnalyzer::StartAnalysis()
{
	_analyzer.Clear();
	_analyzer.StartDemoAnalysis();
}

void udtCutByFlagCaptureAnalyzer::FinishAnalysis()
{
	_analyzer.FinishDemoAnalysis();

	const udtCutByPatternArg& info = PlugIn->GetInfo();
	const udtCutByFlagCaptureArg& extraInfo = GetExtraInfo<udtCutByFlagCaptureArg>();
	const s32 trackedPlayerIndex = PlugIn->GetTrackedPlayerIndex();
	const bool allowBaseToBase = extraInfo.AllowBaseToBase != 0;
	const bool allowMissingToBase = extraInfo.AllowMissingToBase != 0;

	const u32 count = _analyzer.Captures.GetSize();
	for(u32 i = 0; i < count; ++i)
	{
		const udtParseDataCapture& capture = _analyzer.Captures[i];

		const bool playerIndexFound = (capture.Flags & (u32)udtParseDataCaptureFlags::PlayerIndexValid) != 0;
		if(!playerIndexFound ||
		   capture.PlayerIndex != trackedPlayerIndex)
		{
			continue;
		}

		const bool baseToBase = (capture.Flags & (u32)udtParseDataCaptureFlags::BaseToBase) != 0;
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
		CutSections.Add(cut);
	}
}

void udtCutByFlagCaptureAnalyzer::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
{
	_analyzer.ProcessGamestateMessage(arg, parser);
}

void udtCutByFlagCaptureAnalyzer::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	_analyzer.ProcessSnapshotMessage(arg, parser);
}

void udtCutByFlagCaptureAnalyzer::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	_analyzer.ProcessCommandMessage(arg, parser);
}
