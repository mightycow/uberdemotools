#pragma once


#include "parser.hpp"
#include "parser_plug_in.hpp"
#include "array.hpp"


struct udtMidAirAnalyzer
{
public:
	udtMidAirAnalyzer();

	~udtMidAirAnalyzer()
	{
	}

	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser);

	udtVMArray<udtParseDataMidAir> MidAirs;
	s32 RecordingPlayerIndex;

private:
	UDT_NO_COPY_SEMANTICS(udtMidAirAnalyzer);
	
	void AddOrUpdateProjectile(const idEntityStateBase* entity, s32 serverTimeMs);
	void RemoveProjectile(s32 entityNumber);

	struct ProjectileInfo
	{
		s32 UsedSlot;
		s32 IdEntityNumber;
		f32 CreationPosition[3];
		s32 CreationTimeMs;
		s32 IdWeapon;
	};
	
	udtProtocol::Id _protocol;
	ProjectileInfo _projectiles[64];
	const idEntityStateBase* _playerEntities[64];
	s32 _gameStateIndex;
};

struct udtParserPlugInMidAir : udtBaseParserPlugIn
{
public:
	udtParserPlugInMidAir()
	{
	}

	~udtParserPlugInMidAir()
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
		return Analyzer.MidAirs.GetSize();
	}

	u32 GetElementSize() const
	{
		return (u32)sizeof(udtParseDataMidAir);
	};

	void* GetFirstElementAddress()
	{
		return Analyzer.MidAirs.GetStartAddress();
	}

	udtMidAirAnalyzer Analyzer;

private:
	UDT_NO_COPY_SEMANTICS(udtParserPlugInMidAir);
};
