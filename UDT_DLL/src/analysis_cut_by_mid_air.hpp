#pragma once


#include "analysis_cut_by_pattern.hpp"


struct udtCutByMidAirAnalyzer : public udtCutByPatternAnalyzerBase
{
public:
	udtCutByMidAirAnalyzer();
	~udtCutByMidAirAnalyzer();

	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser);
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser);
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser);

private:
	UDT_NO_COPY_SEMANTICS(udtCutByMidAirAnalyzer);

	struct ProjectileInfo;
	void            AddProjectile(s32 weapon, const f32* position, s32 serverTimeMs);
	ProjectileInfo* FindBestProjectileMatch(s32 udtWeapon, const f32* targetPosition, s32 serverTimeMs);

	struct ProjectileInfo
	{
		s32 UsedSlot;
		f32 CreationPosition[3];
		s32 CreationTimeMs;
		s32 IdWeapon;
	};

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