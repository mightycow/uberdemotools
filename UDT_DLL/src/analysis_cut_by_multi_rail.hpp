#pragma once


#include "analysis_cut_by_pattern.hpp"


struct udtCutByMultiRailAnalyzer : public udtCutByPatternAnalyzerBase
{
public:
	udtCutByMultiRailAnalyzer();
	~udtCutByMultiRailAnalyzer();

	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);

private:
	UDT_NO_COPY_SEMANTICS(udtCutByMultiRailAnalyzer);

	s32 _gameStateIndex;
};
