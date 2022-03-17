#pragma once


#define  UDT_VERSION_MAJOR     1
#define  UDT_VERSION_MINOR     3
#define  UDT_VERSION_REVISION  1

#define  UDT_QUOTE(name)            #name
#define  UDT_STR(macro)             UDT_QUOTE(macro)
#define  UDT_MAKE_VERSION(a, b, c)  UDT_STR(a) "." UDT_STR(b) "." UDT_STR(c)
#define  UDT_VERSION_STRING         UDT_MAKE_VERSION(UDT_VERSION_MAJOR, UDT_VERSION_MINOR, UDT_VERSION_REVISION)


#if defined(_MSC_VER)
#	define UDT_MSVC
#	define UDT_MSVC_VER _MSC_VER
#endif

#if defined(__GNUC__)
#	define UDT_GCC
#	define UDT_GCC_VER __GNUC__
#endif

#if defined(__ICC) || defined(__INTEL_COMPILER)
#	define UDT_ICC
#endif

#if defined(__BORLANDC__) || defined(__BCPLUSPLUS__)
#	define UDT_BORLAND
#endif

#if defined(__MINGW32__)
#	define UDT_MINGWIN
#endif

#if defined(__CYGWIN32__)
#	define UDT_CYGWIN
#endif

#if defined(_M_IX86) || defined(__i386__)
#	define UDT_X86
#endif

#if defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64)
#	define UDT_X64
#endif

#if defined(UDT_X86) || defined(UDT_X64)
#	define UDT_ARCH_X86
#endif

#if defined(UDT_MSVC)
	typedef __int8   s8;
	typedef __int16 s16;
	typedef __int32 s32;
	typedef __int64 s64;
	typedef unsigned __int8   u8;
	typedef unsigned __int16 u16;
	typedef unsigned __int32 u32;
	typedef unsigned __int64 u64;
#elif defined(UDT_GCC)
#	include <stdint.h>
	typedef int8_t    s8;
	typedef int16_t  s16;
	typedef int32_t  s32;
	typedef int64_t  s64;
	typedef uint8_t   u8;
	typedef uint16_t u16;
	typedef uint32_t u32;
	typedef uint64_t u64;
#else
#	error Sorry, your compiler is not supported.
#endif

#if defined(UDT_X86)
	typedef s32 sptr;
	typedef u32 uptr;
#else
	typedef s64 sptr;
	typedef u64 uptr;
#endif

typedef float  f32;
typedef double f64;

#define UDT_S16_MIN (-32768)
#define UDT_S16_MAX (32767)
#define UDT_S32_MIN (-2147483647 - 1)
#define UDT_S32_MAX (2147483647)
#define UDT_U32_MAX (0xFFFFFFFF)

#if defined(UDT_MSVC)
#	if defined(UDT_CREATE_DLL)
#		define UDT_EXPORT_DLL          __declspec(dllexport)
#	else
#		define UDT_EXPORT_DLL          __declspec(dllimport)
#	endif
#	define UDT_API_DECL(ReturnType)    extern UDT_EXPORT_DLL ReturnType
#	define UDT_API_DEF(ReturnType)     UDT_EXPORT_DLL ReturnType
#elif defined(UDT_GCC)
#	if defined(UDT_CREATE_DLL)
#		define UDT_EXPORT_DLL
#	else
#		define UDT_EXPORT_DLL
#	endif
#	define UDT_API_DECL(ReturnType)    extern ReturnType UDT_EXPORT_DLL
#	define UDT_API_DEF(ReturnType)     ReturnType UDT_EXPORT_DLL
#endif

#define UDT_API    UDT_API_DECL
#define UDT_BIT(x) (1 << (x))

typedef struct udtParserContext_s udtParserContext;
typedef struct udtParserContextGroup_s udtParserContextGroup;
typedef struct udtPatternSearchContext_s udtPatternSearchContext;

#if defined(__cplusplus)

#define UDT_ERROR_LIST(N) \
	N(None, "no error") \
	N(InvalidArgument, "invalid argument") \
	N(OperationFailed, "operation failed") \
	N(OperationCanceled, "operation canceled") \
	N(Unprocessed, "unprocessed job") \
	N(InsufficientBufferSize, "insufficient buffer size")

#define UDT_ERROR_ITEM(Enum, Desc) Enum,
struct udtErrorCode
{
	enum Id
	{
		UDT_ERROR_LIST(UDT_ERROR_ITEM)
		AfterLastError
	};
};
#undef UDT_ERROR_ITEM

#define UDT_PROTOCOL_LIST(N) \
	N(3 , ".dm3"  , "Quake 3 1.11-1.17", udtProtocolFlags::Quake3 | udtProtocolFlags::ReadOnly) \
	N(48, ".dm_48", "Quake 3 1.27",      udtProtocolFlags::Quake3 | udtProtocolFlags::ReadOnly) \
	N(57, ".dm_57", "RtCW 1.00-1.10",    udtProtocolFlags::RTCW) \
	N(58, ".dm_58", "RtCW 1.30-1.31",    udtProtocolFlags::RTCW) \
	N(59, ".dm_59", "RtCW 1.32-1.33",    udtProtocolFlags::RTCW) \
	N(60, ".dm_60", "RtCW 1.40-1.41",    udtProtocolFlags::RTCW) \
	N(66, ".dm_66", "Quake 3 1.29-1.30", udtProtocolFlags::Quake3) \
	N(67, ".dm_67", "Quake 3 1.31",      udtProtocolFlags::Quake3) \
	N(68, ".dm_68", "Quake 3 1.32",      udtProtocolFlags::Quake3) \
	N(73, ".dm_73", "Quake Live",        udtProtocolFlags::QuakeLive) \
	N(90, ".dm_90", "Quake Live",        udtProtocolFlags::QuakeLive) \
	N(91, ".dm_91", "Quake Live",        udtProtocolFlags::QuakeLive)

#define UDT_PROTOCOL_ITEM(Number, Ext, Desc, Flags) Dm##Number,
struct udtProtocol
{
	enum Id
	{
		UDT_PROTOCOL_LIST(UDT_PROTOCOL_ITEM)
		Count,
		Invalid
	};
};
#undef UDT_PROTOCOL_ITEM

struct udtProtocolFlags
{
	enum Mask
	{
		ReadOnly = UDT_BIT(0),
		Quake3 = UDT_BIT(1),
		QuakeLive = UDT_BIT(2),
		RTCW = UDT_BIT(3),
		ET = UDT_BIT(4),
		Last = ET,
		Quake = Quake3 | QuakeLive,
		Wolfenstein = RTCW | ET
	};
};

struct udtChatOperator
{
	enum Id
	{
		Contains,
		StartsWith,
		EndsWith,
		Count
	};
};

struct udtCrashType
{
	enum Id
	{
		FatalError,
		ReadAccess,
		WriteAccess,
		Count
	};
};

/*
Macro arguments:
1. enum name 
2. string description
3. plug-in class type (internal)
4. plug-in output class type (for reading back the results of Analyzer plug-ins)
*/
#define UDT_PLUG_IN_LIST(N) \
	N(Chat,             "chat messages",      udtParserPlugInChat,             udtParseDataChatBuffers) \
	N(GameState,        "game states",        udtParserPlugInGameState,        udtParseDataGameStateBuffers) \
	N(Obituaries,       "obituaries",         udtParserPlugInObituaries,       udtParseDataObituaryBuffers) \
	N(Stats,            "match stats",        udtParserPlugInStats,            udtParseDataStatsBuffers) \
	N(RawCommands,      "raw commands",       udtParserPlugInRawCommands,      udtParseDataRawCommandBuffers) \
	N(RawConfigStrings, "raw config strings", udtParserPlugInRawConfigStrings, udtParseDataRawConfigStringBuffers) \
	N(Captures,         "captures",           udtParserPlugInCaptures,         udtParseDataCaptureBuffers) \
	N(Scores,           "scores",             udtParserPlugInScores,           udtParseDataScoreBuffers)

#define UDT_PLUG_IN_ITEM(Enum, Desc, Type, OutputType) Enum,
struct udtParserPlugIn
{
	enum Id
	{
		UDT_PLUG_IN_LIST(UDT_PLUG_IN_ITEM)
		Count
	};
};
#undef UDT_PLUG_IN_ITEM

#define UDT_WEAPON_LIST(N) \
	N(Gauntlet, "gauntlet", 0) \
	N(MachineGun, "machine gun", 1) \
	N(Shotgun, "shotgun", 2) \
	N(GrenadeLauncher, "grenade launcher", 3) \
	N(RocketLauncher, "rocket launcher", 4) \
	N(PlasmaGun, "plasma gun", 5) \
	N(Railgun, "railgun", 6) \
	N(LightningGun, "lightning gun", 7) \
	N(BFG, "BFG", 8) \
	N(NailGun, "nailgun", 9) \
	N(ChainGun, "chaingun", 10) \
	N(ProximityMineLauncher, "proximity mine launcher", 11) \
	N(HeavyMachineGun, "heavy machine gun", 12) \
	N(GrapplingHook, "grappling hook", 13) \
	N(Knife, "knife", 14) \
	N(Luger, "luger 9mm", 15) \
	N(MP40, "MP40", 16) \
	N(Mauser, "mauser", 17) \
	N(FG42, "FG42", 18) \
	N(Panzerfaust, "panzerfaust", 19) \
	N(Venom, "venom", 20) \
	N(Flamethrower, "flamethrower", 21) \
	N(Tesla, "tesla", 22) \
	N(Speargun, "speargun", 23) \
	N(Knife2, "knife 2", 24) \
	N(Colt, ".45ACP 1911", 25) \
	N(Thompson, "thompson", 26) \
	N(Garand, "garand", 27) \
	N(Bar, "BAR", 28) \
	N(GrenadePineapple, "pineapple grenade", 29) \
	N(SniperRifle, "sniper rifle", 30) \
	N(SnooperScope, "snooper", 31) \
	N(VenomFull, "venom", 32) \
	N(SpeargunCO2, "speargun", 33) \
	N(FG42Scope, "FG42 scope", 34) \
	N(Bar2, "BAR 2", 35) \
	N(Sten, "sten", 36) \
	N(MedicSyringe, "syringe", 37) \
	N(Ammo, "ammo", 38) \
	N(Artillery, "artillery support", 39) \
	N(Silencer, "silencer", 40) \
	N(Akimbo, "dual colts", 41) \
	N(Cross, "cross", 42) \
	N(Dynamite, "dynamite", 43) \
	N(Dynamite2, "dynamite 2", 44) \
	N(Prox, "prox", 45) \
	N(MonsterAttack1, "monster attack", 46) \
	N(MonsterAttack2, "monster attack 2", 47) \
	N(MonsterAttack3, "monster attack 3", 48) \
	N(SmokeTrail, "smoke trail", 49) \
	N(Sniper, "sniper", 50) \
	N(Mortar, "mortar", 51) \
	N(VeryBigExplosion, "explosion", 52) \
	N(Medkit, "medkit", 53) \
	N(Pliers, "pliers", 54) \
	N(SmokeGrenade, "smoke grenade", 55) \
	N(Binoculars, "binoculars", 56)

#define UDT_WEAPON_ITEM(Enum, Desc, Bit) Enum = Bit,
struct udtWeapon
{
	enum Id
	{
		UDT_WEAPON_LIST(UDT_WEAPON_ITEM)
		Count
	};
};
#undef UDT_WEAPON_ITEM

#define UDT_POWER_UP_LIST(N) \
	N(QuadDamage, "quad damage", 0) \
	N(BattleSuit, "battle suit", 1) \
	N(Haste, "haste", 2) \
	N(Invisibility, "invisibility", 3) \
	N(Regeneration, "regeneration", 4) \
	N(Flight, "flight", 5) \
	N(RedFlag, "red flag", 6) \
	N(BlueFlag, "blue flag", 7) \
	N(NeutralFlag, "neutral flag", 8) \
	N(Scout, "scout", 9) \
	N(Guard, "guard", 10) \
	N(Doubler, "doubler", 11) \
	N(ArmorRegeneration, "armor regeneration", 12) \
	N(Invulnerability, "invulnerability", 13) \
	N(Wolf_Fire, "fire", 14) \
	N(Wolf_Electric, "electric", 15) \
	N(Wolf_Breather, "breather", 16) \
	N(Wolf_NoFatigue, "stamina", 17) \
	N(Wolf_Ready, "ready", 18) \
	N(Wolf_Blackout, "speclock", 19)

#define UDT_POWER_UP_ITEM(Enum, Desc, Bit) Enum = Bit,
struct udtPowerUp
{
	enum Id
	{
		UDT_POWER_UP_LIST(UDT_POWER_UP_ITEM)
		Count
	};
};
#undef UDT_POWER_UP_ITEM

#define UDT_POWER_UP_ITEM(Enum, Desc, Bit) Enum = UDT_BIT(Bit),
struct udtPowerUpMask
{
	enum Id
	{
		UDT_POWER_UP_LIST(UDT_POWER_UP_ITEM)
		AfterLast
	};
};
#undef UDT_POWER_UP_ITEM

#define UDT_MEAN_OF_DEATH_LIST(N) \
	N(Shotgun, "shotgun", PlayerKill, 0) \
	N(Gauntlet, "gauntlet", PlayerKill, 1) \
	N(MachineGun, "machine gun", PlayerKill, 2) \
	N(Grenade, "grenade", PlayerKill, 3) \
	N(GrenadeSplash, "grenade splash", PlayerKill, 4) \
	N(Rocket, "rocket", PlayerKill, 5) \
	N(RocketSplash, "rocket splash", PlayerKill, 6) \
	N(Plasma, "plasma", PlayerKill, 7) \
	N(PlasmaSplash, "plasma splash", PlayerKill, 8) \
	N(Railgun, "railgun", PlayerKill, 9) \
	N(Lightning, "lightning", PlayerKill, 10) \
	N(BFG, "BFG", PlayerKill, 11) \
	N(BFGSplash, "BFG splash", PlayerKill, 12) \
	N(Water, "water", WorldKill, 13) \
	N(Slime, "slime", WorldKill, 14) \
	N(Lava, "lava", WorldKill, 15) \
	N(Crush, "crush", WorldKill, 16) \
	N(TeleFrag, "telefrag", PlayerKill, 17) \
	N(Fall, "fall", WorldKill, 18) \
	N(Suicide, "suicide", PlayerKill, 19) \
	N(TargetLaser, "target laser", WorldKill, 20) \
	N(TriggerHurt, "trigger hurt", WorldKill, 21) \
	N(NailGun, "nailgun", PlayerKill, 22) \
	N(ChainGun, "chaingun", PlayerKill, 23) \
	N(ProximityMine, "proximity mine", PlayerKill, 24) \
	N(Kamikaze, "kamikaze", PlayerKill, 25) \
	N(Juiced, "juiced", WorldKill, 26) \
	N(Grapple, "grapple", PlayerKill, 27) \
	N(TeamSwitch, "team switch", WorldKill, 28) \
	N(Thaw, "thaw", WorldKill, 29) \
	N(HeavyMachineGun, "heavy machine gun", PlayerKill, 30) \
	N(Knife, "knife", PlayerKill, 31) \
	N(Knife2, "knife 2", PlayerKill, 32) \
	N(KnifeStealth, "knife stealth", PlayerKill, 33) \
	N(Luger, "luger 9mm", PlayerKill, 34) \
	N(Colt, ".45ACP 1911", PlayerKill, 35) \
	N(MP40, "MP40", PlayerKill, 36) \
	N(Thompson, "thompson", PlayerKill, 37) \
	N(Sten, "sten", PlayerKill, 38) \
	N(Mauser, "mauser", PlayerKill, 39) \
	N(SniperRifle, "sniper rifle", PlayerKill, 40) \
	N(Garand, "garand", PlayerKill, 41) \
	N(SnooperScope, "snooper", PlayerKill, 42) \
	N(Akimbo, "dual colts", PlayerKill, 43) \
	N(Panzerfaust, "panzerfaust", PlayerKill, 44) \
	N(PanzerfaustSplash, "panzerfaust splash", PlayerKill, 45) \
	N(GrenadePineapple, "pineapple grenade", PlayerKill, 46) \
	N(Venom, "venom", PlayerKill, 47) \
	N(VenomFull, "venom", PlayerKill, 48) \
	N(Flamethrower, "flamethrower", PlayerKill, 49) \
	N(Kicked, "kicked", WorldKill, 50) \
	N(Mortar, "mortar", PlayerKill, 51) \
	N(MortarSplash, "mortar splash", PlayerKill, 52) \
	N(Grabber, "grabber", WorldKill, 53) \
	N(Dynamite, "dynamite", PlayerKill, 54) \
	N(DynamiteSplash, "dynamite splash", PlayerKill, 55) \
	N(Silencer, "silencer", PlayerKill, 56) \
	N(Bar, "BAR", PlayerKill, 57) \
	N(FG42, "FG42", PlayerKill, 58) \
	N(FG42Scope, "FG42 scope", PlayerKill, 59) \
	N(Airstrike, "support fire", PlayerKill, 60) \
	N(Artillery, "artillery support", PlayerKill, 61) \
	N(Explosive, "explosive", PlayerKill, 62) \
	N(Syringe, "syringe", PlayerKill, 63) \
	N(PoisonGas, "poison gas", PlayerKill, 64) \
	N(GrenadeLauncher, "grenade launcher", PlayerKill, 65)

