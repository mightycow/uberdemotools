#include "look_up_tables.hpp"
#include "timer.hpp"

#include <stdlib.h>
#include <string.h>


#define UNDEFINED UDT_S16_MIN


static s16 PowerUps_3_90_U2Q[udtPowerUpIndex::Count];
static s16 PowerUps_3_90_Q2U[udtPowerUpIndex::Count * 2];
static const s16 PowerUps_3_90[udtPowerUpIndex::Count * 2] =
{
	(s16)udtPowerUpIndex::QuadDamage, 1,
	(s16)udtPowerUpIndex::BattleSuit, 2,
	(s16)udtPowerUpIndex::Haste, 3,
	(s16)udtPowerUpIndex::Invisibility, 4,
	(s16)udtPowerUpIndex::Regeneration, 5,
	(s16)udtPowerUpIndex::Flight, 6,
	(s16)udtPowerUpIndex::RedFlag, 7,
	(s16)udtPowerUpIndex::BlueFlag, 8,
	(s16)udtPowerUpIndex::NeutralFlag, UNDEFINED,
	(s16)udtPowerUpIndex::Scout, UNDEFINED,
	(s16)udtPowerUpIndex::Guard, UNDEFINED,
	(s16)udtPowerUpIndex::Doubler, UNDEFINED,
	(s16)udtPowerUpIndex::ArmorRegeneration, UNDEFINED,
	(s16)udtPowerUpIndex::Invulnerability, UNDEFINED
};

static s16 PowerUps_91_U2Q[udtPowerUpIndex::Count];
static s16 PowerUps_91_Q2U[udtPowerUpIndex::Count * 2];
static const s16 PowerUps_91[udtPowerUpIndex::Count * 2] =
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
	(s16)udtPowerUpIndex::Invulnerability, 11
};

static const s16* PowerUpTables[udtProtocol::Count * 3] =
{
	PowerUps_3_90, PowerUps_3_90_U2Q, PowerUps_3_90_Q2U,
	PowerUps_3_90, PowerUps_3_90_U2Q, PowerUps_3_90_Q2U,
	PowerUps_3_90, PowerUps_3_90_U2Q, PowerUps_3_90_Q2U,
	PowerUps_3_90, PowerUps_3_90_U2Q, PowerUps_3_90_Q2U,
	PowerUps_3_90, PowerUps_3_90_U2Q, PowerUps_3_90_Q2U,
	PowerUps_3_90, PowerUps_3_90_U2Q, PowerUps_3_90_Q2U,
	PowerUps_3_90, PowerUps_3_90_U2Q, PowerUps_3_90_Q2U,
	PowerUps_91, PowerUps_91_U2Q, PowerUps_91_Q2U
};

static s16 LifeStats_3_68_U2Q[udtLifeStatsIndex::Count];
static s16 LifeStats_3_68_Q2U[udtLifeStatsIndex::Count * 2];
static const s16 LifeStats_3_68[udtLifeStatsIndex::Count * 2] =
{
	(s16)udtLifeStatsIndex::Health, 0,
	(s16)udtLifeStatsIndex::HoldableItem, 1,
	(s16)udtLifeStatsIndex::Weapons, 2,
	(s16)udtLifeStatsIndex::Armor, 3,
	(s16)udtLifeStatsIndex::MaxHealth, 6
};

static s16 LifeStats_73p_U2Q[udtLifeStatsIndex::Count];
static s16 LifeStats_73p_Q2U[udtLifeStatsIndex::Count * 2];
static const s16 LifeStats_73p[udtLifeStatsIndex::Count * 2] =
{
	(s16)udtLifeStatsIndex::Health, 0,
	(s16)udtLifeStatsIndex::HoldableItem, 1,
	(s16)udtLifeStatsIndex::Weapons, 3,
	(s16)udtLifeStatsIndex::Armor, 4,
	(s16)udtLifeStatsIndex::MaxHealth, 7
};

static const s16* LifeStatsTables[udtProtocol::Count * 3] =
{
	LifeStats_3_68, LifeStats_3_68_U2Q, LifeStats_3_68_Q2U,
	LifeStats_3_68, LifeStats_3_68_U2Q, LifeStats_3_68_Q2U,
	LifeStats_3_68, LifeStats_3_68_U2Q, LifeStats_3_68_Q2U,
	LifeStats_3_68, LifeStats_3_68_U2Q, LifeStats_3_68_Q2U,
	LifeStats_3_68, LifeStats_3_68_U2Q, LifeStats_3_68_Q2U,
	LifeStats_73p, LifeStats_73p_U2Q, LifeStats_73p_Q2U,
	LifeStats_73p, LifeStats_73p_U2Q, LifeStats_73p_Q2U,
	LifeStats_73p, LifeStats_73p_U2Q, LifeStats_73p_Q2U
};

static s16 PersStats_3_U2Q[udtPersStatsIndex::Count];
static s16 PersStats_3_Q2U[udtPersStatsIndex::Count * 2];
static const s16 PersStats_3[udtPersStatsIndex::Count * 2] =
{
	(s16)udtPersStatsIndex::FlagCaptures, UNDEFINED,
	(s16)udtPersStatsIndex::Score, 0,
	(s16)udtPersStatsIndex::DamageGiven, 1,
	(s16)udtPersStatsIndex::Rank, 2,
	(s16)udtPersStatsIndex::Team, 3,
	(s16)udtPersStatsIndex::SpawnCount, 4,
	(s16)udtPersStatsIndex::LastAttacker, 7,
	(s16)udtPersStatsIndex::LastTargetHealthAndArmor, UNDEFINED,
	(s16)udtPersStatsIndex::Deaths, 8,
	(s16)udtPersStatsIndex::Impressives, 9,
	(s16)udtPersStatsIndex::Excellents, 10,
	(s16)udtPersStatsIndex::Defends, UNDEFINED,
	(s16)udtPersStatsIndex::Assists, UNDEFINED,
	(s16)udtPersStatsIndex::Humiliations, 11
};

