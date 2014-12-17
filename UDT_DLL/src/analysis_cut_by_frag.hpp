#pragma once


#include "analysis_obituaries.hpp"
#include "analysis_cut_base.hpp"


struct udtCutByFragAnalyzer : public udtCutAnalyzerBase
{
public:
	udtCutByFragAnalyzer(const udtCutByFragArg& info) 
		: _info(info)
	{
		_protocol = udtProtocol::Invalid;
	}

	~udtCutByFragAnalyzer()
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
	UDT_NO_COPY_SEMANTICS(udtCutByFragAnalyzer);

	void AddCurrentSectionIfValid();
	void AddMatch(const udtParseDataObituary& data);

	struct Frag
	{
		s32 GameStateIndex;
		s32 ServerTimeMs;
	};

	const udtCutByFragArg& _info;
	udtVMArray<Frag> _frags;
	udtObituariesAnalyzer _analyzer;
	udtProtocol::Id _protocol;
};
