#pragma once


#include "uberdemotools.h"
#include "array.hpp"
#include "string.hpp"
#include "timer.hpp"


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
		HasFlag,
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

// The maximum amount of entities presented in a frame is 256.
#define  MAX_STATIC_ITEMS  256
#define  MAX_DYN_ITEMS     256
#define  MAX_RAIL_BEAMS     64

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
	s32 DisplayTimeMs;
	s32 ServerTimeMs;
};

#pragma pack(pop)

struct ChatMessage
{
	u32 Location;
	u32 PlayerName;
	u32 Message;
	u32 TeamMessage;
	s32 DisplayTimeMs;
};

struct HeatMapPlayer
{
	u32 Name;
	u8 Team; // udtTeam::Id
	u8 Present;
};

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
	s32 PowerUpFlagRed;
	s32 PowerUpFlagBlue;
};

struct Demo
{
	typedef void (*ProgressCallback)(f32 progress, void* userData);

	Demo();
	~Demo();

	bool        Init(ProgressCallback progressCallback, void* userData);
	void        Load(const char* filePath, bool keepOnlyFirstMatch, bool removeTimeOuts);
	void		GenerateHeatMap(u32* histogram, u32 width, u32 height, const f32* min, const f32* max, u32 clientNumber);

	const char* GetFilePath() const;
	s32         GetFirstSnapshotTimeMs() const { return _firstSnapshotTimeMs; }
	u32         GetDurationMs() const { return (u32)(_lastSnapshotTimeMs - _firstSnapshotTimeMs); }
	bool        IsValid() const { return _snapshots[_readIndex].GetSize() > 0; }
	udtString   GetMapName() const { return _mapName; }
	const f32*  GetMapMin() const { return _min; }
	const f32*  GetMapMax() const { return _max; }
	u32         GetSnapshotCount() const { return _snapshots[_readIndex].GetSize(); }
	const char* GetString(u32 offset) const { return _stringAllocator.GetStringAt(offset); }
	const char* GetStringSafe(u32 offset, const char* replacement) const;

	u32         GetSnapshotIndexFromDisplayTime(s32 displayTimeMs) const;
	s32         GetSnapshotDisplayTimeMs(u32 index) const;
	s32         GetSnapshotServerTimeMs(u32 index);
	bool        GetSnapshotData(Snapshot& snapshot, u32 index) const;

	u32         GetChatMessageIndexFromDisplayTime(s32 displayTimeMs) const;
	u32         GetChatMessageCount() const;
	bool        GetChatMessage(ChatMessage& message, u32 index) const;

	void        GetHeatMapPlayers(const HeatMapPlayer*& players) const;

	udtMod::Id      GetMod() const { return (udtMod::Id)_protocol; }
	udtGameType::Id GetGameType() const { return (udtGameType::Id)_gameType; }
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

	bool GetDynamicItemsOnly(Snapshot& snapshot, u32 index) const;
	void Read(uptr& offset, void* data, u32 byteCount) const;
	void Write(const void* data, u32 byteCount);
	void ParseDemo(const char* filePath, MessageHandler messageHandler);
	bool ProcessMessage_Mod(const udtCuMessageOutput& message);
	bool ProcessMessage_OSPEncryption(const udtCuMessageOutput& message);
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
	bool AnalyzeDemo(const char* filePath, bool keepOnlyFirstMatch);
	u32  CloneString(const void* buffer, u32 offset);
	void ReportProgress(f32 subProgress);
	void NextStep();
	bool ProcessTimeOut(s32& timeOffsetMs, s32 serverTimeMs); // Returns true if in a time-out.

	enum Constants
	{
		MaxItemMaskByteCount = 64
	};

	struct SnapshotDesc
	{
		u32 Offset;
		s32 DisplayTimeMs;
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
		s32 DisplayTimeMs;
	};

	struct TimeOut
	{
		s32 StartTimeMs;
		s32 EndTimeMs;
	};

	PlayerEx _players[64];
	HeatMapPlayer _heatMapPlayers[64];
	char _filePath[512];
	idProtocolNumbers _protocolNumbers;
	u8 _staticItemBits[MaxItemMaskByteCount];
	f32 _min[3];
	f32 _max[3];
	u32 _readIndex = 0;
	u32 _writeIndex = 0;
	udtVMArray<SnapshotDesc> _snapshots[2];
	udtVMLinearAllocator _snapshotAllocators[2];
	udtVMLinearAllocator _stringAllocator { "Demo::Strings" };
	udtVMArray<StaticItem> _staticItems { "Demo::StaticItemsArray" };
	udtVMArray<Player> _tempPlayers { "Demo::TempPlayersArray" };
	udtVMArray<DynamicItem> _tempDynamicItems { "Demo::TempDynamicItemsArray" };
	udtVMArray<RailBeam> _tempBeams { "Demo::TempBeamsArray" };
	udtVMArray<RailBeamEx> _beams { "Demo::BeamsArray" };
	udtVMArray<Impact> _tempShaftImpacts { "Demo::TempShaftImpactsArray" };
	udtVMArray<Impact> _explosions { "Demo::ExplosionsArray" };
	udtVMArray<Impact> _bulletImpacts { "Demo::BulletImpactsArray" };
	udtVMArray<Score> _scores { "Demo::ScoresArray" };
	udtVMArray<ChatMessage> _chatMessages { "Demo::ChatMessagesArray" };
	udtVMArray<TimeOut> _timeOuts { "Demo::TimeOutsArray" };
	udtString _mapName = udtString::NewEmptyConstant();
	udtTimer _loadTimer;
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
	u32 _loadStep = 0;
	s32 _timeOutIndex = 0;
	bool _ospEncryptedPlayers = false;
	bool _removeTimeOuts = false;
};