#define UDT_MEAN_OF_DEATH_ITEM(Enum, Desc, KillType, Bit) Enum = Bit,
struct udtMeanOfDeath
{
	enum Id
	{
		UDT_MEAN_OF_DEATH_LIST(UDT_MEAN_OF_DEATH_ITEM)
		Count
	};
};
#undef UDT_MEAN_OF_DEATH_ITEM

#define PlayerKill(Enum) Enum,
#define WorldKill(Enum)
#define UDT_PLAYER_MOD_ITEM(Enum, Desc, KillType, Bit) KillType(Enum)
struct udtPlayerMeanOfDeath
{
	enum Id
	{
		UDT_MEAN_OF_DEATH_LIST(UDT_PLAYER_MOD_ITEM)
		Count
	};
};
#undef UDT_PLAYER_MOD_ITEM
#undef WorldKill
#undef PlayerKill

#define UDT_TEAM_LIST(N) \
	N(Free, "free") \
	N(Red, "red") \
	N(Blue, "blue") \
	N(Spectators, "spectators") \
	N(Axis, "axis") \
	N(Allies, "allies")

#define UDT_TEAM_ITEM(Enum, Desc) Enum,
struct udtTeam
{
	enum Id
	{
		UDT_TEAM_LIST(UDT_TEAM_ITEM)
		Count
	};
};
#undef UDT_TEAM_ITEM

#define UDT_WOLF_CLASS_LIST(N) \
	N(Soldier, "soldier") \
	N(Medic, "medic") \
	N(Engineer, "engineer") \
	N(Lieutenant, "lieutenant") \
	N(FieldOps, "field ops") \
	N(CovertOps, "covert ops")

#define UDT_WOLF_CLASS_ITEM(Enum, Desc) Enum,
struct udtWolfClass
{
	enum Id
	{
		UDT_WOLF_CLASS_LIST(UDT_WOLF_CLASS_ITEM)
		Count
	};
};
#undef UDT_WOLF_CLASS_ITEM

struct udtStringArray
{
	enum Id
	{
		Weapons,
		PowerUps,
		MeansOfDeath,
		PlayerMeansOfDeath,
		Teams,
		CutPatterns,
		GameTypes,
		ShortGameTypes,
		ModNames,
		GamePlayNames,
		ShortGamePlayNames,
		OverTimeTypes,
		TeamStatsNames,
		PlayerStatsNames,
		PlugInNames,
		PerfStatsNames,
		WolfClassNames,
		Count
	};
};

struct udtByteArray
{
	enum Id
	{
		TeamStatsCompModes,
		PlayerStatsCompModes,
		TeamStatsDataTypes,
		PlayerStatsDataTypes,
		PerfStatsDataTypes,
		GameTypeFlags,
		Count
	};
};

#define UDT_PATTERN_LIST(N) \
	N(Chat, "chat", udtChatPatternArg, udtChatPatternAnalyzer) \
	N(FragSequences, "frag sequences", udtFragRunPatternArg, udtFragRunPatternAnalyzer) \
	N(MidAirFrags, "mid-air frags", udtMidAirPatternArg, udtMidAirPatternAnalyzer) \
	N(MultiFragRails, "multi-frag rails", udtMultiRailPatternArg, udtMultiRailPatternAnalyzer) \
	N(FlagCaptures, "flag captures", udtFlagCapturePatternArg, udtFlagCapturePatternAnalyzer) \
	N(FlickRailFrags, "flick rails", udtFlickRailPatternArg, udtFlickRailPatternAnalyzer) \
	N(Matches, "matches", udtMatchPatternArg, udtMatchPatternAnalyzer)

#define UDT_PATTERN_ITEM(Enum, Desc, ArgType, AnalyzerType) Enum,
struct udtPatternType
{
	enum Id
	{
		UDT_PATTERN_LIST(UDT_PATTERN_ITEM)
		Count
	};
};
#undef UDT_PATTERN_ITEM

struct udtStatsCompMode
{
	enum Id
	{
		NeitherWins,
		BiggerWins,
		SmallerWins,
		Count
	};
};

struct udtMatchStatsDataType
{
	enum Id
	{
		Generic,          /* Format as a normal signed integer. */
		Team,             /* The integer is of type udtTeam::Id. */
		Minutes,          /* Append minutes at the end. */
		Seconds,          /* Duration in seconds, use the UDT format instead. */
		Percentage,       /* Append a percentage sign at the end. */
		Weapon,           /* The integer is of type udtWeapon::Id. */
		Ping,             /* The ping in milli-seconds. */
		Positive,         /* The value must be positive or zero. */
		Boolean,          /* The value must be 0 or 1. */
		WolfClass,        /* The integer is of type udtWolfClass::Id. */
		WolfRespawnsLeft, /* -1 means infinite. -2 means 0 and already dead. */
		Count
	};
};

#define UDT_PLAYER_STATS_LIST(N) \
	N(TeamIndex, "team index", NeitherWins, Team) \
	N(Score, "score", BiggerWins, Generic) \
	N(Ping, "ping", SmallerWins, Ping) \
	N(Time, "play time", BiggerWins, Minutes) \
	N(Kills, "kills", BiggerWins, Positive) \
	N(Deaths, "deaths", SmallerWins, Positive) \
	N(Accuracy, "accuracy", BiggerWins, Percentage) \
	N(BestWeapon, "best weapon", NeitherWins, Weapon) \
	N(BestWeaponAccuracy, "best weapon accuracy", BiggerWins, Percentage) \
	N(Perfect, "perfect", BiggerWins, Boolean) \
	N(Impressives, "impressives", BiggerWins, Positive) \
	N(Excellents, "excellents", BiggerWins, Positive) \
	N(Gauntlets, "gauntlets", BiggerWins, Positive) \
	N(TeamKills, "team kills", SmallerWins, Positive) \
	N(TeamKilled, "team killed", NeitherWins, Positive) \
	N(Suicides, "suicides", SmallerWins, Positive) \
	N(DamageGiven, "damage given", BiggerWins, Positive) \
	N(DamageReceived, "damage received", SmallerWins, Positive) \
	N(TeamDamage, "team damage", SmallerWins, Positive) \
	N(Efficiency, "efficiency", BiggerWins, Positive) \
	N(Headshots, "headshots", BiggerWins, Positive) \
	N(Revives, "revives", BiggerWins, Positive) \
	N(TeleFrags, "telefrags", BiggerWins, Positive) \
	N(ArmorTaken, "armor taken", BiggerWins, Positive) \
	N(HealthTaken, "health taken", BiggerWins, Positive) \
	N(Captures, "captures", BiggerWins, Positive) \
	N(Defends, "defends", BiggerWins, Positive) \
	N(Assists, "assists", BiggerWins, Positive) \
	N(Returns, "returns", BiggerWins, Positive) \
	N(FlagPickups, "flag pickups", BiggerWins, Positive) \
	N(FlagTime, "flag possession time", BiggerWins, Seconds) \
	N(Thaws, "thaws", BiggerWins, Positive) \
	N(RedArmorPickups, "red armor pickups", BiggerWins, Positive) \
	N(YellowArmorPickups, "yellow armor pickups", BiggerWins, Positive) \
	N(GreenArmorPickups, "green armor pickups", BiggerWins, Positive) \
	N(MegaHealthPickups, "mega health pickups", BiggerWins, Positive) \
	N(QuadDamagePickups, "quad damage pickups", BiggerWins, Positive) \
	N(BattleSuitPickups, "battle suit pickups", BiggerWins, Positive) \
	N(RedArmorPickupTime, "red armor time", NeitherWins, Generic) \
	N(YellowArmorPickupTime, "yellow armor time", NeitherWins, Generic) \
	N(GreenArmorPickupTime, "green armor time", NeitherWins, Generic) \
	N(MegaHealthPickupTime, "mega health time", NeitherWins, Generic) \
	N(RegenPickups, "regeneration pickups", BiggerWins, Positive) \
	N(HastePickups, "haste pickups", BiggerWins, Positive) \
	N(InvisPickups, "invisibility pickups", BiggerWins, Positive) \
	N(MedkitPickups, "medkit pickups", BiggerWins, Positive) \
	N(RespawnsLeft, "respawns left", NeitherWins, WolfRespawnsLeft) \
	N(PlayerClass, "player class", NeitherWins, WolfClass) \
	N(GibbedBodies, "gibbed bodies", BiggerWins, Positive) \
	N(GauntletKills, "gauntlet kills", BiggerWins, Positive) \
	N(GauntletAccuracy, "gauntlet accuracy", BiggerWins, Percentage) \
	N(GauntletShots, "gauntlet shots", BiggerWins, Positive) \
	N(GauntletHits, "gauntlet hits", BiggerWins, Positive) \
	N(GauntletDamage, "gauntlet damage", BiggerWins, Positive) \
	N(GauntletDrops, "gauntlet drops", SmallerWins, Positive) \
	N(GauntletDeaths, "gauntlet deaths", SmallerWins, Positive) \
	N(GauntletPickups, "gauntlet pickups", BiggerWins, Positive) \
	N(MachineGunKills, "machinegun kills", BiggerWins, Positive) \
	N(MachineGunAccuracy, "machinegun accuracy", BiggerWins, Percentage) \
	N(MachineGunShots, "machinegun shots", BiggerWins, Positive) \
	N(MachineGunHits, "machinegun hits", BiggerWins, Positive) \
	N(MachineGunDamage, "machinegun damage", BiggerWins, Positive) \
	N(MachineGunDrops, "machinegun drops", SmallerWins, Positive) \
	N(MachineGunDeaths, "machinegun deaths", SmallerWins, Positive) \
	N(MachineGunPickups, "machinegun pickups", BiggerWins, Positive) \
	N(ShotgunKills, "shotgun kills", BiggerWins, Positive) \
	N(ShotgunAccuracy, "shotgun accuracy", BiggerWins, Percentage) \
	N(ShotgunShots, "shotgun shots", BiggerWins, Positive) \
	N(ShotgunHits, "shotgun hits", BiggerWins, Positive) \
	N(ShotgunDamage, "shotgun damage", BiggerWins, Positive) \
	N(ShotgunDrops, "shotgun drops", SmallerWins, Positive) \
	N(ShotgunDeaths, "shotgun deaths", SmallerWins, Positive) \
	N(ShotgunPickups, "shotgun pickups", BiggerWins, Positive) \
	N(GrenadeLauncherKills, "grenade launcher kills", BiggerWins, Positive) \
	N(GrenadeLauncherAccuracy, "grenade launcher accuracy", BiggerWins, Percentage) \
	N(GrenadeLauncherShots, "grenade launcher shots", BiggerWins, Positive) \
	N(GrenadeLauncherHits, "grenade launcher hits", BiggerWins, Positive) \
	N(GrenadeLauncherDamage, "grenade launcher damage", BiggerWins, Positive) \
	N(GrenadeLauncherDrops, "grenade launcher drops", SmallerWins, Positive) \
	N(GrenadeLauncherDeaths, "grenade launcher deaths", SmallerWins, Positive) \
	N(GrenadeLauncherPickups, "grenade launcher pickups", BiggerWins, Positive) \
	N(RocketLauncherKills, "rocket launcher kills", BiggerWins, Positive) \
	N(RocketLauncherAccuracy, "rocket launcher accuracy", BiggerWins, Percentage) \
	N(RocketLauncherSkill, "rocket launcher skill", BiggerWins, Percentage) \
	N(RocketLauncherShots, "rocket launcher shots", BiggerWins, Positive) \
	N(RocketLauncherHits, "rocket launcher hits", BiggerWins, Positive) \
	N(RocketLauncherDirectHits, "rocket launcher direct hits", BiggerWins, Positive) \
	N(RocketLauncherDamage, "rocket launcher damage", BiggerWins, Positive) \
	N(RocketLauncherDrops, "rocket launcher drops", SmallerWins, Positive) \
	N(RocketLauncherDeaths, "rocket launcher deaths", SmallerWins, Positive) \
	N(RocketLauncherPickups, "rocket launcher pickups", BiggerWins, Positive) \
	N(PlasmaGunKills, "plasma gun kills", BiggerWins, Positive) \
	N(PlasmaGunAccuracy, "plasma gun accuracy", BiggerWins, Percentage) \
	N(PlasmaGunShots, "plasma gun shots", BiggerWins, Positive) \
	N(PlasmaGunHits, "plasma gun hits", BiggerWins, Positive) \
	N(PlasmaGunDamage, "plasma gun damage", BiggerWins, Positive) \
	N(PlasmaGunDrops, "plasma gun drops", SmallerWins, Positive) \
	N(PlasmaGunDeaths, "plasma gun deaths", SmallerWins, Positive) \
	N(PlasmaGunPickups, "plasma gun pickups", BiggerWins, Positive) \
	N(RailgunKills, "railgun kills", BiggerWins, Positive) \
	N(RailgunAccuracy, "railgun accuracy", BiggerWins, Percentage) \
	N(RailgunShots, "railgun shots", BiggerWins, Positive) \
	N(RailgunHits, "railgun hits", BiggerWins, Positive) \
	N(RailgunDamage, "railgun damage", BiggerWins, Positive) \
	N(RailgunDrops, "railgun drops", SmallerWins, Positive) \
	N(RailgunDeaths, "railgun deaths", SmallerWins, Positive) \
	N(RailgunPickups, "railgun pickups", BiggerWins, Positive) \
	N(LightningGunKills, "lightning gun kills", BiggerWins, Positive) \
	N(LightningGunAccuracy, "lightning gun accuracy", BiggerWins, Percentage) \
	N(LightningGunShots, "lightning gun shots", BiggerWins, Positive) \
	N(LightningGunHits, "lightning gun hits", BiggerWins, Positive) \
	N(LightningGunDamage, "lightning gun damage", BiggerWins, Positive) \
	N(LightningGunDrops, "lightning gun drops", SmallerWins, Positive) \
	N(LightningGunDeaths, "lightning gun deaths", SmallerWins, Positive) \
	N(LightningGunPickups, "lightning gun pickups", BiggerWins, Positive) \
	N(BFGKills, "bfg kills", BiggerWins, Positive) \
	N(BFGAccuracy, "bfg accuracy", BiggerWins, Percentage) \
	N(BFGShots, "bfg shots", BiggerWins, Positive) \
	N(BFGHits, "bfg hits", BiggerWins, Positive) \
	N(BFGDamage, "bfg damage", BiggerWins, Positive) \
	N(BFGDrops, "bfg drops", SmallerWins, Positive) \
	N(BFGDeaths, "bfg deaths", SmallerWins, Positive) \
	N(BFGPickups, "bfg pickups", BiggerWins, Positive) \
	N(GrapplingHookKills, "grappling hook kills", BiggerWins, Positive) \
	N(GrapplingHookAccuracy, "grappling hook accuracy", BiggerWins, Percentage) \
	N(GrapplingHookShots, "grappling hook shots", BiggerWins, Positive) \
	N(GrapplingHookHits, "grappling hook hits", BiggerWins, Positive) \
	N(GrapplingHookDamage, "grappling hook damage", BiggerWins, Positive) \
	N(GrapplingHookDrops, "grappling hook drops", SmallerWins, Positive) \
	N(GrapplingHookDeaths, "grappling hook deaths", SmallerWins, Positive) \
	N(GrapplingHookPickups, "grappling hook pickups", BiggerWins, Positive) \
	N(NailGunKills, "nailgun kills", BiggerWins, Positive) \
	N(NailGunAccuracy, "nailgun accuracy", BiggerWins, Percentage) \
	N(NailGunShots, "nailgun shots", BiggerWins, Positive) \
	N(NailGunHits, "nailgun hits", BiggerWins, Positive) \
	N(NailGunDamage, "nailgun damage", BiggerWins, Positive) \
	N(NailGunDrops, "nailgun drops", SmallerWins, Positive) \
	N(ChainGunKills, "chaingun kills", BiggerWins, Positive) \
	N(ChainGunAccuracy, "chaingun accuracy", BiggerWins, Percentage) \
	N(ChainGunShots, "chaingun shots", BiggerWins, Positive) \
	N(ChainGunHits, "chaingun hits", BiggerWins, Positive) \
	N(ChainGunDamage, "chaingun damage", BiggerWins, Positive) \
	N(ChainGunDrops, "chaingun drops", SmallerWins, Positive) \
	N(ProximityMineLauncherKills, "proximity mine kills", BiggerWins, Positive) \
	N(ProximityMineLauncherAccuracy, "proximity mine accuracy", BiggerWins, Percentage) \
	N(ProximityMineLauncherShots, "proximity mine shots", BiggerWins, Positive) \
	N(ProximityMineLauncherHits, "proximity mine hits", BiggerWins, Positive) \
	N(ProximityMineLauncherDamage, "proximity mine damage", BiggerWins, Positive) \
	N(ProximityMineLauncherDrops, "proximity mine drops", SmallerWins, Positive) \
	N(HeavyMachineGunKills, "heavy machinegun kills", BiggerWins, Positive) \
	N(HeavyMachineGunAccuracy, "heavy machinegun accuracy", BiggerWins, Percentage) \
	N(HeavyMachineGunShots, "heavy machinegun shots", BiggerWins, Positive) \
	N(HeavyMachineGunHits, "heavy machinegun hits", BiggerWins, Positive) \
	N(HeavyMachineGunDamage, "heavy machinegun damage", BiggerWins, Positive) \
	N(HeavyMachineGunDrops, "heavy machinegun drops", SmallerWins, Positive) \
	N(KnifeKills, "knife kills", BiggerWins, Positive) \
	N(KnifeShots, "knife attacks", BiggerWins, Positive) \
	N(KnifeHits, "knife hits", BiggerWins, Positive) \
	N(KnifeDamage, "knife damage", BiggerWins, Positive) \
	N(KnifeDeaths, "knife deaths", SmallerWins, Positive) \
	N(KnifeHeadshots, "knife headshots", BiggerWins, Positive) \
	N(KnifeAccuracy, "knife accuracy", BiggerWins, Percentage) \
	N(LugerKills, "luger kills", BiggerWins, Positive) \
	N(LugerShots, "luger shots", BiggerWins, Positive) \
	N(LugerHits, "luger hits", BiggerWins, Positive) \
	N(LugerDamage, "luger damage", BiggerWins, Positive) \
	N(LugerDeaths, "luger deaths", SmallerWins, Positive) \
	N(LugerHeadshots, "luger headshots", BiggerWins, Positive) \
	N(LugerAccuracy, "luger accuracy", BiggerWins, Percentage) \
	N(ColtKills, "colt kills", BiggerWins, Positive) \
	N(ColtShots, "colt shots", BiggerWins, Positive) \
	N(ColtHits, "colt hits", BiggerWins, Positive) \
	N(ColtDamage, "colt damage", BiggerWins, Positive) \
	N(ColtDeaths, "colt deaths", SmallerWins, Positive) \
	N(ColtHeadshots, "colt headshots", BiggerWins, Positive) \
	N(ColtAccuracy, "colt accuracy", BiggerWins, Percentage) \
	N(MP40Kills, "MP40 kills", BiggerWins, Positive) \
	N(MP40Shots, "MP40 shots", BiggerWins, Positive) \
	N(MP40Hits, "MP40 hits", BiggerWins, Positive) \
	N(MP40Damage, "MP40 damage", BiggerWins, Positive) \
	N(MP40Deaths, "MP40 deaths", SmallerWins, Positive) \
	N(MP40Headshots, "MP40 headshots", BiggerWins, Positive) \
	N(MP40Accuracy, "MP40 accuracy", BiggerWins, Percentage) \
	N(ThompsonKills, "thompson kills", BiggerWins, Positive) \
	N(ThompsonShots, "thompson shots", BiggerWins, Positive) \
	N(ThompsonHits, "thompson hits", BiggerWins, Positive) \
	N(ThompsonDamage, "thompson damage", BiggerWins, Positive) \
	N(ThompsonDeaths, "thompson deaths", SmallerWins, Positive) \
	N(ThompsonHeadshots, "thompson headshots", BiggerWins, Positive) \
	N(ThompsonAccuracy, "thompson accuracy", BiggerWins, Percentage) \
	N(StenKills, "sten kills", BiggerWins, Positive) \
	N(StenShots, "sten shots", BiggerWins, Positive) \
	N(StenHits, "sten hits", BiggerWins, Positive) \
	N(StenDamage, "sten damage", BiggerWins, Positive) \
	N(StenDeaths, "sten deaths", SmallerWins, Positive) \
	N(StenHeadshots, "sten headshots", BiggerWins, Positive) \
	N(StenAccuracy, "sten accuracy", BiggerWins, Percentage) \
	N(FG42Kills, "FG42 kills", BiggerWins, Positive) \
	N(FG42Shots, "FG42 shots", BiggerWins, Positive) \
	N(FG42Hits, "FG42 hits", BiggerWins, Positive) \
	N(FG42Damage, "FG42 damage", BiggerWins, Positive) \
	N(FG42Deaths, "FG42 deaths", SmallerWins, Positive) \
	N(FG42Headshots, "FG42 headshots", BiggerWins, Positive) \
	N(FG42Accuracy, "FG42 accuracy", BiggerWins, Percentage) \
	N(PanzerfaustKills, "panzerfaust kills", BiggerWins, Positive) \
	N(PanzerfaustShots, "panzerfaust shots", BiggerWins, Positive) \
	N(PanzerfaustHits, "panzerfaust hits", BiggerWins, Positive) \
	N(PanzerfaustDamage, "panzerfaust damage", BiggerWins, Positive) \
	N(PanzerfaustDeaths, "panzerfaust deaths", SmallerWins, Positive) \
	N(PanzerfaustHeadshots, "panzerfaust headshots", BiggerWins, Positive) \
	N(PanzerfaustAccuracy, "panzerfaust accuracy", BiggerWins, Percentage) \
	N(FlamethrowerKills, "flamethrower kills", BiggerWins, Positive) \
	N(FlamethrowerShots, "flamethrower shots", BiggerWins, Positive) \
	N(FlamethrowerHits, "flamethrower hits", BiggerWins, Positive) \
	N(FlamethrowerDamage, "flamethrower damage", BiggerWins, Positive) \
	N(FlamethrowerDeaths, "flamethrower deaths", SmallerWins, Positive) \
	N(FlamethrowerHeadshots, "flamethrower headshots", BiggerWins, Positive) \
	N(FlamethrowerAccuracy, "flamethrower accuracy", BiggerWins, Percentage) \
	N(GrenadeKills, "grenade kills", BiggerWins, Positive) \
	N(GrenadeShots, "grenade shots", BiggerWins, Positive) \
	N(GrenadeHits, "grenade hits", BiggerWins, Positive) \
	N(GrenadeDamage, "grenade damage", BiggerWins, Positive) \
	N(GrenadeDeaths, "grenade deaths", SmallerWins, Positive) \
	N(GrenadeHeadshots, "grenade headshots", BiggerWins, Positive) \
	N(GrenadeAccuracy, "grenade accuracy", BiggerWins, Percentage) \
	N(MortarKills, "mortar kills", BiggerWins, Positive) \
	N(MortarShots, "mortar shots", BiggerWins, Positive) \
	N(MortarHits, "mortar hits", BiggerWins, Positive) \
	N(MortarDamage, "mortar damage", BiggerWins, Positive) \
	N(MortarDeaths, "mortar deaths", SmallerWins, Positive) \
	N(MortarHeadshots, "mortar headshots", BiggerWins, Positive) \
	N(MortarAccuracy, "mortar accuracy", BiggerWins, Percentage) \
	N(DynamiteKills, "dynamite kills", BiggerWins, Positive) \
	N(DynamiteShots, "dynamite shots", BiggerWins, Positive) \
	N(DynamiteHits, "dynamite hits", BiggerWins, Positive) \
	N(DynamiteDamage, "dynamite damage", BiggerWins, Positive) \
	N(DynamiteDeaths, "dynamite deaths", SmallerWins, Positive) \
	N(DynamiteHeadshots, "dynamite headshots", BiggerWins, Positive) \
	N(DynamiteAccuracy, "dynamite accuracy", BiggerWins, Percentage) \
	N(AirstrikeKills, "airstrike kills", BiggerWins, Positive) \
	N(AirstrikeShots, "airstrike shots", BiggerWins, Positive) \
	N(AirstrikeHits, "airstrike hits", BiggerWins, Positive) \
	N(AirstrikeDamage, "airstrike damage", BiggerWins, Positive) \
	N(AirstrikeDeaths, "airstrike deaths", SmallerWins, Positive) \
	N(AirstrikeHeadshots, "airstrike headshots", BiggerWins, Positive) \
	N(AirstrikeAccuracy, "airstrike accuracy", BiggerWins, Percentage) \
	N(ArtilleryKills, "artillery kills", BiggerWins, Positive) \
	N(ArtilleryShots, "artillery shots", BiggerWins, Positive) \
	N(ArtilleryHits, "artillery hits", BiggerWins, Positive) \
	N(ArtilleryDamage, "artillery damage", BiggerWins, Positive) \
	N(ArtilleryDeaths, "artillery deaths", SmallerWins, Positive) \
	N(ArtilleryHeadshots, "artillery headshots", BiggerWins, Positive) \
	N(ArtilleryAccuracy, "artillery accuracy", BiggerWins, Percentage) \
	N(SyringeKills, "syringe kills", BiggerWins, Positive) \
	N(SyringeShots, "syringe shots", BiggerWins, Positive) \
	N(SyringeHits, "syringe hits", BiggerWins, Positive) \
	N(SyringeDamage, "syringe damage", BiggerWins, Positive) \
	N(SyringeDeaths, "syringe deaths", SmallerWins, Positive) \
	N(SyringeHeadshots, "syringe headshots", BiggerWins, Positive) \
	N(SyringeAccuracy, "syringe accuracy", BiggerWins, Percentage) \
	N(SmokeKills, "smoke kills", BiggerWins, Positive) \
	N(SmokeShots, "smoke shots", BiggerWins, Positive) \
	N(SmokeHits, "smoke hits", BiggerWins, Positive) \
	N(SmokeDamage, "smoke damage", BiggerWins, Positive) \
	N(SmokeDeaths, "smoke deaths", SmallerWins, Positive) \
	N(SmokeHeadshots, "smoke headshots", BiggerWins, Positive) \
	N(SmokeAccuracy, "smoke accuracy", BiggerWins, Percentage) \
	N(MG42Kills, "MG42 kills", BiggerWins, Positive) \
	N(MG42Shots, "MG42 shots", BiggerWins, Positive) \
	N(MG42Hits, "MG42 hits", BiggerWins, Positive) \
	N(MG42Damage, "MG42 damage", BiggerWins, Positive) \
	N(MG42Deaths, "MG42 deaths", SmallerWins, Positive) \
	N(MG42Headshots, "MG42 headshots", BiggerWins, Positive) \
	N(MG42Accuracy, "MG42 accuracy", BiggerWins, Percentage) \
	N(RifleKills, "rifle kills", BiggerWins, Positive) \
	N(RifleShots, "rifle shots", BiggerWins, Positive) \
	N(RifleHits, "rifle hits", BiggerWins, Positive) \
	N(RifleDamage, "rifle damage", BiggerWins, Positive) \
	N(RifleDeaths, "rifle deaths", SmallerWins, Positive) \
	N(RifleHeadshots, "rifle headshots", BiggerWins, Positive) \
	N(RifleAccuracy, "rifle accuracy", BiggerWins, Percentage) \
	N(VenomKills, "venom kills", BiggerWins, Positive) \
	N(VenomShots, "venom shots", BiggerWins, Positive) \
	N(VenomHits, "venom hits", BiggerWins, Positive) \
	N(VenomDamage, "venom damage", BiggerWins, Positive) \
	N(VenomDeaths, "venom deaths", SmallerWins, Positive) \
	N(VenomHeadshots, "venom headshots", BiggerWins, Positive) \
	N(VenomAccuracy, "venom accuracy", BiggerWins, Percentage)