static s16 PersStats_48_68_U2Q[udtPersStatsIndex::Count];
static s16 PersStats_48_68_Q2U[udtPersStatsIndex::Count * 2];
static const s16 PersStats_48_68[udtPersStatsIndex::Count * 2] =
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
	(s16)udtPersStatsIndex::Humiliations, 13
};

static s16 PersStats_73p_U2Q[udtPersStatsIndex::Count];
static s16 PersStats_73p_Q2U[udtPersStatsIndex::Count * 2];
static const s16 PersStats_73p[udtPersStatsIndex::Count * 2] =
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
	(s16)udtPersStatsIndex::Humiliations, 12
};

static const s16* PersStatsTables[udtProtocol::Count * 3] =
{
	PersStats_3, PersStats_3_U2Q, PersStats_3_Q2U,
	PersStats_48_68, PersStats_48_68_U2Q, PersStats_48_68_Q2U,
	PersStats_48_68, PersStats_48_68_U2Q, PersStats_48_68_Q2U,
	PersStats_48_68, PersStats_48_68_U2Q, PersStats_48_68_Q2U,
	PersStats_48_68, PersStats_48_68_U2Q, PersStats_48_68_Q2U,
	PersStats_73p, PersStats_73p_U2Q, PersStats_73p_Q2U,
	PersStats_73p, PersStats_73p_U2Q, PersStats_73p_Q2U,
	PersStats_73p, PersStats_73p_U2Q, PersStats_73p_Q2U
};

static s16 EntityTypes_3_U2Q[udtEntityType::Count];
static s16 EntityTypes_3_Q2U[udtEntityType::Count * 2];
static const s16 EntityTypes_3[udtEntityType::Count * 2] =
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
	(s16)udtEntityType::Team, UNDEFINED
};

static s16 EntityTypes_48p_U2Q[udtEntityType::Count];
static s16 EntityTypes_48p_Q2U[udtEntityType::Count * 2];
static const s16 EntityTypes_48p[udtEntityType::Count * 2] =
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
	(s16)udtEntityType::Team, 12
};

static const s16* EntityTypeTables[udtProtocol::Count * 3] =
{
	EntityTypes_3, EntityTypes_3_U2Q, EntityTypes_3_Q2U,
	EntityTypes_48p, EntityTypes_48p_U2Q, EntityTypes_48p_Q2U,
	EntityTypes_48p, EntityTypes_48p_U2Q, EntityTypes_48p_Q2U,
	EntityTypes_48p, EntityTypes_48p_U2Q, EntityTypes_48p_Q2U,
	EntityTypes_48p, EntityTypes_48p_U2Q, EntityTypes_48p_Q2U,
	EntityTypes_48p, EntityTypes_48p_U2Q, EntityTypes_48p_Q2U,
	EntityTypes_48p, EntityTypes_48p_U2Q, EntityTypes_48p_Q2U,
	EntityTypes_48p, EntityTypes_48p_U2Q, EntityTypes_48p_Q2U
};

static s16 EntityFlagBits_3_U2Q[udtEntityFlag::Count];
static s16 EntityFlagBits_3_Q2U[udtEntityFlag::Count * 2];
static const s16 EntityFlagBits_3[udtEntityFlag::Count * 2] =
{
	(s16)udtEntityFlag::Dead, 0,
	(s16)udtEntityFlag::TeleportBit, 2,
	(s16)udtEntityFlag::AwardExcellent, 3,
	(s16)udtEntityFlag::PlayerEvent, UNDEFINED,
	(s16)udtEntityFlag::AwardHumiliation, 6,
	(s16)udtEntityFlag::NoDraw, 7,
	(s16)udtEntityFlag::Firing, 8,
	(s16)udtEntityFlag::AwardCapture, 11,
	(s16)udtEntityFlag::Chatting, 12,
	(s16)udtEntityFlag::ConnectionInterrupted, 13,
	(s16)udtEntityFlag::HasVoted, 14,
	(s16)udtEntityFlag::AwardImpressive, 15,
	(s16)udtEntityFlag::AwardDefense, UNDEFINED,
	(s16)udtEntityFlag::AwardAssist, UNDEFINED,
	(s16)udtEntityFlag::AwardDenied, UNDEFINED,
	(s16)udtEntityFlag::HasTeamVoted, UNDEFINED,
	(s16)udtEntityFlag::Spectator, UNDEFINED
};

static s16 EntityFlagBits_48_U2Q[udtEntityFlag::Count];
static s16 EntityFlagBits_48_Q2U[udtEntityFlag::Count * 2];
static const s16 EntityFlagBits_48[udtEntityFlag::Count * 2] =
{
	(s16)udtEntityFlag::Dead, 0,
	(s16)udtEntityFlag::TeleportBit, 2,
	(s16)udtEntityFlag::AwardExcellent, 3,
	(s16)udtEntityFlag::PlayerEvent, UNDEFINED,
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
	(s16)udtEntityFlag::Spectator, UNDEFINED
};

static s16 EntityFlagBits_66_90_U2Q[udtEntityFlag::Count];
static s16 EntityFlagBits_66_90_Q2U[udtEntityFlag::Count * 2];
static const s16 EntityFlagBits_66_90[udtEntityFlag::Count * 2] =
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
	(s16)udtEntityFlag::Spectator, UNDEFINED
};

static s16 EntityFlagBits_91_U2Q[udtEntityFlag::Count];
static s16 EntityFlagBits_91_Q2U[udtEntityFlag::Count * 2];
static const s16 EntityFlagBits_91[udtEntityFlag::Count * 2] =
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
	(s16)udtEntityFlag::HasVoted, UNDEFINED,
	(s16)udtEntityFlag::AwardImpressive, 15,
	(s16)udtEntityFlag::AwardDefense, 16,
	(s16)udtEntityFlag::AwardAssist, 17,
	(s16)udtEntityFlag::AwardDenied, 18,
	(s16)udtEntityFlag::HasTeamVoted, UNDEFINED,
	(s16)udtEntityFlag::Spectator, 14
};

