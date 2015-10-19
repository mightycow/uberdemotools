#pragma once


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

#define S32_MIN (-2147483647 - 1)
#define S32_MAX (2147483647)
#define U32_MAX (0xFFFFFFFF)

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
#define UDT_BIT(x) (1 << x)

typedef struct udtParserContext_s udtParserContext;
typedef struct udtParserContextGroup_s udtParserContextGroup;

#if defined(__cplusplus)

#define UDT_ERROR_LIST(N) \
	N(None, "no error") \
	N(InvalidArgument, "invalid argument") \
	N(OperationFailed, "operation failed") \
	N(OperationCanceled, "operation canceled") \
	N(Unprocessed, "unprocessed job")

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
	N(Invalid, NULL) \
	N(Dm3 , ".dm3"  ) \
	N(Dm48, ".dm_48") \
	N(Dm66, ".dm_66") \
	N(Dm67, ".dm_67") \
	N(Dm68, ".dm_68") \
	N(Dm73, ".dm_73") \
	N(Dm90, ".dm_90") \
	N(Dm91, ".dm_91")

#define UDT_PROTOCOL_ITEM(Enum, Ext) Enum,
struct udtProtocol
{
	enum Id
	{
		UDT_PROTOCOL_LIST(UDT_PROTOCOL_ITEM)
		AfterLastProtocol,
		FirstProtocol = Dm68,
		Count = AfterLastProtocol - 1,
		LatestProtocol = Count
	};
};
#undef UDT_PROTOCOL_ITEM

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
	N(Chat,             "chat messages",      udtParserPlugInChat,             udtParseDataChat) \
	N(GameState,        "game states",        udtParserPlugInGameState,        udtParseDataGameState) \
	N(Obituaries,       "obituaries",         udtParserPlugInObituaries,       udtParseDataObituary) \
	N(Stats,            "match stats",        udtParserPlugInStats,            udtParseDataStats) \
	N(RawCommands,      "raw commands",       udtParserPlugInRawCommands,      udtParseDataRawCommand) \
	N(RawConfigStrings, "raw config strings", udtParserPlugInRawConfigStrings, udtParseDataRawConfigString) \
	N(Captures,         "captures",           udtParserPlugInCaptures,         udtParseDataCapture)

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
	N(Grenade, "grenade launcher", 3) \
	N(Rocket, "rocket launcher", 4) \
	N(Plasma, "plasma gun", 5) \
	N(Railgun, "railgun", 6) \
	N(LightningGun, "lightning gun", 7) \
	N(BFG, "BFG", 8) \
	N(NailGun, "nailgun", 9) \
	N(ChainGun, "chaingun", 10) \
	N(ProximityMineLauncher, "proximity mine launcher", 11) \
	N(HeavyMachineGun, "heavy machine gun", 12)

#define UDT_WEAPON_ITEM(Enum, Desc, Bit) Enum = UDT_BIT(Bit),
struct udtWeaponBits
{
	enum Id
	{
		UDT_WEAPON_LIST(UDT_WEAPON_ITEM)
		AfterLast
	};
};
#undef UDT_WEAPON_ITEM

