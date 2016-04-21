#pragma once


#include "analysis_cut_by_pattern.hpp"
#include "analysis_captures.hpp"


struct udtCutByFlagCaptureAnalyzer : public udtCutByPatternAnalyzerBase
{
public:
	udtCutByFlagCaptureAnalyzer();
	~udtCutByFlagCaptureAnalyzer();

	void InitAllocators(u32 demoCount) override;
	void StartAnalysis() override;
	void FinishAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtCutByFlagCaptureAnalyzer);

	udtCapturesAnalyzer _analyzer;
};
