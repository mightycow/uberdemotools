#pragma once


#include "analysis_cut_by_pattern.hpp"
#include "analysis_mid_air.hpp"


struct udtCutByMidAirAnalyzer : public udtCutByPatternAnalyzerBase
{
public:
	udtCutByMidAirAnalyzer()
	{
	}

	~udtCutByMidAirAnalyzer()
	{
	}

	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
	{
		_analyzer.ProcessGamestateMessage(arg, parser);
	}

	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
	{
		_analyzer.ProcessCommandMessage(arg, parser);
	}

	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
	{
		_analyzer.ProcessSnapshotMessage(arg, parser);
	}

	void FinishAnalysis();

private:
	UDT_NO_COPY_SEMANTICS(udtCutByMidAirAnalyzer);

	udtMidAirAnalyzer _analyzer;
};
