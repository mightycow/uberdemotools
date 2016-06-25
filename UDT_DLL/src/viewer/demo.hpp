#pragma once


#include "uberdemotools.h"
#include "array.hpp"
#include "string.hpp"


#pragma pack(push, 1)

struct PlayerFlags
{
	enum Id
	{
		Dead,
		Firing,
		ShortLGBeam,
		Followed,
		TelePortBit,
		Count
	};
};

struct DynamicItemType
{
	enum Id
	{
		// What follows corresponds to udtItem::Id values 
		// of the same name.
		AmmoBullets,
		AmmoCells,
		AmmoGrenades,
		AmmoLightning,
		AmmoRockets,
		AmmoShells,
		AmmoSlugs,
		ItemEnviro,
		ItemFlight,
		ItemHaste,
		ItemInvis,
		ItemQuad,
		ItemRegen,
		FlagBlue,
		FlagRed,
		WeaponBFG,
		WeaponGauntlet,
		WeaponGrenadeLauncher,
		WeaponLightningGun,
		WeaponMachinegun,
		WeaponPlasmaGun,
		WeaponRailgun,
		WeaponRocketLauncher,
		WeaponShotgun,
		// Other stuff:
		ProjectileRocket,
		ProjectileGrenade,
		ProjectilePlasma,
		Explosion, // For rockets and grenades.
		ImpactPlasma,
		ImpactBullet,
		ImpactGeneric,
		Count
	};
};

struct Player
{
	f32 Position[3];
	f32 LGEndPoint[3];
	f32 Angle;
	u32 Name;
	u8 IdClientNumber;
	u8 Team;
	u8 WeaponId; // udtWeapon::Id
	u8 Flags; // See PlayerFlags::Id
};

// armors, health, etc
struct StaticItem
{
	f32 Position[3];
	s32 Id; // udtItem::Id
};

// weapons, projectiles, etc
struct DynamicItem
{
	f32 Position[3];
	f32 Angle;
	u16 IdEntityNumber;
	u8 Id; // DynamicItemType::Id
	u8 SpriteOffset;
};

struct RailBeam
{
	f32 StartPosition[3];
	f32 EndPosition[3];
	f32 Alpha;
	u8 Team;
};

struct SnapshotCore
{
	u32 FollowedName;
	s16 FollowedHealth;
	s16 FollowedArmor;
	s16 FollowedAmmo;
	u8 FollowedTeam;
	u8 FollowedWeapon;
};

struct SnapshotScore
{
	u32 Score1Name;
	u32 Score2Name;
	s16 Score1;
	s16 Score2;
	u8 Score1Id;
	u8 Score2Id;
	u8 IsScoreTeamBased;
};

#define  MAX_STATIC_ITEMS  64
#define  MAX_DYN_ITEMS     64
#define  MAX_RAIL_BEAMS    16

struct Snapshot
{
	Player Players[64];
	StaticItem StaticItems[MAX_STATIC_ITEMS];
	DynamicItem DynamicItems[MAX_DYN_ITEMS];
	RailBeam RailBeams[MAX_RAIL_BEAMS];
	SnapshotCore Core;
	SnapshotScore Score;
	u32 PlayerCount;
	u32 StaticItemCount;
	u32 DynamicItemCount;
	u32 RailBeamCount;
	s32 ServerTimeMs;
};

#pragma pack(pop)

struct idProtocolNumbers
{
	idProtocolNumbers();

	void GetNumbers(u32 protocol, u32 mod);

	s32 DynamicItemIds[DynamicItemType::Count];
	s32 EntityTypePlayer;
	s32 EntityTypeItem;
	s32 EntityTypeMissile;
	s32 EntityTypeGeneral;
	s32 EntityTypeEvent;
	s32 WeaponRocket;
	s32 WeaponGrenade;
	s32 WeaponPlasma;
	s32 WeaponShaft;
	s32 CSIndexFirstPlayer;
	s32 EntityFlagDead;
	s32 EntityFlagFiring;
	s32 EntityFlagNoDraw;
	s32 EntityFlagPlayerEvent;
	s32 EntityFlagTelePortBit;
	s32 EntityEventBulletHitFlesh;
	s32 EntityEventBulletHitWall;
	s32 EntityEventMissileHit;
	s32 EntityEventMissileMiss;
	s32 EntityEventMissileMissMetal;
	s32 EntityEventRailTrail;
	s32 PlayerStatsHealth;
	s32 PlayerStatsArmor;
};

struct Demo
{
	typedef void (*ProgressCallback)(f32 progress, void* userData);

	Demo();
	~Demo();

	bool        Init(ProgressCallback progressCallback, void* userData);
	void        Load(const char* filePath);