#define UDT_PLAYER_STATS_ITEM(Enum, Desc, Comp, Type) Enum,
struct udtPlayerStatsField
{
	enum Id
	{
		UDT_PLAYER_STATS_LIST(UDT_PLAYER_STATS_ITEM)
		Count
	};
};
#undef UDT_PLAYER_STATS_ITEM

#define UDT_TEAM_STATS_LIST(N) \
	N(Score, "score", BiggerWins, Generic) \
	N(RedArmorPickups, "red armor pickups", BiggerWins, Positive) \
	N(YellowArmorPickups, "yellow armor pickups", BiggerWins, Positive) \
	N(GreenArmorPickups, "green armor pickups", BiggerWins, Positive) \
	N(MegaHealthPickups, "mega health pickups", BiggerWins, Positive) \
	N(QuadDamagePickups, "quad damage pickups", BiggerWins, Positive) \
	N(BattleSuitPickups, "battle suit pickups", BiggerWins, Positive) \
	N(RegenPickups, "regeneration pickups", BiggerWins, Positive) \
	N(HastePickups, "haste pickups", BiggerWins, Positive) \
	N(InvisPickups, "invisibility pickups", BiggerWins, Positive) \
	N(FlagPickups, "flag pickups", BiggerWins, Positive) \
	N(FlagTime, "flag possession time", BiggerWins, Seconds) \
	N(MedkitPickups, "medkit pickups", BiggerWins, Positive) \
	N(QuadDamageTime, "quad damage possession time", BiggerWins, Seconds) \
	N(BattleSuitTime, "battle suit possession time", BiggerWins, Seconds) \
	N(RegenTime, "regeneration possession time", BiggerWins, Seconds) \
	N(HasteTime, "haste possession time", BiggerWins, Seconds) \
	N(InvisTime, "invisibility possession time", BiggerWins, Seconds) \
	N(Kills, "kills", BiggerWins, Positive) \
	N(Deaths, "deaths", SmallerWins, Positive) \
	N(Suicides, "suicides", SmallerWins, Positive) \
	N(DamageGiven, "damage given", BiggerWins, Positive) \
	N(DamageReceived, "damage received", SmallerWins, Positive) \
	N(TeamKills, "team kills", SmallerWins, Positive) \
	N(TeamDamage, "team damage", SmallerWins, Positive) \
	N(Captures, "captures", BiggerWins, Positive) \
	N(Defends, "defends", BiggerWins, Positive) \
	N(Assists, "assists", BiggerWins, Positive) \
	N(Returns, "returns", BiggerWins, Positive) \
	N(ShotgunPickups, "shotgun pickups", BiggerWins, Positive) \
	N(GrenadeLauncherPickups, "grenade launcher pickups", BiggerWins, Positive) \
	N(RocketLauncherPickups, "rocket launcher pickups", BiggerWins, Positive) \
	N(PlasmaGunPickups, "plasma gun pickups", BiggerWins, Positive) \
	N(RailgunPickups, "railgun pickups", BiggerWins, Positive) \
	N(LightningGunPickups, "lightning gun pickups", BiggerWins, Positive) \
	N(Efficiency, "efficiency", BiggerWins, Positive) \
	N(GibbedBodies, "gibbed bodies", BiggerWins, Positive) \
	N(Headshots, "headshots", BiggerWins, Positive) \
	N(Revives, "revives", BiggerWins, Positive) \
	N(Accuracy, "accuracy", BiggerWins, Percentage)

#define UDT_TEAM_STATS_ITEM(Enum, Desc, Comp, Type) Enum,
struct udtTeamStatsField
{
	enum Id
	{
		UDT_TEAM_STATS_LIST(UDT_TEAM_STATS_ITEM)
		Count
	};
};
#undef UDT_TEAM_STATS_ITEM

struct udtGameTypeMask
{
	enum Id
	{
		None = 0,
		Team = UDT_BIT(0),
		RoundBased = UDT_BIT(1),
		HasCaptureLimit = UDT_BIT(2),
		HasFragLimit = UDT_BIT(3),
		HasScoreLimit = UDT_BIT(4),
		HasRoundLimit = UDT_BIT(5)
	};
};

/* @TODO: investigate obelisk harvester domination checkpoint capture-and-hold */
/* The first team mode is always TDM. */
#define UDT_GAME_TYPE_LIST(N) \
	N(SP, "SP", "Single Player", udtGameTypeMask::HasFragLimit) \
	N(FFA, "FFA", "Free for All", udtGameTypeMask::HasFragLimit) \
	N(Duel, "1v1", "Duel", udtGameTypeMask::HasFragLimit) \
	N(Race, "race", "Race", udtGameTypeMask::None) \
	N(HM, "HM", "HoonyMode", udtGameTypeMask::HasRoundLimit | udtGameTypeMask::RoundBased) \
	N(RedRover, "RR", "Red Rover", udtGameTypeMask::HasRoundLimit | udtGameTypeMask::RoundBased) \
	N(TDM, "TDM", "Team DeathMatch", udtGameTypeMask::HasFragLimit | udtGameTypeMask::Team) \
	N(CBTDM, "CBTDM", "ClanBase Team DeathMatch", udtGameTypeMask::HasFragLimit | udtGameTypeMask::Team) \
	N(CA, "CA", "Clan Arena", udtGameTypeMask::HasRoundLimit | udtGameTypeMask::Team | udtGameTypeMask::RoundBased) \
	N(CTF, "CTF", "Capture The Flag", udtGameTypeMask::Team | udtGameTypeMask::HasCaptureLimit) \
	N(OneFlagCTF, "1FCTF", "One Flag CTF", udtGameTypeMask::Team | udtGameTypeMask::HasCaptureLimit) \
	N(Obelisk, "OB", "Obelisk", udtGameTypeMask::HasScoreLimit | udtGameTypeMask::Team) \
	N(Harvester, "HAR", "Harvester", udtGameTypeMask::HasScoreLimit | udtGameTypeMask::Team) \
	N(Domination, "DOM", "Domination", udtGameTypeMask::HasScoreLimit | udtGameTypeMask::Team) \
	N(CTFS, "CTFS", "Capture Strike", udtGameTypeMask::HasScoreLimit | udtGameTypeMask::Team | udtGameTypeMask::RoundBased) \
	N(NTF, "NTF", "Not Team Fortress", udtGameTypeMask::Team | udtGameTypeMask::HasCaptureLimit) \
	N(TwoVsTwo, "2v2", "2v2 TDM", udtGameTypeMask::HasFragLimit | udtGameTypeMask::Team) \
	N(FT, "FT", "Freeze Tag", udtGameTypeMask::HasRoundLimit | udtGameTypeMask::Team | udtGameTypeMask::RoundBased) \
	N(Wolf_Objective, "MP", "Objective", udtGameTypeMask::Team) \
	N(Wolf_Stopwatch, "SW", "Stopwatch", udtGameTypeMask::Team | udtGameTypeMask::RoundBased) \
	N(Wolf_Checkpoint, "CP", "Checkpoint", udtGameTypeMask::Team) \
	N(Wolf_CaptureAndHold, "CPH", "Capture and Hold", udtGameTypeMask::Team)
	
