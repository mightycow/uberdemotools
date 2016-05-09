#pragma once


#include "analysis_pattern_base.hpp"


struct udtMidAirPatternAnalyzer : public udtPatternSearchAnalyzerBase
{
public:
	udtMidAirPatternAnalyzer();
	~udtMidAirPatternAnalyzer();

	void StartAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtMidAirPatternAnalyzer);

	struct ProjectileInfo;
	void            AddProjectile(s32 idWeapon, const f32* position, s32 serverTimeMs);
	ProjectileInfo* FindBestProjectileMatch(u32 udtWeapon, const f32* targetPosition, s32 serverTimeMs);

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
		s32 LastUpdateTime;
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