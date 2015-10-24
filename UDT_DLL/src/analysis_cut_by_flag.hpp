#pragma once


#include "analysis_cut_by_pattern.hpp"


struct udtCutByFlagCaptureAnalyzer : public udtCutByPatternAnalyzerBase
{
public:
	udtCutByFlagCaptureAnalyzer();
	~udtCutByFlagCaptureAnalyzer();

	void StartAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtCutByFlagCaptureAnalyzer);

	s32 _gameStateIndex;
	s32 _pickupTimeMs;
	s32 _previousCaptureCount;
	u8 _prevFlagState[2]; // 0=red, 1=blue
	u8 _flagState[2]; // 0=red, 1=blue
	bool _previousCapped;
};
