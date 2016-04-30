#pragma once


#include "analysis_pattern_base.hpp"
#include "analysis_captures.hpp"


struct udtFlagCapturePatternAnalyzer : public udtPatternSearchAnalyzerBase
{
public:
	udtFlagCapturePatternAnalyzer();
	~udtFlagCapturePatternAnalyzer();

	void InitAllocators(u32 demoCount) override;
	void StartAnalysis() override;
	void FinishAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtFlagCapturePatternAnalyzer);

	udtCapturesAnalyzer _analyzer;
};
