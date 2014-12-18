#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"


struct udtAwardsAnalyzer
{
public:
	typedef udtVMArray<udtParseDataAward> AwardArray;

public:
	udtAwardsAnalyzer()
	{
		RecordingPlayerIndex = -1;
		_lastProcessedServerCommandNumber = -1;
		_gameStateIndex = -1;
		_firstSnapshot = true;
		for(s32 i = 0; i < MAX_PERSISTANT; ++i)
		{
			_persistant[i] = 0;
		}
	}

	~udtAwardsAnalyzer()
	{
	}

	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser);

	AwardArray AwardEvents;
	s32 RecordingPlayerIndex;

private:
	UDT_NO_COPY_SEMANTICS(udtAwardsAnalyzer);

	s32	_persistant[MAX_PERSISTANT];
	s32 _gameStateIndex;
	s32 _lastProcessedServerCommandNumber;
	bool _firstSnapshot;
};

struct udtParserPlugInAwards : udtBaseParserPlugIn
{
public:
	udtParserPlugInAwards()
	{
	}

	~udtParserPlugInAwards()
	{
	}

	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
	{
		Analyzer.ProcessSnapshotMessage(arg, parser);
	}

	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser)
	{
		Analyzer.ProcessGamestateMessage(arg, parser);
	}

	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
	{
		Analyzer.ProcessCommandMessage(arg, parser);
	}

	void FinishAnalysis()
	{
	}

	u32 GetElementCount() const
	{
		return Analyzer.AwardEvents.GetSize();
	}

	u32 GetElementSize() const
	{
		return (u32)sizeof(udtParseDataObituary);
	};

	void* GetFirstElementAddress()
	{
		return Analyzer.AwardEvents.GetStartAddress();
	}

	udtAwardsAnalyzer Analyzer;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInAwards);
};