static const s16* EntityFlagBitTables[udtProtocol::Count * 3] =
{
	EntityFlagBits_3, EntityFlagBits_3_U2Q, EntityFlagBits_3_Q2U,
	EntityFlagBits_48, EntityFlagBits_48_U2Q, EntityFlagBits_48_Q2U,
	EntityFlagBits_66_90, EntityFlagBits_66_90_U2Q, EntityFlagBits_66_90_Q2U,
	EntityFlagBits_66_90, EntityFlagBits_66_90_U2Q, EntityFlagBits_66_90_Q2U,
	EntityFlagBits_66_90, EntityFlagBits_66_90_U2Q, EntityFlagBits_66_90_Q2U,
	EntityFlagBits_66_90, EntityFlagBits_66_90_U2Q, EntityFlagBits_66_90_Q2U,
	EntityFlagBits_66_90, EntityFlagBits_66_90_U2Q, EntityFlagBits_66_90_Q2U,
	EntityFlagBits_91, EntityFlagBits_91_U2Q, EntityFlagBits_91_Q2U
};

static s16 EntityEvents_3_U2Q[udtEntityEvent::Count];
static s16 EntityEvents_3_Q2U[udtEntityEvent::Count * 2];
static const s16 EntityEvents_3[udtEntityEvent::Count * 2] =
{
	(s16)udtEntityEvent::Obituary, 58,
	(s16)udtEntityEvent::WeaponFired, 23,
	(s16)udtEntityEvent::ItemPickup, 19,
	(s16)udtEntityEvent::GlobalItemPickup, 20,
	(s16)udtEntityEvent::GlobalSound, 46,
	(s16)udtEntityEvent::GlobalTeamSound, UNDEFINED,
	(s16)udtEntityEvent::ItemRespawn, 40,
	(s16)udtEntityEvent::ItemPop, 41,
	(s16)udtEntityEvent::PlayerTeleportIn, 42,
	(s16)udtEntityEvent::PlayerTeleportOut, 43,
	(s16)udtEntityEvent::BulletHitFlesh, 47,
	(s16)udtEntityEvent::BulletHitWall, 48,
	(s16)udtEntityEvent::MissileHit, 49,
	(s16)udtEntityEvent::MissileMiss, 50,
	(s16)udtEntityEvent::MissileMissMetal, UNDEFINED,
	(s16)udtEntityEvent::RailTrail, 51,
	(s16)udtEntityEvent::PowerUpQuad, 59,
	(s16)udtEntityEvent::PowerUpBattleSuit, 60,
	(s16)udtEntityEvent::PowerUpRegen, 61,
	(s16)udtEntityEvent::QL_Overtime, UNDEFINED,
	(s16)udtEntityEvent::QL_GameOver, UNDEFINED
};

static s16 EntityEvents_48_68_U2Q[udtEntityEvent::Count];
static s16 EntityEvents_48_68_Q2U[udtEntityEvent::Count * 2];
static const s16 EntityEvents_48_68[udtEntityEvent::Count * 2] =
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
	(s16)udtEntityEvent::QL_Overtime, UNDEFINED,
	(s16)udtEntityEvent::QL_GameOver, UNDEFINED
};

static s16 EntityEvents_73p_U2Q[udtEntityEvent::Count];
static s16 EntityEvents_73p_Q2U[udtEntityEvent::Count * 2];
static const s16 EntityEvents_73p[udtEntityEvent::Count * 2] =
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
	(s16)udtEntityEvent::QL_GameOver, 85
};

static const s16* EntityEventTables[udtProtocol::Count * 3] =
{
	EntityEvents_3, EntityEvents_3_U2Q, EntityEvents_3_Q2U,
	EntityEvents_48_68, EntityEvents_48_68_U2Q, EntityEvents_48_68_Q2U,
	EntityEvents_48_68, EntityEvents_48_68_U2Q, EntityEvents_48_68_Q2U,
	EntityEvents_48_68, EntityEvents_48_68_U2Q, EntityEvents_48_68_Q2U,
	EntityEvents_48_68, EntityEvents_48_68_U2Q, EntityEvents_48_68_Q2U,
	EntityEvents_73p, EntityEvents_73p_U2Q, EntityEvents_73p_Q2U,
	EntityEvents_73p, EntityEvents_73p_U2Q, EntityEvents_73p_Q2U,
	EntityEvents_73p, EntityEvents_73p_U2Q, EntityEvents_73p_Q2U
};

