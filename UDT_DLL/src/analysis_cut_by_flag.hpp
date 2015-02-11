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

private:
	UDT_NO_COPY_SEMANTICS(udtCutByFlagCaptureAnalyzer);

	void ProcessSnapshotPlayerState(const udtSnapshotCallbackArg& arg, udtBaseParser& parser, idPlayerStateBase* ps);
	void ProcessSnapshotEntityStates(const udtSnapshotCallbackArg& arg, s32 trackedPlayerIdx);

	s32 _gameStateIndex;
	s32 _pickupTimeMs;
	s32 _previousCaptureCount;
	bool _previousCapped;
};