	s32         GetFirstSnapshotTimeMs() const { return _firstSnapshotTimeMs; }
	u32         GetDurationMs() const { return (u32)(_lastSnapshotTimeMs - _firstSnapshotTimeMs); }
	bool        IsValid() const { return _snapshots[_readIndex].GetSize() > 0; }
	udtString   GetMapName() const { return _mapName; }
	const f32*  GetMapMin() const { return _min; }
	const f32*  GetMapMax() const { return _max; }
	u32         GetSnapshotCount() const { return _snapshots[_readIndex].GetSize(); }
	const char* GetString(u32 offset) const { return _stringAllocator.GetStringAt(offset); }
	const char* GetStringSafe(u32 offset, const char* replacement) const;

	u32         GetSnapshotIndexFromServerTime(s32 serverTimeMs) const;
	s32         GetSnapshotServerTimeMs(u32 index) const;
	bool        GetSnapshotData(Snapshot& snapshot, u32 index) const;

	udtMod::Id      GetMod() const { return (udtMod::Id)_protocol; }
	udtGameType::Id GetGameType() const { return (udtGameType::Id)_protocol; }
	udtProtocol::Id GetProtocol() const { return (udtProtocol::Id)_protocol; }

private:
	UDT_NO_COPY_SEMANTICS(Demo);

	typedef bool (Demo::*MessageHandler)(const udtCuMessageOutput& message); // Return false to stop parsing.

	template<typename T>
	void Read(uptr& offset, T& data) const
	{
		Read(offset, &data, (u32)sizeof(T));
	}

	template<typename T>
	void Write(const T& data)
	{
		Write(&data, (u32)sizeof(T));
	}

	void Read(uptr& offset, void* data, u32 byteCount) const;
	void Write(const void* data, u32 byteCount);
	void ParseDemo(const char* filePath, MessageHandler messageHandler);
	bool ProcessMessage_Mod(const udtCuMessageOutput& message);
	bool ProcessMessage_StaticItems(const udtCuMessageOutput& message);
	bool ProcessMessage_FinalPass(const udtCuMessageOutput& message);
	bool ProcessPlayer(const idEntityStateBase& player, s32 serverTimeMs, bool followed);
	void ProcessPlayerConfigString(u32 csIndex, u32 playerIndex);
	void RegisterStaticItem(const idEntityStateBase& item, s32 itemId);
	bool IsSame(const idEntityStateBase& a, const StaticItem& b, s32 udtItemId);
	void FixStaticItems();
	void FixDynamicItemsAndPlayers();
	void FixPlayers(const Snapshot& prevSnap, Snapshot& currSnap, Snapshot& snap2, u32 s, u32 snapshotCount, bool alive);
	void FixLGEndPoints();
	void WriteSnapshot(const Snapshot& snapshot);
	void ComputeLGEndPoint(Player& player, const f32* start, const f32* angles);
	bool FindPlayer(const Player*& player, u32 snapshotIndex, u8 idClientNumber);
	bool AnalyzeDemo(const char* filePath);

	enum Constants
	{
		MaxItemMaskByteCount = 64
	};

	struct SnapshotDesc
	{
		u32 Offset;
		s32 ServerTimeMs;
	};

	struct PlayerEx
	{
		u32 Name;
		s32 Team; // udtTeam::Id
	};

	struct RailBeamEx
	{
		RailBeam Base;
		s32 ServerTimeMs;
	};
	
	struct Impact
	{
		f32 Position[3];
		u32 SnapshotIndex;
	};

	struct Score
	{
		SnapshotScore Base;
		s32 ServerTimeMs;
	};
	
	PlayerEx _players[64];
	idProtocolNumbers _protocolNumbers;
	u8 _staticItemBits[MaxItemMaskByteCount];
	f32 _min[3];
	f32 _max[3];
	u32 _readIndex = 0;
	u32 _writeIndex = 0;
	udtVMArray<SnapshotDesc> _snapshots[2];
	udtVMLinearAllocator _snapshotAllocators[2];
	udtVMLinearAllocator _stringAllocator;
	udtVMArray<StaticItem> _staticItems;
	udtVMArray<Player> _tempPlayers;
	udtVMArray<DynamicItem> _tempDynamicItems;
	udtVMArray<RailBeam> _tempBeams;
	udtVMArray<RailBeamEx> _beams;
	udtVMArray<Impact> _tempShaftImpacts;
	udtVMArray<Impact> _explosions;
	udtVMArray<Impact> _bulletImpacts;
	udtVMArray<Score> _scores;
	udtString _mapName = udtString::NewEmptyConstant();
	udtCuContext* _context = nullptr;
	u8* _messageData = nullptr;
	Snapshot* _snapshot = nullptr;
	ProgressCallback _progressCallback = nullptr;
	void* _userData = nullptr;
	s32 _firstMatchStartTimeMs = UDT_S32_MAX;
	s32 _firstMatchEndTimeMs = UDT_S32_MIN;
	s32 _firstSnapshotTimeMs = UDT_S32_MAX;
	s32 _lastSnapshotTimeMs = UDT_S32_MIN;
	u32 _mod = udtMod::None;
	u32 _gameType = udtGameType::Count;
	u32 _protocol = udtProtocol::Invalid;
};