static s16 ConfigStringIndices_3_U2Q[udtConfigStringIndex::Count];
static s16 ConfigStringIndices_3_Q2U[udtConfigStringIndex::Count * 2];
static const s16 ConfigStringIndices_3[udtConfigStringIndex::Count * 2] =
{
	(s16)udtConfigStringIndex::FirstPlayer, 544,
	(s16)udtConfigStringIndex::Intermission, 14,
	(s16)udtConfigStringIndex::LevelStartTime, 13,
	(s16)udtConfigStringIndex::WarmUpEndTime, 5,
	(s16)udtConfigStringIndex::FirstPlacePlayerName, UNDEFINED,
	(s16)udtConfigStringIndex::SecondPlacePlayerName, UNDEFINED,
	(s16)udtConfigStringIndex::PauseStart, UNDEFINED,
	(s16)udtConfigStringIndex::PauseEnd, UNDEFINED,
	(s16)udtConfigStringIndex::FlagStatus, 15,
	(s16)udtConfigStringIndex::ServerInfo, 0,
	(s16)udtConfigStringIndex::SystemInfo, 1,
	(s16)udtConfigStringIndex::Scores1, 6,
	(s16)udtConfigStringIndex::Scores2, 7,
	(s16)udtConfigStringIndex::VoteTime, 8,
	(s16)udtConfigStringIndex::VoteString, 9,
	(s16)udtConfigStringIndex::VoteYes, 10,
	(s16)udtConfigStringIndex::VoteNo, 11,
	(s16)udtConfigStringIndex::TeamVoteTime, UNDEFINED,
	(s16)udtConfigStringIndex::TeamVoteString, UNDEFINED,
	(s16)udtConfigStringIndex::TeamVoteYes, UNDEFINED,
	(s16)udtConfigStringIndex::TeamVoteNo, UNDEFINED,
	(s16)udtConfigStringIndex::GameVersion, 12,
	(s16)udtConfigStringIndex::ItemFlags, 27,
	(s16)udtConfigStringIndex::QL_TimeoutStartTime, 669,
	(s16)udtConfigStringIndex::QL_TimeoutEndTime, 670,
	(s16)udtConfigStringIndex::QL_RedTeamTimeoutsLeft, UNDEFINED,
	(s16)udtConfigStringIndex::QL_BlueTeamTimeoutsLeft, UNDEFINED,
	(s16)udtConfigStringIndex::QL_ReadTeamClanName, 693,
	(s16)udtConfigStringIndex::QL_BlueTeamClanName, 694,
	(s16)udtConfigStringIndex::QL_RedTeamClanTag, 695,
	(s16)udtConfigStringIndex::QL_BlueTeamClanTag, 696,
	(s16)udtConfigStringIndex::CPMA_GameInfo, 672,
	(s16)udtConfigStringIndex::CPMA_RoundInfo, 710,
	(s16)udtConfigStringIndex::OSP_GamePlay, 806
};

static s16 ConfigStringIndices_48_68_U2Q[udtConfigStringIndex::Count];
static s16 ConfigStringIndices_48_68_Q2U[udtConfigStringIndex::Count * 2];
static const s16 ConfigStringIndices_48_68[udtConfigStringIndex::Count * 2] =
{
	(s16)udtConfigStringIndex::FirstPlayer, 544,
	(s16)udtConfigStringIndex::Intermission, 22,
	(s16)udtConfigStringIndex::LevelStartTime, 21,
	(s16)udtConfigStringIndex::WarmUpEndTime, 5,
	(s16)udtConfigStringIndex::FirstPlacePlayerName, UNDEFINED,
	(s16)udtConfigStringIndex::SecondPlacePlayerName, UNDEFINED,
	(s16)udtConfigStringIndex::PauseStart, UNDEFINED,
	(s16)udtConfigStringIndex::PauseEnd, UNDEFINED,
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
	(s16)udtConfigStringIndex::QL_RedTeamTimeoutsLeft, UNDEFINED,
	(s16)udtConfigStringIndex::QL_BlueTeamTimeoutsLeft, UNDEFINED,
	(s16)udtConfigStringIndex::QL_ReadTeamClanName, 693,
	(s16)udtConfigStringIndex::QL_BlueTeamClanName, 694,
	(s16)udtConfigStringIndex::QL_RedTeamClanTag, 695,
	(s16)udtConfigStringIndex::QL_BlueTeamClanTag, 696,
	(s16)udtConfigStringIndex::CPMA_GameInfo, 672,
	(s16)udtConfigStringIndex::CPMA_RoundInfo, 710,
	(s16)udtConfigStringIndex::OSP_GamePlay, 806
};

static s16 ConfigStringIndices_73_90_U2Q[udtConfigStringIndex::Count];
static s16 ConfigStringIndices_73_90_Q2U[udtConfigStringIndex::Count * 2];
static const s16 ConfigStringIndices_73_90[udtConfigStringIndex::Count * 2] =
{
	(s16)udtConfigStringIndex::FirstPlayer, 529,
	(s16)udtConfigStringIndex::Intermission, 14,
	(s16)udtConfigStringIndex::LevelStartTime, 13,
	(s16)udtConfigStringIndex::WarmUpEndTime, 5,
	(s16)udtConfigStringIndex::FirstPlacePlayerName, UNDEFINED,
	(s16)udtConfigStringIndex::SecondPlacePlayerName, UNDEFINED,
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
	(s16)udtConfigStringIndex::TeamVoteTime, UNDEFINED,
	(s16)udtConfigStringIndex::TeamVoteString, UNDEFINED,
	(s16)udtConfigStringIndex::TeamVoteYes, UNDEFINED,
	(s16)udtConfigStringIndex::TeamVoteNo, UNDEFINED,
	(s16)udtConfigStringIndex::GameVersion, 12,
	(s16)udtConfigStringIndex::ItemFlags, 15,
	(s16)udtConfigStringIndex::QL_TimeoutStartTime, 669,
	(s16)udtConfigStringIndex::QL_TimeoutEndTime, 670,
	(s16)udtConfigStringIndex::QL_RedTeamTimeoutsLeft, UNDEFINED,
	(s16)udtConfigStringIndex::QL_BlueTeamTimeoutsLeft, UNDEFINED,
	(s16)udtConfigStringIndex::QL_ReadTeamClanName, 693,
	(s16)udtConfigStringIndex::QL_BlueTeamClanName, 694,
	(s16)udtConfigStringIndex::QL_RedTeamClanTag, 695,
	(s16)udtConfigStringIndex::QL_BlueTeamClanTag, 696,
	(s16)udtConfigStringIndex::CPMA_GameInfo, UNDEFINED,
	(s16)udtConfigStringIndex::CPMA_RoundInfo, UNDEFINED,
	(s16)udtConfigStringIndex::OSP_GamePlay, UNDEFINED
};