#define UDT_WEAPON_ITEM(Enum, Desc, Bit) Enum = Bit,
struct udtWeapon
{
	enum Id
	{
		Gauntlet,
		MachineGun,
		Shotgun,
		GrenadeLauncher,
		RocketLauncher,
		PlasmaGun,
		Railgun,
		LightningGun,
		BFG,
		GrapplingHook,
		NailGun,
		ChainGun,
		ProximityMineLauncher,
		HeavyMachineGun,
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
	N(Invulnerability, "invulnerability", 13)

#define UDT_POWER_UP_ITEM(Enum, Desc, Bit) Enum = UDT_BIT(Bit),
struct udtPowerUpBits
{
	enum Id
	{
		UDT_POWER_UP_LIST(UDT_POWER_UP_ITEM)
		AfterLast
	};
};
#undef UDT_POWER_UP_ITEM

#define UDT_MOD_LIST(N) \
	N(Shotgun, "shotgun", 0) \
	N(Gauntlet, "gauntlet", 1) \
	N(MachineGun, "machine gun", 2) \
	N(Grenade, "grenade", 3) \
	N(GrenadeSplash, "grenade splash", 4) \
	N(Rocket, "rocket", 5) \
	N(RocketSplash, "rocket splash", 6) \
	N(Plasma, "plasma", 7) \
	N(PlasmaSplash, "plasma splash", 8) \
	N(Railgun, "railgun", 9) \
	N(Lightning, "lightning", 10) \
	N(BFG, "BFG", 11) \
	N(BFGSplash, "BFG splash", 12) \
	N(Water, "water", 13) \
	N(Slime, "slime", 14) \
	N(Lava, "lava", 15) \
	N(Crush, "crush", 16) \
	N(TeleFrag, "telefrag", 17) \
	N(Fall, "fall", 18) \
	N(Suicide, "suicide", 19) \
	N(TargetLaser, "target laser", 20) \
	N(TriggerHurt, "trigger hurt", 21) \
	N(NailGun, "nailgun", 22) \
	N(ChainGun, "chaingun", 23) \
	N(ProximityMine, "proximity mine", 24) \
	N(Kamikaze, "kamikaze", 25) \
	N(Juiced, "juiced", 26) \
	N(Grapple, "grapple", 27) \
	N(TeamSwitch, "team switch", 28) \
	N(Thaw, "thaw", 29) \
	N(UnknownQlMod1, "unknown QL MOD #1", 30) \
	N(HeavyMachineGun, "heavy machine gun", 31)

#define UDT_MOD_ITEM(Enum, Desc, Bit) Enum = UDT_BIT(Bit),
struct udtMeansOfDeathBits
{
	enum Id
	{
		UDT_MOD_LIST(UDT_MOD_ITEM)
		AfterLast,
		MissionPackStart = NailGun,
		MissionPackEnd = Juiced,
		QLStart = TeamSwitch,
		QLEnd = HeavyMachineGun,
	};
};
#undef UDT_MOD_ITEM

#define UDT_PLAYER_MOD_LIST(N) \
	N(Shotgun, "shotgun", 0) \
	N(Gauntlet, "gauntlet", 1) \
	N(MachineGun, "machine gun", 2) \
	N(Grenade, "grenade", 3) \
	N(GrenadeSplash, "grenade splash", 4) \
	N(Rocket, "rocket", 5) \
	N(RocketSplash, "rocket splash", 6) \
	N(Plasma, "plasma", 7) \
	N(PlasmaSplash, "plasma splash", 8) \
	N(Railgun, "railgun", 9) \
	N(Lightning, "lightning", 10) \
	N(BFG, "BFG", 11) \
	N(BFGSplash, "BFG splash", 12) \
	N(TeleFrag, "telefrag", 13) \
	N(NailGun, "nailgun", 14) \
	N(ChainGun, "chaingun", 15) \
	N(ProximityMine, "proximity mine", 16) \
	N(Kamikaze, "kamikaze", 17) \
	N(Grapple, "grapple", 18) \
	N(Thaw, "thaw", 19) \
	N(HeavyMachineGun, "heavy machine gun", 20)

#define UDT_PLAYER_MOD_ITEM(Enum, Desc, Bit) Enum = UDT_BIT(Bit),
struct udtPlayerMeansOfDeathBits
{
	enum Id
	{
		UDT_PLAYER_MOD_LIST(UDT_PLAYER_MOD_ITEM)
		AfterLast
	};
};
#undef UDT_PLAYER_MOD_ITEM

#define UDT_MEAN_OF_DEATH_LIST(N) \
	N(Shotgun, "shotgun") \
	N(Gauntlet, "gauntlet") \
	N(MachineGun, "machine gun") \
	N(Grenade, "grenade") \
	N(GrenadeSplash, "grenade splash") \
	N(Rocket, "rocket") \
	N(RocketSplash, "rocket splash") \
	N(Plasma, "plasma") \
	N(PlasmaSplash, "plasma splash") \
	N(Railgun, "railgun") \
	N(Lightning, "lightning") \
	N(BFG, "BFG") \
	N(BFGSplash, "BFG splash") \
	N(Water, "water") \
	N(Slime, "slime") \
	N(Lava, "lava") \
	N(Crush, "crush") \
	N(TeleFrag, "telefrag") \
	N(Fall, "fall") \
	N(Suicide, "suicide") \
	N(TargetLaser, "target laser") \
	N(TriggerHurt, "trigger hurt") \
	N(NailGun, "nailgun") \
	N(ChainGun, "chaingun") \
	N(ProximityMine, "proximity mine") \
	N(Kamikaze, "kamikaze") \
	N(Juiced, "juiced") \
	N(Grapple, "grapple") \
	N(TeamSwitch, "team switch") \
	N(Thaw, "thaw") \
	N(HeavyMachineGun, "heavy machine gun")

#define UDT_MEAN_OF_DEATH_ITEM(Enum, Desc) Enum,
struct udtMeanOfDeath
{
	enum Id
	{
		UDT_MEAN_OF_DEATH_LIST(UDT_MEAN_OF_DEATH_ITEM)
		Count
	};
};
#undef UDT_MEAN_OF_DEATH_ITEM

#define UDT_TEAM_LIST(N) \
	N(Free, "free") \
	N(Red, "red") \
	N(Blue, "blue") \
	N(Spectators, "spectators")

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

#define UDT_CUT_PATTERN_LIST(N) \
	N(Chat, "chat", udtCutByChatArg, udtCutByChatAnalyzer) \
	N(FragSequences, "frag sequences", udtCutByFragArg, udtCutByFragAnalyzer) \
	N(MidAirFrags, "mid-air frags", udtCutByMidAirArg, udtCutByMidAirAnalyzer) \
	N(MultiFragRails, "multi-frag rails", udtCutByMultiRailArg, udtCutByMultiRailAnalyzer) \
	N(FlagCaptures, "flag captures", udtCutByFlagCaptureArg, udtCutByFlagCaptureAnalyzer) \
	N(FlickRailFrags, "flick rails", udtCutByFlickRailArg, udtCutByFlickRailAnalyzer) \
	N(Matches, "matches", udtCutByMatchArg, udtCutByMatchAnalyzer)

#define UDT_CUT_PATTERN_ITEM(Enum, Desc, ArgType, AnalyzerType) Enum,
struct udtPatternType
{
	enum Id
	{
		UDT_CUT_PATTERN_LIST(UDT_CUT_PATTERN_ITEM)
		Count
	};
};
#undef UDT_CUT_PATTERN_ITEM

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
		Generic,    /* Format as a normal signed integer. */
		Team,       /* The integer is of type udtTeam::Id. */
		Minutes,    /* Append minutes at the end. */
		Seconds,    /* Duration in seconds, use the UDT format instead. */
		Percentage, /* Append a percentage sign at the end. */
		Weapon,     /* The integer is of type udtWeapon::Id. */
		Ping,       /* The ping in milli-seconds. */
		Positive,   /* The value must be positive or zero. */
		Boolean,    /* The value must be 0 or 1. */
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
	N(HeavyMachineGunDrops, "heavy machinegun drops", SmallerWins, Positive)

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
	N(LightningGunPickups, "lightning gun pickups", BiggerWins, Positive)

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

struct udtGameTypeFlags
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

/* @TODO: investigate obelisk harvester domination */
#define UDT_GAME_TYPE_LIST(N) \
	N(SP, "SP", "Single Player", udtGameTypeFlags::HasFragLimit) \
	N(FFA, "FFA", "Free for All", udtGameTypeFlags::HasFragLimit) \
	N(Duel, "1v1", "Duel", udtGameTypeFlags::HasFragLimit) \
	N(Race, "race", "Race", udtGameTypeFlags::None) \
	N(HM, "HM", "HoonyMode", udtGameTypeFlags::HasRoundLimit | udtGameTypeFlags::RoundBased) \
	N(RedRover, "RR", "Red Rover", udtGameTypeFlags::HasRoundLimit | udtGameTypeFlags::RoundBased) \
	N(TDM, "TDM", "Team DeathMatch", udtGameTypeFlags::HasFragLimit | udtGameTypeFlags::Team) \
	N(CBTDM, "CBTDM", "ClanBase Team DeathMatch", udtGameTypeFlags::HasFragLimit | udtGameTypeFlags::Team) \
	N(CA, "CA", "Clan Arena", udtGameTypeFlags::HasRoundLimit | udtGameTypeFlags::Team | udtGameTypeFlags::RoundBased) \
	N(CTF, "CTF", "Capture The Flag", udtGameTypeFlags::Team | udtGameTypeFlags::HasCaptureLimit) \
	N(OneFlagCTF, "1FCTF", "One Flag CTF", udtGameTypeFlags::Team | udtGameTypeFlags::HasCaptureLimit) \
	N(Obelisk, "OB", "Obelisk", udtGameTypeFlags::HasScoreLimit | udtGameTypeFlags::Team) \
	N(Harvester, "HAR", "Harvester", udtGameTypeFlags::HasScoreLimit | udtGameTypeFlags::Team) \
	N(Domination, "DOM", "Domination", udtGameTypeFlags::HasScoreLimit | udtGameTypeFlags::Team) \
	N(CTFS, "CTFS", "Capture Strike", udtGameTypeFlags::HasScoreLimit | udtGameTypeFlags::Team | udtGameTypeFlags::RoundBased) \
	N(NTF, "NTF", "Not Team Fortress", udtGameTypeFlags::Team | udtGameTypeFlags::HasCaptureLimit) \
	N(TwoVsTwo, "2v2", "2v2 TDM", udtGameTypeFlags::HasFragLimit | udtGameTypeFlags::Team) \
	N(FT, "FT", "Freeze Tag", udtGameTypeFlags::HasRoundLimit | udtGameTypeFlags::Team | udtGameTypeFlags::RoundBased)
	
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
	N(VQ3, "VQ3", "Vanilla Quake 3") \
	N(CQ3, "CQ3", "Challenge Quake 3") \
	N(PMC, "PMC", "Classic ProMode") \
	N(CPM, "CPM", "ProMode") \
	N(PMD, "PMD", "ProMode DEV") \
	N(CQL, "VQL", "Classic Quake Live") \
	N(PQL, "PQL", "Turbo Quake Live") \
	N(DQL, "QL",  "Default Quake Live")

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
		Duration,   /* Duration in milli-seconds. */
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
	N(MemoryEfficiency, "memory usage efficiency", Percentage)

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
#define    UDT_PLAYER_STATS_MASK_BYTE_COUNT    32


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

#pragma pack(push, 1)

#if defined(__cplusplus)
	struct udtParseArgFlags
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

