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

	struct ProjectileInfo
	{
		s32 UsedSlot;
		f32 CreationPosition[3];
		s32 CreationTimeMs;
		s32 IdWeapon;
	};
	
	void            AddProjectile(s32 weapon, const f32* position, s32 serverTimeMs);
	ProjectileInfo* FindBestProjectileMatch(s32 udtWeapon, const f32* targetPosition, s32 serverTimeMs);
	
	struct PlayerInfo
	{
		f32 Position[3];
		s32 LastGroundContactTime;
		s32 LastZDirChangeTime;
		s32 ZDir;
	};

	udtProtocol::Id _protocol;
	ProjectileInfo _projectiles[64];
	PlayerInfo _players[64];
	s32 _gameStateIndex;
	s32 _lastEventSequence;
	f32 _rocketSpeed;
	f32 _bfgSpeed;
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