static s16 ConfigStringIndices_91_U2Q[udtConfigStringIndex::Count];
static s16 ConfigStringIndices_91_Q2U[udtConfigStringIndex::Count * 2];
static const s16 ConfigStringIndices_91[udtConfigStringIndex::Count * 2] =
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
	(s16)udtConfigStringIndex::TeamVoteTime, UNDEFINED,
	(s16)udtConfigStringIndex::TeamVoteString, UNDEFINED,
	(s16)udtConfigStringIndex::TeamVoteYes, UNDEFINED,
	(s16)udtConfigStringIndex::TeamVoteNo, UNDEFINED,
	(s16)udtConfigStringIndex::GameVersion, 12,
	(s16)udtConfigStringIndex::ItemFlags, 15,
	(s16)udtConfigStringIndex::QL_TimeoutStartTime, UNDEFINED,
	(s16)udtConfigStringIndex::QL_TimeoutEndTime, UNDEFINED,
	(s16)udtConfigStringIndex::QL_RedTeamTimeoutsLeft, 671,
	(s16)udtConfigStringIndex::QL_BlueTeamTimeoutsLeft, 672,
	(s16)udtConfigStringIndex::QL_ReadTeamClanName, UNDEFINED,
	(s16)udtConfigStringIndex::QL_BlueTeamClanName, UNDEFINED,
	(s16)udtConfigStringIndex::QL_RedTeamClanTag, UNDEFINED,
	(s16)udtConfigStringIndex::QL_BlueTeamClanTag, UNDEFINED,
	(s16)udtConfigStringIndex::CPMA_GameInfo, UNDEFINED,
	(s16)udtConfigStringIndex::CPMA_RoundInfo, UNDEFINED,
	(s16)udtConfigStringIndex::OSP_GamePlay, UNDEFINED
};

static const s16* ConfigStringIndexTables[udtProtocol::Count * 3] =
{
	ConfigStringIndices_3, ConfigStringIndices_3_U2Q, ConfigStringIndices_3_Q2U,
	ConfigStringIndices_48_68, ConfigStringIndices_48_68_U2Q, ConfigStringIndices_48_68_Q2U,
	ConfigStringIndices_48_68, ConfigStringIndices_48_68_U2Q, ConfigStringIndices_48_68_Q2U,
	ConfigStringIndices_48_68, ConfigStringIndices_48_68_U2Q, ConfigStringIndices_48_68_Q2U,
	ConfigStringIndices_48_68, ConfigStringIndices_48_68_U2Q, ConfigStringIndices_48_68_Q2U,
	ConfigStringIndices_73_90, ConfigStringIndices_73_90_U2Q, ConfigStringIndices_73_90_Q2U,
	ConfigStringIndices_73_90, ConfigStringIndices_73_90_U2Q, ConfigStringIndices_73_90_Q2U,
	ConfigStringIndices_91, ConfigStringIndices_91_U2Q, ConfigStringIndices_91_Q2U
};

static s16 Teams_U2Q[udtTeam::Count];
static s16 Teams_Q2U[udtTeam::Count * 2];
static const s16 Teams[udtTeam::Count * 2] =
{
	(s16)udtTeam::Free, 0,
	(s16)udtTeam::Red, 1,
	(s16)udtTeam::Blue, 2,
	(s16)udtTeam::Spectators, 3
};

static const s16* TeamTables[udtProtocol::Count * 3] =
{
	Teams, Teams_U2Q, Teams_Q2U,
	Teams, Teams_U2Q, Teams_Q2U,
	Teams, Teams_U2Q, Teams_Q2U,
	Teams, Teams_U2Q, Teams_Q2U,
	Teams, Teams_U2Q, Teams_Q2U,
	Teams, Teams_U2Q, Teams_Q2U,
	Teams, Teams_U2Q, Teams_Q2U,
	Teams, Teams_U2Q, Teams_Q2U
};

static s16 GameTypes_3_U2Q[udtGameType::Count];
static s16 GameTypes_3_Q2U[udtGameType::Count * 2];
static const s16 GameTypes_3[udtGameType::Count * 2] =
{
	(s16)udtGameType::SP, UNDEFINED,
	(s16)udtGameType::FFA, 0,
	(s16)udtGameType::Duel, 1,
	(s16)udtGameType::Race, UNDEFINED,
	(s16)udtGameType::HM, UNDEFINED,
	(s16)udtGameType::RedRover, UNDEFINED,
	(s16)udtGameType::TDM, 3,
	(s16)udtGameType::CBTDM, UNDEFINED,
	(s16)udtGameType::CA, UNDEFINED,
	(s16)udtGameType::CTF, 4,
	(s16)udtGameType::OneFlagCTF, UNDEFINED,
	(s16)udtGameType::Obelisk, UNDEFINED,
	(s16)udtGameType::Harvester, UNDEFINED,
	(s16)udtGameType::Domination, UNDEFINED,
	(s16)udtGameType::CTFS, UNDEFINED,
	(s16)udtGameType::NTF, UNDEFINED,
	(s16)udtGameType::TwoVsTwo, UNDEFINED,
	(s16)udtGameType::FT, UNDEFINED
};

static s16 GameTypes_48_68_U2Q[udtGameType::Count];
static s16 GameTypes_48_68_Q2U[udtGameType::Count * 2];
static const s16 GameTypes_48_68[udtGameType::Count * 2] =
{
	(s16)udtGameType::SP, UNDEFINED,
	(s16)udtGameType::FFA, 0,
	(s16)udtGameType::Duel, 1,
	(s16)udtGameType::Race, UNDEFINED,
	(s16)udtGameType::HM, UNDEFINED,
	(s16)udtGameType::RedRover, UNDEFINED,
	(s16)udtGameType::TDM, 3,
	(s16)udtGameType::CBTDM, UNDEFINED,
	(s16)udtGameType::CA, UNDEFINED,
	(s16)udtGameType::CTF, 4,
	(s16)udtGameType::OneFlagCTF, 5,
	(s16)udtGameType::Obelisk, 6,
	(s16)udtGameType::Harvester, 7,
	(s16)udtGameType::Domination, UNDEFINED,
	(s16)udtGameType::CTFS, UNDEFINED,
	(s16)udtGameType::NTF, UNDEFINED,
	(s16)udtGameType::TwoVsTwo, UNDEFINED,
	(s16)udtGameType::FT, UNDEFINED
};

