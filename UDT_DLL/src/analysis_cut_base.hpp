#pragma once


#include "parser_plug_in.hpp"


struct udtCutAnalyzerBase
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
	udtCutAnalyzerBase() {}
	virtual ~udtCutAnalyzerBase() {}

	virtual void ProcessGamestateMessage(const udtGamestateCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessSnapshotMessage(const udtSnapshotCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void ProcessCommandMessage(const udtCommandCallbackArg& /*arg*/, udtBaseParser& /*parser*/) {}
	virtual void FinishAnalysis() {}

	CutSectionVector CutSections;

private:
	UDT_NO_COPY_SEMANTICS(udtCutAnalyzerBase);
};

template<class udtCutByWhateverAnalyzer, class udtCutByWhateverArg>
struct udtCutPlugInBase : udtBaseParserPlugIn
{
public:
	udtCutPlugInBase(const udtCutByWhateverArg& info)
		: Analyzer(info)
	{
	}

	~udtCutPlugInBase()
	{
	}

	void ProcessGamestateMessage(const udtGamestateCallbackArg& info, udtBaseParser& parser)
	{
		Analyzer.ProcessGamestateMessage(info, parser);
	}

	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& info, udtBaseParser& parser)
	{
		Analyzer.ProcessSnapshotMessage(info, parser);
	}

	void ProcessCommandMessage(const udtCommandCallbackArg& info, udtBaseParser& parser)
	{
		Analyzer.ProcessCommandMessage(info, parser);
	}

	void FinishAnalysis()
	{
		Analyzer.FinishAnalysis();
	}

	u32 GetElementCount() const
	{
		return Analyzer.CutSections.GetSize();
	}

	u32 GetElementSize() const
	{
		return (u32)sizeof(udtCutAnalyzerBase::CutSection);
	};

	void* GetFirstElementAddress()
	{
		return GetElementCount() > 0 ? Analyzer.CutSections.GetStartAddress() : NULL;
	}

	udtCutByWhateverAnalyzer Analyzer;

private:
	UDT_NO_COPY_SEMANTICS(udtCutPlugInBase);
};
