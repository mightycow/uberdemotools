#pragma once


#include "analysis_awards.hpp"


struct udtCutByAwardAnalyzer
{
public:
	struct CutSection
	{
		s32 GameStateIndex;
		s32 StartTimeMs;
		s32 EndTimeMs;
	};

	typedef udtVMArray<CutSection> CutSectionVector;

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

	void FindCutSections();

	CutSectionVector CutSections;

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

struct udtParserPlugInCutByAward : udtBaseParserPlugIn
{
public:
	udtParserPlugInCutByAward(const udtCutByAwardArg& info) : Analyzer(info)
	{
	}

	~udtParserPlugInCutByAward()
	{
	}

	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
	{
		Analyzer.ProcessGamestateMessage(arg, parser);
	}

	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
	{
		Analyzer.ProcessCommandMessage(arg, parser);
	}

	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& info, udtBaseParser& parser)
	{
		Analyzer.ProcessSnapshotMessage(info, parser);
	}

	void FinishAnalysis()
	{
		Analyzer.FindCutSections();
	}

	u32 GetElementCount() const 
	{ 
		return Analyzer.CutSections.GetSize();
	}

	u32 GetElementSize() const 
	{
		return (u32)sizeof(udtCutByAwardAnalyzer::CutSection);
	};

	void* GetFirstElementAddress() 
	{
		return Analyzer.CutSections.GetStartAddress();
	}

	udtCutByAwardAnalyzer Analyzer;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInCutByAward);
};