static s16 GameTypes_73p_U2Q[udtGameType::Count];
static s16 GameTypes_73p_Q2U[udtGameType::Count * 2];
static const s16 GameTypes_73p[udtGameType::Count * 2] =
{
	(s16)udtGameType::SP, UNDEFINED,
	(s16)udtGameType::FFA, 0,
	(s16)udtGameType::Duel, 1,
	(s16)udtGameType::Race, 2,
	(s16)udtGameType::HM, UNDEFINED,
	(s16)udtGameType::RedRover, 12,
	(s16)udtGameType::TDM, 3,
	(s16)udtGameType::CBTDM, UNDEFINED,
	(s16)udtGameType::CA, 4,
	(s16)udtGameType::CTF, 5,
	(s16)udtGameType::OneFlagCTF, 6,
	(s16)udtGameType::Obelisk, 7,
	(s16)udtGameType::Harvester, 8,
	(s16)udtGameType::Domination, 10,
	(s16)udtGameType::CTFS, 11,
	(s16)udtGameType::NTF, UNDEFINED,
	(s16)udtGameType::TwoVsTwo, UNDEFINED,
	(s16)udtGameType::FT, 9
};

static const s16* GameTypeTables[udtProtocol::Count * 3] =
{
	GameTypes_3, GameTypes_3_U2Q, GameTypes_3_Q2U,
	GameTypes_48_68, GameTypes_48_68_U2Q, GameTypes_48_68_Q2U,
	GameTypes_48_68, GameTypes_48_68_U2Q, GameTypes_48_68_Q2U,
	GameTypes_48_68, GameTypes_48_68_U2Q, GameTypes_48_68_Q2U,
	GameTypes_48_68, GameTypes_48_68_U2Q, GameTypes_48_68_Q2U,
	GameTypes_73p, GameTypes_73p_U2Q, GameTypes_73p_Q2U,
	GameTypes_73p, GameTypes_73p_U2Q, GameTypes_73p_Q2U,
	GameTypes_73p, GameTypes_73p_U2Q, GameTypes_73p_Q2U
};

static s16 FlagStatus_U2Q[udtFlagStatus::Count];
static s16 FlagStatus_Q2U[udtFlagStatus::Count * 2];
static const s16 FlagStatus[udtFlagStatus::Count * 2] =
{
	(s16)udtFlagStatus::InBase, 0,
	(s16)udtFlagStatus::Carried, 1,
	(s16)udtFlagStatus::Missing, 2
};

static const s16* FlagStatusTables[udtProtocol::Count * 3] =
{
	FlagStatus, FlagStatus_U2Q, FlagStatus_Q2U,
	FlagStatus, FlagStatus_U2Q, FlagStatus_Q2U,
	FlagStatus, FlagStatus_U2Q, FlagStatus_Q2U,
	FlagStatus, FlagStatus_U2Q, FlagStatus_Q2U,
	FlagStatus, FlagStatus_U2Q, FlagStatus_Q2U,
	FlagStatus, FlagStatus_U2Q, FlagStatus_Q2U,
	FlagStatus, FlagStatus_U2Q, FlagStatus_Q2U,
	FlagStatus, FlagStatus_U2Q, FlagStatus_Q2U
};

static s16 Weapons_3_68_U2Q[udtWeapon::Count];
static s16 Weapons_3_68_Q2U[udtWeapon::Count * 2];
static const s16 Weapons_3_68[udtWeapon::Count * 2] =
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
	(s16)udtWeapon::NailGun, UNDEFINED,
	(s16)udtWeapon::ChainGun, UNDEFINED,
	(s16)udtWeapon::ProximityMineLauncher, UNDEFINED,
	(s16)udtWeapon::HeavyMachineGun, UNDEFINED,
	(s16)udtWeapon::GrapplingHook, 10
};

static s16 Weapons_73p_U2Q[udtWeapon::Count];
static s16 Weapons_73p_Q2U[udtWeapon::Count * 2];
static const s16 Weapons_73p[udtWeapon::Count * 2] =
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
	(s16)udtWeapon::GrapplingHook, 10
};

static const s16* WeaponTables[udtProtocol::Count * 3] =
{
	Weapons_3_68, Weapons_3_68_U2Q, Weapons_3_68_Q2U,
	Weapons_3_68, Weapons_3_68_U2Q, Weapons_3_68_Q2U,
	Weapons_3_68, Weapons_3_68_U2Q, Weapons_3_68_Q2U,
	Weapons_3_68, Weapons_3_68_U2Q, Weapons_3_68_Q2U,
	Weapons_3_68, Weapons_3_68_U2Q, Weapons_3_68_Q2U,
	Weapons_73p, Weapons_73p_U2Q, Weapons_73p_Q2U,
	Weapons_73p, Weapons_73p_U2Q, Weapons_73p_Q2U,
	Weapons_73p, Weapons_73p_U2Q, Weapons_73p_Q2U
};

static s16 MeansOfDeath_3_68_U2Q[udtMeanOfDeath::Count];
static s16 MeansOfDeath_3_68_Q2U[udtMeanOfDeath::Count * 2];
static const s16 MeansOfDeath_3_68[udtMeanOfDeath::Count * 2] =
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
	(s16)udtMeanOfDeath::NailGun, UNDEFINED,
	(s16)udtMeanOfDeath::ChainGun, UNDEFINED,
	(s16)udtMeanOfDeath::ProximityMine, UNDEFINED,
	(s16)udtMeanOfDeath::Kamikaze, UNDEFINED,
	(s16)udtMeanOfDeath::Juiced, UNDEFINED,
	(s16)udtMeanOfDeath::Grapple, 23,
	(s16)udtMeanOfDeath::TeamSwitch, UNDEFINED,
	(s16)udtMeanOfDeath::Thaw, UNDEFINED,
	(s16)udtMeanOfDeath::HeavyMachineGun, UNDEFINED
};

