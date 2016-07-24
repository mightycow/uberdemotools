#pragma once


#include "analysis_pattern_base.hpp"
#include "plug_in_stats.hpp"


struct udtMatchPatternAnalyzer : public udtPatternSearchAnalyzerBase
{
public:
	udtMatchPatternAnalyzer();
	~udtMatchPatternAnalyzer();

	void InitAllocators(u32 demoCount) override;
	void StartAnalysis() override;
	void FinishAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtMatchPatternAnalyzer);

	udtParserPlugInStats _statsAnalyzer;
	udtVMLinearAllocator _tempAllocator { "CutByMatchAnalyzer::TempAllocator" };
};
