#include "look_up_tables.hpp"
#include "timer.hpp"
#include "utils.hpp"

#include <stdlib.h>
#include <string.h>


#define TABLE_END UDT_S16_MIN

#define TABLE_ENTRY(Table) Table, Table##_U2Q, Table##_Q2U

#define VALIDATE_TABLE_SIZE(Table) \
	static_assert(sizeof(Table) == sizeof(const s16*) * udtProtocol::Count * 3, #Table " invalid")

// can't validate the last table entry at compile time... *sigh*
// static_assert(Table[(sizeof(Table) / sizeof(Table[0])) - 1] == END_OF_LIST, #Table " invalid");
// maybe constexpr can help, but my compiler doesn't handle it
#define VALIDATE_TABLE_SIZES(Table, Count) \
	static_assert(sizeof(Table##_U2Q) == sizeof(s16) * Count, #Table "_U2Q invalid"); \
	static_assert(sizeof(Table##_Q2U) == sizeof(s16) * Count * 2, #Table "_Q2U invalid")


static s16 PowerUps_3_90_U2Q[udtPowerUpIndex::Count];
static s16 PowerUps_3_90_Q2U[udtPowerUpIndex::Count * 2];
static const s16 PowerUps_3_90[] =
{
	(s16)udtPowerUpIndex::QuadDamage, 1,
	(s16)udtPowerUpIndex::BattleSuit, 2,
	(s16)udtPowerUpIndex::Haste, 3,
	(s16)udtPowerUpIndex::Invisibility, 4,
	(s16)udtPowerUpIndex::Regeneration, 5,
	(s16)udtPowerUpIndex::Flight, 6,
	(s16)udtPowerUpIndex::RedFlag, 7,
	(s16)udtPowerUpIndex::BlueFlag, 8,
	TABLE_END
};
VALIDATE_TABLE_SIZES(PowerUps_3_90, udtPowerUpIndex::Count);

static s16 PowerUps_91_U2Q[udtPowerUpIndex::Count];
static s16 PowerUps_91_Q2U[udtPowerUpIndex::Count * 2];
static const s16 PowerUps_91[] =
{
	(s16)udtPowerUpIndex::QuadDamage, 5,
	(s16)udtPowerUpIndex::BattleSuit, 6,
	(s16)udtPowerUpIndex::Haste, 7,
	(s16)udtPowerUpIndex::Invisibility, 8,
	(s16)udtPowerUpIndex::Regeneration, 9,
	(s16)udtPowerUpIndex::Flight, 10,
	(s16)udtPowerUpIndex::RedFlag, 2,
	(s16)udtPowerUpIndex::BlueFlag, 3,
	(s16)udtPowerUpIndex::NeutralFlag, 4,
	(s16)udtPowerUpIndex::Scout, 12,
	(s16)udtPowerUpIndex::Guard, 13,
	(s16)udtPowerUpIndex::Doubler, 14,
	(s16)udtPowerUpIndex::ArmorRegeneration, 15,
	(s16)udtPowerUpIndex::Invulnerability, 11,
	TABLE_END
};
VALIDATE_TABLE_SIZES(PowerUps_91, udtPowerUpIndex::Count);

static s16 PowerUps_57_60_U2Q[udtPowerUpIndex::Count];
static s16 PowerUps_57_60_Q2U[udtPowerUpIndex::Count * 2];
static const s16 PowerUps_57_60[] =
{
	(s16)udtPowerUpIndex::QuadDamage, 1,
	(s16)udtPowerUpIndex::BattleSuit, 2,
	(s16)udtPowerUpIndex::Haste, 3,
	(s16)udtPowerUpIndex::Invisibility, 4,
	(s16)udtPowerUpIndex::Regeneration, 5,
	(s16)udtPowerUpIndex::Flight, 6,
	(s16)udtPowerUpIndex::RedFlag, 12,
	(s16)udtPowerUpIndex::BlueFlag, 13,
	(s16)udtPowerUpIndex::Invulnerability, 7,
	(s16)udtPowerUpIndex::Wolf_Fire, 8,
	(s16)udtPowerUpIndex::Wolf_Electric, 9,
	(s16)udtPowerUpIndex::Wolf_Breather, 10,
	(s16)udtPowerUpIndex::Wolf_NoFatigue, 11,
	(s16)udtPowerUpIndex::Wolf_Ready, 15,
	(s16)udtPowerUpIndex::Wolf_Blackout, 16,
	TABLE_END
};
VALIDATE_TABLE_SIZES(PowerUps_57_60, udtPowerUpIndex::Count);

static const s16* PowerUpTables[] =
{
	TABLE_ENTRY(PowerUps_3_90), // 3
	TABLE_ENTRY(PowerUps_3_90), // 48
	TABLE_ENTRY(PowerUps_57_60), // 57
	TABLE_ENTRY(PowerUps_57_60), // 58
	TABLE_ENTRY(PowerUps_57_60), // 59
	TABLE_ENTRY(PowerUps_57_60), // 60
	TABLE_ENTRY(PowerUps_3_90), // 66
	TABLE_ENTRY(PowerUps_3_90), // 67
	TABLE_ENTRY(PowerUps_3_90), // 68
	TABLE_ENTRY(PowerUps_3_90), // 73
	TABLE_ENTRY(PowerUps_3_90), // 90
	TABLE_ENTRY(PowerUps_91) // 91
};
VALIDATE_TABLE_SIZE(PowerUpTables);

static s16 LifeStats_3_68_U2Q[udtLifeStatsIndex::Count];
static s16 LifeStats_3_68_Q2U[udtLifeStatsIndex::Count * 2];
static const s16 LifeStats_3_68[] =
{
	(s16)udtLifeStatsIndex::Health, 0,
	(s16)udtLifeStatsIndex::HoldableItem, 1,
	(s16)udtLifeStatsIndex::Weapons, 2,
	(s16)udtLifeStatsIndex::Armor, 3,
	(s16)udtLifeStatsIndex::MaxHealth, 6,
	TABLE_END
};
VALIDATE_TABLE_SIZES(LifeStats_3_68, udtLifeStatsIndex::Count);

static s16 LifeStats_73p_U2Q[udtLifeStatsIndex::Count];
static s16 LifeStats_73p_Q2U[udtLifeStatsIndex::Count * 2];
static const s16 LifeStats_73p[] =
{
	(s16)udtLifeStatsIndex::Health, 0,
	(s16)udtLifeStatsIndex::HoldableItem, 1,
	(s16)udtLifeStatsIndex::Weapons, 3,
	(s16)udtLifeStatsIndex::Armor, 4,
	(s16)udtLifeStatsIndex::MaxHealth, 7,
	TABLE_END
};
VALIDATE_TABLE_SIZES(LifeStats_73p, udtLifeStatsIndex::Count);

static s16 LifeStats_57_60_U2Q[udtLifeStatsIndex::Count];
static s16 LifeStats_57_60_Q2U[udtLifeStatsIndex::Count * 2];
static const s16 LifeStats_57_60[] =
{
	(s16)udtLifeStatsIndex::Health, 0,
	(s16)udtLifeStatsIndex::HoldableItem, 1,
	(s16)udtLifeStatsIndex::Armor, 2,
	(s16)udtLifeStatsIndex::Wolf_Keys, 3,
	(s16)udtLifeStatsIndex::Wolf_ClientsReady, 5,
	(s16)udtLifeStatsIndex::MaxHealth, 6,
	(s16)udtLifeStatsIndex::Wolf_PlayerClass, 7,
	(s16)udtLifeStatsIndex::Wolf_RedScore, 8,
	(s16)udtLifeStatsIndex::Wolf_BlueScore, 9,
	TABLE_END
};
VALIDATE_TABLE_SIZES(LifeStats_57_60, udtLifeStatsIndex::Count);

static const s16* LifeStatsTables[] =
{
	TABLE_ENTRY(LifeStats_3_68), // 3
	TABLE_ENTRY(LifeStats_3_68), // 48
	TABLE_ENTRY(LifeStats_57_60), // 57
	TABLE_ENTRY(LifeStats_57_60), // 58
	TABLE_ENTRY(LifeStats_57_60), // 59
	TABLE_ENTRY(LifeStats_57_60), // 60
	TABLE_ENTRY(LifeStats_3_68), // 66
	TABLE_ENTRY(LifeStats_3_68), // 67
	TABLE_ENTRY(LifeStats_3_68), // 68
	TABLE_ENTRY(LifeStats_73p), // 73
	TABLE_ENTRY(LifeStats_73p), // 90
	TABLE_ENTRY(LifeStats_73p) // 91
};
VALIDATE_TABLE_SIZE(LifeStatsTables);

static s16 PersStats_3_U2Q[udtPersStatsIndex::Count];
static s16 PersStats_3_Q2U[udtPersStatsIndex::Count * 2];
static const s16 PersStats_3[] =
{
	(s16)udtPersStatsIndex::Score, 0,
	(s16)udtPersStatsIndex::DamageGiven, 1,
	(s16)udtPersStatsIndex::Rank, 2,
	(s16)udtPersStatsIndex::Team, 3,
	(s16)udtPersStatsIndex::SpawnCount, 4,
	(s16)udtPersStatsIndex::LastAttacker, 7,
	(s16)udtPersStatsIndex::Deaths, 8,
	(s16)udtPersStatsIndex::Impressives, 9,
	(s16)udtPersStatsIndex::Excellents, 10,
	(s16)udtPersStatsIndex::Humiliations, 11,
	TABLE_END
};
VALIDATE_TABLE_SIZES(PersStats_3, udtPersStatsIndex::Count);

static s16 PersStats_48_68_U2Q[udtPersStatsIndex::Count];
static s16 PersStats_48_68_Q2U[udtPersStatsIndex::Count * 2];
static const s16 PersStats_48_68[] =
{
	(s16)udtPersStatsIndex::FlagCaptures, 14,
	(s16)udtPersStatsIndex::Score, 0,
	(s16)udtPersStatsIndex::DamageGiven, 1,
	(s16)udtPersStatsIndex::Rank, 2,
	(s16)udtPersStatsIndex::Team, 3,
	(s16)udtPersStatsIndex::SpawnCount, 4,
	(s16)udtPersStatsIndex::LastAttacker, 6,
	(s16)udtPersStatsIndex::LastTargetHealthAndArmor, 7,
	(s16)udtPersStatsIndex::Deaths, 8,
	(s16)udtPersStatsIndex::Impressives, 9,
	(s16)udtPersStatsIndex::Excellents, 10,
	(s16)udtPersStatsIndex::Defends, 11,
	(s16)udtPersStatsIndex::Assists, 12,
	(s16)udtPersStatsIndex::Humiliations, 13,
	TABLE_END
};
VALIDATE_TABLE_SIZES(PersStats_48_68, udtPersStatsIndex::Count);

static s16 PersStats_73p_U2Q[udtPersStatsIndex::Count];
static s16 PersStats_73p_Q2U[udtPersStatsIndex::Count * 2];
static const s16 PersStats_73p[] =
{
	(s16)udtPersStatsIndex::FlagCaptures, 13,
	(s16)udtPersStatsIndex::Score, 0,
	(s16)udtPersStatsIndex::DamageGiven, 1,
	(s16)udtPersStatsIndex::Rank, 2,
	(s16)udtPersStatsIndex::Team, 3,
	(s16)udtPersStatsIndex::SpawnCount, 4,
	(s16)udtPersStatsIndex::LastAttacker, 6,
	(s16)udtPersStatsIndex::LastTargetHealthAndArmor, 14,
	(s16)udtPersStatsIndex::Deaths, 7,
	(s16)udtPersStatsIndex::Impressives, 8,
	(s16)udtPersStatsIndex::Excellents, 9,
	(s16)udtPersStatsIndex::Defends, 10,
	(s16)udtPersStatsIndex::Assists, 11,
	(s16)udtPersStatsIndex::Humiliations, 12,
	TABLE_END
};
VALIDATE_TABLE_SIZES(PersStats_73p, udtPersStatsIndex::Count);

static s16 PersStats_57_60_U2Q[udtPersStatsIndex::Count];
static s16 PersStats_57_60_Q2U[udtPersStatsIndex::Count * 2];
static const s16 PersStats_57_60[] =
{
	(s16)udtPersStatsIndex::Score, 0,
	(s16)udtPersStatsIndex::DamageGiven, 1,
	(s16)udtPersStatsIndex::Rank, 2,
	(s16)udtPersStatsIndex::Team, 3,
	(s16)udtPersStatsIndex::SpawnCount, 4,
	(s16)udtPersStatsIndex::LastAttacker, 7,
	(s16)udtPersStatsIndex::Deaths, 8,
	(s16)udtPersStatsIndex::Wolf_RespawnsLeft, 9,
	(s16)udtPersStatsIndex::Wolf_AccuracyHits , 11,
	TABLE_END
};
VALIDATE_TABLE_SIZES(PersStats_57_60, udtPersStatsIndex::Count);

static const s16* PersStatsTables[] =
{
	TABLE_ENTRY(PersStats_3), // 3
	TABLE_ENTRY(PersStats_48_68), // 48
	TABLE_ENTRY(PersStats_57_60), // 57
	TABLE_ENTRY(PersStats_57_60), // 58
	TABLE_ENTRY(PersStats_57_60), // 59
	TABLE_ENTRY(PersStats_57_60), // 60
	TABLE_ENTRY(PersStats_48_68), // 66
	TABLE_ENTRY(PersStats_48_68), // 67
	TABLE_ENTRY(PersStats_48_68), // 68
	TABLE_ENTRY(PersStats_73p), // 73
	TABLE_ENTRY(PersStats_73p), // 90
	TABLE_ENTRY(PersStats_73p) // 91
};
VALIDATE_TABLE_SIZE(PersStatsTables);

static s16 EntityTypes_3_U2Q[udtEntityType::Count];
static s16 EntityTypes_3_Q2U[udtEntityType::Count * 2];
static const s16 EntityTypes_3[] =
{
	(s16)udtEntityType::Event, 12,
	(s16)udtEntityType::General, 0,
	(s16)udtEntityType::Player, 1,
	(s16)udtEntityType::Item, 2,
	(s16)udtEntityType::Missile, 3,
	(s16)udtEntityType::Mover, 4,
	(s16)udtEntityType::Beam, 5,
	(s16)udtEntityType::Portal, 6,
	(s16)udtEntityType::Speaker, 7,
	(s16)udtEntityType::PushTrigger, 8,
	(s16)udtEntityType::TeleportTrigger, 9,
	(s16)udtEntityType::Invisible, 10,
	(s16)udtEntityType::Grapple, 11,
	TABLE_END
};
VALIDATE_TABLE_SIZES(EntityTypes_3, udtEntityType::Count);

static s16 EntityTypes_48p_U2Q[udtEntityType::Count];
static s16 EntityTypes_48p_Q2U[udtEntityType::Count * 2];
static const s16 EntityTypes_48p[] =
{
	(s16)udtEntityType::Event, 13,
	(s16)udtEntityType::General, 0,
	(s16)udtEntityType::Player, 1,
	(s16)udtEntityType::Item, 2,
	(s16)udtEntityType::Missile, 3,
	(s16)udtEntityType::Mover, 4,
	(s16)udtEntityType::Beam, 5,
	(s16)udtEntityType::Portal, 6,
	(s16)udtEntityType::Speaker, 7,
	(s16)udtEntityType::PushTrigger, 8,
	(s16)udtEntityType::TeleportTrigger, 9,
	(s16)udtEntityType::Invisible, 10,
	(s16)udtEntityType::Grapple, 11,
	(s16)udtEntityType::Team, 12,
	TABLE_END
};
VALIDATE_TABLE_SIZES(EntityTypes_48p, udtEntityType::Count);

#if 0 // ET
static s16 EntityTypes_84_U2Q[udtEntityType::Count];
static s16 EntityTypes_84_Q2U[udtEntityType::Count * 2];
static const s16 EntityTypes_84[] =
{
	(s16)udtEntityType::Event, 61,
	(s16)udtEntityType::General, 0,
	(s16)udtEntityType::Player, 1,
	(s16)udtEntityType::Item, 2,
	(s16)udtEntityType::Missile, 3,
	(s16)udtEntityType::Mover, 4,
	(s16)udtEntityType::Beam, 5,
	(s16)udtEntityType::Portal, 6,
	(s16)udtEntityType::Speaker, 7,
	(s16)udtEntityType::PushTrigger, 8,
	(s16)udtEntityType::TeleportTrigger, 9,
	(s16)udtEntityType::Invisible, 10,
	(s16)udtEntityType::Grapple, 11,
	(s16)udtEntityType::Team, 12,
	TABLE_END
};
VALIDATE_TABLE_SIZES(EntityTypes_84, udtEntityType::Count);
#endif

static s16 EntityTypes_57_60_U2Q[udtEntityType::Count];
static s16 EntityTypes_57_60_Q2U[udtEntityType::Count * 2];
static const s16 EntityTypes_57_60[] =
{
	(s16)udtEntityType::Event, 40,
	(s16)udtEntityType::General, 0,
	(s16)udtEntityType::Player, 1,
	(s16)udtEntityType::Item, 2,
	(s16)udtEntityType::Missile, 3,
	(s16)udtEntityType::Mover, 4,
	(s16)udtEntityType::Beam, 5,
	(s16)udtEntityType::Portal, 6,
	(s16)udtEntityType::Speaker, 7,
	(s16)udtEntityType::PushTrigger, 8,
	(s16)udtEntityType::TeleportTrigger, 9,
	(s16)udtEntityType::Invisible, 10,
	(s16)udtEntityType::Grapple, 11,
	TABLE_END
};
VALIDATE_TABLE_SIZES(EntityTypes_57_60, udtEntityType::Count);

static const s16* EntityTypeTables[] =
{
	TABLE_ENTRY(EntityTypes_3), // 3
	TABLE_ENTRY(EntityTypes_48p), // 48
	TABLE_ENTRY(EntityTypes_57_60), // 57
	TABLE_ENTRY(EntityTypes_57_60), // 58
	TABLE_ENTRY(EntityTypes_57_60), // 59
	TABLE_ENTRY(EntityTypes_57_60), // 60
	TABLE_ENTRY(EntityTypes_48p), // 66
	TABLE_ENTRY(EntityTypes_48p), // 67
	TABLE_ENTRY(EntityTypes_48p), // 68
	TABLE_ENTRY(EntityTypes_48p), // 73
	TABLE_ENTRY(EntityTypes_48p), // 90
	TABLE_ENTRY(EntityTypes_48p) // 91
};
VALIDATE_TABLE_SIZE(EntityTypeTables);

static s16 EntityFlagBits_3_U2Q[udtEntityFlag::Count];
static s16 EntityFlagBits_3_Q2U[udtEntityFlag::Count * 2];
static const s16 EntityFlagBits_3[] =
{
	(s16)udtEntityFlag::Dead, 0,
	(s16)udtEntityFlag::TeleportBit, 2,
	(s16)udtEntityFlag::AwardExcellent, 3,
	(s16)udtEntityFlag::AwardHumiliation, 6,
	(s16)udtEntityFlag::NoDraw, 7,
	(s16)udtEntityFlag::Firing, 8,
	(s16)udtEntityFlag::AwardCapture, 11,
	(s16)udtEntityFlag::Chatting, 12,
	(s16)udtEntityFlag::ConnectionInterrupted, 13,
	(s16)udtEntityFlag::HasVoted, 14,
	(s16)udtEntityFlag::AwardImpressive, 15,
	TABLE_END
};
VALIDATE_TABLE_SIZES(EntityFlagBits_3, udtEntityFlag::Count);

static s16 EntityFlagBits_48_U2Q[udtEntityFlag::Count];
static s16 EntityFlagBits_48_Q2U[udtEntityFlag::Count * 2];
static const s16 EntityFlagBits_48[] =
{
	(s16)udtEntityFlag::Dead, 0,
	(s16)udtEntityFlag::TeleportBit, 2,
	(s16)udtEntityFlag::AwardExcellent, 3,
	(s16)udtEntityFlag::AwardHumiliation, 6,
	(s16)udtEntityFlag::NoDraw, 7,
	(s16)udtEntityFlag::Firing, 8,
	(s16)udtEntityFlag::AwardCapture, 11,
	(s16)udtEntityFlag::Chatting, 12,
	(s16)udtEntityFlag::ConnectionInterrupted, 13,
	(s16)udtEntityFlag::HasVoted, 14,
	(s16)udtEntityFlag::AwardImpressive, 15,
	(s16)udtEntityFlag::AwardDefense, 16,
	(s16)udtEntityFlag::AwardAssist, 17,
	(s16)udtEntityFlag::AwardDenied, 18,
	(s16)udtEntityFlag::HasTeamVoted, 19,
	TABLE_END
};
VALIDATE_TABLE_SIZES(EntityFlagBits_48, udtEntityFlag::Count);

static s16 EntityFlagBits_66_90_U2Q[udtEntityFlag::Count];
static s16 EntityFlagBits_66_90_Q2U[udtEntityFlag::Count * 2];
static const s16 EntityFlagBits_66_90[] =
{
	(s16)udtEntityFlag::Dead, 0,
	(s16)udtEntityFlag::TeleportBit, 2,
	(s16)udtEntityFlag::AwardExcellent, 3,
	(s16)udtEntityFlag::PlayerEvent, 4,
	(s16)udtEntityFlag::AwardHumiliation, 6,
	(s16)udtEntityFlag::NoDraw, 7,
	(s16)udtEntityFlag::Firing, 8,
	(s16)udtEntityFlag::AwardCapture, 11,
	(s16)udtEntityFlag::Chatting, 12,
	(s16)udtEntityFlag::ConnectionInterrupted, 13,
	(s16)udtEntityFlag::HasVoted, 14,
	(s16)udtEntityFlag::AwardImpressive, 15,
	(s16)udtEntityFlag::AwardDefense, 16,
	(s16)udtEntityFlag::AwardAssist, 17,
	(s16)udtEntityFlag::AwardDenied, 18,
	(s16)udtEntityFlag::HasTeamVoted, 19,
	TABLE_END
};
VALIDATE_TABLE_SIZES(EntityFlagBits_66_90, udtEntityFlag::Count);

static s16 EntityFlagBits_91_U2Q[udtEntityFlag::Count];
static s16 EntityFlagBits_91_Q2U[udtEntityFlag::Count * 2];
static const s16 EntityFlagBits_91[] =
{
	(s16)udtEntityFlag::Dead, 0,
	(s16)udtEntityFlag::TeleportBit, 2,
	(s16)udtEntityFlag::AwardExcellent, 3,
	(s16)udtEntityFlag::PlayerEvent, 4,
	(s16)udtEntityFlag::AwardHumiliation, 6,
	(s16)udtEntityFlag::NoDraw, 7,
	(s16)udtEntityFlag::Firing, 8,
	(s16)udtEntityFlag::AwardCapture, 11,
	(s16)udtEntityFlag::Chatting, 12,
	(s16)udtEntityFlag::ConnectionInterrupted, 13,
	(s16)udtEntityFlag::AwardImpressive, 15,
	(s16)udtEntityFlag::AwardDefense, 16,
	(s16)udtEntityFlag::AwardAssist, 17,
	(s16)udtEntityFlag::AwardDenied, 18,
	(s16)udtEntityFlag::Spectator, 14,
	TABLE_END
};
VALIDATE_TABLE_SIZES(EntityFlagBits_91, udtEntityFlag::Count);

static s16 EntityFlagBits_57_60_U2Q[udtEntityFlag::Count];
static s16 EntityFlagBits_57_60_Q2U[udtEntityFlag::Count * 2];
static const s16 EntityFlagBits_57_60[] =
{
	(s16)udtEntityFlag::Dead, 0,
	(s16)udtEntityFlag::TeleportBit, 2,
	(s16)udtEntityFlag::Wolf_Crouching, 5,
	(s16)udtEntityFlag::NoDraw, 7,
	(s16)udtEntityFlag::Firing, 8,
	(s16)udtEntityFlag::Chatting, 12,
	(s16)udtEntityFlag::ConnectionInterrupted, 13,
	(s16)udtEntityFlag::Wolf_Headshot, 15,
	(s16)udtEntityFlag::HasVoted, 17,
	(s16)udtEntityFlag::Wolf_Zooming, 22,
	TABLE_END
};
VALIDATE_TABLE_SIZES(EntityFlagBits_57_60, udtEntityFlag::Count);

static const s16* EntityFlagBitTables[] =
{
	TABLE_ENTRY(EntityFlagBits_3), // 3
	TABLE_ENTRY(EntityFlagBits_48), // 48
	TABLE_ENTRY(EntityFlagBits_57_60), // 57
	TABLE_ENTRY(EntityFlagBits_57_60), // 58
	TABLE_ENTRY(EntityFlagBits_57_60), // 59
	TABLE_ENTRY(EntityFlagBits_57_60), // 60
	TABLE_ENTRY(EntityFlagBits_66_90), // 66
	TABLE_ENTRY(EntityFlagBits_66_90), // 67
	TABLE_ENTRY(EntityFlagBits_66_90), // 68
	TABLE_ENTRY(EntityFlagBits_66_90), // 73
	TABLE_ENTRY(EntityFlagBits_66_90), // 90
	TABLE_ENTRY(EntityFlagBits_91) // 91
};
VALIDATE_TABLE_SIZE(EntityFlagBitTables);

static s16 EntityEvents_3_U2Q[udtEntityEvent::Count];
static s16 EntityEvents_3_Q2U[udtEntityEvent::Count * 2];
static const s16 EntityEvents_3[] =
{
	(s16)udtEntityEvent::Obituary, 58,
	(s16)udtEntityEvent::WeaponFired, 23,
	(s16)udtEntityEvent::ItemPickup, 19,
	(s16)udtEntityEvent::GlobalItemPickup, 20,
	(s16)udtEntityEvent::GlobalSound, 46,
	(s16)udtEntityEvent::ItemRespawn, 40,
	(s16)udtEntityEvent::ItemPop, 41,
	(s16)udtEntityEvent::PlayerTeleportIn, 42,
	(s16)udtEntityEvent::PlayerTeleportOut, 43,
	(s16)udtEntityEvent::BulletHitFlesh, 47,
	(s16)udtEntityEvent::BulletHitWall, 48,
	(s16)udtEntityEvent::MissileHit, 49,
	(s16)udtEntityEvent::MissileMiss, 50,
	(s16)udtEntityEvent::RailTrail, 51,
	(s16)udtEntityEvent::PowerUpQuad, 59,
	(s16)udtEntityEvent::PowerUpBattleSuit, 60,
	(s16)udtEntityEvent::PowerUpRegen, 61,
	TABLE_END
};
VALIDATE_TABLE_SIZES(EntityEvents_3, udtEntityEvent::Count);

static s16 EntityEvents_48_68_U2Q[udtEntityEvent::Count];
static s16 EntityEvents_48_68_Q2U[udtEntityEvent::Count * 2];
static const s16 EntityEvents_48_68[] =
{
	(s16)udtEntityEvent::Obituary, 60,
	(s16)udtEntityEvent::WeaponFired, 23,
	(s16)udtEntityEvent::ItemPickup, 19,
	(s16)udtEntityEvent::GlobalItemPickup, 20,
	(s16)udtEntityEvent::GlobalSound, 46,
	(s16)udtEntityEvent::GlobalTeamSound, 47,
	(s16)udtEntityEvent::ItemRespawn, 40,
	(s16)udtEntityEvent::ItemPop, 41,
	(s16)udtEntityEvent::PlayerTeleportIn, 42,
	(s16)udtEntityEvent::PlayerTeleportOut, 43,
	(s16)udtEntityEvent::BulletHitFlesh, 48,
	(s16)udtEntityEvent::BulletHitWall, 49,
	(s16)udtEntityEvent::MissileHit, 50,
	(s16)udtEntityEvent::MissileMiss, 51,
	(s16)udtEntityEvent::MissileMissMetal, 52,
	(s16)udtEntityEvent::RailTrail, 53,
	(s16)udtEntityEvent::PowerUpQuad, 61,
	(s16)udtEntityEvent::PowerUpBattleSuit, 62,
	(s16)udtEntityEvent::PowerUpRegen, 63,
	TABLE_END
};
VALIDATE_TABLE_SIZES(EntityEvents_48_68, udtEntityEvent::Count);

static s16 EntityEvents_73p_U2Q[udtEntityEvent::Count];
static s16 EntityEvents_73p_Q2U[udtEntityEvent::Count * 2];
static const s16 EntityEvents_73p[] =
{
	(s16)udtEntityEvent::Obituary, 58,
	(s16)udtEntityEvent::WeaponFired, 20,
	(s16)udtEntityEvent::ItemPickup, 15,
	(s16)udtEntityEvent::GlobalItemPickup, 16,
	(s16)udtEntityEvent::GlobalSound, 43,
	(s16)udtEntityEvent::GlobalTeamSound, 44,
	(s16)udtEntityEvent::ItemRespawn, 37,
	(s16)udtEntityEvent::ItemPop, 38,
	(s16)udtEntityEvent::PlayerTeleportIn, 39,
	(s16)udtEntityEvent::PlayerTeleportOut, 40,
	(s16)udtEntityEvent::BulletHitFlesh, 45,
	(s16)udtEntityEvent::BulletHitWall, 46,
	(s16)udtEntityEvent::MissileHit, 47,
	(s16)udtEntityEvent::MissileMiss, 48,
	(s16)udtEntityEvent::MissileMissMetal, 49,
	(s16)udtEntityEvent::RailTrail, 50,
	(s16)udtEntityEvent::PowerUpQuad, 59,
	(s16)udtEntityEvent::PowerUpBattleSuit, 60,
	(s16)udtEntityEvent::PowerUpRegen, 61,
	(s16)udtEntityEvent::QL_Overtime, 84,
	(s16)udtEntityEvent::QL_GameOver, 85,
	TABLE_END
};
VALIDATE_TABLE_SIZES(EntityEvents_73p, udtEntityEvent::Count);

#if 0 // ET
static s16 EntityEvents_84_U2Q[udtEntityEvent::Count];
static s16 EntityEvents_84_Q2U[udtEntityEvent::Count * 2];
static const s16 EntityEvents_84[] =
{
	(s16)udtEntityEvent::Obituary, 70,
	(s16)udtEntityEvent::BulletHitFlesh, 57,
	TABLE_END
};
VALIDATE_TABLE_SIZES(EntityEvents_84, udtEntityEvent::Count);
#endif

static s16 EntityEvents_57_58_U2Q[udtEntityEvent::Count];
static s16 EntityEvents_57_58_Q2U[udtEntityEvent::Count * 2];
static const s16 EntityEvents_57_58[] =
{
	(s16)udtEntityEvent::Obituary, 81,
	TABLE_END
};
VALIDATE_TABLE_SIZES(EntityEvents_57_58, udtEntityEvent::Count);

static s16 EntityEvents_59_U2Q[udtEntityEvent::Count];
static s16 EntityEvents_59_Q2U[udtEntityEvent::Count * 2];
static const s16 EntityEvents_59[] =
{
	(s16)udtEntityEvent::Obituary, 83,
	TABLE_END
};
VALIDATE_TABLE_SIZES(EntityEvents_59, udtEntityEvent::Count);

static s16 EntityEvents_60_U2Q[udtEntityEvent::Count];
static s16 EntityEvents_60_Q2U[udtEntityEvent::Count * 2];
static const s16 EntityEvents_60[] =
{
	(s16)udtEntityEvent::Obituary, 85,
	(s16)udtEntityEvent::WeaponFired, 40,
	(s16)udtEntityEvent::ItemPickup, 30,
	(s16)udtEntityEvent::GlobalItemPickup, 32,
	(s16)udtEntityEvent::GlobalSound, 68,
	(s16)udtEntityEvent::ItemRespawn, 62,
	(s16)udtEntityEvent::ItemPop, 63,
	(s16)udtEntityEvent::PlayerTeleportIn, 64,
	(s16)udtEntityEvent::PlayerTeleportOut, 65,
	(s16)udtEntityEvent::BulletHitFlesh, 70,
	(s16)udtEntityEvent::BulletHitWall, 71,
	(s16)udtEntityEvent::MissileHit, 72,
	(s16)udtEntityEvent::MissileMiss, 73,
	(s16)udtEntityEvent::RailTrail, 74,
	(s16)udtEntityEvent::PowerUpQuad, 87,
	(s16)udtEntityEvent::PowerUpBattleSuit, 88,
	(s16)udtEntityEvent::PowerUpRegen, 89,
	TABLE_END
};
VALIDATE_TABLE_SIZES(EntityEvents_60, udtEntityEvent::Count);

static const s16* EntityEventTables[] =
{
	TABLE_ENTRY(EntityEvents_3), // 3
	TABLE_ENTRY(EntityEvents_48_68), // 48
	TABLE_ENTRY(EntityEvents_57_58), // 57
	TABLE_ENTRY(EntityEvents_57_58), // 58
	TABLE_ENTRY(EntityEvents_59), // 59
	TABLE_ENTRY(EntityEvents_60), // 60
	TABLE_ENTRY(EntityEvents_48_68), // 66
	TABLE_ENTRY(EntityEvents_48_68), // 67
	TABLE_ENTRY(EntityEvents_48_68), // 68
	TABLE_ENTRY(EntityEvents_73p), // 73
	TABLE_ENTRY(EntityEvents_73p), // 90
	TABLE_ENTRY(EntityEvents_73p) // 91
};
VALIDATE_TABLE_SIZE(EntityEventTables);

static s16 ConfigStringIndices_3_U2Q[udtConfigStringIndex::Count];
static s16 ConfigStringIndices_3_Q2U[udtConfigStringIndex::Count * 2];
static const s16 ConfigStringIndices_3[] =
{
	(s16)udtConfigStringIndex::FirstPlayer, 544,
	(s16)udtConfigStringIndex::Intermission, 14,
	(s16)udtConfigStringIndex::LevelStartTime, 13,
	(s16)udtConfigStringIndex::WarmUpEndTime, 5,
	(s16)udtConfigStringIndex::FlagStatus, 15,
	(s16)udtConfigStringIndex::ServerInfo, 0,
	(s16)udtConfigStringIndex::SystemInfo, 1,
	(s16)udtConfigStringIndex::Scores1, 6,
	(s16)udtConfigStringIndex::Scores2, 7,
	(s16)udtConfigStringIndex::VoteTime, 8,
	(s16)udtConfigStringIndex::VoteString, 9,
	(s16)udtConfigStringIndex::VoteYes, 10,
	(s16)udtConfigStringIndex::VoteNo, 11,
	(s16)udtConfigStringIndex::GameVersion, 12,
	(s16)udtConfigStringIndex::ItemFlags, 27,
	(s16)udtConfigStringIndex::QL_TimeoutStartTime, 669,
	(s16)udtConfigStringIndex::QL_TimeoutEndTime, 670,
	(s16)udtConfigStringIndex::QL_ReadTeamClanName, 693,
	(s16)udtConfigStringIndex::QL_BlueTeamClanName, 694,
	(s16)udtConfigStringIndex::QL_RedTeamClanTag, 695,
	(s16)udtConfigStringIndex::QL_BlueTeamClanTag, 696,
	(s16)udtConfigStringIndex::CPMA_GameInfo, 672,
	(s16)udtConfigStringIndex::CPMA_RoundInfo, 710,
	(s16)udtConfigStringIndex::OSP_GamePlay, 806,
	TABLE_END
};
VALIDATE_TABLE_SIZES(ConfigStringIndices_3, udtConfigStringIndex::Count);

static s16 ConfigStringIndices_48_68_U2Q[udtConfigStringIndex::Count];
static s16 ConfigStringIndices_48_68_Q2U[udtConfigStringIndex::Count * 2];
static const s16 ConfigStringIndices_48_68[] =
{
	(s16)udtConfigStringIndex::FirstPlayer, 544,
	(s16)udtConfigStringIndex::Intermission, 22,
	(s16)udtConfigStringIndex::LevelStartTime, 21,
	(s16)udtConfigStringIndex::WarmUpEndTime, 5,
	(s16)udtConfigStringIndex::FlagStatus, 23,
	(s16)udtConfigStringIndex::ServerInfo, 0,
	(s16)udtConfigStringIndex::SystemInfo, 1,
	(s16)udtConfigStringIndex::Scores1, 6,
	(s16)udtConfigStringIndex::Scores2, 7,
	(s16)udtConfigStringIndex::VoteTime, 8,
	(s16)udtConfigStringIndex::VoteString, 9,
	(s16)udtConfigStringIndex::VoteYes, 10,
	(s16)udtConfigStringIndex::VoteNo, 11,
	(s16)udtConfigStringIndex::TeamVoteTime, 12,
	(s16)udtConfigStringIndex::TeamVoteString, 14,
	(s16)udtConfigStringIndex::TeamVoteYes, 16,
	(s16)udtConfigStringIndex::TeamVoteNo, 18,
	(s16)udtConfigStringIndex::GameVersion, 20,
	(s16)udtConfigStringIndex::ItemFlags, 27,
	(s16)udtConfigStringIndex::QL_TimeoutStartTime, 669,
	(s16)udtConfigStringIndex::QL_TimeoutEndTime, 670,
	(s16)udtConfigStringIndex::QL_ReadTeamClanName, 693,
	(s16)udtConfigStringIndex::QL_BlueTeamClanName, 694,
	(s16)udtConfigStringIndex::QL_RedTeamClanTag, 695,
	(s16)udtConfigStringIndex::QL_BlueTeamClanTag, 696,
	(s16)udtConfigStringIndex::CPMA_GameInfo, 672,
	(s16)udtConfigStringIndex::CPMA_RoundInfo, 710,
	(s16)udtConfigStringIndex::OSP_GamePlay, 806,
	TABLE_END
};
VALIDATE_TABLE_SIZES(ConfigStringIndices_48_68, udtConfigStringIndex::Count);

static s16 ConfigStringIndices_73_90_U2Q[udtConfigStringIndex::Count];
static s16 ConfigStringIndices_73_90_Q2U[udtConfigStringIndex::Count * 2];
static const s16 ConfigStringIndices_73_90[] =
{
	(s16)udtConfigStringIndex::FirstPlayer, 529,
	(s16)udtConfigStringIndex::Intermission, 14,
	(s16)udtConfigStringIndex::LevelStartTime, 13,
	(s16)udtConfigStringIndex::WarmUpEndTime, 5,
	(s16)udtConfigStringIndex::PauseStart, 669,
	(s16)udtConfigStringIndex::PauseEnd, 670,
	(s16)udtConfigStringIndex::FlagStatus, 658,
	(s16)udtConfigStringIndex::ServerInfo, 0,
	(s16)udtConfigStringIndex::SystemInfo, 1,
	(s16)udtConfigStringIndex::Scores1, 6,
	(s16)udtConfigStringIndex::Scores2, 7,
	(s16)udtConfigStringIndex::VoteTime, 8,
	(s16)udtConfigStringIndex::VoteString, 9,
	(s16)udtConfigStringIndex::VoteYes, 10,
	(s16)udtConfigStringIndex::VoteNo, 11,
	(s16)udtConfigStringIndex::GameVersion, 12,
	(s16)udtConfigStringIndex::ItemFlags, 15,
	(s16)udtConfigStringIndex::QL_TimeoutStartTime, 669,
	(s16)udtConfigStringIndex::QL_TimeoutEndTime, 670,
	(s16)udtConfigStringIndex::QL_ReadTeamClanName, 693,
	(s16)udtConfigStringIndex::QL_BlueTeamClanName, 694,
	(s16)udtConfigStringIndex::QL_RedTeamClanTag, 695,
	(s16)udtConfigStringIndex::QL_BlueTeamClanTag, 696,
	TABLE_END
};
VALIDATE_TABLE_SIZES(ConfigStringIndices_73_90, udtConfigStringIndex::Count);

#if 0 // ET
static s16 ConfigStringIndices_84_U2Q[udtConfigStringIndex::Count];
static s16 ConfigStringIndices_84_Q2U[udtConfigStringIndex::Count * 2];
static const s16 ConfigStringIndices_84[] =
{
	(s16)udtConfigStringIndex::FirstPlayer, 689,
	(s16)udtConfigStringIndex::Intermission, 12,
	(s16)udtConfigStringIndex::LevelStartTime, 11,
	(s16)udtConfigStringIndex::WarmUpEndTime, 5,
	(s16)udtConfigStringIndex::ServerInfo, 0,
	(s16)udtConfigStringIndex::SystemInfo, 1,
	(s16)udtConfigStringIndex::GameVersion, 10,
	TABLE_END
};
VALIDATE_TABLE_SIZES(ConfigStringIndices_84, udtConfigStringIndex::Count);
#endif

static s16 ConfigStringIndices_91_U2Q[udtConfigStringIndex::Count];
static s16 ConfigStringIndices_91_Q2U[udtConfigStringIndex::Count * 2];
static const s16 ConfigStringIndices_91[] =
{
	(s16)udtConfigStringIndex::FirstPlayer, 529,
	(s16)udtConfigStringIndex::Intermission, 14,
	(s16)udtConfigStringIndex::LevelStartTime, 13,
	(s16)udtConfigStringIndex::WarmUpEndTime, 5,
	(s16)udtConfigStringIndex::FirstPlacePlayerName, 659,
	(s16)udtConfigStringIndex::SecondPlacePlayerName, 660,
	(s16)udtConfigStringIndex::PauseStart, 669,
	(s16)udtConfigStringIndex::PauseEnd, 670,
	(s16)udtConfigStringIndex::FlagStatus, 658,
	(s16)udtConfigStringIndex::ServerInfo, 0,
	(s16)udtConfigStringIndex::SystemInfo, 1,
	(s16)udtConfigStringIndex::Scores1, 6,
	(s16)udtConfigStringIndex::Scores2, 7,
	(s16)udtConfigStringIndex::VoteTime, 8,
	(s16)udtConfigStringIndex::VoteString, 9,
	(s16)udtConfigStringIndex::VoteYes, 10,
	(s16)udtConfigStringIndex::VoteNo, 11,
	(s16)udtConfigStringIndex::GameVersion, 12,
	(s16)udtConfigStringIndex::ItemFlags, 15,
	(s16)udtConfigStringIndex::QL_RedTeamTimeoutsLeft, 671,
	(s16)udtConfigStringIndex::QL_BlueTeamTimeoutsLeft, 672,
	TABLE_END
};
VALIDATE_TABLE_SIZES(ConfigStringIndices_91, udtConfigStringIndex::Count);

static s16 ConfigStringIndices_57_60_U2Q[udtConfigStringIndex::Count];
static s16 ConfigStringIndices_57_60_Q2U[udtConfigStringIndex::Count * 2];
static const s16 ConfigStringIndices_57_60[] =
{
	(s16)udtConfigStringIndex::FirstPlayer, 576,
	(s16)udtConfigStringIndex::Intermission, 14,
	(s16)udtConfigStringIndex::LevelStartTime, 13,
	(s16)udtConfigStringIndex::WarmUpEndTime, 5,
	(s16)udtConfigStringIndex::ServerInfo, 0,
	(s16)udtConfigStringIndex::SystemInfo, 1,
	(s16)udtConfigStringIndex::Scores1, 6,
	(s16)udtConfigStringIndex::Scores2, 7,
	(s16)udtConfigStringIndex::VoteTime, 8,
	(s16)udtConfigStringIndex::VoteString, 9,
	(s16)udtConfigStringIndex::VoteYes, 10,
	(s16)udtConfigStringIndex::VoteNo, 11,
	(s16)udtConfigStringIndex::Wolf_Info, 36,
	(s16)udtConfigStringIndex::Wolf_Paused, 40,
	(s16)udtConfigStringIndex::Wolf_Ready, 41,
	TABLE_END
};
VALIDATE_TABLE_SIZES(ConfigStringIndices_57_60, udtConfigStringIndex::Count);

static const s16* ConfigStringIndexTables[] =
{
	TABLE_ENTRY(ConfigStringIndices_3), // 3
	TABLE_ENTRY(ConfigStringIndices_48_68), // 48
	TABLE_ENTRY(ConfigStringIndices_57_60), // 57
	TABLE_ENTRY(ConfigStringIndices_57_60), // 58
	TABLE_ENTRY(ConfigStringIndices_57_60), // 59
	TABLE_ENTRY(ConfigStringIndices_57_60), // 60
	TABLE_ENTRY(ConfigStringIndices_48_68), // 66
	TABLE_ENTRY(ConfigStringIndices_48_68), // 67
	TABLE_ENTRY(ConfigStringIndices_48_68), // 68
	TABLE_ENTRY(ConfigStringIndices_73_90), // 73
	TABLE_ENTRY(ConfigStringIndices_73_90), // 90
	TABLE_ENTRY(ConfigStringIndices_91) // 91
};
VALIDATE_TABLE_SIZE(ConfigStringIndexTables);

static s16 TeamsQuake_U2Q[udtTeam::Count];
static s16 TeamsQuake_Q2U[udtTeam::Count * 2];
static const s16 TeamsQuake[] =
{
	(s16)udtTeam::Free, 0,
	(s16)udtTeam::Red, 1,
	(s16)udtTeam::Blue, 2,
	(s16)udtTeam::Spectators, 3,
	TABLE_END
};
VALIDATE_TABLE_SIZES(TeamsQuake, udtTeam::Count);

static s16 TeamsWolf_U2Q[udtTeam::Count];
static s16 TeamsWolf_Q2U[udtTeam::Count * 2];
static const s16 TeamsWolf[] =
{
	(s16)udtTeam::Free, 0,
	(s16)udtTeam::Axis, 1,
	(s16)udtTeam::Allies, 2,
	(s16)udtTeam::Spectators, 3,
	TABLE_END
};
VALIDATE_TABLE_SIZES(TeamsWolf, udtTeam::Count);

static const s16* TeamTables[] =
{
	TABLE_ENTRY(TeamsQuake), // 3
	TABLE_ENTRY(TeamsQuake), // 48
	TABLE_ENTRY(TeamsWolf), // 57
	TABLE_ENTRY(TeamsWolf), // 58
	TABLE_ENTRY(TeamsWolf), // 59
	TABLE_ENTRY(TeamsWolf), // 60
	TABLE_ENTRY(TeamsQuake), // 66
	TABLE_ENTRY(TeamsQuake), // 67
	TABLE_ENTRY(TeamsQuake), // 68
	TABLE_ENTRY(TeamsQuake), // 73
	TABLE_ENTRY(TeamsQuake), // 90
	TABLE_ENTRY(TeamsQuake) // 91
};
VALIDATE_TABLE_SIZE(TeamTables);

static s16 GameTypes_3_U2Q[udtGameType::Count];
static s16 GameTypes_3_Q2U[udtGameType::Count * 2];
static const s16 GameTypes_3[] =
{
	(s16)udtGameType::FFA, 0,
	(s16)udtGameType::Duel, 1,
	(s16)udtGameType::TDM, 3,
	(s16)udtGameType::CTF, 4,
	TABLE_END
};
VALIDATE_TABLE_SIZES(GameTypes_3, udtGameType::Count);

static s16 GameTypes_48_68_U2Q[udtGameType::Count];
static s16 GameTypes_48_68_Q2U[udtGameType::Count * 2];
static const s16 GameTypes_48_68[] =
{
	(s16)udtGameType::FFA, 0,
	(s16)udtGameType::Duel, 1,
	(s16)udtGameType::TDM, 3,
	(s16)udtGameType::CTF, 4,
	(s16)udtGameType::OneFlagCTF, 5,
	(s16)udtGameType::Obelisk, 6,
	(s16)udtGameType::Harvester, 7,
	TABLE_END
};
VALIDATE_TABLE_SIZES(GameTypes_48_68, udtGameType::Count);

static s16 GameTypes_73p_U2Q[udtGameType::Count];
static s16 GameTypes_73p_Q2U[udtGameType::Count * 2];
static const s16 GameTypes_73p[] =
{
	(s16)udtGameType::FFA, 0,
	(s16)udtGameType::Duel, 1,
	(s16)udtGameType::Race, 2,
	(s16)udtGameType::RedRover, 12,
	(s16)udtGameType::TDM, 3,
	(s16)udtGameType::CA, 4,
	(s16)udtGameType::CTF, 5,
	(s16)udtGameType::OneFlagCTF, 6,
	(s16)udtGameType::Obelisk, 7,
	(s16)udtGameType::Harvester, 8,
	(s16)udtGameType::Domination, 10,
	(s16)udtGameType::CTFS, 11,
	(s16)udtGameType::FT, 9,
	TABLE_END
};
VALIDATE_TABLE_SIZES(GameTypes_73p, udtGameType::Count);

static s16 GameTypes_57_60_U2Q[udtGameType::Count];
static s16 GameTypes_57_60_Q2U[udtGameType::Count * 2];
static const s16 GameTypes_57_60[] =
{
	(s16)udtGameType::FFA, 0,
	(s16)udtGameType::Duel, 1,
	(s16)udtGameType::SP, 2,
	(s16)udtGameType::TDM, 3,
	(s16)udtGameType::CTF, 4,
	(s16)udtGameType::Wolf_Objective, 5,
	(s16)udtGameType::Wolf_Stopwatch, 6,
	(s16)udtGameType::Wolf_Checkpoint, 7,
	(s16)udtGameType::Wolf_CaptureAndHold, 8,
	TABLE_END
};
VALIDATE_TABLE_SIZES(GameTypes_57_60, udtGameType::Count);

static const s16* GameTypeTables[] =
{
	TABLE_ENTRY(GameTypes_3), // 3
	TABLE_ENTRY(GameTypes_48_68), // 48
	TABLE_ENTRY(GameTypes_57_60), // 57
	TABLE_ENTRY(GameTypes_57_60), // 58
	TABLE_ENTRY(GameTypes_57_60), // 59
	TABLE_ENTRY(GameTypes_57_60), // 60
	TABLE_ENTRY(GameTypes_48_68), // 66
	TABLE_ENTRY(GameTypes_48_68), // 67
	TABLE_ENTRY(GameTypes_48_68), // 68
	TABLE_ENTRY(GameTypes_73p), // 73
	TABLE_ENTRY(GameTypes_73p), // 90
	TABLE_ENTRY(GameTypes_73p) // 91
};
VALIDATE_TABLE_SIZE(GameTypeTables);

static s16 FlagStatus_U2Q[udtFlagStatus::Count];
static s16 FlagStatus_Q2U[udtFlagStatus::Count * 2];
static const s16 FlagStatus[] =
{
	(s16)udtFlagStatus::InBase, 0,
	(s16)udtFlagStatus::Carried, 1,
	(s16)udtFlagStatus::Missing, 2,
	TABLE_END
};
VALIDATE_TABLE_SIZES(FlagStatus, udtFlagStatus::Count);

static const s16* FlagStatusTables[] =
{
	TABLE_ENTRY(FlagStatus), // 3
	TABLE_ENTRY(FlagStatus), // 48
	TABLE_ENTRY(FlagStatus), // 57 - not relevant but no need to make a new table for it
	TABLE_ENTRY(FlagStatus), // 58 - not relevant but no need to make a new table for it
	TABLE_ENTRY(FlagStatus), // 59 - not relevant but no need to make a new table for it
	TABLE_ENTRY(FlagStatus), // 60 - not relevant but no need to make a new table for it
	TABLE_ENTRY(FlagStatus), // 66
	TABLE_ENTRY(FlagStatus), // 67
	TABLE_ENTRY(FlagStatus), // 68
	TABLE_ENTRY(FlagStatus), // 73
	TABLE_ENTRY(FlagStatus), // 90
	TABLE_ENTRY(FlagStatus) // 91
};
VALIDATE_TABLE_SIZE(FlagStatusTables);

static s16 Weapons_3_68_U2Q[udtWeapon::Count];
static s16 Weapons_3_68_Q2U[udtWeapon::Count * 2];
static const s16 Weapons_3_68[] =
{
	(s16)udtWeapon::Gauntlet, 1,
	(s16)udtWeapon::MachineGun, 2,
	(s16)udtWeapon::Shotgun, 3,
	(s16)udtWeapon::GrenadeLauncher, 4,
	(s16)udtWeapon::RocketLauncher, 5,
	(s16)udtWeapon::PlasmaGun, 8,
	(s16)udtWeapon::Railgun, 7,
	(s16)udtWeapon::LightningGun, 6,
	(s16)udtWeapon::BFG, 9,
	(s16)udtWeapon::GrapplingHook, 10,
	TABLE_END
};
VALIDATE_TABLE_SIZES(Weapons_3_68, udtWeapon::Count);

static s16 Weapons_73p_U2Q[udtWeapon::Count];
static s16 Weapons_73p_Q2U[udtWeapon::Count * 2];
static const s16 Weapons_73p[] =
{
	(s16)udtWeapon::Gauntlet, 1,
	(s16)udtWeapon::MachineGun, 2,
	(s16)udtWeapon::Shotgun, 3,
	(s16)udtWeapon::GrenadeLauncher, 4,
	(s16)udtWeapon::RocketLauncher, 5,
	(s16)udtWeapon::PlasmaGun, 8,
	(s16)udtWeapon::Railgun, 7,
	(s16)udtWeapon::LightningGun, 6,
	(s16)udtWeapon::BFG, 9,
	(s16)udtWeapon::NailGun, 11,
	(s16)udtWeapon::ChainGun, 13,
	(s16)udtWeapon::ProximityMineLauncher, 12,
	(s16)udtWeapon::HeavyMachineGun, 14,
	(s16)udtWeapon::GrapplingHook, 10,
	TABLE_END
};
VALIDATE_TABLE_SIZES(Weapons_73p, udtWeapon::Count);

static s16 Weapons_57_60_U2Q[udtWeapon::Count];
static s16 Weapons_57_60_Q2U[udtWeapon::Count * 2];
static const s16 Weapons_57_60[] =
{
	(s16)udtWeapon::Knife, 1,
	(s16)udtWeapon::Luger, 2,
	(s16)udtWeapon::MP40, 3,
	(s16)udtWeapon::Mauser, 4,
	(s16)udtWeapon::FG42, 5,
	(s16)udtWeapon::GrenadeLauncher, 6,
	(s16)udtWeapon::Panzerfaust, 7,
	(s16)udtWeapon::Venom, 8,
	(s16)udtWeapon::Flamethrower, 9,
	(s16)udtWeapon::Tesla, 10,
	(s16)udtWeapon::Speargun, 11,
	(s16)udtWeapon::Knife2, 12,
	(s16)udtWeapon::Colt, 13,
	(s16)udtWeapon::Thompson, 14,
	(s16)udtWeapon::Garand, 15,
	(s16)udtWeapon::Bar, 16,
	(s16)udtWeapon::GrenadePineapple, 17,
	(s16)udtWeapon::RocketLauncher, 18,
	(s16)udtWeapon::SniperRifle, 19,
	(s16)udtWeapon::SnooperScope, 20,
	(s16)udtWeapon::VenomFull, 21,
	(s16)udtWeapon::SpeargunCO2, 22,
	(s16)udtWeapon::FG42Scope, 23,
	(s16)udtWeapon::Bar2, 24,
	(s16)udtWeapon::Sten, 25,
	(s16)udtWeapon::MedicSyringe, 26,
	(s16)udtWeapon::Ammo, 27,
	(s16)udtWeapon::Artillery, 28,
	(s16)udtWeapon::Silencer, 29,
	(s16)udtWeapon::Akimbo, 30,
	(s16)udtWeapon::Cross, 31,
	(s16)udtWeapon::Dynamite, 32,
	(s16)udtWeapon::Dynamite2, 33,
	(s16)udtWeapon::Prox, 34,
	(s16)udtWeapon::MonsterAttack1, 35,
	(s16)udtWeapon::MonsterAttack2, 36,
	(s16)udtWeapon::MonsterAttack3, 37,
	(s16)udtWeapon::SmokeTrail, 38,
	(s16)udtWeapon::Gauntlet, 39,
	(s16)udtWeapon::Sniper, 40,
	(s16)udtWeapon::Mortar, 41,
	(s16)udtWeapon::VeryBigExplosion, 42,
	(s16)udtWeapon::Medkit, 43,
	(s16)udtWeapon::Pliers, 44,
	(s16)udtWeapon::SmokeGrenade, 45,
	(s16)udtWeapon::Binoculars, 46,
	TABLE_END
};
VALIDATE_TABLE_SIZES(Weapons_57_60, udtWeapon::Count);

static const s16* WeaponTables[] =
{
	TABLE_ENTRY(Weapons_3_68), // 3
	TABLE_ENTRY(Weapons_3_68), // 48
	TABLE_ENTRY(Weapons_57_60), // 57
	TABLE_ENTRY(Weapons_57_60), // 58
	TABLE_ENTRY(Weapons_57_60), // 59
	TABLE_ENTRY(Weapons_57_60), // 60
	TABLE_ENTRY(Weapons_3_68), // 66
	TABLE_ENTRY(Weapons_3_68), // 67
	TABLE_ENTRY(Weapons_3_68), // 68
	TABLE_ENTRY(Weapons_73p), // 73
	TABLE_ENTRY(Weapons_73p), // 90
	TABLE_ENTRY(Weapons_73p) // 91
};
VALIDATE_TABLE_SIZE(WeaponTables);

static s16 MeansOfDeath_3_68_U2Q[udtMeanOfDeath::Count];
static s16 MeansOfDeath_3_68_Q2U[udtMeanOfDeath::Count * 2];
static const s16 MeansOfDeath_3_68[] =
{
	(s16)udtMeanOfDeath::Shotgun, 1,
	(s16)udtMeanOfDeath::Gauntlet, 2,
	(s16)udtMeanOfDeath::MachineGun, 3,
	(s16)udtMeanOfDeath::Grenade, 4,
	(s16)udtMeanOfDeath::GrenadeSplash, 5,
	(s16)udtMeanOfDeath::Rocket, 6,
	(s16)udtMeanOfDeath::RocketSplash, 7,
	(s16)udtMeanOfDeath::Plasma, 8,
	(s16)udtMeanOfDeath::PlasmaSplash, 9,
	(s16)udtMeanOfDeath::Railgun, 10,
	(s16)udtMeanOfDeath::Lightning, 11,
	(s16)udtMeanOfDeath::BFG, 12,
	(s16)udtMeanOfDeath::BFGSplash, 13,
	(s16)udtMeanOfDeath::Water, 14,
	(s16)udtMeanOfDeath::Slime, 15,
	(s16)udtMeanOfDeath::Lava, 16,
	(s16)udtMeanOfDeath::Crush, 17,
	(s16)udtMeanOfDeath::TeleFrag, 18,
	(s16)udtMeanOfDeath::Fall, 19,
	(s16)udtMeanOfDeath::Suicide, 20,
	(s16)udtMeanOfDeath::TargetLaser, 21,
	(s16)udtMeanOfDeath::TriggerHurt, 22,
	(s16)udtMeanOfDeath::Grapple, 23,
	TABLE_END
};
VALIDATE_TABLE_SIZES(MeansOfDeath_3_68, udtMeanOfDeath::Count);

static s16 MeansOfDeath_73p_U2Q[udtMeanOfDeath::Count];
static s16 MeansOfDeath_73p_Q2U[udtMeanOfDeath::Count * 2];
static const s16 MeansOfDeath_73p[] =
{
	(s16)udtMeanOfDeath::Shotgun, 1,
	(s16)udtMeanOfDeath::Gauntlet, 2,
	(s16)udtMeanOfDeath::MachineGun, 3,
	(s16)udtMeanOfDeath::Grenade, 4,
	(s16)udtMeanOfDeath::GrenadeSplash, 5,
	(s16)udtMeanOfDeath::Rocket, 6,
	(s16)udtMeanOfDeath::RocketSplash, 7,
	(s16)udtMeanOfDeath::Plasma, 8,
	(s16)udtMeanOfDeath::PlasmaSplash, 9,
	(s16)udtMeanOfDeath::Railgun, 10,
	(s16)udtMeanOfDeath::Lightning, 11,
	(s16)udtMeanOfDeath::BFG, 12,
	(s16)udtMeanOfDeath::BFGSplash, 13,
	(s16)udtMeanOfDeath::Water, 14,
	(s16)udtMeanOfDeath::Slime, 15,
	(s16)udtMeanOfDeath::Lava, 16,
	(s16)udtMeanOfDeath::Crush, 17,
	(s16)udtMeanOfDeath::TeleFrag, 18,
	(s16)udtMeanOfDeath::Fall, 19,
	(s16)udtMeanOfDeath::Suicide, 20,
	(s16)udtMeanOfDeath::TargetLaser, 21,
	(s16)udtMeanOfDeath::TriggerHurt, 22,
	(s16)udtMeanOfDeath::NailGun, 23,
	(s16)udtMeanOfDeath::ChainGun, 24,
	(s16)udtMeanOfDeath::ProximityMine, 25,
	(s16)udtMeanOfDeath::Kamikaze, 26,
	(s16)udtMeanOfDeath::Juiced, 27,
	(s16)udtMeanOfDeath::Grapple, 28,
	(s16)udtMeanOfDeath::TeamSwitch, 29,
	(s16)udtMeanOfDeath::Thaw, 30,
	(s16)udtMeanOfDeath::HeavyMachineGun, 32,
	TABLE_END
};
VALIDATE_TABLE_SIZES(MeansOfDeath_73p, udtMeanOfDeath::Count);

static s16 MeansOfDeath_57_60_U2Q[udtMeanOfDeath::Count];
static s16 MeansOfDeath_57_60_Q2U[udtMeanOfDeath::Count * 2];
static const s16 MeansOfDeath_57_60[] =
{
	(s16)udtMeanOfDeath::Shotgun, 1,
	(s16)udtMeanOfDeath::Gauntlet, 2,
	(s16)udtMeanOfDeath::MachineGun, 3,
	(s16)udtMeanOfDeath::Grenade, 4,
	(s16)udtMeanOfDeath::GrenadeSplash, 5,
	(s16)udtMeanOfDeath::Railgun, 8,
	(s16)udtMeanOfDeath::Lightning, 9,
	(s16)udtMeanOfDeath::BFG, 10,
	(s16)udtMeanOfDeath::BFGSplash, 11,
	(s16)udtMeanOfDeath::Water, 50,
	(s16)udtMeanOfDeath::Slime, 51,
	(s16)udtMeanOfDeath::Lava, 52,
	(s16)udtMeanOfDeath::Crush, 53,
	(s16)udtMeanOfDeath::TeleFrag, 54,
	(s16)udtMeanOfDeath::Fall, 55,
	(s16)udtMeanOfDeath::Suicide, 56, // self kill is 75, what's the difference?
	(s16)udtMeanOfDeath::TargetLaser, 57,
	(s16)udtMeanOfDeath::TriggerHurt, 58,
	(s16)udtMeanOfDeath::Grapple, 59,
	(s16)udtMeanOfDeath::TeamSwitch, 77,
	(s16)udtMeanOfDeath::Knife, 12,
	(s16)udtMeanOfDeath::Knife2, 13,
	(s16)udtMeanOfDeath::KnifeStealth, 14,
	(s16)udtMeanOfDeath::Luger, 15,
	(s16)udtMeanOfDeath::Colt, 16,
	(s16)udtMeanOfDeath::MP40, 17,
	(s16)udtMeanOfDeath::Thompson, 18,
	(s16)udtMeanOfDeath::Sten, 19,
	(s16)udtMeanOfDeath::Mauser, 20,
	(s16)udtMeanOfDeath::SniperRifle, 21,
	(s16)udtMeanOfDeath::Garand, 22,
	(s16)udtMeanOfDeath::SnooperScope, 23,
	(s16)udtMeanOfDeath::Akimbo, 25,
	(s16)udtMeanOfDeath::Panzerfaust, 6, // replaces RL
	(s16)udtMeanOfDeath::PanzerfaustSplash, 7, // replaces RL
	(s16)udtMeanOfDeath::GrenadeLauncher, 31,
	(s16)udtMeanOfDeath::GrenadePineapple, 38,
	(s16)udtMeanOfDeath::Venom, 32,
	(s16)udtMeanOfDeath::VenomFull, 33,
	(s16)udtMeanOfDeath::Flamethrower, 34,
	(s16)udtMeanOfDeath::Kicked, 42,
	(s16)udtMeanOfDeath::Mortar, 40,
	(s16)udtMeanOfDeath::MortarSplash, 41,
	(s16)udtMeanOfDeath::Grabber, 43,
	(s16)udtMeanOfDeath::Dynamite, 44,
	(s16)udtMeanOfDeath::DynamiteSplash, 45,
	(s16)udtMeanOfDeath::Silencer, 24,
	(s16)udtMeanOfDeath::Bar, 26,
	(s16)udtMeanOfDeath::FG42, 27,
	(s16)udtMeanOfDeath::FG42Scope, 28,
	(s16)udtMeanOfDeath::Airstrike, 46,
	(s16)udtMeanOfDeath::Artillery, 76,
	(s16)udtMeanOfDeath::Explosive, 60,
	(s16)udtMeanOfDeath::Syringe, 47,
	(s16)udtMeanOfDeath::PoisonGas, 61,
	TABLE_END
};
VALIDATE_TABLE_SIZES(MeansOfDeath_57_60, udtMeanOfDeath::Count);

static const s16* MeanOfDeathTables[] =
{
	TABLE_ENTRY(MeansOfDeath_3_68), // 3
	TABLE_ENTRY(MeansOfDeath_3_68), // 48
	TABLE_ENTRY(MeansOfDeath_57_60), // 57
	TABLE_ENTRY(MeansOfDeath_57_60), // 58
	TABLE_ENTRY(MeansOfDeath_57_60), // 59
	TABLE_ENTRY(MeansOfDeath_57_60), // 60
	TABLE_ENTRY(MeansOfDeath_3_68), // 66
	TABLE_ENTRY(MeansOfDeath_3_68), // 67
	TABLE_ENTRY(MeansOfDeath_3_68), // 68
	TABLE_ENTRY(MeansOfDeath_73p), // 73
	TABLE_ENTRY(MeansOfDeath_73p), // 90
	TABLE_ENTRY(MeansOfDeath_73p) // 91
};
VALIDATE_TABLE_SIZE(MeanOfDeathTables);

static s16 Items_3_68_U2Q[udtItem::Count];
static s16 Items_3_68_Q2U[udtItem::Count * 2];
static const s16 Items_3_68[] =
{
	(s16)udtItem::AmmoBFG, 25,
	(s16)udtItem::AmmoBullets, 19,
	(s16)udtItem::AmmoCells, 21,
	(s16)udtItem::AmmoGrenades, 20,
	(s16)udtItem::AmmoLightning, 22,
	(s16)udtItem::AmmoRockets, 23,
	(s16)udtItem::AmmoShells, 18,
	(s16)udtItem::AmmoSlugs, 24,
	(s16)udtItem::HoldableMedkit, 27,
	(s16)udtItem::HoldableTeleporter, 26,
	(s16)udtItem::ItemArmorBody, 3,
	(s16)udtItem::ItemArmorCombat, 2,
	(s16)udtItem::ItemArmorShard, 1,
	(s16)udtItem::ItemEnviro, 29,
	(s16)udtItem::ItemFlight, 33,
	(s16)udtItem::ItemHaste, 30,
	(s16)udtItem::ItemHealth, 5,
	(s16)udtItem::ItemHealthLarge, 6,
	(s16)udtItem::ItemHealthMega, 7,
	(s16)udtItem::ItemHealthSmall, 4,
	(s16)udtItem::ItemInvis, 31,
	(s16)udtItem::ItemQuad, 28,
	(s16)udtItem::ItemRegen, 32,
	(s16)udtItem::FlagBlue, 35,
	(s16)udtItem::FlagRed, 34,
	(s16)udtItem::WeaponBFG, 16,
	(s16)udtItem::WeaponGauntlet, 8,
	(s16)udtItem::WeaponGrapplingHook, 17,
	(s16)udtItem::WeaponGrenadeLauncher, 11,
	(s16)udtItem::WeaponLightningGun, 13,
	(s16)udtItem::WeaponMachinegun, 10,
	(s16)udtItem::WeaponPlasmaGun, 15,
	(s16)udtItem::WeaponRailgun, 14,
	(s16)udtItem::WeaponRocketLauncher, 12,
	(s16)udtItem::WeaponShotgun, 9,
	TABLE_END
};
VALIDATE_TABLE_SIZES(Items_3_68, udtItem::Count);

static s16 Items_73_U2Q[udtItem::Count];
static s16 Items_73_Q2U[udtItem::Count * 2];
static const s16 Items_73[] =
{
	(s16)udtItem::AmmoBFG, 26,
	(s16)udtItem::AmmoBelt, 42,
	(s16)udtItem::AmmoBullets, 20,
	(s16)udtItem::AmmoCells, 22,
	(s16)udtItem::AmmoGrenades, 21,
	(s16)udtItem::AmmoLightning, 23,
	(s16)udtItem::AmmoMines, 41,
	(s16)udtItem::AmmoNails, 40,
	(s16)udtItem::AmmoRockets, 24,
	(s16)udtItem::AmmoShells, 19,
	(s16)udtItem::AmmoSlugs, 25,
	(s16)udtItem::HoldableInvulnerability, 39,
	(s16)udtItem::HoldableKamikaze, 37,
	(s16)udtItem::HoldableMedkit, 28,
	(s16)udtItem::HoldablePortal, 38,
	(s16)udtItem::HoldableTeleporter, 27,
	(s16)udtItem::ItemAmmoRegen, 46,
	(s16)udtItem::ItemArmorBody, 3,
	(s16)udtItem::ItemArmorCombat, 2,
	(s16)udtItem::ItemArmorJacket, 4,
	(s16)udtItem::ItemArmorShard, 1,
	(s16)udtItem::ItemBlueCube, 49,
	(s16)udtItem::ItemDoubler, 45,
	(s16)udtItem::ItemEnviro, 30,
	(s16)udtItem::ItemFlight, 34,
	(s16)udtItem::ItemGuard, 44,
	(s16)udtItem::ItemHaste, 31,
	(s16)udtItem::ItemHealth, 6,
	(s16)udtItem::ItemHealthLarge, 7,
	(s16)udtItem::ItemHealthMega, 8,
	(s16)udtItem::ItemHealthSmall, 5,
	(s16)udtItem::ItemInvis, 32,
	(s16)udtItem::ItemQuad, 29,
	(s16)udtItem::ItemRedCube, 48,
	(s16)udtItem::ItemRegen, 33,
	(s16)udtItem::ItemScout, 43,
	(s16)udtItem::FlagBlue, 36,
	(s16)udtItem::FlagNeutral, 47,
	(s16)udtItem::FlagRed, 35,
	(s16)udtItem::WeaponBFG, 17,
	(s16)udtItem::WeaponChaingun, 52,
	(s16)udtItem::WeaponGauntlet, 9,
	(s16)udtItem::WeaponGrapplingHook, 18,
	(s16)udtItem::WeaponGrenadeLauncher, 12,
	(s16)udtItem::WeaponLightningGun, 14,
	(s16)udtItem::WeaponMachinegun, 11,
	(s16)udtItem::WeaponNailgun, 50,
	(s16)udtItem::WeaponPlasmaGun, 16,
	(s16)udtItem::WeaponProxLauncher, 51,
	(s16)udtItem::WeaponRailgun, 15,
	(s16)udtItem::WeaponRocketLauncher, 13,
	(s16)udtItem::WeaponShotgun, 10,
	TABLE_END
};
VALIDATE_TABLE_SIZES(Items_73, udtItem::Count);

static s16 Items_90p_U2Q[udtItem::Count];
static s16 Items_90p_Q2U[udtItem::Count * 2];
static const s16 Items_90p[] =
{
	(s16)udtItem::AmmoBFG, 26,
	(s16)udtItem::AmmoBelt, 42,
	(s16)udtItem::AmmoBullets, 20,
	(s16)udtItem::AmmoCells, 22,
	(s16)udtItem::AmmoGrenades, 21,
	(s16)udtItem::AmmoHMG, 55,
	(s16)udtItem::AmmoLightning, 23,
	(s16)udtItem::AmmoMines, 41,
	(s16)udtItem::AmmoNails, 40,
	(s16)udtItem::AmmoPack, 56,
	(s16)udtItem::AmmoRockets, 24,
	(s16)udtItem::AmmoShells, 19,
	(s16)udtItem::AmmoSlugs, 25,
	(s16)udtItem::HoldableInvulnerability, 39,
	(s16)udtItem::HoldableKamikaze, 37,
	(s16)udtItem::HoldableMedkit, 28,
	(s16)udtItem::HoldablePortal, 38,
	(s16)udtItem::HoldableTeleporter, 27,
	(s16)udtItem::ItemAmmoRegen, 46,
	(s16)udtItem::ItemArmorBody, 3,
	(s16)udtItem::ItemArmorCombat, 2,
	(s16)udtItem::ItemArmorJacket, 4,
	(s16)udtItem::ItemArmorShard, 1,
	(s16)udtItem::ItemBlueCube, 49,
	(s16)udtItem::ItemDoubler, 45,
	(s16)udtItem::ItemEnviro, 30,
	(s16)udtItem::ItemFlight, 34,
	(s16)udtItem::ItemGuard, 44,
	(s16)udtItem::ItemHaste, 31,
	(s16)udtItem::ItemHealth, 6,
	(s16)udtItem::ItemHealthLarge, 7,
	(s16)udtItem::ItemHealthMega, 8,
	(s16)udtItem::ItemHealthSmall, 5,
	(s16)udtItem::ItemInvis, 32,
	(s16)udtItem::ItemKeyGold, 58,
	(s16)udtItem::ItemKeyMaster, 59,
	(s16)udtItem::ItemKeySilver, 57,
	(s16)udtItem::ItemQuad, 29,
	(s16)udtItem::ItemRedCube, 48,
	(s16)udtItem::ItemRegen, 33,
	(s16)udtItem::ItemScout, 43,
	(s16)udtItem::ItemSpawnArmor, 53,
	(s16)udtItem::FlagBlue, 36,
	(s16)udtItem::FlagNeutral, 47,
	(s16)udtItem::FlagRed, 35,
	(s16)udtItem::WeaponBFG, 17,
	(s16)udtItem::WeaponChaingun, 52,
	(s16)udtItem::WeaponGauntlet, 9,
	(s16)udtItem::WeaponGrapplingHook, 18,
	(s16)udtItem::WeaponGrenadeLauncher, 12,
	(s16)udtItem::WeaponHMG, 54,
	(s16)udtItem::WeaponLightningGun, 14,
	(s16)udtItem::WeaponMachinegun, 11,
	(s16)udtItem::WeaponNailgun, 50,
	(s16)udtItem::WeaponPlasmaGun, 16,
	(s16)udtItem::WeaponProxLauncher, 51,
	(s16)udtItem::WeaponRailgun, 15,
	(s16)udtItem::WeaponRocketLauncher, 13,
	(s16)udtItem::WeaponShotgun, 10,
	TABLE_END
};
VALIDATE_TABLE_SIZES(Items_90p, udtItem::Count);

static s16 Items_57_60_U2Q[udtItem::Count];
static s16 Items_57_60_Q2U[udtItem::Count * 2];
static const s16 Items_57_60[] =
{
	(s16)udtItem::AmmoCells, 53,
	(s16)udtItem::AmmoGrenades, 50,
	(s16)udtItem::AmmoRockets, 58,
	(s16)udtItem::HoldableMedkit, 61,
	(s16)udtItem::ItemArmorBody, 10,
	(s16)udtItem::ItemHealth, 4,
	(s16)udtItem::ItemHealthLarge, 5,
	(s16)udtItem::ItemHealthSmall, 3,
	(s16)udtItem::FlagBlue, 76,
	(s16)udtItem::FlagRed, 75,
	(s16)udtItem::WeaponGrenadeLauncher, 21,
	TABLE_END
};
VALIDATE_TABLE_SIZES(Items_57_60, udtItem::Count);

static const s16* ItemTables[] =
{
	TABLE_ENTRY(Items_3_68), // 3
	TABLE_ENTRY(Items_3_68), // 48
	TABLE_ENTRY(Items_57_60), // 57
	TABLE_ENTRY(Items_57_60), // 58
	TABLE_ENTRY(Items_57_60), // 59
	TABLE_ENTRY(Items_57_60), // 60
	TABLE_ENTRY(Items_3_68), // 66
	TABLE_ENTRY(Items_3_68), // 67
	TABLE_ENTRY(Items_3_68), // 68
	TABLE_ENTRY(Items_73), // 73
	TABLE_ENTRY(Items_90p), // 90
	TABLE_ENTRY(Items_90p) // 91
};
VALIDATE_TABLE_SIZE(ItemTables);

static s16 PMTypes_U2Q[udtPlayerMovementType::Count];
static s16 PMTypes_Q2U[udtPlayerMovementType::Count * 2];
static const s16 PMTypes[] =
{
	(s16)udtPlayerMovementType::Normal, 0,
	(s16)udtPlayerMovementType::NoClip, 1,
	(s16)udtPlayerMovementType::Spectator, 2,
	(s16)udtPlayerMovementType::Dead, 3,
	(s16)udtPlayerMovementType::Freeze, 4,
	(s16)udtPlayerMovementType::Intermission, 5,
	(s16)udtPlayerMovementType::SPIntermission, 6,
	TABLE_END
};
VALIDATE_TABLE_SIZES(PMTypes, udtPlayerMovementType::Count);

static const s16* PMTypeTables[] =
{
	TABLE_ENTRY(PMTypes), // 3
	TABLE_ENTRY(PMTypes), // 48
	TABLE_ENTRY(PMTypes), // 57 - same in RtCW as in Q3
	TABLE_ENTRY(PMTypes), // 58 - same in RtCW as in Q3
	TABLE_ENTRY(PMTypes), // 59 - same in RtCW as in Q3
	TABLE_ENTRY(PMTypes), // 60 - same in RtCW as in Q3
	TABLE_ENTRY(PMTypes), // 66
	TABLE_ENTRY(PMTypes), // 67
	TABLE_ENTRY(PMTypes), // 68
	TABLE_ENTRY(PMTypes), // 73
	TABLE_ENTRY(PMTypes), // 90
	TABLE_ENTRY(PMTypes) // 91
};
VALIDATE_TABLE_SIZE(PMTypeTables);

struct MagicNumberTableGroup
{
	MagicNumberTableGroup(udtMagicNumberType::Id type, const s16** tables, u32 count)
		: Tables(tables)
		, Count(count)
		, Type(type)
	{
	}

	const s16** Tables;
	u32 Count;
	udtMagicNumberType::Id Type;
};

static const MagicNumberTableGroup MagicNumberTables[udtMagicNumberType::Count] =
{
	MagicNumberTableGroup(udtMagicNumberType::PowerUpIndex, PowerUpTables, udtPowerUpIndex::Count),
	MagicNumberTableGroup(udtMagicNumberType::LifeStatsIndex, LifeStatsTables, udtLifeStatsIndex::Count),
	MagicNumberTableGroup(udtMagicNumberType::PersStatsIndex, PersStatsTables, udtPersStatsIndex::Count),
	MagicNumberTableGroup(udtMagicNumberType::EntityType, EntityTypeTables, udtEntityType::Count),
	MagicNumberTableGroup(udtMagicNumberType::EntityFlag, EntityFlagBitTables, udtEntityFlag::Count),
	MagicNumberTableGroup(udtMagicNumberType::EntityEvent, EntityEventTables, udtEntityEvent::Count),
	MagicNumberTableGroup(udtMagicNumberType::ConfigStringIndex, ConfigStringIndexTables, udtConfigStringIndex::Count),
	MagicNumberTableGroup(udtMagicNumberType::Team, TeamTables, udtTeam::Count),
	MagicNumberTableGroup(udtMagicNumberType::GameType, GameTypeTables, udtGameType::Count),
	MagicNumberTableGroup(udtMagicNumberType::FlagStatus, FlagStatusTables, udtFlagStatus::Count),
	MagicNumberTableGroup(udtMagicNumberType::Weapon, WeaponTables, udtWeapon::Count),
	MagicNumberTableGroup(udtMagicNumberType::MeanOfDeath, MeanOfDeathTables, udtMeanOfDeath::Count),
	MagicNumberTableGroup(udtMagicNumberType::Item, ItemTables, udtItem::Count),
	MagicNumberTableGroup(udtMagicNumberType::PlayerMovementType, PMTypeTables, udtPlayerMovementType::Count)
};


#define UNDEFINED UDT_S16_MIN


static int SortCallback(const void* a, const void* b)
{
	return *((const s16*)a + 1) - *((const s16*)b + 1);
}

void BuildLookUpTables()
{
	for(u32 mnt = 0; mnt < (u32)udtMagicNumberType::Count; ++mnt)
	{
		const MagicNumberTableGroup& tableGroup = MagicNumberTables[mnt];
		s16* prevTable_U2Q = NULL;
		s16* prevTable_Q2U = NULL;
		for(u32 p = 0; p < (u32)udtProtocol::Count; ++p)
		{
			const s16* const table = tableGroup.Tables[3 * p + 0];
			s16* const table_U2Q = (s16*)tableGroup.Tables[3 * p + 1];
			s16* const table_Q2U = (s16*)tableGroup.Tables[3 * p + 2];
			if(table_U2Q != prevTable_U2Q)
			{
				for(u32 i = 0; i < tableGroup.Count; ++i)
				{
					table_U2Q[i] = UNDEFINED;
				}
				for(u32 i = 0; i < tableGroup.Count; ++i)
				{
					const s16 newIdx = table[2 * i + 0];
					if(newIdx < 0)
					{
						break;
					}
					table_U2Q[newIdx] = table[2 * i + 1];
				}
			}
			if(table_Q2U != prevTable_Q2U)
			{
				for(u32 i = 0; i < tableGroup.Count * 2; ++i)
				{
					table_Q2U[i] = UNDEFINED;
				}
				for(u32 i = 0; i < tableGroup.Count; ++i)
				{
					const s16 newIdx = table[2 * i + 0];
					if(newIdx < 0)
					{
						break;
					}
					table_Q2U[2 * i + 0] = newIdx;
					table_Q2U[2 * i + 1] = table[2 * i + 1];
				}
				qsort(table_Q2U, (size_t)tableGroup.Count, 2 * sizeof(s16), &SortCallback);
			}
			prevTable_U2Q = table_U2Q;
			prevTable_Q2U = table_Q2U;
		}
	}
}

#define ID_ENTITY_EVENT_60_RTCW_PRO_LIST(N) \
	N(Obituary, 86) \
	N(WeaponFired, 40) \
	N(ItemPickup, 30) \
	N(GlobalItemPickup, 32) \
	N(GlobalSound, 68) \
	N(ItemRespawn, 62) \
	N(ItemPop, 63) \
	N(PlayerTeleportIn, 64) \
	N(PlayerTeleportOut, 65) \
	N(BulletHitFlesh, 71) \
	N(BulletHitWall, 72) \
	N(MissileHit, 73) \
	N(MissileMiss, 74) \
	N(RailTrail, 75) \
	N(PowerUpQuad, 88) \
	N(PowerUpBattleSuit, 89) \
	N(PowerUpRegen, 90)

static s32 GetUDTEntityEventRtcwPro(s32 et)
{
#define ITEM(UDTEnum, IdValue) case IdValue: return (s32)udtEntityEvent::UDTEnum;
	switch(et)
	{
		ID_ENTITY_EVENT_60_RTCW_PRO_LIST(ITEM)
		default: return UDT_S32_MIN;
	}
#undef ITEM
}

static s32 GetIdEntityEventRtcwPro(s32 et)
{
#define ITEM(UDTEnum, IdValue) case udtEntityEvent::UDTEnum: return IdValue;
	switch((udtEntityEvent::Id)et)
	{
		ID_ENTITY_EVENT_60_RTCW_PRO_LIST(ITEM)
		default: return UDT_S32_MIN;
	}
#undef ITEM
}

#undef ID_ENTITY_EVENT_60_RTCW_PRO_LIST

struct idGameType68_CPMA
{
	enum Id
	{
		HM = -1,
		FFA = 0,
		Duel = 1,
		SP = 2,
		TDM = 3,
		CTF = 4,
		CA = 5,
		FT = 6,
		CTFS = 7,
		NTF = 8,
		TwoVsTwo = 9
	};
};

static s32 GetUDTGameTypeCPMA(s32 gt)
{
	switch((idGameType68_CPMA::Id)gt)
	{
		case idGameType68_CPMA::HM: return (s32)udtGameType::HM;
		case idGameType68_CPMA::FFA: return (s32)udtGameType::FFA;
		case idGameType68_CPMA::Duel: return (s32)udtGameType::Duel;
		case idGameType68_CPMA::SP: return (s32)udtGameType::SP;
		case idGameType68_CPMA::TDM: return (s32)udtGameType::TDM;
		case idGameType68_CPMA::CTF: return (s32)udtGameType::CTF;
		case idGameType68_CPMA::CA: return (s32)udtGameType::CA;
		case idGameType68_CPMA::FT: return (s32)udtGameType::FT;
		case idGameType68_CPMA::CTFS: return (s32)udtGameType::CTFS;
		case idGameType68_CPMA::NTF: return (s32)udtGameType::NTF;
		case idGameType68_CPMA::TwoVsTwo: return (s32)udtGameType::TwoVsTwo;
		default: return UDT_S32_MIN;
	}
}

static s32 GetIdGameTypeCPMA(s32 gt)
{
	switch((udtGameType::Id)gt)
	{
		case udtGameType::HM: return (s32)idGameType68_CPMA::HM;
		case udtGameType::FFA: return (s32)idGameType68_CPMA::FFA;
		case udtGameType::Duel: return (s32)idGameType68_CPMA::Duel;
		case udtGameType::SP: return (s32)idGameType68_CPMA::SP;
		case udtGameType::TDM: return (s32)idGameType68_CPMA::TDM;
		case udtGameType::CTF: return (s32)idGameType68_CPMA::CTF;
		case udtGameType::CA: return (s32)idGameType68_CPMA::CA;
		case udtGameType::FT: return (s32)idGameType68_CPMA::FT;
		case udtGameType::CTFS: return (s32)idGameType68_CPMA::CTFS;
		case udtGameType::NTF: return (s32)idGameType68_CPMA::NTF;
		case udtGameType::TwoVsTwo: return (s32)idGameType68_CPMA::TwoVsTwo;
		default: return UDT_S32_MIN;
	}
}

struct idItem68_CPMA
{
	enum Id
	{
		ItemArmorJacket = 36,
		ItemBackpack,
		TeamCTFNeutralflag
	};
};

static s32 GetIdExtraItemCPMA(s32 item)
{
	switch((udtItem::Id)item)
	{
		case udtItem::ItemArmorJacket: return (s32)idItem68_CPMA::ItemArmorJacket;
		case udtItem::ItemBackpack: return (s32)idItem68_CPMA::ItemBackpack;
		case udtItem::FlagNeutral: return (s32)idItem68_CPMA::TeamCTFNeutralflag;
		default: return UDT_S32_MIN;
	}
}

static s32 GetUDTExtraItemCPMA(s32 item)
{
	switch((idItem68_CPMA::Id)item)
	{
		case idItem68_CPMA::ItemArmorJacket: return (s32)udtItem::ItemArmorJacket;
		case idItem68_CPMA::ItemBackpack: return (s32)udtItem::ItemBackpack;
		case idItem68_CPMA::TeamCTFNeutralflag: return (s32)udtItem::FlagNeutral;
		default: return UDT_S32_MIN;
	}
}

bool GetIdNumber(s32& idNumber, udtMagicNumberType::Id numberType, u32 udtNumber, udtProtocol::Id protocol, udtMod::Id mod)
{
	if((u32)numberType >= (u32)udtMagicNumberType::Count ||
	   (u32)protocol >= (u32)udtProtocol::Count)
	{
		return false;
	}

	if(numberType == udtMagicNumberType::EntityEvent &&
	   AreAllProtocolFlagsSet(protocol, udtProtocolFlags::RTCW) &&
	   mod == udtMod::RTCWPro)
	{
		const s32 result = GetIdEntityEventRtcwPro((s32)udtNumber);
		const bool success = result != UDT_S32_MIN;
		if(success) idNumber = result;
		return success;
	}

	if(numberType == udtMagicNumberType::GameType &&
	   AreAllProtocolFlagsSet(protocol, udtProtocolFlags::Quake3) &&
	   mod == udtMod::CPMA)
	{
		const s32 result = GetIdGameTypeCPMA((s32)udtNumber);
		const bool success = result != UDT_S32_MIN;
		if(success) idNumber = result;
		return success;
	}

	if(numberType == udtMagicNumberType::Item &&
	   AreAllProtocolFlagsSet(protocol, udtProtocolFlags::Quake3) &&
	   mod == udtMod::CPMA)
	{
		const s32 result = GetIdExtraItemCPMA((s32)udtNumber);
		if(result != UDT_S32_MIN)
		{
			idNumber = result;
			return true;
		}
	}

	const MagicNumberTableGroup* tableGroup = NULL;
	for(u32 i = 0; i < (u32)udtMagicNumberType::Count; ++i)
	{
		if(MagicNumberTables[i].Type == numberType)
		{
			tableGroup = &MagicNumberTables[i];
			break;
		}
	}

	if(tableGroup == NULL || udtNumber >= tableGroup->Count)
	{
		return false;
	}

	const s16 result = tableGroup->Tables[(u32)protocol * 3 + 1][udtNumber];
	if(result == UNDEFINED)
	{
		return false;
	}

	idNumber = (s32)result;

	return true;
}

static int BinarySearchCallback(const void* a, const void* b)
{
	return *(const s16*)a - *(const s16*)b;
}

bool GetUDTNumber(u32& udtNumber, udtMagicNumberType::Id numberType, s32 idNumber, udtProtocol::Id protocol, udtMod::Id mod)
{
	if((u32)numberType >= (u32)udtMagicNumberType::Count ||
	   (u32)protocol >= (u32)udtProtocol::Count)
	{
		return false;
	}

	if(numberType == udtMagicNumberType::EntityEvent &&
	   AreAllProtocolFlagsSet(protocol, udtProtocolFlags::RTCW) &&
	   mod == udtMod::RTCWPro)
	{
		const s32 result = GetUDTEntityEventRtcwPro(idNumber);
		const bool success = result != UDT_S32_MIN;
		if(success) udtNumber = (u32)result;
		return success;
	}

	if(numberType == udtMagicNumberType::GameType && 
	   AreAllProtocolFlagsSet(protocol, udtProtocolFlags::Quake3) &&
	   mod == udtMod::CPMA)
	{
		const s32 result = GetUDTGameTypeCPMA(idNumber);
		const bool success = result != UDT_S32_MIN;
		if(success) udtNumber = (u32)result;
		return success;
	}

	if(numberType == udtMagicNumberType::Item && 
	   AreAllProtocolFlagsSet(protocol, udtProtocolFlags::Quake3) &&
	   mod == udtMod::CPMA)
	{
		const s32 result = GetUDTExtraItemCPMA(idNumber);
		if(result != UDT_S32_MIN)
		{
			udtNumber = (u32)result;
			return true;
		}
	}

	const MagicNumberTableGroup* tableGroup = NULL;
	for(u32 i = 0; i < (u32)udtMagicNumberType::Count; ++i)
	{
		if(MagicNumberTables[i].Type == numberType)
		{
			tableGroup = &MagicNumberTables[i];
			break;
		}
	}

	if(tableGroup == NULL)
	{
		return false;
	}

	const s16* const table = tableGroup->Tables[(u32)protocol * 3 + 2];
	const void* const idNumberPtr = bsearch(&idNumber, table + 1, (size_t)tableGroup->Count, sizeof(s16) * 2, &BinarySearchCallback);
	if(idNumberPtr == NULL)
	{
		return false;
	}

	udtNumber = (u32)*((const s16*)idNumberPtr - 1);

	return true;
}

s32 GetIdNumber(udtMagicNumberType::Id numberType, u32 udtNumber, udtProtocol::Id protocol, udtMod::Id mod)
{
	s32 idNumber = UDT_S32_MIN;
	GetIdNumber(idNumber, numberType, udtNumber, protocol, mod);

	return idNumber;
}

s32 GetIdEntityStateFlagMask(udtEntityFlag::Id udtFlagId, udtProtocol::Id protocol)
{
	const s32 flagBit = GetIdNumber(udtMagicNumberType::EntityFlag, (u32)udtFlagId, protocol);
	if(flagBit == UDT_S32_MIN)
	{
		return 0;
	}

	return 1 << flagBit;
}