static s16 MeansOfDeath_73p_U2Q[udtMeanOfDeath::Count];
static s16 MeansOfDeath_73p_Q2U[udtMeanOfDeath::Count * 2];
static const s16 MeansOfDeath_73p[udtMeanOfDeath::Count * 2] =
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
	(s16)udtMeanOfDeath::HeavyMachineGun, 32
};

static const s16* MeanOfDeathTables[udtProtocol::Count * 3] =
{
	MeansOfDeath_3_68, MeansOfDeath_3_68_U2Q, MeansOfDeath_3_68_Q2U,
	MeansOfDeath_3_68, MeansOfDeath_3_68_U2Q, MeansOfDeath_3_68_Q2U,
	MeansOfDeath_3_68, MeansOfDeath_3_68_U2Q, MeansOfDeath_3_68_Q2U,
	MeansOfDeath_3_68, MeansOfDeath_3_68_U2Q, MeansOfDeath_3_68_Q2U,
	MeansOfDeath_3_68, MeansOfDeath_3_68_U2Q, MeansOfDeath_3_68_Q2U,
	MeansOfDeath_73p, MeansOfDeath_73p_U2Q, MeansOfDeath_73p_Q2U,
	MeansOfDeath_73p, MeansOfDeath_73p_U2Q, MeansOfDeath_73p_Q2U,
	MeansOfDeath_73p, MeansOfDeath_73p_U2Q, MeansOfDeath_73p_Q2U
};

static s16 Items_3_68_U2Q[udtItem::Count];
static s16 Items_3_68_Q2U[udtItem::Count * 2];
static const s16 Items_3_68[udtItem::Count * 2] =
{
	(s16)udtItem::AmmoBFG, 25,
	(s16)udtItem::AmmoBelt, UNDEFINED,
	(s16)udtItem::AmmoBullets, 19,
	(s16)udtItem::AmmoCells, 21,
	(s16)udtItem::AmmoGrenades, 20,
	(s16)udtItem::AmmoHMG, UNDEFINED,
	(s16)udtItem::AmmoLightning, 22,
	(s16)udtItem::AmmoMines, UNDEFINED,
	(s16)udtItem::AmmoNails, UNDEFINED,
	(s16)udtItem::AmmoPack, UNDEFINED,
	(s16)udtItem::AmmoRockets, 23,
	(s16)udtItem::AmmoShells, 18,
	(s16)udtItem::AmmoSlugs, 24,
	(s16)udtItem::HoldableInvulnerability, UNDEFINED,
	(s16)udtItem::HoldableKamikaze, UNDEFINED,
	(s16)udtItem::HoldableMedkit, 27,
	(s16)udtItem::HoldablePortal, UNDEFINED,
	(s16)udtItem::HoldableTeleporter, 26,
	(s16)udtItem::ItemAmmoRegen, UNDEFINED,
	(s16)udtItem::ItemArmorBody, 3,
	(s16)udtItem::ItemArmorCombat, 2,
	(s16)udtItem::ItemArmorJacket, UNDEFINED,
	(s16)udtItem::ItemArmorShard, 1,
	(s16)udtItem::ItemBackpack, UNDEFINED,
	(s16)udtItem::ItemBlueCube, UNDEFINED,
	(s16)udtItem::ItemDoubler, UNDEFINED,
	(s16)udtItem::ItemEnviro, 29,
	(s16)udtItem::ItemFlight, 33,
	(s16)udtItem::ItemGuard, UNDEFINED,
	(s16)udtItem::ItemHaste, 30,
	(s16)udtItem::ItemHealth, 5,
	(s16)udtItem::ItemHealthLarge, 6,
	(s16)udtItem::ItemHealthMega, 7,
	(s16)udtItem::ItemHealthSmall, 4,
	(s16)udtItem::ItemInvis, 31,
	(s16)udtItem::ItemKeyGold, UNDEFINED,
	(s16)udtItem::ItemKeyMaster, UNDEFINED,
	(s16)udtItem::ItemKeySilver, UNDEFINED,
	(s16)udtItem::ItemQuad, 28,
	(s16)udtItem::ItemRedCube, UNDEFINED,
	(s16)udtItem::ItemRegen, 32,
	(s16)udtItem::ItemScout, UNDEFINED,
	(s16)udtItem::ItemSpawnArmor, UNDEFINED,
	(s16)udtItem::FlagBlue, 35,
	(s16)udtItem::FlagNeutral, UNDEFINED,
	(s16)udtItem::FlagRed, 34,
	(s16)udtItem::WeaponBFG, 16,
	(s16)udtItem::WeaponChaingun, UNDEFINED,
	(s16)udtItem::WeaponGauntlet, 8,
	(s16)udtItem::WeaponGrapplingHook, 17,
	(s16)udtItem::WeaponGrenadeLauncher, 11,
	(s16)udtItem::WeaponHMG, UNDEFINED,
	(s16)udtItem::WeaponLightningGun, 13,
	(s16)udtItem::WeaponMachinegun, 10,
	(s16)udtItem::WeaponNailgun, UNDEFINED,
	(s16)udtItem::WeaponPlasmaGun, 15,
	(s16)udtItem::WeaponProxLauncher, UNDEFINED,
	(s16)udtItem::WeaponRailgun, 14,
	(s16)udtItem::WeaponRocketLauncher, 12,
	(s16)udtItem::WeaponShotgun, 9
};

