#pragma once


#include "analysis_pattern_base.hpp"


struct udtMultiRailPatternAnalyzer : public udtPatternSearchAnalyzerBase
{
public:
	udtMultiRailPatternAnalyzer();
	~udtMultiRailPatternAnalyzer();

	void StartAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override;

protected:
	void OnResetForNextDemo();

private:
	UDT_NO_COPY_SEMANTICS(udtMultiRailPatternAnalyzer);

	s32 _gameStateIndex;
};