#define UDT_GAME_TYPE_ITEM(Enum, ShortDesc, Desc, Flags) Enum,
struct udtGameType
{
	enum Id
	{
		UDT_GAME_TYPE_LIST(UDT_GAME_TYPE_ITEM)
		Count,
		Invalid,
		FirstTeamMode = TDM
	};
};
#undef UDT_GAME_TYPE_ITEM

#define UDT_MOD_NAME_LIST(N) \
	N(None, "None") \
	N(CPMA, "CPMA") \
	N(OSP, "OSP") \
	N(Defrag, "DeFRaG") \
	N(RTCWPro, "RtcwPro") \
	N(RTCWOSP, "OSP") \
	N(Unknown, "Unknown")

#define UDT_MOD_NAME_ITEM(Enum, Name) Enum,
struct udtMod
{
	enum Id
	{
		UDT_MOD_NAME_LIST(UDT_MOD_NAME_ITEM)
		Count
	};
};
#undef UDT_MOD_NAME_ITEM

#define UDT_GAMEPLAY_LIST(N) \
	N(VQ3,   "VQ3",   "Vanilla Quake 3") \
	N(CQ3,   "CQ3",   "Challenge Quake 3") \
	N(PMC,   "PMC",   "Classic ProMode") \
	N(CPM,   "CPM",   "ProMode") \
	N(PMD,   "PMD",   "ProMode DEV") \
	N(CQL,   "VQL",   "Classic Quake Live") \
	N(PQL,   "PQL",   "Turbo Quake Live") \
	N(DQL,   "QL",    "Default Quake Live") \
	N(VRTCW, "VRTCW", "Vanilla Return to Castle Wolfenstein") \
	N(RTCWPRO, "RtcwPro", "Return to Castle Wolfenstein RtcwPro") \
	N(RTCWOSP, "RtCW OSP", "Return to Castle Wolfenstein OSP")

#define UDT_GAMEPLAY_ITEM(Enum, ShortName, LongName) Enum,
struct udtGamePlay
{
	enum Id
	{
		UDT_GAMEPLAY_LIST(UDT_GAMEPLAY_ITEM)
		Count
	};
};
#undef UDT_GAMEPLAY_ITEM

#define UDT_OVERTIME_TYPE_LIST(N) \
	N(None, "None") \
	N(Timed, "Timed") \
	N(SuddenDeath, "Sudden Death")

#define UDT_OVERTIME_TYPE_ITEM(Enum, Desc) Enum,
struct udtOvertimeType
{
	enum Id
	{
		UDT_OVERTIME_TYPE_LIST(UDT_OVERTIME_TYPE_ITEM)
		Count
	};
};
#undef UDT_OVERTIME_TYPE_ITEM

struct udtPerfStatsDataType
{
	enum Id
	{
		Generic,    /* Format as a normal unsigned integer. */
		Bytes,      /* Data size, in bytes. */
		Throughput, /* Data throughput, in bytes/second. */
		Duration,   /* Duration in micro-seconds. */
		Percentage, /* Percentage multiplied by 10. */
		Count
	};
};

#define UDT_PERF_STATS_LIST(N) \
	N(Duration, "duration", Duration) \
	N(DataProcessed, "data processed", Bytes) \
	N(DataThroughput, "data throughput", Throughput) \
	N(ThreadCount, "thread count", Generic) \
	N(AllocatorCount, "allocator count", Generic) \
	N(MemoryReserved, "memory reserved", Bytes) \
	N(MemoryCommitted, "memory committed", Bytes) \
	N(MemoryUsed, "memory used", Bytes) \
	N(MemoryEfficiency, "memory usage efficiency", Percentage) \
	N(ResizeCount, "buffer relocation count", Generic)

#define UDT_PERF_STATS_ITEM(Enum, Desc, Type) Enum,
struct udtPerfStatsField
{
	enum Id
	{
		UDT_PERF_STATS_LIST(UDT_PERF_STATS_ITEM)
		Count
	};
};
#undef UDT_PERF_STATS_ITEM

#endif


#define    UDT_MAX_MERGE_DEMO_COUNT             8
#define    UDT_TEAM_STATS_MASK_BYTE_COUNT       8
#define    UDT_PLAYER_STATS_MASK_BYTE_COUNT    40


#if defined(__cplusplus)
#	define UDT_ENFORCE_API_STRUCT_SIZE(x) static_assert(sizeof(x) % 8 == 0, "Invalid struct size!");
#else
#	define UDT_ENFORCE_API_STRUCT_SIZE(x) 
#endif