static s16 Items_73_U2Q[udtItem::Count];
static s16 Items_73_Q2U[udtItem::Count * 2];
static const s16 Items_73[udtItem::Count * 2] =
{
	(s16)udtItem::AmmoBFG, 26,
	(s16)udtItem::AmmoBelt, 42,
	(s16)udtItem::AmmoBullets, 20,
	(s16)udtItem::AmmoCells, 22,
	(s16)udtItem::AmmoGrenades, 21,
	(s16)udtItem::AmmoHMG, UNDEFINED,
	(s16)udtItem::AmmoLightning, 23,
	(s16)udtItem::AmmoMines, 41,
	(s16)udtItem::AmmoNails, 40,
	(s16)udtItem::AmmoPack, UNDEFINED,
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
	(s16)udtItem::ItemBackpack, UNDEFINED,
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
	(s16)udtItem::ItemKeyGold, UNDEFINED,
	(s16)udtItem::ItemKeyMaster, UNDEFINED,
	(s16)udtItem::ItemKeySilver, UNDEFINED,
	(s16)udtItem::ItemQuad, 29,
	(s16)udtItem::ItemRedCube, 48,
	(s16)udtItem::ItemRegen, 33,
	(s16)udtItem::ItemScout, 43,
	(s16)udtItem::ItemSpawnArmor, UNDEFINED,
	(s16)udtItem::FlagBlue, 36,
	(s16)udtItem::FlagNeutral, 47,
	(s16)udtItem::FlagRed, 35,
	(s16)udtItem::WeaponBFG, 17,
	(s16)udtItem::WeaponChaingun, 52,
	(s16)udtItem::WeaponGauntlet, 9,
	(s16)udtItem::WeaponGrapplingHook, 18,
	(s16)udtItem::WeaponGrenadeLauncher, 12,
	(s16)udtItem::WeaponHMG, UNDEFINED,
	(s16)udtItem::WeaponLightningGun, 14,
	(s16)udtItem::WeaponMachinegun, 11,
	(s16)udtItem::WeaponNailgun, 50,
	(s16)udtItem::WeaponPlasmaGun, 16,
	(s16)udtItem::WeaponProxLauncher, 51,
	(s16)udtItem::WeaponRailgun, 15,
	(s16)udtItem::WeaponRocketLauncher, 13,
	(s16)udtItem::WeaponShotgun, 10
};

static s16 Items_90p_U2Q[udtItem::Count];
static s16 Items_90p_Q2U[udtItem::Count * 2];
static const s16 Items_90p[udtItem::Count * 2] =
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
	(s16)udtItem::ItemBackpack, UNDEFINED,
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
	(s16)udtItem::WeaponShotgun, 10
};

static const s16* ItemTables[udtProtocol::Count * 3] =
{
	Items_3_68, Items_3_68_U2Q, Items_3_68_Q2U,
	Items_3_68, Items_3_68_U2Q, Items_3_68_Q2U,
	Items_3_68, Items_3_68_U2Q, Items_3_68_Q2U,
	Items_3_68, Items_3_68_U2Q, Items_3_68_Q2U,
	Items_3_68, Items_3_68_U2Q, Items_3_68_Q2U,
	Items_73, Items_73_U2Q, Items_73_Q2U,
	Items_90p, Items_90p_U2Q, Items_90p_Q2U,
	Items_90p, Items_90p_U2Q, Items_90p_Q2U
};

static s16 PMTypes_U2Q[udtPlayerMovementType::Count];
static s16 PMTypes_Q2U[udtPlayerMovementType::Count * 2];
static const s16 PMTypes[udtPlayerMovementType::Count * 2] =
{
	(s16)udtPlayerMovementType::Normal, 0,
	(s16)udtPlayerMovementType::NoClip, 1,
	(s16)udtPlayerMovementType::Spectator, 2,
	(s16)udtPlayerMovementType::Dead, 3,
	(s16)udtPlayerMovementType::Freeze, 4,
	(s16)udtPlayerMovementType::Intermission, 5,
	(s16)udtPlayerMovementType::SPIntermission, 6
};

static const s16* PMTypeTables[udtProtocol::Count * 3] =
{
	PMTypes, PMTypes_U2Q, PMTypes_Q2U,
	PMTypes, PMTypes_U2Q, PMTypes_Q2U,
	PMTypes, PMTypes_U2Q, PMTypes_Q2U,
	PMTypes, PMTypes_U2Q, PMTypes_Q2U,
	PMTypes, PMTypes_U2Q, PMTypes_Q2U,
	PMTypes, PMTypes_U2Q, PMTypes_Q2U,
	PMTypes, PMTypes_U2Q, PMTypes_Q2U,
	PMTypes, PMTypes_U2Q, PMTypes_Q2U
};

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
					const s16 newIdx = table[2 * i + 0];
					table_U2Q[newIdx] = table[2 * i + 1];
				}
			}
			if(table_Q2U != prevTable_Q2U)
			{
				memcpy(table_Q2U, table, (size_t)(tableGroup.Count * 2) * sizeof(s16));
				qsort(table_Q2U, (size_t)tableGroup.Count, 2 * sizeof(s16), &SortCallback);
			}
			prevTable_U2Q = table_U2Q;
			prevTable_Q2U = table_Q2U;
		}
	}
}

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
		TwoVsTwo = 9,
		Count
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

	if(numberType == udtMagicNumberType::GameType &&
	   protocol <= udtProtocol::Dm68 &&
	   mod == udtMod::CPMA)
	{
		const s32 result = GetIdGameTypeCPMA((s32)udtNumber);
		const bool success = result != UDT_S32_MIN;
		if(success) idNumber = result;
		return success;
	}

	if(numberType == udtMagicNumberType::Item &&
	   protocol <= udtProtocol::Dm68 &&
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

	if(numberType == udtMagicNumberType::GameType && 
	   protocol <= udtProtocol::Dm68 && 
	   mod == udtMod::CPMA)
	{
		const s32 result = GetUDTGameTypeCPMA(idNumber);
		const bool success = result != UDT_S32_MIN;
		if(success) udtNumber = (u32)result;
		return success;
	}

	if(numberType == udtMagicNumberType::Item && 
	   protocol <= udtProtocol::Dm68 &&
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