		/* May be NULL. */
		/* This is passed as "userData" to "ProgressCb". */
		void* ProgressContext;

		/* May be NULL. */
		/* Zero to proceed, non-zero to cancel the current operation. */
		const s32* CancelOperation;

		/* May be NULL. */
		/* The array size should be udtPerfStatsField::Count. */
		u64* PerformanceStats;

		/* Ignore this. */
		void* Reserved1;

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

		/* Of type udtParseArgFlags::Id. */
		u32 Flags;

		/* Minimum duration, in milli-seconds, between 2 consecutive calls to ProgressCb. */
		u32 MinProgressTimeMs;

		/* Ignore this. */
		s32 Reserved2;
	}
	udtParseArg;

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
	
	typedef struct udtCut_s
	{
		/* Cut start time in milli-seconds. */
		s32 StartTimeMs;

		/* Cut end time in milli-seconds. */
		s32 EndTimeMs;

		/* The game state index for which this cut is applied */
		s32 GameStateIndex;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtCut;

	typedef struct udtPatternInfo_s
	{
		/* Pointer to the data structure describing the patterns/filters. */
		/* May not be NULL. */
		const void* TypeSpecificInfo;

		/* Of type udtPatternType::Id. */
		u32 Type;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtPatternInfo;

#if defined(__cplusplus)
	struct udtPlayerIndex
	{
		enum Id
		{
			FirstPersonPlayer = -2,
			DemoTaker = -1
		};
	};

	struct udtCutByPatternArgFlags
	{
		enum Id
		{
			MergeCutSections = UDT_BIT(0) /* Enable/disable merging cut sections from different patterns. */
		};
	};
#endif

	typedef struct udtCutByPatternArg_s
	{
		/* Pointer to an array of filters. */
		/* May not be NULL. */
		const udtPatternInfo* Patterns;

		/* A null-terminated lower-case string containing the player's name. */
		/* May be NULL. */
		const char* PlayerName;

		/* Number of elements in the array pointed by Patterns. */
		u32 PatternCount;

		/* Negative offset from the first matching time, in seconds. */
		u32 StartOffsetSec;

		/* Positive offset from the last matching time, in seconds. */
		u32 EndOffsetSec;

		/* The index of the player whose action we're tracking. */
		/* If not in the [0;63] range, is of type udtPlayerIndex::Id. */
		s32 PlayerIndex;

		/* Of type udtCutByPatternArgFlags::Id. */
		u32 Flags;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtCutByPatternArg;

	typedef struct udtCutByTimeArg_s
	{
		/* Pointer to an array of cut times. */
		/* May not be NULL. */
		const udtCut* Cuts;

		/* Number of elements in the array pointed by Cuts. */
		u32 CutCount;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtCutByTimeArg;

	typedef struct udtCutByChatRule_s
	{
		/* May not be NULL. */
		const char* Pattern;

		/* Of type udtChatOperator::Id. */
		u32 ChatOperator;

		/* Non-zero means case-sensitive. */
		u32 CaseSensitive;

		/* Non-zero means color codes are ignored. */
		u32 IgnoreColorCodes;

		/* Non-zero means we search team chat messages too. */
		u32 SearchTeamChat;
	}
	udtCutByChatRule;

	/* Used as udtPatternInfo::TypeSpecificInfo */
	/* when udtPatternInfo::Type is udtPatternType::GlobalChat. */
	typedef struct udtCutByChatArg_s
	{
		/* Pointer to an array of chat cutting rules. */
		/* Rules are OR'd together. */
		/* May not be NULL. */
		const udtCutByChatRule* Rules;

		/* Number of elements in the array pointed to by the Rules pointer. */
		/* May not be 0. */
		u32 RuleCount;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtCutByChatArg;

#if defined(__cplusplus)
	struct udtCutByFragArgFlags
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
	typedef struct udtCutByFragArg_s
	{
		/* The minimum amount of frags in a sequence. */
		u32 MinFragCount;

		/* Time interval between 2 consecutive frags, in seconds. */
		/* See TimeMode for the interpretation of this value. */
		u32 TimeBetweenFragsSec;

		/* @TODO: Not supported for now. */
		/* If 0, TimeBetweenFragsSec is the maximum time interval between  */
		/* 2 consecutive frags, in seconds. */
		/* If 1, TimeBetweenFragsSec is the maximum average time between frags */
		/* for the entire frag run, in seconds. */
		u32 TimeMode;

		/* Boolean options. */
		/* See udtCutByFragArgFlags. */
		u32 Flags;
		
		/* All the allowed weapons. */
		/* See udtPlayerMeansOfDeathBits. */
		u32 AllowedMeansOfDeaths;

		/* Ignore this. */
		s32 Reserved1;

		/* @TODO: */
		/*u32 AllowedPowerUps;*/
	}
	udtCutByFragArg;

	/* Used as udtPatternInfo::TypeSpecificInfo */
	/* when udtPatternInfo::Type is udtPatternType::MidAirFrags. */
	typedef struct udtCutByMidAirArg_s
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
	udtCutByMidAirArg;

	/* Used as udtPatternInfo::TypeSpecificInfo */
	/* when udtPatternInfo::Type is udtPatternType::MultiRailFrags. */
	typedef struct udtCutByMultiRailArg_s
	{
		/* The minimum amount of kills with a single rail shot. */
		/* Must be 2 or greater. */
		u32 MinKillCount;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtCutByMultiRailArg;

	/* Used as udtPatternInfo::TypeSpecificInfo */
	/* when udtPatternInfo::Type is udtPatternType::FlagCaptures. */
	typedef struct udtCutByFlagCaptureArg_s
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
	udtCutByFlagCaptureArg;
	
	/* Used as udtPatternInfo::TypeSpecificInfo */
	/* when udtPatternInfo::Type is udtPatternType::FlickRailFrags. */
	typedef struct udtCutByFlickRailArg_s
	{
		/* Minimum angular velocity, in radians/second. */
		f32 MinSpeed;

		/* How many snapshots to take into account for computing the top speed. */
		/* Range: [2;4]. */
		u32 MinSpeedSnapshotCount;

		/* Minimum angle change, in radians. */
		f32 MinAngleDelta;

		/* How many snapshots to take into account for computing the top speed. */
		/* Range: [2;4]. */
		u32 MinAngleDeltaSnapshotCount;
	}
	udtCutByFlickRailArg;

	/* Used as udtPatternInfo::TypeSpecificInfo */
	/* when udtPatternInfo::Type is udtPatternType::Matches. */
	typedef struct udtCutByMatchArg_s
	{
		/* If no match count-down was found, */
		/* start the cut this amount of time before the match's start. */
		u32 MatchStartOffsetMs;

		/* If no post-match intermission is found, */
		/* end the cut this amount of time after the match's end. */
		u32 MatchEndOffsetMs;
	}
	udtCutByMatchArg;

	typedef struct udtMapConversionRule_s
	{
		/* If the input name matches this... */
		const char* InputName;

		/* ...replace it with this. */
		const char* OutputName;

		/* Coordinates by which to shift everything. */
		/* Includes players, items, etc. Only necessary for some maps. */
		f32 PositionOffsets[3];

		/* Ignore this. */
		s32 Reserved1;
	}
	udtMapConversionRule;

	typedef struct udtProtocolConversionArg_s
	{
		/* Pointer to an array of map rules. */
		const udtMapConversionRule* MapRules;

		/* Number of elements in the array pointed to by the MapRules pointer. */
		u32 MapRuleCount;

		/* Of type udtProtocol::Id. */
		u32 OutputProtocol;
	}
	udtProtocolConversionArg;

	typedef struct udtChatEventData_s
	{
		/* All C string pointers can be NULL if extraction failed. */

		/* The original, unmodified command string. */
		const char* OriginalCommand;

		/* The player's active clan name at the time the demo was recorded. */
		/* Not available in protocol version 68. */
		/* Points to a null string if not available. */
		const char* ClanName;

		/* The player's name. */
		const char* PlayerName;

		/* The message itself. */
		const char* Message;

		/* For team messages, where the player was. */
		/* May be NULL if unavailable. */
		const char* Location;

		/* Ignore this. */
		const char* Reserved1;
	}
	udtChatEventData;

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

	typedef struct udtGameStateKeyValuePair_s
	{
		/* The name of the config string variable. */
		const char* Name;

		/* The value of the config string variable. */
		const char* Value;
	}
	udtGameStateKeyValuePair;

	typedef struct udtGameStatePlayerInfo_s
	{
		/* The player's name without color codes. */
		/* If QL, the only name. */
		/* If Q3, may have renamed later. */
		const char* FirstName;

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

	typedef struct udtParseDataGameState_s
	{
		/* Pointer to an array of match information. */
		const udtMatchInfo* Matches;

		/* Pointer to an array of string key/value pairs. */
		const udtGameStateKeyValuePair* KeyValuePairs;

		/* Pointer to an array of player information. */
		const udtGameStatePlayerInfo* Players;

		/* Name of the player who recorded the demo without color codes. */
		const char* DemoTakerName;

		/* Number of elements in the array pointed to by the Matches pointer. */
		u32 MatchCount;

		/* Number of elements in the array pointed to by the KeyValuePairs pointer. */
		u32 KeyValuePairCount;

		/* Number of elements in the array pointed to by the Players pointer. */
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

	typedef struct udtParseDataObituary_s
	{
		/* The name of the attacker or another string. */
		/* Never NULL. */
		const char* AttackerName;

		/* The name of the attacker or another string. */
		/* Never NULL. */
		const char* TargetName;

		/* The name of the attacker or another string. */
		/* Never NULL. */
		const char* MeanOfDeathName;

		/* Ignore this. */
		const char* Reserved1;

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
		s32 Reserved2;
	}
	udtParseDataObituary;

	typedef struct udtPlayerStats_s
	{
		/* The player's name at the time the stats were given by the server. */
		/* May be NULL. */
		const char* Name;

		/* The player's name at the time the stats were given by the server. */
		/* This version has color codes stripped out for clarity. */
		/* May be NULL. */
		const char* CleanName;
	}
	udtPlayerStats;

	typedef struct udtParseDataStats_s
	{
		/* A bit mask describing which teams are valid (1 is red, 2 is blue). */
		u64 ValidTeams;

		/* A bit mask describing which players are valid. */
		u64 ValidPlayers;

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

		/* NULL if nothing was found. */
		const char* ModVersion;

		/* NULL if nothing was found. */
		const char* Map;

		/* Name of the first place player or team name. */
		const char* FirstPlaceName;

		/* Name of the second place player or team name. */
		const char* SecondPlaceName;

		/* Custom red team name or NULL if not available. */
		const char* CustomRedName;

		/* Custom blue team name or NULL if not available. */
		const char* CustomBlueName;

		/* Start and end times for each time-out. */
		/* Order: start0, end0, start1, end1, etc. */
		const s32* TimeOutStartAndEndTimes;

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

		/* Ignore this. */
		s32 Reserved1;
	}
	udtParseDataStats;

	typedef struct udtParseDataRawCommand_s
	{
		/* The raw command, as it was sent from the server. */
		const char* RawCommand;

		/* The command with no color codes etc. */
		const char* CleanCommand;

		/* The time at which the server command was sent from the client. */
		s32 ServerTimeMs;

		/* The index of the last gamestate message after which this server command was sent. */
		/* Negative if invalid or not available. */
		s32 GameStateIndex;
	}
	udtParseDataRawCommand;

	typedef struct udtParseDataRawConfigString_s
	{
		/* The raw config string, as it was sent from the server. */
		const char* RawConfigString;

		/* The config string with no color codes etc. */
		const char* CleanConfigString;

		/* The index of the config string. */
		u32 ConfigStringIndex;

		/* The index of the gamestate message this config string was in. */
		/* Negative if invalid or not available. */
		s32 GameStateIndex;
	}
	udtParseDataRawConfigString;

#if defined(__cplusplus)
	struct udtParseDataCaptureFlags
	{
		enum Id
		{
			BaseToBase = UDT_BIT(0),       /* Flag picked up from its default return position. */
			DemoTaker = UDT_BIT(1),        /* Flag captured by the player who recorded the demo. */
			FirstPersonPlayer = UDT_BIT(2) /* Flag captured by a player being spectated by whoever recorded the demo. */
		};
	};
#endif

	typedef struct udtParseDataCapture_s
	{
		/* Name of the map. Can't be NULL. */
		const char* MapName;

		/* Name of the player who capped. */
		const char* PlayerName;

		/* The index of the gamestate message this config string was in. */
		/* Negative if invalid or not available. */
		s32 GameStateIndex;

		/* Server time at which the flag was picked up, in milli-seconds. */
		s32 PickUpTimeMs;

		/* Server time at which the flag was captured, in milli-seconds. */
		s32 CaptureTimeMs;

		/* Distance between the pick-up spot and the capture spot, in Quake units. */
		/* This is not the distance traveled by the capping player. */
		f32 Distance;

		/* Check the bits against values in udtParseDataCaptureFlags::Id. */
		u32 Flags;

		/* Index of the player who capped (the "client number"). */
		s32 PlayerIndex;
	}
	udtParseDataCapture;

	typedef struct udtTimeShiftArg_s
	{
		/* By how many snapshots do we shift the position of */
		/* non-first-person players back in time. */
		s32 SnapshotCount;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtTimeShiftArg;

	typedef struct udtJSONArg_s
	{
		/* Output the data to stdout when non-zero. */
		u32 ConsoleOutput;

		/* Ignore this. */
		s32 Reserved1;
	}
	udtJSONArg;

#pragma pack(pop)

	/*
	A bunch of simple stand-alone helper functions.
	*/

	/* Returns a null-terminated string describing the library's version. */
	/* Never returns NULL. */
	UDT_API(const char*) udtGetVersionString();

	/* Returns a null-terminated string describing the error. */
	/* Never returns NULL. */
	UDT_API(const char*) udtGetErrorCodeString(s32 errorCode);

	/* Returns zero if not a valid protocol. */
	/* The protocol argument is of type udtProtocol::Id. */
	UDT_API(s32) udtIsValidProtocol(u32 protocol);

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

	/* Gets the address and element count for the requested parse data type. */
	/* The "plugInId" argument is of type udtParserPlugIn::Id. */
	UDT_API(s32) udtGetDemoDataInfo(udtParserContext* context, u32 demoIdx, u32 plugInId, void** buffer, u32* count);

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
	UDT_API(s32) udtCutDemoFilesByPattern(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtCutByPatternArg* patternInfo);

	/* Creates, for each demo that isn't in the target protocol, a new demo file with the specified protocol. */
	UDT_API(s32) udtConvertDemoFiles(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtProtocolConversionArg* conversionArg);

	/* Creates, for each demo, a new demo where non-first-person player entities are shifted back in time by the specified amount of snapshots. */
	UDT_API(s32) udtTimeShiftDemoFiles(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtTimeShiftArg* timeShiftArg);

	/* Creates, for each demo, a .JSON file with the data from all the selected plug-ins. */
	UDT_API(s32) udtSaveDemoFilesAnalysisDataToJSON(const udtParseArg* info, const udtMultiParseArg* extraInfo, const udtJSONArg* jsonInfo);

#ifdef __cplusplus
}
#endif


#undef UDT_API