#ifdef __cplusplus
extern "C" 
{
#endif
	
	/* "userData" is the member variable udtParseArg::ProgressContext that you pass to API functions. */
	typedef void (*udtProgressCallback)(f32 progress, void* userData);

	/* Log levels: 0=info, 1=warning, 2=error, 3=error+crash. */
	typedef void (*udtMessageCallback)(s32 logLevel, const char* message);

	/* Called when UDT can't recover from the error. */
	/* Implement to throw an exception, generate a stack trace, etc. */
	/* Default behavior: calls the C function exit. */
	typedef void (*udtCrashCallback)(const char* message);

	/* Returns the protocol version of the file pointed to by "filePath". */
	/* The return value is of type udtProtocol::Id. */
	typedef u32 (*udtProtocolCallback)(const char* filePath);

#pragma pack(push, 1)

#if defined(__cplusplus)
	struct udtParseArgFlag
	{
		enum Id
		{
		};
	};
#endif
	
	typedef struct udtParseArg_s
	{
		/* Pointer to an array of plug-ins IDs. */
		/* Of type udtParserPlugIn::Id. */
		/* May be NULL. */
		/* Unused when cutting. */
		const u32* PlugIns;

		/* May be NULL. */
		/* Unused when not cutting. */
		const char* OutputFolderPath;

		/* May be NULL. */
		/* Used for info, warning and non-fatal error messages. */
		/* Fatal errors are routed through a udtCrashCallback callback. */
		udtMessageCallback MessageCb;

		/* May be NULL. */
		udtProgressCallback ProgressCb;

		/* May be NULL */
		/* When not specified, udtGetProtocolByFilePath is used instead. */
		/* The return value is of type udtProtocol::Id. */
		udtProtocolCallback ProtocolCb;

		/* May be NULL. */
		/* This is passed as "userData" to "ProgressCb". */
		void* ProgressContext;

		/* May be NULL. */
		/* Zero to proceed, non-zero to cancel the current operation. */
		const s32* CancelOperation;

		/* May be NULL. */
		/* The array size should be udtPerfStatsField::Count. */
		u64* PerformanceStats;

		/* Number of elements in the array pointed to by the PlugIns pointer. */
		/* May be 0. */
		/* Unused when cutting. */
		u32 PlugInCount;

		/* The index of the game state that will be read when starting at offset FileOffset. */
		/* Unused in batch operations. */
		s32 GameStateIndex;

		/* The offset, in bytes, at which to start reading from the file. */
		/* Unused in batch operations. */
		u32 FileOffset;

		/* See udtParseArgFlag::Id. */
		u32 Flags;

		/* Minimum duration, in milli-seconds, between 2 consecutive calls to ProgressCb. */
		u32 MinProgressTimeMs;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtParseArg;
	UDT_ENFORCE_API_STRUCT_SIZE(udtParseArg)

	typedef struct udtMultiParseArg_s
	{
		/* Pointer to an array of file paths. */
		const char** FilePaths;

		/* Pointer to an array of returned error codes. */
		s32* OutputErrorCodes;

		/* Number of elements in the arrays pointed by FilePaths and OutputErrorCodes. */
		u32 FileCount;

		/* The maximum amount of threads that should be used to process the demos. */
		u32 MaxThreadCount;
	}
	udtMultiParseArg;
	UDT_ENFORCE_API_STRUCT_SIZE(udtMultiParseArg)
	
	typedef struct udtCut_s
	{
		/* Output file path. */
		/* May be NULL, in which case udtParseArg::OutputFolderPath is used. */
		const char* FilePath;

		/* Ignore this. */
		const void* Reserved1;

		/* Cut start time in milli-seconds. */
		s32 StartTimeMs;

		/* Cut end time in milli-seconds. */
		s32 EndTimeMs;

		/* The game state index for which this cut is applied */
		s32 GameStateIndex;

		/* Ignore this. */
		s32 Reserved2;
	}
	udtCut;
	UDT_ENFORCE_API_STRUCT_SIZE(udtCut)

	typedef struct udtPatternInfo_s
	{
		/* Pointer to the data structure describing the patterns/filters. */
		/* May not be NULL. */
		const void* TypeSpecificInfo;

		/* Ignore this. */
		const void* Reserved1;

		/* Of type udtPatternType::Id. */
		u32 Type;

		/* Ignore this. */
		s32 Reserved2;
	}
	udtPatternInfo;
	UDT_ENFORCE_API_STRUCT_SIZE(udtPatternInfo)

#if defined(__cplusplus)

	struct udtPlayerIndex
	{
		enum Id
		{
			FirstPersonPlayer = -2,
			DemoTaker = -1
		};
	};

	struct udtPatternSearchArgMask
	{
		enum Id
		{
			MergeCutSections = UDT_BIT(0) /* Enable/disable merging cut sections from different patterns. */
		};
	};

	struct udtStringComparisonMode
	{
		enum Id
		{
			Equals,
			Contains,
			StartsWith,
			EndsWith,
			Count
		};
	};

	struct udtStringMatchingRuleMask
	{
		enum Id
		{
			CaseSensitive = UDT_BIT(0),
			IgnoreColorCodes = UDT_BIT(1)
		};
	};

#endif

	typedef struct udtStringMatchingRule_s
	{
		/* A null-terminated string containing the player's name. */
		/* May not be NULL. */
		const char* Value;

		/* Ignore this. */
		const void* Reserved1;

		/* Of type udtStringComparisonMode::Id. */
		u32 ComparisonMode;

		/* See udtStringMatchingRuleMask::Id. */
		u32 Flags;
	}
	udtStringMatchingRule;
	UDT_ENFORCE_API_STRUCT_SIZE(udtStringMatchingRule)

	typedef struct udtPatternSearchArg_s
	{
		/* If the player name rules are valid, they will be used to find the player to track. */
		/* Otherwise, the PlayerIndex field will be used instead. */

		/* Pointer to an array of filters. */
		/* May not be NULL. */
		const udtPatternInfo* Patterns;

		/* Pointer to an array of player name matching rules. */
		/* The player that will be tracked will be the first one to match any of the rules. */
		/* May be NULL. */
		const udtStringMatchingRule* PlayerNameRules;

		/* Number of elements in the array pointed by Patterns. */
		u32 PatternCount;

		/* Length of the PlayerNameRules array. */
		u32 PlayerNameRuleCount;

		/* Negative offset from the first matching time, in seconds. */
		u32 StartOffsetSec;

		/* Positive offset from the last matching time, in seconds. */
		u32 EndOffsetSec;

		/* The index of the player whose action we're tracking. */
		/* If in the [0;63]  range: the number will be used directly. */
		/* If in the [-2;-1] range: is of type udtPlayerIndex::Id. */
		/* Ignored if player name rules are properly defined. */
		s32 PlayerIndex;

		/* See udtPatternSearchArgMask::Id. */
		u32 Flags;
	}
	udtPatternSearchArg;
	UDT_ENFORCE_API_STRUCT_SIZE(udtPatternSearchArg)

	typedef struct udtPatternMatch_s
	{
		/* Index into the input demo file path array. */
		u32 DemoInputIndex;

		/* The index of the game state. */
		u32 GameStateIndex;

		/* Server time, in milli-seconds. */
		s32 StartTimeMs;

		/* Server time, in milli-seconds. */
		s32 EndTimeMs;

		/* A bit is set for every pattern that matched this result. */
		/* The bits are indexed with udtPatternType::Id. */
		u32 Patterns;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtPatternMatch;
	UDT_ENFORCE_API_STRUCT_SIZE(udtPatternMatch)

	typedef struct udtPatternSearchResults_s
	{
		/* Pointer to the array of results. */
		const udtPatternMatch* Matches;

		/* Ignore this. */
		const void* Reserved1;

		/* Length of the Matches array. */
		u32 MatchCount;

		/* Ignore this. */
		s32 Reserved2;
	}
	udtPatternSearchResults;
	UDT_ENFORCE_API_STRUCT_SIZE(udtPatternSearchResults)

	typedef struct udtCutByTimeArg_s
	{
		/* Pointer to an array of cut times. */
		/* May not be NULL. */
		const udtCut* Cuts;

		/* Ignore this. */
		const void* Reserved1;

		/* Number of elements in the array pointed by Cuts. */
		u32 CutCount;

		/* Ignore this. */
		s32 Reserved2;
	}
	udtCutByTimeArg;
	UDT_ENFORCE_API_STRUCT_SIZE(udtCutByTimeArg)

	typedef struct udtChatPatternRule_s
	{
		/* May not be NULL. */
		const char* Pattern;

		/* Ignore this. */
		const void* Reserved1;

		/* Of type udtChatOperator::Id. */
		u32 ChatOperator;

		/* Non-zero means case-sensitive. */
		u32 CaseSensitive;

		/* Non-zero means color codes are ignored. */
		u32 IgnoreColorCodes;

		/* Non-zero means we search team chat messages too. */
		u32 SearchTeamChat;
	}
	udtChatPatternRule;
	UDT_ENFORCE_API_STRUCT_SIZE(udtChatPatternRule)

	/* Used as udtPatternInfo::TypeSpecificInfo */
	/* when udtPatternInfo::Type is udtPatternType::GlobalChat. */
	typedef struct udtChatPatternArg_s
	{
		/* Pointer to an array of chat cutting rules. */
		/* Rules are OR'd together. */
		/* May not be NULL. */
		const udtChatPatternRule* Rules;

		/* Ignore this. */
		const void* Reserved1;

		/* Number of elements in the array pointed to by the Rules pointer. */
		/* May not be 0. */
		u32 RuleCount;

		/* Ignore this. */
		s32 Reserved2;
	}
	udtChatPatternArg;
	UDT_ENFORCE_API_STRUCT_SIZE(udtChatPatternArg)

#if defined(__cplusplus)
	struct udtFragRunPatternArgMask
	{
		enum Id
		{
			AllowSelfKills = UDT_BIT(0),
			AllowTeamKills = UDT_BIT(1),
			AllowDeaths    = UDT_BIT(2)
		};
	};
#endif

	/* Used as udtPatternInfo::TypeSpecificInfo */
	/* when udtPatternInfo::Type is udtPatternType::FragSequences. */
	typedef struct udtFragRunPatternArg_s
	{
		/* All the allowed means of death. */
		/* See udtPlayerMeanOfDeath. */
		u64 AllowedMeansOfDeaths;

		/* Ignore this. */
		u64 Reserved1;

		/* The minimum amount of frags in a sequence. */
		u32 MinFragCount;

		/* Maximum time interval between 2 consecutive frags, in seconds. */
		/* See TimeMode for the interpretation of this value. */
		u32 TimeBetweenFragsSec;

		/* @TODO: Not supported for now. */
		/* If 0, TimeBetweenFragsSec is the maximum time interval between  */
		/* 2 consecutive frags, in seconds. */
		/* If 1, TimeBetweenFragsSec is the maximum average time between frags */
		/* for the entire frag run, in seconds. */
		u32 TimeMode;

		/* Boolean options. */
		/* See udtFragRunPatternArgMask. */
		u32 Flags;
	}
	udtFragRunPatternArg;
	UDT_ENFORCE_API_STRUCT_SIZE(udtFragRunPatternArg)

	/* Used as udtPatternInfo::TypeSpecificInfo */
	/* when udtPatternInfo::Type is udtPatternType::MidAirFrags. */
	typedef struct udtMidAirPatternArg_s
	{
		/* All the allowed weapons. */
		/* See udtWeaponBits::Id. */
		u32 AllowedWeapons;

		/* The minimum distance between the projectile's  */
		/* start (where the weapon was fired) and end (position of impact) points. */
		u32 MinDistance;

		/* The minimum time the victim was in the air prior to the hit.  */
		u32 MinAirTimeMs;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtMidAirPatternArg;
	UDT_ENFORCE_API_STRUCT_SIZE(udtMidAirPatternArg)

	/* Used as udtPatternInfo::TypeSpecificInfo */
	/* when udtPatternInfo::Type is udtPatternType::MultiRailFrags. */
	typedef struct udtMultiRailPatternArg_s
	{
		/* The minimum amount of kills with a single rail shot. */
		/* Must be 2 or greater. */
		u32 MinKillCount;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtMultiRailPatternArg;
	UDT_ENFORCE_API_STRUCT_SIZE(udtMultiRailPatternArg)

	/* Used as udtPatternInfo::TypeSpecificInfo */
	/* when udtPatternInfo::Type is udtPatternType::FlagCaptures. */
	typedef struct udtFlagCapturePatternArg_s
	{
		/* Minimum allowed flag carry time, in milli-seconds. */
		u32 MinCarryTimeMs;

		/* Maximum allowed flag carry time, in milli-seconds. */
		u32 MaxCarryTimeMs;

		/* Non-zero to allow pick-ups from the original flag spot. */
		u32 AllowBaseToBase;

		/* Non-zero to allow pick-ups that are not from the original flag spot. */
		u32 AllowMissingToBase;
	}
	udtFlagCapturePatternArg;
	UDT_ENFORCE_API_STRUCT_SIZE(udtFlagCapturePatternArg)
	
	/* Used as udtPatternInfo::TypeSpecificInfo */
	/* when udtPatternInfo::Type is udtPatternType::FlickRailFrags. */
	typedef struct udtFlickRailPatternArg_s
	{
		/* Minimum angular velocity, in radians/second. */
		f32 MinSpeed;

		/* How many snapshots to take into account for computing the top speed. */
		/* Range: [2;4]. */
		u32 MinSpeedSnapshotCount;

		/* Minimum angle change, in radians. */
		f32 MinAngleDelta;

		/* How many snapshots to take into account for computing the angle difference. */
		/* Range: [2;4]. */
		u32 MinAngleDeltaSnapshotCount;
	}
	udtFlickRailPatternArg;
	UDT_ENFORCE_API_STRUCT_SIZE(udtFlickRailPatternArg)

	/* Used as udtPatternInfo::TypeSpecificInfo */
	/* when udtPatternInfo::Type is udtPatternType::Matches. */
	typedef struct udtMatchPatternArg_s
	{
		/* If no match count-down was found, */
		/* start the cut this amount of time before the match's start. */
		u32 MatchStartOffsetMs;

		/* If no post-match intermission is found, */
		/* end the cut this amount of time after the match's end. */
		u32 MatchEndOffsetMs;
	}
	udtMatchPatternArg;
	UDT_ENFORCE_API_STRUCT_SIZE(udtMatchPatternArg)

	typedef struct udtProtocolConversionArg_s
	{
		/* Of type udtProtocol::Id. */
		u32 OutputProtocol;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtProtocolConversionArg;
	UDT_ENFORCE_API_STRUCT_SIZE(udtProtocolConversionArg)

	/* Used when extracting analysis data. */
	typedef struct udtParseDataBufferRange_s
	{
		/* Index of the first item in the buffer. */
		u32 FirstIndex;

		/* The number of items starting at index FirstIndex. */
		u32 Count;
	}
	udtParseDataBufferRange;
	UDT_ENFORCE_API_STRUCT_SIZE(udtParseDataBufferRange)

	typedef struct udtChatEventData_s
	{
		/* String offset. The original, unmodified command string. */
		u32 OriginalCommand;

		/* String length. */
		u32 OriginalCommandLength;

		/* String offset. The player's active clan name at the time the demo was recorded. */
		/* Not available in protocol version 68. */
		u32 ClanName;

		/* String length. */
		u32 ClanNameLength;

		/* String offset. The player's name. */
		u32 PlayerName;

		/* String length. */
		u32 PlayerNameLength;

		/* String offset. The message itself. */
		u32 Message;

		/* String length. */
		u32 MessageLength;

		/* String offset. For team messages, where the player was. */
		u32 Location;

		/* String length. */
		u32 LocationLength;
	}
	udtChatEventData;
	UDT_ENFORCE_API_STRUCT_SIZE(udtChatEventData)

	typedef struct udtParseDataChat_s
	{
		/* String data for this chat message. */
		/* Index 0 with color codes, 1 without. */
		udtChatEventData Strings[2];

		/* The time at which the chat message was sent from the client. */
		s32 ServerTimeMs;

		/* The index of the player who sent the message. */
		/* Not available in protocol version 68. */
		/* Negative if not available. */
		/* If available, in range [0;63]. */
		s32 PlayerIndex;

		/* The index of the last gamestate message after which this chat event occurred. */
		/* Negative if invalid or not available. */
		s32 GameStateIndex;

		/* Non-zero if it's a team message. */
		u32 TeamMessage;
	}
	udtParseDataChat;
	UDT_ENFORCE_API_STRUCT_SIZE(udtParseDataChat)

	/* Complete chat data for all demos in a context. */
	typedef struct udtParseDataChatBuffers_s
	{
		/* The chat messages. */
		const udtParseDataChat* ChatMessages;

		/* Array length: the context' demo count. */
		/* For a demo index, tells you which indices of ChatMessages to use. */
		const udtParseDataBufferRange* ChatMessageRanges;

		/* Pointer to a buffer containing all UTF-8 strings. */
		const u8* StringBuffer;

		/* Ignore this. */
		const void* Reserved1;

		/* The length of the ChatMessages array. */
		u32 ChatMessageCount;

		/* The byte count of the StringBuffer. */
		u32 StringBufferSize;
	}
	udtParseDataChatBuffers;
	UDT_ENFORCE_API_STRUCT_SIZE(udtParseDataChatBuffers)

	typedef struct udtMatchInfo_s
	{
		/* The time between the warm-up end and the match start is the countdown phase, */
		/* whose length might vary depending on the game, mod, mode, etc. */

		/* The time the warm-up ends (countdown start), in milli-seconds. */
		/* S32_MIN if not available. */
		s32 WarmUpEndTimeMs; 

		/* The time the match starts (countdown end), in milli-seconds. */
		/* S32_MIN if not available. */
		s32 MatchStartTimeMs;

		/* The time the match ends, in milli-seconds. */
		/* S32_MIN if not available. */
		s32 MatchEndTimeMs;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtMatchInfo;
	UDT_ENFORCE_API_STRUCT_SIZE(udtMatchInfo)

	typedef struct udtGameStateKeyValuePair_s
	{
		/* String offset. The name of the config string variable. */
		u32 Name;

		/* String length. */
		u32 NameLength;

		/* String offset. The value of the config string variable. */
		u32 Value;

		/* String length. */
		u32 ValueLength;
	}
	udtGameStateKeyValuePair;
	UDT_ENFORCE_API_STRUCT_SIZE(udtGameStateKeyValuePair)

	typedef struct udtGameStatePlayerInfo_s
	{
		/* String offset. The player's name without color codes. */
		/* If QL, the only name. */
		/* If Q3, may have renamed later. */
		u32 FirstName;

		/* String length. */
		u32 FirstNameLength;

		/* The client number. */
		/* Range: [0;63]. */
		s32 Index;

		/* Time of the first snapshot, in milli-seconds. */
		s32 FirstSnapshotTimeMs;

		/* Time of the last snapshot, in milli-seconds. */
		s32 LastSnapshotTimeMs;

		/* Index of the team the player started with. */
		/* Of type udtTeam::Id. */
		u32 FirstTeam;
	}
	udtGameStatePlayerInfo;
	UDT_ENFORCE_API_STRUCT_SIZE(udtGameStatePlayerInfo)

	typedef struct udtParseDataGameState_s
	{
		/* String offset. Name of the player who recorded the demo without color codes. */
		u32 DemoTakerName;

		/* String length. */
		u32 DemoTakerNameLength;

		/* The index of the first match in udtParseDataGameStateBuffers::Matches. */
		u32 FirstMatchIndex;

		/* The match count for this gamestate. */
		u32 MatchCount;

		/* The index of the first pair in udtParseDataGameStateBuffers::KeyValuePairs. */
		u32 FirstKeyValuePairIndex;

		/* The key/value pair count for this gamestate. */
		u32 KeyValuePairCount;

		/* The index of the first player in udtParseDataGameStateBuffers::Players. */
		u32 FirstPlayerIndex;

		/* The player count for this gamestate. */
		u32 PlayerCount;

		/* Index the player who recorded the demo. */
		/* Range: [0;63]. */
		s32 DemoTakerPlayerIndex;

		/* File offset, in bytes, where the "gamestate" message is. */
		u32 FileOffset;

		/* Time of the first snapshot, in milli-seconds. */
		s32 FirstSnapshotTimeMs;

		/* Time of the last snapshot, in milli-seconds. */
		s32 LastSnapshotTimeMs;
	}
	udtParseDataGameState;
	UDT_ENFORCE_API_STRUCT_SIZE(udtParseDataGameState)

	/* Complete gamestate data for all demos in a context. */
	typedef struct udtParseDataGameStateBuffers_s
	{
		/* The gamestate descriptors. */
		const udtParseDataGameState* GameStates;

		/* Array length: the context' demo count. */
		/* For a demo index, tells you which indices of GameStates to use. */
		const udtParseDataBufferRange* GameStateRanges;

		/* The match descriptors. */
		const udtMatchInfo* Matches;

		/* The string key/value pairs from gamestate config strings 0 and 1. */
		const udtGameStateKeyValuePair* KeyValuePairs;

		/* The player descriptors. */
		const udtGameStatePlayerInfo* Players;

		/* Pointer to a buffer containing all UTF-8 strings. */
		const u8* StringBuffer;

		/* The length of the GameStates array. */
		u32 GameStateCount;

		/* The length of the Matches array. */
		u32 MatchCount;

		/* The length of the KeyValuePairs array. */
		u32 KeyValuePairCount;

		/* The length of the Players array. */
		u32 PlayerCount;

		/* The byte count of the StringBuffer. */
		u32 StringBufferSize;

		/* Ignore this. */
		u32 Reserved1;
	}
	udtParseDataGameStateBuffers;
	UDT_ENFORCE_API_STRUCT_SIZE(udtParseDataGameStateBuffers)

	typedef struct udtParseDataObituary_s
	{
		/* String offset. The name of the attacker or another string. */
		u32 AttackerName;

		/* String length. */
		u32 AttackerNameLength;

		/* String offset. The name of the attacker or another string. */
		u32 TargetName;

		/* String length. */
		u32 TargetNameLength;

		/* String offset. The name of the attacker or another string. */
		u32 MeanOfDeathName;

		/* String length. */
		u32 MeanOfDeathNameLength;

		/* The index of the last gamestate message after which this death event occurred. */
		/* Negative if invalid or not available. */
		s32 GameStateIndex;

		/* The time at which the death happened. */
		s32 ServerTimeMs;

		/* The index of the attacking player. */
		/* If available, in range [0;63]. */
		s32 AttackerIdx;

		/* The index of the player who died. */
		/* If available, in range [0;63]. */
		s32 TargetIdx;

		/* The way the target died. */
		/* Of type udtMeanOfDeath::Id. */
		s32 MeanOfDeath;

		/* The index of the attacker's team. */
		/* Of type udtTeam::Id. */
		/* Negative if not available. */
		s32 AttackerTeamIdx;

		/* The index of the target's team. */
		/* Of type udtTeam::Id. */
		/* Negative if not available. */
		s32 TargetTeamIdx;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtParseDataObituary;
	UDT_ENFORCE_API_STRUCT_SIZE(udtParseDataObituary)

	/* Complete obituary data for all demos in a context. */
	typedef struct udtParseDataObituaryBuffers_s
	{
		/* The obituary descriptors. */
		const udtParseDataObituary* Obituaries;

		/* Array length: the context' demo count. */
		/* For a demo index, tells you which indices of Obituaries to use. */
		const udtParseDataBufferRange* ObituaryRanges;

		/* Pointer to a buffer containing all UTF-8 strings. */
		const u8* StringBuffer;

		/* Ignore this. */
		const void* Reserved1;

		/* The length of the Obituaries array. */
		u32 ObituaryCount;

		/* The byte count of the StringBuffer. */
		u32 StringBufferSize;
	}
	udtParseDataObituaryBuffers;
	UDT_ENFORCE_API_STRUCT_SIZE(udtParseDataObituaryBuffers)

	typedef struct udtPlayerStats_s
	{
		/* String offset. The player's name at the time the stats were given by the server. */
		/* May be invalid. */
		u32 Name;

		/* String length. */
		u32 NameLength;

		/* String offset. The player's name at the time the stats were given by the server. */
		/* This version has color codes stripped out for clarity. */
		/* May be invalid. */
		u32 CleanName;

		/* String length. */
		u32 CleanNameLength;
	}
	udtPlayerStats;
	UDT_ENFORCE_API_STRUCT_SIZE(udtPlayerStats)

	typedef struct udtParseDataStats_s
	{
		/* A bit mask describing which teams are valid (1 is red, 2 is blue). */
		u64 ValidTeams;

		/* A bit mask describing which players are valid. */
		u64 ValidPlayers;

		/* String offset. */
		u32 ModVersion;

		/* String length. */
		u32 ModVersionLength;

		/* String offset. */
		u32 MapName;

		/* String length. */
		u32 MapNameLength;

		/* String offset. Name of the first place player or team name. */
		u32 FirstPlaceName;

		/* String length. */
		u32 FirstPlaceNameLength;

		/* String offset. Name of the second place player or team name. */
		u32 SecondPlaceName;

		/* String length. */
		u32 SecondPlaceNameLength;

		/* String offset. Custom red team name. */
		u32 CustomRedName;

		/* String length. */
		u32 CustomRedNameLength;

		/* String offset. Custom blue team name. */
		u32 CustomBlueName;

		/* String length. */
		u32 CustomBlueNameLength;

		/* The index of the first time-out in udtParseDataStatsBuffers::TimeOutStartAndEndTimes. */
		/* It is the index into the s32 integers pair array, not the s32 integers array. */
		u32 FirstTimeOutRangeIndex;

		/* The index of the first team flag in udtParseDataStatsBuffers::TeamFlags. */
		u32 FirstTeamFlagIndex;

		/* The index of the first player flag in udtParseDataStatsBuffers::PlayerFlags. */
		u32 FirstPlayerFlagIndex;

		/* The index of the first team field in udtParseDataStatsBuffers::TeamFields. */
		u32 FirstTeamFieldIndex;

		/* The index of the first player field in udtParseDataStatsBuffers::PlayerFields. */
		u32 FirstPlayerFieldIndex;

		/* The index of the first player stats descriptor in udtParseDataStatsBuffers::PlayerStats. */
		u32 FirstPlayerStatsIndex;

		/* Of type udtGameType::Id. */
		/* Defaults to (u32)-1 when invalid or uninitialized. */
		u32 GameType;

		/* The duration of the match. */
		u32 MatchDurationMs;

		/* Of type udtMod::Id. */
		u32 Mod;

		/* Of type udtGamePlay::Id. */
		u32 GamePlay;

		/* Of type udtOvertime::Id. */
		u32 OverTimeType;

		/* Total number of overtimes in the match. */
		u32 OverTimeCount;

		/* Non-zero if the loser left the game before it was supposed to end, 0 otherwise. */
		u32 Forfeited;

		/* Total number of time-outs in the match. */
		u32 TimeOutCount;

		/* The total amount of time spent in time-outs. */
		u32 TotalTimeOutDurationMs;

		/* Did the winning team hit the mercy limit? (QL TDM) */
		u32 MercyLimited;

		/* Score of whoever is 1st place. */
		s32 FirstPlaceScore;

		/* Score of whoever is 2nd place. */
		s32 SecondPlaceScore;

		/* Non-zero if the player/team with the lowest score won by forfeit, zero otherwise. */
		u32 SecondPlaceWon;

		/* Non-zero if the game type is a team mode, zero otherwise. */
		u32 TeamMode;

		/* Zero when invalid, a UNIX/POSIX timestamp otherwise. */
		u32 StartDateEpoch;

		/* Zero when there isn't any. */
		u32 TimeLimit;

		/* Zero when there isn't any. */
		u32 ScoreLimit;

		/* Zero when there isn't any. */
		u32 FragLimit;

		/* Zero when there isn't any. */
		u32 CaptureLimit;

		/* Zero when there isn't any. */
		u32 RoundLimit;

		/* Match start time (server time), in milli-seconds. */
		s32 StartTimeMs;

		/* Match end time (server time), in milli-seconds. */
		s32 EndTimeMs;

		/* The index of the gamestate message after which this match took place. */
		u32 GameStateIndex;

		/* Count down start time (server time), in milli-seconds. */
		/* If there was no count-down, then this holds the match's start time. */
		s32 CountDownStartTimeMs;

		/* Post-match intermission end time (server time), in milli-seconds. */
		/* If there was no intermission (e.g. CPMA forfeits), this holds the match's end time. */
		s32 IntermissionEndTimeMs;

		/* String offset. Name of the team who was defending this round. For RtCW Stopwatch. */
		u32 DefenderName;

		/* String length. */
		u32 DefenderNameLength;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtParseDataStats;
	UDT_ENFORCE_API_STRUCT_SIZE(udtParseDataStats)

	/* Complete match statistics data for all demos in a context. */
	typedef struct udtParseDataStatsBuffers_s
	{
		/* The match statistics descriptors. */
		const udtParseDataStats* MatchStats;

		/* Array length: the context' demo count. */
		/* For a demo index, tells you which indices of MatchStats to use. */
		const udtParseDataBufferRange* MatchStatsRanges;

		/* Start and end times for each time-out. */
		/* Order: start0, end0, start1, end1, etc. */
		const s32* TimeOutStartAndEndTimes;

		/* A bit set describing which team stats fields are valid. */
		/* Array length: popcnt(ValidTeams) * UDT_TEAM_STATS_MASK_BYTE_COUNT bytes. */
		/* See udtTeamStatsField::Id. */
		const u8* TeamFlags;

		/* A bit set describing which players stats fields are valid. */
		/* Array length: popcnt(ValidPlayers) * UDT_PLAYER_STATS_MASK_BYTE_COUNT bytes. */
		/* See udtPlayerStatsField::Id. */
		const u8* PlayerFlags;

		/* The team stats. */
		/* Array length: popcnt(RedTeam.Flags) + popcnt(BlueTeam.Flags) */
		const s32* TeamFields;

		/* The player stats. */
		/* Array length: popcnt(Player1.Flags) + ... + popcnt(PlayerN.Flags) */
		const s32* PlayerFields;

		/* The length of the array is the number of bits set in ValidPlayers. */
		/* The player's client numbers will correspond to the indices of the bits set in ValidPlayers. */
		const udtPlayerStats* PlayerStats;

		/* Pointer to a buffer containing all UTF-8 strings. */
		const u8* StringBuffer;

		/* Ignore this. */
		const void* Reserved1;

		/* The match count for this match statistics descriptor. */
		u32 MatchCount;

		/* The time-out range count for this match statistics descriptor (i.e. the number of s32 values over 2). */
		u32 TimeOutRangeCount;

		/* The team flag count for this match statistics descriptor. */
		u32 TeamFlagCount;

		/* The player flag count for this match statistics descriptor. */
		u32 PlayerFlagCount;

		/* The team field count for this match statistics descriptor. */
		u32 TeamFieldCount;

		/* The player field count for this match statistics descriptor. */
		u32 PlayerFieldCount;

		/* The player statistics count for this match statistics descriptor. */
		u32 PlayerStatsCount;

		/* The byte count of the StringBuffer. */
		u32 StringBufferSize;
	}
	udtParseDataStatsBuffers;
	UDT_ENFORCE_API_STRUCT_SIZE(udtParseDataStatsBuffers)

	typedef struct udtParseDataRawCommand_s
	{
		/* String offset. The raw command, as it was sent from the server. */
		u32 RawCommand;

		/* String length. */
		u32 RawCommandLength;

		/* The time at which the server command was sent from the client. */
		s32 ServerTimeMs;

		/* The index of the last gamestate message after which this server command was sent. */
		/* Negative if invalid or not available. */
		s32 GameStateIndex;
	}
	udtParseDataRawCommand;
	UDT_ENFORCE_API_STRUCT_SIZE(udtParseDataRawCommand)

	/* Complete server-to-client command data for all demos in a context. */
	typedef struct udtParseDataRawCommandBuffers_s
	{
		/* The command descriptors. */
		const udtParseDataRawCommand* Commands;

		/* Array length: the context' demo count. */
		/* For a demo index, tells you which indices of CommandRanges to use. */
		const udtParseDataBufferRange* CommandRanges;

		/* Pointer to a buffer containing all UTF-8 strings. */
		const u8* StringBuffer;

		/* Ignore this. */
		const void* Reserved1;

		/* The length of the Commands array. */
		u32 CommandCount;

		/* The byte count of the StringBuffer. */
		u32 StringBufferSize;
	}
	udtParseDataRawCommandBuffers;
	UDT_ENFORCE_API_STRUCT_SIZE(udtParseDataRawCommandBuffers)

	typedef struct udtParseDataRawConfigString_s
	{
		/* String Offset. The raw config string, as it was sent from the server. */
		u32 RawConfigString;

		/* String length. */
		u32 RawConfigStringLength;

		/* The index of the config string. */
		u32 ConfigStringIndex;

		/* The index of the gamestate message this config string was in. */
		/* Negative if invalid or not available. */
		s32 GameStateIndex;
	}
	udtParseDataRawConfigString;
	UDT_ENFORCE_API_STRUCT_SIZE(udtParseDataRawConfigString)

	/* Complete config string data for all demos in a context. */
	typedef struct udtParseDataRawConfigStringBuffers_s
	{
		/* The config string descriptors. */
		const udtParseDataRawConfigString* ConfigStrings;

		/* Array length: the context' demo count. */
		/* For a demo index, tells you which indices of ConfigStringRanges to use. */
		const udtParseDataBufferRange* ConfigStringRanges;

		/* Pointer to a buffer containing all UTF-8 strings. */
		const u8* StringBuffer;

		/* Ignore this. */
		const void* Reserved1;

		/* The length of the ConfigStrings array. */
		u32 ConfigStringCount;

		/* The byte count of the StringBuffer. */
		u32 StringBufferSize;
	}
	udtParseDataRawConfigStringBuffers;
	UDT_ENFORCE_API_STRUCT_SIZE(udtParseDataRawConfigStringBuffers)

#if defined(__cplusplus)
	struct udtParseDataCaptureMask
	{
		enum Id
		{
			BaseToBase = UDT_BIT(0),        /* Flag picked up from its default return position. */
			DemoTaker = UDT_BIT(1),         /* Flag captured by the player who recorded the demo. */
			FirstPersonPlayer = UDT_BIT(2), /* Flag captured by a player being spectated by whoever recorded the demo. */
			PlayerIndexValid = UDT_BIT(3),  /* The PlayerIndex field is valid. */
			PlayerNameValid = UDT_BIT(4),   /* The PlayerName and PlayerNameLength fields are valid. */
			DistanceValid = UDT_BIT(5)      /* The Distance field is valid. */
		};
	};
#endif

	typedef struct udtParseDataCapture_s
	{
		/* String offset. Name of the map. */
		u32 MapName;

		/* String length. */
		u32 MapNameLength;

		/* String offset. Name of the player who capped. */
		/* Not always available: check for the PlayerNameValid bit of Flags. */
		u32 PlayerName;

		/* String length. */
		/* Not always available: check for the PlayerNameValid bit of Flags. */
		u32 PlayerNameLength;

		/* The index of the gamestate message this config string was in. */
		/* Negative if invalid or not available. */
		s32 GameStateIndex;

		/* Server time at which the flag was picked up, in milli-seconds. */
		s32 PickUpTimeMs;

		/* Server time at which the flag was captured, in milli-seconds. */
		s32 CaptureTimeMs;

		/* Distance between the pick-up spot and the capture spot, in Quake units. */
		/* This is not the distance traveled by the capping player. */
		/* Not always available: check for the DistanceValid bit of Flags. */
		f32 Distance;

		/* See udtParseDataCaptureFlags::Id. */
		u32 Flags;

		/* Index of the player who capped (the "client number"). */
		/* Not always available: check for the PlayerIndexValid bit of Flags. */
		s32 PlayerIndex;
	}
	udtParseDataCapture;
	UDT_ENFORCE_API_STRUCT_SIZE(udtParseDataCapture)

	/* Complete flag capture data for all demos in a context. */
	typedef struct udtParseDataCaptureBuffers_s
	{
		/* The flag capture descriptors. */
		const udtParseDataCapture* Captures;

		/* Array length: the context' demo count. */
		/* For a demo index, tells you which indices of CaptureRanges to use. */
		const udtParseDataBufferRange* CaptureRanges;

		/* Pointer to a buffer containing all UTF-8 strings. */
		const u8* StringBuffer;

		/* Ignore this. */
		const void* Reserved1;

		/* The length of the Captures array. */
		u32 CaptureCount;

		/* The byte count of the StringBuffer. */
		u32 StringBufferSize;
	}
	udtParseDataCaptureBuffers;
	UDT_ENFORCE_API_STRUCT_SIZE(udtParseDataCaptureBuffers)

#if defined(__cplusplus)
	struct udtParseDataScoreMask
	{
		enum Id
		{
			TeamBased = UDT_BIT(0)
		};
	};
#endif

	typedef struct udtParseDataScore_s
	{
		/* The index of the current gamestate. */
		s32 GameStateIndex;

		/* The first snapshot from which this is valid. */
		s32 ServerTimeMs;

		/* First place player or red team score. */
		s32 Score1; 

		/* Second place player or blue team score. */
		s32 Score2;

		/* First place player client number or 0. */
		u32 Id1;

		/* Second place player client number or 1. */
		u32 Id2;

		/* See udtParseDataScoreMask::Id. */
		u32 Flags;

		/* First place player name. */
		u32 Name1;

		/* String length. */
		u32 Name1Length;

		/* Second place player name. */
		u32 Name2;

		/* String length. */
		u32 Name2Length;

		/* First place player name. */
		u32 CleanName1;

		/* String length. */
		u32 CleanName1Length;

		/* Second place player name. */
		u32 CleanName2;

		/* String length. */
		u32 CleanName2Length;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtParseDataScore;
	UDT_ENFORCE_API_STRUCT_SIZE(udtParseDataScore)

	/* Complete score data for all demos in a context. */
	typedef struct udtParseDataScoreBuffers_s
	{
		/* The score descriptors. */
		const udtParseDataScore* Scores;

		/* Array length: the context' demo count. */
		/* For a demo index, tells you which indices of Scores to use. */
		const udtParseDataBufferRange* ScoreRanges;

		/* Pointer to a buffer containing all UTF-8 strings. */
		const u8* StringBuffer;

		/* Ignore this. */
		const void* Reserved1;

		/* The length of the Scores array. */
		u32 ScoreCount;

		/* The byte count of the StringBuffer. */
		u32 StringBufferSize;
	}
	udtParseDataScoreBuffers;
	UDT_ENFORCE_API_STRUCT_SIZE(udtParseDataScoreBuffers)

	typedef struct udtTimeShiftArg_s
	{
		/* By how many snapshots do we shift the position of */
		/* non-first-person players back in time. */
		s32 SnapshotCount;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtTimeShiftArg;
	UDT_ENFORCE_API_STRUCT_SIZE(udtTimeShiftArg)

	typedef struct udtJSONArg_s
	{
		/* Output the data to stdout when non-zero. */
		u32 ConsoleOutput;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtJSONArg;
	UDT_ENFORCE_API_STRUCT_SIZE(udtJSONArg)

	typedef struct udtProtocolList_s
	{
		/* With the leading '.' character. */
		const char** Extensions;

		/* Contains the game's name and the engine versions. */
		const char** Descriptions;

		/* Use udtProtocolFlags for the numbers. */
		const u32* Flags;

		/* Ignore this. */
		void* Reserved1;

		/* Check against udtProtocol::Count to ensure no header/DLL desync. */
		u32 Count;

		/* Ignore this. */
		u32 Reserved2;
	}
	udtProtocolList;
	UDT_ENFORCE_API_STRUCT_SIZE(udtProtocolList)

#pragma pack(pop)

	/*
	A bunch of simple stand-alone helper functions.
	*/

	/* Returns the version numbers. */
	/* Returns zero if any argument is NULL. */
	UDT_API(s32) udtGetVersionNumbers(u32* major, u32* minor, u32* revision);

	/* Returns a null-terminated string describing the library's version. */
	/* Never returns NULL. */
	UDT_API(const char*) udtGetVersionString();

	/* Checks if this header file is compatible with the library you're linking against. */
	/* Returns zero if not exactly the same version. */
#if defined(__cplusplus)
	inline s32 udtSameVersion()
	{
		u32 maj, min, rev; udtGetVersionNumbers(&maj, &min, &rev);
		return (maj == UDT_VERSION_MAJOR && min == UDT_VERSION_MINOR && rev == UDT_VERSION_REVISION) ? (s32)1 : (s32)0;
	}
#endif

	/* Returns a null-terminated string describing the error. */
	/* Never returns NULL. */
	UDT_API(const char*) udtGetErrorCodeString(s32 errorCode);

	/* Returns zero if not a valid protocol. */
	/* The protocol argument is of type udtProtocol::Id. */
	UDT_API(s32) udtIsValidProtocol(u32 protocol);

	/* Returns zero if the protocol is invalid or UDT doesn't support writing demos in that protocol. */
	/* The protocol argument is of type udtProtocol::Id. */
	UDT_API(s32) udtIsProtocolWriteSupported(u32 protocol);

	/* Returns zero if not a valid protocol. */
	/* The protocol argument is of type udtProtocol::Id. */
	UDT_API(u32) udtGetSizeOfIdEntityState(u32 protocol);

	/* Returns zero if not a valid protocol. */
	/* The protocol argument is of type udtProtocol::Id. */
	UDT_API(u32) udtGetSizeOfIdPlayerState(u32 protocol);

	/* Returns zero if not a valid protocol. */
	/* The protocol argument is of type udtProtocol::Id. */
	UDT_API(u32) udtGetSizeOfidClientSnapshot(u32 protocol);

	/* Returns NULL if invalid. */
	/* The protocol argument is of type udtProtocol::Id. */
	UDT_API(const char*) udtGetFileExtensionByProtocol(u32 protocol);

	/* The return value is of type udtProtocol::Id. */
	UDT_API(u32) udtGetProtocolByFilePath(const char* filePath);

	/* Can only fail with udtErrorCode::InvalidArgument. */
	UDT_API(s32) udtGetProtocolList(udtProtocolList* protocolList);
	
	/* Raises the type of error asked for. */
	/* The crashType argument is of type udtCrashType::Id. */
	UDT_API(s32) udtCrash(u32 crashType);

	/* Retrieve the string array for the given array identifier. */
	/* The arrayId argument is of type udtStringArray::Id. */
	UDT_API(s32) udtGetStringArray(u32 arrayId, const char*** elements, u32* elementCount);

	/* Retrieve the byte array for the given array identifier. */
	/* The arrayId argument is of type udtByteArray::Id. */
	UDT_API(s32) udtGetByteArray(u32 arrayId, const u8** elements, u32* elementCount);

	/* Get the magic constants needed to parse stats properly. */
	UDT_API(s32) udtGetStatsConstants(u32* playerMaskByteCount, u32* teamMaskByteCount, u32* playerFieldCount, u32* teamFieldCount, u32* perfFieldCount);

	/* Merges/add/replaces the stats of both arrays and writes the result to destPerfStats. */
	UDT_API(s32) udtMergeBatchPerfStats(u64* destPerfStats, const u64* sourcePerfStats);

	/* Adds the stats of both arrays and writes the result to destPerfStats. */
	UDT_API(s32) udtAddThreadPerfStats(u64* destPerfStats, const u64* sourcePerfStats);

	/* Gets the processor core count. */
	UDT_API(s32) udtGetProcessorCoreCount(u32* cpuCoreCount);

	/*
	Init and shut down functions.
	*/

	/* Should be called and waited for before calling any other function except for udtSetCrashHandler. */
	UDT_API(s32) udtInitLibrary();

	/* Should only be called after every call to other functions has terminated. */
	UDT_API(s32) udtShutDownLibrary();

	/*
	The configurable API for fine-grained task selection.
	All functions returning a s32 value return an error code of type udtErrorCode::Id.
	*/

	/* Sets the global fatal error handler. */
	/* If you pass NULL, will set it back to the default handler. */
	UDT_API(s32) udtSetCrashHandler(udtCrashCallback crashHandler);

	/* Creates a context that can be used by multiple parsers. */
	UDT_API(udtParserContext*) udtCreateContext();

	/* Releases all the resources associated to the context. */
	UDT_API(s32) udtDestroyContext(udtParserContext* context);

	/* Splits a demo into multiple sub-demos if the input demo has more than 1 gamestate server message. */
	UDT_API(s32) udtSplitDemoFile(udtParserContext* context, const udtParseArg* info, const char* demoFilePath);

	/* Creates a sub-demo starting and ending at the specified times. */
	UDT_API(s32) udtCutDemoFileByTime(udtParserContext* context, const udtParseArg* info, const udtCutByTimeArg* cutInfo, const char* demoFilePath);

	/* Creates a new demo that is basically the first demo passed with extra entity data from the other demos. */
	/* The maximum amount of demos merged (i.e. the maximum value of fileCount) is UDT_MAX_MERGE_DEMO_COUNT. */
	UDT_API(s32) udtMergeDemoFiles(const udtParseArg* info, const char** filePaths, u32 fileCount);

	/* For a given plug-in id, gets the complete buffer descriptor table for all demos in the context. */
	/* The buffersStruct argument points to a data structure of type udtParseData*Buffers which corresponds to the plug-in type. */
	/* All strings are UTF-8 encoded and string lengths represent the number of bytes (not characters) excluding the terminating NULL byte. */
	UDT_API(s32) udtGetContextPlugInBuffers(udtParserContext* context, u32 plugInId, void* buffersStruct);

	/*
	The configurable API for fine-grained task selection.
	All functions returning a s32 value return an error code of type udtErrorCode::Id.
	Batch processing functions.
	*/

	/* Reads through a group of demo files. */
	/* Can be configured for various analysis and data extraction tasks. */
	UDT_API(s32) udtParseDemoFiles(udtParserContextGroup** contextGroup, const udtParseArg* info, const udtMultiParseArg* extraInfo);

	/* Gets the amount of contexts stored in the context group. */
	UDT_API(s32) udtGetContextCountFromGroup(udtParserContextGroup* contextGroup, u32* count);

	/* Gets a context in a context group. */
	UDT_API(s32) udtGetContextFromGroup(udtParserContextGroup* contextGroup, u32 contextIdx, udtParserContext** context);

	/* Gets the total demo count for which plug-in data is stored in a context group. */
	UDT_API(s32) udtGetDemoCountFromGroup(udtParserContextGroup* contextGroup, u32* count);

	/* Gets the demo count for which plug-in data is stored in a context. */
	UDT_API(s32) udtGetDemoCountFromContext(udtParserContext* context, u32* count);

	/* Gets the input index of the specified demo. */
	UDT_API(s32) udtGetDemoInputIndex(udtParserContext* context, u32 demoIdx, u32* demoInputIdx);

	/* Releases all the resources associated to the context group. */
	UDT_API(s32) udtDestroyContextGroup(udtParserContextGroup* contextGroup);

	/* Creates, for each demo, sub-demos around every occurrence of a matching pattern. */
	UDT_API(s32) udtCutDemoFilesByPattern(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtPatternSearchArg* patternInfo);

	/* Creates a list of matches for the requested patterns in the newly created search context. */
	UDT_API(s32) udtFindPatternsInDemoFiles(udtPatternSearchContext** context, const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtPatternSearchArg* patternInfo);

	/* Gets the search results from the given search context. */
	UDT_API(s32) udtGetSearchResults(udtPatternSearchContext* context, udtPatternSearchResults* results);

	/* Releases all the resources associated to the search context. */
	UDT_API(s32) udtDestroySearchContext(udtPatternSearchContext* context);

	/* Creates, for each demo that isn't in the target protocol, a new demo file with the specified protocol. */
	UDT_API(s32) udtConvertDemoFiles(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtProtocolConversionArg* conversionArg);

	/* Creates, for each demo, a new demo where non-first-person player entities are shifted back in time by the specified amount of snapshots. */
	UDT_API(s32) udtTimeShiftDemoFiles(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtTimeShiftArg* timeShiftArg);

	/* Creates, for each demo, a .JSON file with the data from all the selected plug-ins. */
	UDT_API(s32) udtSaveDemoFilesAnalysisDataToJSON(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtJSONArg* jsonInfo);

	/*
	Custom parsing constants and data structures.
	*/

#define	ID_MAX_PS_STATS		       16
#define	ID_MAX_PS_PERSISTANT       16
#define	ID_MAX_PS_POWERUPS         16
#define	ID_MAX_PS_EVENTS	        2
#define	ID_MAX_PARSE_ENTITIES    2048
#define	ID_MAX_CLIENTS	           64 /* max player count */
#define ID_MAX_MSG_LENGTH       32768 /* max length of a message, which may be fragmented into multiple packets */
                                      /* Q3 16384 - RtCW/ET 32768 */

	typedef f32   idVec;
	typedef idVec idVec2[2];
	typedef idVec idVec3[3];
	typedef idVec idVec4[4];

#if defined(__cplusplus)

	struct udtEntityStateFlag
	{
		enum Id
		{
			AddedOrChanged,
			NewEvent,
			Count
		};
	};

#endif

#pragma pack(push, 1)

	typedef enum
	{
		ID_TR_STATIONARY,
		ID_TR_INTERPOLATE, /* non-parametric, but interpolate between snapshots */
		ID_TR_LINEAR,
		ID_TR_LINEAR_STOP,
		ID_TR_SINE,        /* value = base + sin( time / duration ) * delta */
		ID_TR_GRAVITY,
		ID_TR_TYPE_COUNT
	}
	idTrajectoryType;

	typedef struct idTrajectoryBase_s
	{
		idTrajectoryType trType;
		s32 trTime;
		s32 trDuration; /* if non 0, trTime + trDuration = stop time */
		idVec3 trBase;
		idVec3 trDelta; /* velocity, etc */
	}
	idTrajectoryBase;

	/*
	/This is the information conveyed from the server in an update message about entities that 
	the client will need to render in some way. 
	Different eTypes may use the information in different ways.
	The messages are delta compressed, so it doesn't really matter if the structure size is fairly large.
	*/
	typedef struct idEntityStateBase_s
	{
		s32 number;	/* entity index */
		s32 eType;  /* entityType_t */
		s32 eFlags;
		idTrajectoryBase pos;
		idTrajectoryBase apos;
		s32 time;
		s32	time2;
		idVec3 origin;
		idVec3 origin2;
		idVec3 angles;
		idVec3 angles2;
		s32 otherEntityNum;  /* shotgun sources, etc */
		s32 otherEntityNum2;
		s32 groundEntityNum; /* ENTITYNUM_NONE = in air */
		s32 constantLight;   /* r + (g<<8) + (b<<16) + (intensity<<24) */
		s32 loopSound;       /* constantly loop this sound */
		s32 modelindex;
		s32 modelindex2;
		s32 clientNum; /* 0 to (MAX_CLIENTS - 1), for players and corpses */
		s32 frame;
		s32 solid;     /* for client side prediction, trap_linkentity sets this properly */
		s32 event;     /* impulse events -- muzzle flashes, footsteps, etc */
		s32 eventParm;
		s32 powerups;  /* bit flags */
		s32 weapon;    /* determines weapon and flash model, etc */
		s32 legsAnim;  /* mask off ANIM_TOGGLEBIT */
		s32 torsoAnim; /* mask off ANIM_TOGGLEBIT */
		s32 generic1;
	}
	idEntityStateBase;

#if defined(__cplusplus)

	struct idEntityState3 : idEntityStateBase
	{
	};

	struct idEntityState48 : idEntityStateBase
	{
	};

	struct idEntityState60 : idEntityStateBase
	{
		int dl_intensity;  /* used for coronas */
		int eventSequence; /* pmove generated events */
		int events[4];
		int eventParms[4];
		int density;       /* for particle effects */
		/* to pass along additional information for damage effects for players */
		/* also used for cursorhints for non-player entities */
		int dmgFlags;
		int onFireStart;
		int onFireEnd;
		int aiChar;
		int teamNum;
		int effect1Time;
		int effect2Time;
		int effect3Time;
		int aiState;
		int animMovetype;  /* clients can't derive movetype of other clients for anim scripting system */
	};

	struct idEntityState66 : idEntityStateBase
	{
	};

	struct idEntityState67 : idEntityStateBase
	{
	};

	struct idEntityState68 : idEntityStateBase
	{
	};

	struct idEntityState73 : idEntityStateBase
	{
		s32 pos_gravity;  /* part of idEntityStateBase::pos trajectory */
		s32 apos_gravity; /* part of idEntityStateBase::apos trajectory */
	};

	struct idEntityState90 : idEntityStateBase
	{
		s32 pos_gravity;  /* part of idEntityStateBase::pos trajectory */
		s32 apos_gravity; /* part of idEntityStateBase::apos trajectory */
		s32 jumpTime;
		s32 doubleJumped; /* qboolean */
	};

	struct idEntityState91 : idEntityStateBase
	{
		s32 pos_gravity;  /* part of idEntityStateBase::pos trajectory */
		s32 apos_gravity; /* part of idEntityStateBase::apos trajectory */
		s32 jumpTime;
		s32 doubleJumped; /* qboolean */
		s32 health;
		s32 armor;
		s32 location;
	};

	typedef idEntityState60 idLargestEntityState;

	static_assert(sizeof(idEntityState3 ) <= sizeof(idLargestEntityState), "incorrect idLargestEntityState typedef");
	static_assert(sizeof(idEntityState48) <= sizeof(idLargestEntityState), "incorrect idLargestEntityState typedef");
	static_assert(sizeof(idEntityState60) <= sizeof(idLargestEntityState), "incorrect idLargestEntityState typedef");
	static_assert(sizeof(idEntityState66) <= sizeof(idLargestEntityState), "incorrect idLargestEntityState typedef");
	static_assert(sizeof(idEntityState67) <= sizeof(idLargestEntityState), "incorrect idLargestEntityState typedef");
	static_assert(sizeof(idEntityState68) <= sizeof(idLargestEntityState), "incorrect idLargestEntityState typedef");
	static_assert(sizeof(idEntityState73) <= sizeof(idLargestEntityState), "incorrect idLargestEntityState typedef");
	static_assert(sizeof(idEntityState90) <= sizeof(idLargestEntityState), "incorrect idLargestEntityState typedef");
	static_assert(sizeof(idEntityState91) <= sizeof(idLargestEntityState), "incorrect idLargestEntityState typedef");

#endif

	/*
	This is the information needed by both the client and server to predict player motion and actions.
	Nothing outside of pmove should modify these, or some degree of prediction error will occur.

	This is a full superset of idEntityState as it is used by players,
	so if an idPlayerState is transmitted, the idEntityState can be fully derived from it.
	*/
	typedef struct idPlayerStateBase_s
	{
		s32 commandTime; /* cmd->serverTime of last executed command */
		s32 pm_type;
		s32 bobCycle;    /* for view bobbing and footstep generation */
		s32 pm_flags;    /* ducked, jump_held, etc */
		s32 pm_time;
		idVec3 origin;
		idVec3 velocity;
		s32 weaponTime;
		s32 gravity;
		s32 speed;
		/* add to command angles to get view direction */
		/* changed by spawns, rotating objects, and teleporters */
		s32 delta_angles[3]; 
		s32 groundEntityNum; /* ENTITYNUM_NONE = in air */
		s32 legsTimer;       /* don't change low priority animations until this runs out */
		s32 legsAnim;        /* mask off ANIM_TOGGLEBIT */
		s32 torsoTimer;      /* don't change low priority animations until this runs out */
		s32 torsoAnim;       /* mask off ANIM_TOGGLEBIT */
		/* a number 0 to 7 that represents the relative angle */
		/* of movement to the view angle (axial and diagonals) */
		/* when at rest, the value will remain unchanged */
		/* used to twist the legs during strafing */
		s32 movementDir;
		idVec3 grapplePoint; /* location of grapple to pull towards if PMF_GRAPPLE_PULL */
		s32 eFlags;          /* copied to entityState_t->eFlags */
		s32 eventSequence;   /* pmove generated events */
		s32 events[ID_MAX_PS_EVENTS];
		s32 eventParms[ID_MAX_PS_EVENTS];
		s32 externalEvent;   /* events set on player from another source */
		s32 externalEventParm;
		s32 externalEventTime;
		s32 clientNum; /* ranges from 0 to MAX_CLIENTS-1 */
		s32 weapon;    /* copied to entityState_t->weapon */
		s32 weaponstate;
		idVec3 viewangles; /* for fixed views */
		s32 viewheight;
		s32 damageEvent;   /* when it changes, latch the other parms */
		s32 damageYaw;
		s32 damagePitch;
		s32 damageCount;
		s32 stats[ID_MAX_PS_STATS];
		s32 persistant[ID_MAX_PS_PERSISTANT]; /* stats that aren't cleared on death */
		s32 powerups[ID_MAX_PS_POWERUPS];     /* level.time that the powerup runs out */
		s32 ammo[64];    /* ID_MAX_PS_WEAPONS, 16 for all Quake protocols, 64 for RTCW */
		s32 generic1;
		s32 loopSound;
		s32 jumppad_ent; /* jumppad entity hit this frame */
	}
	idPlayerStateBase;

#if defined(__cplusplus)

	struct idPlayerState3 : idPlayerStateBase
	{
	};

	struct idPlayerState48 : idPlayerStateBase
	{
	};

	struct idPlayerState60 : idPlayerStateBase
	{
		/* for weapons that don't fire immediately when 'fire' is hit (grenades, venom, ...) */
		int weaponDelay;
		/* for delayed grenade throwing. this is set to a #define for grenade */
		/* lifetime when the attack button goes down, then when attack is released * /
		/* this is the amount of time left before the grenade goes off */
		/* (or if it gets to 0 while in player's hand, it explodes) */
		int grenadeTimeLeft;
		float leanf;             /* amount of 'lean' when player is looking around corner */
		int weapons[2];          /* 64 bits for weapons held */
		int weapAnim;            /* mask off ANIM_TOGGLEBIT */
		idVec3 mins, maxs;
		float crouchMaxZ;
		float crouchViewHeight, standViewHeight, deadViewHeight;
		float runSpeedScale, sprintSpeedScale, crouchSpeedScale; /* variable movement speed */
		int viewlocked;          /* view locking for mg42 */
		int viewlocked_entNum;
		/* need this to fix friction problems with slow zombies whereby */
		/* the friction prevents them from accelerating to their full potential */
		float friction;
		int aiChar;              /* AI character id is used for weapon association */
		int teamNum;
		int gunfx;
		int sprintTime;
		int aimSpreadScale;      /* 0-255 increases with angular movement */
		int onFireStart;         /* burning effect is required for view blending effect */
		int classWeaponTime;
		int serverCursorHint;    /* what type of cursor hint the server is dictating */
		int serverCursorHintVal; /* a value (0-255) associated with the above */
		int curWeapHeat;         /* for the currently selected weapon */
		int aiState;
		s32 ammoclip[64];
		s32 holdable[16];
	};

	struct idPlayerState66 : idPlayerStateBase
	{
	};

	struct idPlayerState67 : idPlayerStateBase
	{
	};

	struct idPlayerState68 : idPlayerStateBase
	{
	};

	struct idPlayerState73 : idPlayerStateBase
	{
	};

	struct idPlayerState90 : idPlayerStateBase
	{
		s32 doubleJumped; /* qboolean */
		s32 jumpTime;
	};

	struct idPlayerState91 : idPlayerStateBase
	{
		s32 doubleJumped; /* qboolean */
		s32 jumpTime;
		s32 weaponPrimary;
		s32 crouchTime;
		s32 crouchSlideTime;
		s32 location;
		s32 fov;
		s32 forwardmove;
		s32 rightmove;
		s32 upmove;
	};

	typedef idPlayerState60 idLargestPlayerState;

	static_assert(sizeof(idPlayerState3)  <= sizeof(idLargestPlayerState), "incorrect idLargestPlayerState typedef");
	static_assert(sizeof(idPlayerState48) <= sizeof(idLargestPlayerState), "incorrect idLargestPlayerState typedef");
	static_assert(sizeof(idPlayerState60) <= sizeof(idLargestPlayerState), "incorrect idLargestPlayerState typedef");
	static_assert(sizeof(idPlayerState66) <= sizeof(idLargestPlayerState), "incorrect idLargestPlayerState typedef");
	static_assert(sizeof(idPlayerState67) <= sizeof(idLargestPlayerState), "incorrect idLargestPlayerState typedef");
	static_assert(sizeof(idPlayerState68) <= sizeof(idLargestPlayerState), "incorrect idLargestPlayerState typedef");
	static_assert(sizeof(idPlayerState73) <= sizeof(idLargestPlayerState), "incorrect idLargestPlayerState typedef");
	static_assert(sizeof(idPlayerState90) <= sizeof(idLargestPlayerState), "incorrect idLargestPlayerState typedef");
	static_assert(sizeof(idPlayerState91) <= sizeof(idLargestPlayerState), "incorrect idLargestPlayerState typedef");

#endif

	typedef struct udtCuContext_s udtCuContext;

	/* Only valid until the next call to udtCuParseMessage. */
	typedef struct udtCuCommandMessage_s
	{
		/* Might be NULL. */
		const char* CommandString;

		/* The tokens in CommandString. */
		const char** CommandTokens;

		/* Length of CommandString. */
		u32 CommandStringLength;

		/* The sequence number that uniquely identifies this command. */
		s32 CommandSequence;

		/* Only valid if IsConfigString is true. */
		s32 ConfigStringIndex;

		/* If non-zero, this command is for a config string update. */
		u32 IsConfigString;

		/* The number of tokens in CommandString. */
		/* Length of the CommandTokens array. */
		u32 TokenCount;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtCuCommandMessage;
	UDT_ENFORCE_API_STRUCT_SIZE(udtCuCommandMessage)

	/* Only valid until the next call to udtCuParseMessage. */
	typedef struct udtCuSnapshotMessage_s
	{
		/* Portal area visibility bits. */
		u8 AreaMask[32];

		/* Pointer to the player state. */
		const idPlayerStateBase* PlayerState;

		/* An array of pointers to all entity states . */
		const idEntityStateBase** Entities;

		/* An array of flags for each entity state. */
		const u8* EntityFlags;

		/* An array of pointers to the entity states that changed or were added. */
		const idEntityStateBase** ChangedEntities;

		/* An array of numbers for the entities that were removed. */
		const s32* RemovedEntities;

		/* Ignore this. */
		const void* Reserved1;

		/* The server time the message is valid for. */
		s32 ServerTimeMs;

		/* The message sequence number. */
		s32 MessageNumber;

		/* Execute all commands up to this sequence number before making the snapshot current. */
		s32 CommandNumber;

		/* How many entities have changed or been added. */
		/* Length of the ChangedEntities array. */
		u32 ChangedEntityCount;

		/* How many entities were removed. */
		/* Length of the RemovedEntities array. */
		u32 RemovedEntityCount;

		/* Length of the Entities array. */
		u32 EntityCount;
	}
	udtCuSnapshotMessage;
	UDT_ENFORCE_API_STRUCT_SIZE(udtCuSnapshotMessage)

	/* Only valid until the next call to udtCuParseMessage. */
	typedef struct udtCuGamestateMessage_s
	{
		/* A game state message always marks the start of a server command sequence. */
		s32 ServerCommandSequence;

		/* The client number of the player who recorded the demo file. */
		s32 ClientNumber;

		/* The checksum feed of the server that the client will have to use. */
		s32 ChecksumFeed;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtCuGamestateMessage;
	UDT_ENFORCE_API_STRUCT_SIZE(udtCuGamestateMessage)
	
	/* UDT will not give you redundant snapshots and commands like Quake demos do. */
	/* If a snapshot or a command was already processed, it's dropped so you don't have to deal with duplicates. */
	/* Only valid until the next call to udtCuParseMessage. */
	typedef struct udtCuMessageOutput_s
	{
		/* Can be NULL. */
		/* There are 3 possible scenarios: */
		/* 1. 1 game state and no snapshot */
		/* 2. 1 snapshot and no game state */
		/* 3. no game state and no snapshot */
		union 
		{
			const udtCuGamestateMessage* GameState;
			const udtCuSnapshotMessage* Snapshot;
		}
		GameStateOrSnapshot;

		/* An array of server-to-client string commands. */
		const udtCuCommandMessage* Commands;

		/* Length of the Commands array. */
		u32 CommandCount;

		/* When non-zero, use GameStateOrSnapshot::GameState. */
		/* Else, use GameStateOrSnapshot::Snapshot. */
		u32 IsGameState;
	}
	udtCuMessageOutput;
	UDT_ENFORCE_API_STRUCT_SIZE(udtCuMessageOutput)

	typedef struct udtCuMessageInput_s
	{
		/* The raw Quake message buffer. */
		const void* Buffer;

		/* Ignore this. */
		const void* Reserved1;

		/* The message sequence number. */
		s32 MessageSequence;

		/* The byte size of the buffer pointed to by Buffer. */
		u32 BufferByteCount;
	}
	udtCuMessageInput;
	UDT_ENFORCE_API_STRUCT_SIZE(udtCuMessageInput)

	/* Only valid until the next call to udtCuParseMessage. */
	typedef struct udtCuConfigString_s
	{
		/* NULL if not defined. */
		const char* ConfigString;

		/* Ignore this. */
		const void* Reserved1;

		/* The length of ConfigString. */
		u32 ConfigStringLength;

		/* Ignore this. */
		s32 Reserved2;
	}
	udtCuConfigString;
	UDT_ENFORCE_API_STRUCT_SIZE(udtCuConfigString)

#if defined(__cplusplus)

#define UDT_IDENTITY_WITH_COMMA(x) x,

#define UDT_ENTITY_EVENT_LIST(N) \
	N(Obituary) \
	N(WeaponFired) \
	N(ItemPickup) \
	N(GlobalItemPickup) \
	N(GlobalSound) \
	N(GlobalTeamSound) \
	N(ItemRespawn) \
	N(ItemPop) \
	N(PlayerTeleportIn) \
	N(PlayerTeleportOut) \
	N(BulletHitFlesh) \
	N(BulletHitWall) \
	N(MissileHit) \
	N(MissileMiss) \
	N(MissileMissMetal) \
	N(RailTrail) \
	N(PowerUpQuad) \
	N(PowerUpBattleSuit) \
	N(PowerUpRegen) \
	N(QL_Overtime) \
	N(QL_GameOver)

	struct udtEntityEvent
	{
		enum Id
		{
			UDT_ENTITY_EVENT_LIST(UDT_IDENTITY_WITH_COMMA)
			Count
		};
	};

#define UDT_ENTITY_TYPE_LIST(N) \
	N(Event) \
	N(General) \
	N(Player) \
	N(Item) \
	N(Missile) \
	N(Mover) \
	N(Beam) \
	N(Portal) \
	N(Speaker) \
	N(PushTrigger) \
	N(TeleportTrigger) \
	N(Invisible) \
	N(Grapple) \
	N(Team)

	struct udtEntityType
	{
		enum Id
		{
			UDT_ENTITY_TYPE_LIST(UDT_IDENTITY_WITH_COMMA)
			Count
		};
	};

#define UDT_CONFIG_STRING_LIST(N) \
	N(FirstPlayer) \
	N(Intermission) \
	N(LevelStartTime) \
	N(WarmUpEndTime) \
	N(FirstPlacePlayerName) \
	N(SecondPlacePlayerName) \
	N(PauseStart) \
	N(PauseEnd) \
	N(FlagStatus) \
	N(ServerInfo) \
	N(SystemInfo) \
	N(Scores1) \
	N(Scores2) \
	N(VoteTime) \
	N(VoteString) \
	N(VoteYes) \
	N(VoteNo) \
	N(TeamVoteTime) \
	N(TeamVoteString) \
	N(TeamVoteYes) \
	N(TeamVoteNo) \
	N(GameVersion) \
	N(ItemFlags) \
	N(QL_TimeoutStartTime) \
	N(QL_TimeoutEndTime) \
	N(QL_RedTeamTimeoutsLeft) \
	N(QL_BlueTeamTimeoutsLeft) \
	N(QL_ReadTeamClanName) \
	N(QL_BlueTeamClanName) \
	N(QL_RedTeamClanTag) \
	N(QL_BlueTeamClanTag) \
	N(CPMA_GameInfo) \
	N(CPMA_RoundInfo) \
	N(OSP_GamePlay) \
	N(Wolf_Info) \
	N(Wolf_Paused) \
	N(Wolf_Ready)
	
	struct udtConfigStringIndex
	{
		enum Id
		{
			/* ItemFlags: A string of 0's and 1's that tell which items are present. */
			UDT_CONFIG_STRING_LIST(UDT_IDENTITY_WITH_COMMA)
			Count
		};
	};

#define UDT_POWER_UP_ITEM(Enum, Desc, Bit) Enum,
	struct udtPowerUpIndex
	{
		enum Id
		{
			UDT_POWER_UP_LIST(UDT_POWER_UP_ITEM)
			Count
		};
	};
#undef UDT_POWER_UP_ITEM

#define UDT_LIFE_STATS_LIST(N) \
	N(Health) \
	N(HoldableItem) \
	N(Weapons) \
	N(Armor) \
	N(MaxHealth) \
	N(Wolf_Keys) \
	N(Wolf_ClientsReady) \
	N(Wolf_PlayerClass) \
	N(Wolf_RedScore) \
	N(Wolf_BlueScore)

	struct udtLifeStatsIndex
	{
		enum Id
		{
			/* Wolf_ClientsReady: Bit mask of players ready to leave intermission. */
			UDT_LIFE_STATS_LIST(UDT_IDENTITY_WITH_COMMA)
			Count
		};
	};

#define UDT_PERSISTENT_STATS_LIST(N) \
	N(FlagCaptures) \
	N(Score) \
	N(DamageGiven) \
	N(Rank) \
	N(Team) \
	N(SpawnCount) \
	N(LastAttacker) \
	N(LastTargetHealthAndArmor) \
	N(Deaths) \
	N(Impressives) \
	N(Excellents) \
	N(Defends) \
	N(Assists) \
	N(Humiliations) \
	N(Wolf_RespawnsLeft) \
	N(Wolf_AccuracyHits)

	struct udtPersStatsIndex
	{
		enum Id
		{
			/* Wolf_AccuracyHits: It seems to account for the panzerfaust as well. */
			UDT_PERSISTENT_STATS_LIST(UDT_IDENTITY_WITH_COMMA)
			Count
		};
	};

#define UDT_ENTITY_STATE_FLAG_LIST(N) \
	N(Dead) \
	N(TeleportBit) \
	N(AwardExcellent) \
	N(PlayerEvent) \
	N(AwardHumiliation) \
	N(NoDraw) \
	N(Firing) \
	N(AwardCapture) \
	N(Chatting) \
	N(ConnectionInterrupted) \
	N(HasVoted) \
	N(AwardImpressive) \
	N(AwardDefense) \
	N(AwardAssist) \
	N(AwardDenied) \
	N(HasTeamVoted) \
	N(Spectator) \
	N(Wolf_Crouching) \
	N(Wolf_Headshot) \
	N(Wolf_Zooming)

	struct udtEntityFlag
	{
		enum Id
		{
			/* TeleportBit: Toggled every time the origin abruptly changes. */
			/* Firing: For the LG. */
			UDT_ENTITY_STATE_FLAG_LIST(UDT_IDENTITY_WITH_COMMA)
			Count
		};
	};

#undef UDT_IDENTITY_WITH_COMMA

	struct udtFlagStatus
	{
		enum Id
		{
			InBase,  /* In its spot in base. */
			Carried, /* Being carried by an enemy player. */
			Missing, /* Not being carried by anyone but not in its spot either. */
			Count
		};
	};

	struct udtItem
	{
		enum Id
		{
			AmmoBFG,
			AmmoBelt,
			AmmoBullets,
			AmmoCells,
			AmmoGrenades,
			AmmoHMG,
			AmmoLightning,
			AmmoMines,
			AmmoNails,
			AmmoPack,
			AmmoRockets,
			AmmoShells,
			AmmoSlugs,
			HoldableInvulnerability,
			HoldableKamikaze,
			HoldableMedkit,
			HoldablePortal,
			HoldableTeleporter,
			ItemAmmoRegen,
			ItemArmorBody,
			ItemArmorCombat,
			ItemArmorJacket,
			ItemArmorShard,
			ItemBackpack,
			ItemBlueCube,
			ItemDoubler,
			ItemEnviro,
			ItemFlight,
			ItemGuard,
			ItemHaste,
			ItemHealth,
			ItemHealthLarge,
			ItemHealthMega,
			ItemHealthSmall,
			ItemInvis,
			ItemKeyGold,
			ItemKeyMaster,
			ItemKeySilver,
			ItemQuad,
			ItemRedCube,
			ItemRegen,
			ItemScout,
			ItemSpawnArmor,
			FlagBlue,
			FlagNeutral,
			FlagRed,
			WeaponBFG,
			WeaponChaingun,
			WeaponGauntlet,
			WeaponGrapplingHook,
			WeaponGrenadeLauncher,
			WeaponHMG,
			WeaponLightningGun,
			WeaponMachinegun,
			WeaponNailgun,
			WeaponPlasmaGun,
			WeaponProxLauncher,
			WeaponRailgun,
			WeaponRocketLauncher,
			WeaponShotgun,
			Count,
			AmmoFirst = AmmoBFG,
			AmmoLast = AmmoSlugs,
			HoldableFirst = HoldableInvulnerability,
			HoldableLast = HoldableTeleporter,
			ItemFirst = ItemAmmoRegen,
			ItemLast = ItemSpawnArmor,
			FlagFirst = FlagBlue,
			FlagLast = FlagRed,
			WeaponFirst = WeaponBFG,
			WeaponLast = WeaponShotgun
		};
	};

	struct udtPlayerMovementType
	{
		enum Id
		{
			Normal,         /* can accelerate and turn */
			NoClip,         /* no collision at all */
			Spectator,      /* still run into walls */
			Dead,           /* no acceleration or turning, but free falling */
			Freeze,         /* stuck in place with no control */
			Intermission,   /* no movement or status bar */
			SPIntermission, /* no movement or status bar */
			Count
		};
	};

	struct udtMagicNumberType
	{
		enum Id
		{
			PowerUpIndex,   /* idPlayerStateBase::powerups */
			LifeStatsIndex, /* idPlayerStateBase::stats */
			PersStatsIndex, /* idPlayerStateBase::persistant */
			EntityType,     /* idEntityStateBase::eType */
			EntityFlag,     /* idEntityStateBase::eFlags */
			EntityEvent,    /* idEntityStateBase::event */
			ConfigStringIndex,
			Team,
			GameType,
			FlagStatus,
			Weapon,             /* idPlayerStateBase::weapon and idEntityStateBase::weapon */
			MeanOfDeath,
			Item,               /* idEntityStateBase::modelindex */
			PlayerMovementType, /* idPlayerStateBase::pm_type */
			Count
		};
	};

#endif

#pragma pack(pop)

	/*
	The API for custom parsing.
	With this API, you still have to call udtInitLibrary first and udtShutDownLibrary last.
	To set the crash handler, use udtSetCrashHandler.
	*/

	/* Creates a custom parsing context. */
	/* The same context can be used to parse multiple demos. */
	UDT_API(udtCuContext*) udtCuCreateContext();

	/* Sets the message printing callback. */
	/* The callback argument can be NULL. */
	/* The return value is of type udtErrorCode::Id. */
	UDT_API(s32) udtCuSetMessageCallback(udtCuContext* context, udtMessageCallback callback);

	/* The protocol argument is of type udtProtocol::Id. */
	/* The return value is of type udtErrorCode::Id. */
	UDT_API(s32) udtCuStartParsing(udtCuContext* context, u32 protocol);

	/* Parses a demo message and outputs the results into messageOutput. */
	/* If you should continue parsing, continueParsing will be set to a non-zero value. */
	/* The return value is of type udtErrorCode::Id. */
	UDT_API(s32) udtCuParseMessage(udtCuContext* context, udtCuMessageOutput* messageOutput, u32* continueParsing, const udtCuMessageInput* messageInput);

	/* Gets a config string descriptor. */
	/* The return value is of type udtErrorCode::Id. */
	UDT_API(s32) udtCuGetConfigString(udtCuContext* context, udtCuConfigString* configString, u32 configStringIndex);

	/* Returns a pointer to a baseline entity. */
	/* The return value is of type udtErrorCode::Id. */
	UDT_API(s32) udtCuGetEntityBaseline(udtCuContext* context, idEntityStateBase** entityState, u32 entityIndex);

	/* Returns a pointer to a parsed entity entity. */
	/* The return value is of type udtErrorCode::Id. */
	UDT_API(s32) udtCuGetEntityState(udtCuContext* context, idEntityStateBase** entityState, u32 entityIndex);

	/* Frees all the resources allocated by the custom parsing context. */
	/* The return value is of type udtErrorCode::Id. */
	UDT_API(s32) udtCuDestroyContext(udtCuContext* context);

	/*
	The API for custom parsing.
	Helper functions.
	*/

	/* Will clean up the string for printing by: */
	/* - getting rid of Quake 3/Live and OSP color codes */
	/* - removing unprintable characters for protocols <= 90 (no UTF-8 support) */
	/* The protocol argument is of type udtProtocol::Id. */
	/* The return value is of type udtErrorCode::Id. */
	UDT_API(s32) udtCleanUpString(char* string, u32 protocol);

	/* Finds the UDT number for a given Quake number. */
	/* The magicNumberTypeId argument is of type udtMagicNumberType::Id. */
	/* The protocol argument is of type udtProtocol::Id. */
	/* The mod argument is of type udtMod::Id. */
	/* The return value is of type udtErrorCode::Id. */
	UDT_API(s32) udtGetIdMagicNumber(s32* idNumber, u32 magicNumberTypeId, s32 udtNumber, u32 protocol, u32 mod);

	/* Finds the Quake number for a given UDT number. */
	/* The magicNumberTypeId argument is of type udtMagicNumberType::Id. */
	/* The protocol argument is of type udtProtocol::Id. */
	/* The mod argument is of type udtMod::Id. */
	/* The return value is of type udtErrorCode::Id. */
	UDT_API(s32) udtGetUDTMagicNumber(s32* udtNumber, u32 magicNumberTypeId, s32 idNumber, u32 protocol, u32 mod);

	/* Reads the integer value of a config string variable. */
	/* The temp buffer is used for constructing a search string. */
	/* The return value is of type udtErrorCode::Id. */
	UDT_API(s32) udtParseConfigStringValueAsInteger(s32* res, char* tempBuf, u32 tempBytes, const char* varName, const char* configString);

	/* Reads the string value of a config string variable. */
	/* The temp buffer is used for constructing a search string. */
	/* The return value is of type udtErrorCode::Id. */
	UDT_API(s32) udtParseConfigStringValueAsString(char* resBuf, u32 resBytes, char* tempBuf, u32 tempBytes, const char* varName, const char* configString);

	/* Converts a player state to an entity state. */
	/* Set extrapolate to a non-zero value to enable extrapolation. */
	/* The protocol argument is of type udtProtocol::Id. */
	/* The return value is of type udtErrorCode::Id. */
	UDT_API(s32) udtPlayerStateToEntityState(idEntityStateBase* es, const idPlayerStateBase* ps, u32 extrapolate, s32 serverTimeMs, u32 protocol);

#ifdef __cplusplus
}
#endif


#undef UDT_API
