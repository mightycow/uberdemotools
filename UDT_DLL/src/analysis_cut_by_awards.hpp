#pragma once


#include "analysis_awards.hpp"
#include "analysis_cut_by_pattern.hpp"


struct udtCutByAwardAnalyzer : public udtCutByPatternAnalyzerBase
{
public:
	udtCutByAwardAnalyzer(const udtCutByAwardArg& info)
		: _info(info)
	{
		_protocol = udtProtocol::Invalid;
	}

	~udtCutByAwardAnalyzer()
	{
	}

	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
	{
		_protocol = parser._protocol;
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
	UDT_NO_COPY_SEMANTICS(udtCutByAwardAnalyzer);

	void AddCurrentSectionIfValid();
	void AddMatch(const udtParseDataAward& data);

	struct Award
	{
		s32 GameStateIndex;
		s32 ServerTimeMs;
	};

	const udtCutByAwardArg& _info;
	udtVMArray<Award> _awards;
	udtAwardsAnalyzer _analyzer;
	udtProtocol::Id _protocol;
};
