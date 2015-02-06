#pragma once


#include "api.h"
#include "types.hpp"

#include <string.h>
#include <stdio.h>


typedef s32 qbool;
#define qfalse (qbool)(0)
#define qtrue (qbool)(!0)
typedef qbool qboolean;

#if defined(Q3_VM)
typedef s32 s32ptr_t;
#elif defined(_MSC_VER)
#include <stddef.h>
typedef __int64 int64_t;
typedef __int32 int32_t;
typedef __int16 int16_t;
typedef __int8  int8_t;
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8	 uint8_t;
typedef ptrdiff_t intptr_t;
#else
//#include <stds32.h>
//#include <stdint.h>
#endif

typedef unsigned char		byte;

typedef s32		qhandle_t;

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];


#define	MAX_GLOBAL_SERVERS			4096
#define	MAX_OTHER_SERVERS			128
#define MAX_PINGREQUESTS			32
#define MAX_SERVERSTATUSREQUESTS	16

#define Q_COLOR_ESCAPE	'^'
#define Q_IsColorString(p)	( p && *(p) == Q_COLOR_ESCAPE && *((p)+1) && *((p)+1) != Q_COLOR_ESCAPE )

#define COMPILE_TIME_ASSERT( pred ) 

// angle indexes
#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over

// the game guarantees that no string from the network will ever exceed MAX_STRING_CHARS
#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	1024	// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		1024	// max length of an individual token

#define	MAX_INFO_STRING		1024
#define	MAX_INFO_KEY		1024
#define	MAX_INFO_VALUE		1024

#define	BIG_INFO_STRING		8192  // used for system info key only
#define	BIG_INFO_KEY		8192
#define	BIG_INFO_VALUE		8192


#define	MAX_QPATH			64		// max length of a quake game pathname
#define	MAX_OSPATH			256		// max length of a filesystem pathname

#define	MAX_NAME_LENGTH		32		// max length of a client name


//
// these aren't needed by any of the VMs.  put in another header?
//
#define	MAX_MAP_AREA_BYTES		32		// bit vector of area visibility

//=============================================

// TTimo
// centralized and cleaned, that's the max string you can send to a Com_Printf / Com_DPrintf (above gets truncated)
#define	MAXPRINTMSG	4096

// TTimo
// vsnprintf is ISO/IEC 9899:1999
// abstracting this to make it portable
#if defined(_WIN32) || defined(_MSC_VER) // myT
#define Q_vsnprintf _vsnprintf
#else
// TODO: do we need Mac define?
#define Q_vsnprintf vsnprintf
#endif

//endianness
extern short ShortSwap (short l);
extern s32 LongSwap (s32 l);
extern f32 FloatSwap (const f32 *f);

#define LittleShort
#define LittleLong
#define LittleFloat
#define BigShort(x) ShortSwap(x)
#define BigLong(x) LongSwap(x)
#define BigFloat(x) FloatSwap(&x)

#define Com_Memset memset
#define Com_Memcpy memcpy

// usercmd_t is sent to the server each client frame
typedef struct usercmd_s {
	s32			serverTime;
	s32			angles[3];
	s32			buttons;
	u8		weapon;
	s8	forwardmove, rightmove, upmove;
} usercmd_t;

// if entityState->solid == SOLID_BMODEL, modelindex is an inline model number
#define	SOLID_BMODEL	0xffffff

typedef enum {
	TR_STATIONARY,
	TR_INTERPOLATE,				// non-parametric, but interpolate between snapshots
	TR_LINEAR,
	TR_LINEAR_STOP,
	TR_SINE,					// value = base + sin( time / duration ) * delta
	TR_GRAVITY
} trType_t;

struct idTrajectoryBase
{
	trType_t	trType;
	s32			trTime;
	s32			trDuration;			// if non 0, trTime + trDuration = stop time
	vec3_t		trBase;
	vec3_t		trDelta;			// velocity, etc
};

// entityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large
struct idEntityStateBase
{
	s32		number;			// entity index
	s32		eType;			// entityType_t
	s32		eFlags;

	idTrajectoryBase	pos;
	idTrajectoryBase	apos;

	s32		time;
	s32		time2;

	vec3_t	origin;
	vec3_t	origin2;

	vec3_t	angles;
	vec3_t	angles2;

	s32		otherEntityNum;	// shotgun sources, etc
	s32		otherEntityNum2;

	s32		groundEntityNum;// ENTITYNUM_NONE = in air

	s32		constantLight;	// r + (g<<8) + (b<<16) + (s32ensity<<24)
	s32		loopSound;		// constantly loop this sound

	s32		modelindex;
	s32		modelindex2;
	s32		clientNum;		// 0 to (MAX_CLIENTS - 1), for players and corpses
	s32		frame;

	s32		solid;			// for client side prediction, trap_linkentity sets this properly

	s32		event;			// impulse events -- muzzle flashes, footsteps, etc
	s32		eventParm;

	// for players
	s32		powerups;		// bit flags
	s32		weapon;			// determines weapon and flash model, etc
	s32		legsAnim;		// mask off ANIM_TOGGLEBIT
	s32		torsoAnim;		// mask off ANIM_TOGGLEBIT

	s32		generic1;
};

struct idEntityState68 : idEntityStateBase
{
};

struct idEntityState73 : idEntityStateBase
{
	// New in dm_73.
	s32		pos_gravity;  // part of idEntityStateBase::pos trajectory
	s32		apos_gravity; // part of idEntityStateBase::apos trajectory
};

struct idEntityState90 : idEntityStateBase
{
	// New in dm_73.
	s32		pos_gravity;  // part of idEntityStateBase::pos trajectory
	s32		apos_gravity; // part of idEntityStateBase::apos trajectory

	// New in dm_90.
	int		jumpTime;
	qbool	doubleJumped;
};

typedef idEntityState90 idLargestEntityState;

/*
========================================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

========================================================================
*/

//
// per-level limits
//
#define	MAX_CLIENTS			64		// absolute limit
#define MAX_LOCATIONS		64

#define	GENTITYNUM_BITS		10		// don't need to send any more
#define	MAX_GENTITIES		(1<<GENTITYNUM_BITS)

// entitynums are communicated with GENTITY_BITS, so any reserved
// values that are going to be communcated over the net need to
// also be in this range
#define	ENTITYNUM_NONE		(MAX_GENTITIES-1)
#define	ENTITYNUM_WORLD		(MAX_GENTITIES-2)
#define	ENTITYNUM_MAX_NORMAL	(MAX_GENTITIES-2)


#define	MAX_MODELS			256		// these are sent over the net as 8 bits
#define	MAX_SOUNDS			256		// so they cannot be blindly increased

typedef enum {
	TEAM_FREE,
	TEAM_RED,
	TEAM_BLUE,
	TEAM_SPECTATOR,

	TEAM_NUM_TEAMS
} team_t;

#define	MAX_CONFIGSTRINGS	1024

// these are the only configstrings that the system reserves, all the
// other ones are strictly for servergame to clientgame communication
#define	CS_SERVERINFO			0		// an info string with all the serverinfo cvars
#define	CS_SYSTEMINFO			1		// an info string for server system to client system configuration (timescale, etc)
#define	CS_MUSIC				2
#define	CS_MESSAGE				3		// from the map worldspawn's message field
#define	CS_MOTD					4		// g_motd string for server message of the day
#define	CS_WARMUP				5		// server time when the match will be restarted
#define	CS_SCORES1				6
#define	CS_SCORES2				7
#define CS_VOTE_TIME			8
#define CS_VOTE_STRING			9
#define	CS_VOTE_YES				10
#define	CS_VOTE_NO				11
#define CS_TEAMVOTE_TIME_68		12		// q3 only
#define CS_TEAMVOTE_STRING_68	14		// q3 only
#define	CS_TEAMVOTE_YES_68		16		// q3 only
#define	CS_TEAMVOTE_NO_68		18		// q3 only
#define	CS_GAME_VERSION_68		20
#define	CS_LEVEL_START_TIME_68	21		// so the timer only shows the current level
#define	CS_INTERMISSION_68		22		// when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define CS_FLAGSTATUS_68		23		// string indicating flag status in CTF
#define CS_SHADERSTATE_68		24
#define CS_BOTINFO_68			25		// q3 only
#define	CS_ITEMS_68				27		// string of 0's and 1's that tell which items are present
#define	CS_MODELS_68			32
#define CS_WARMUP_END           13
#define CS_INTERMISSION_73      14
#define CS_PAUSE_START          669
#define CS_PAUSE_COUNTDOWN      670
#define CS_CA_ROUND_INFO        661
#define CS_CA_ROUND_START       662
#define	CS_PLAYERS_68           544
#define CS_RED_CLAN_PLAYERS     663
#define CS_BLUE_CLAN_PLAYERS    664
#define CS_FLAG_STATUS_73       658
#define CS_FIRST_PLACE          659
#define CS_SECOND_PLACE         660
#define CS_AD_WAIT              681
#define	CS_SOUNDS_68			(CS_MODELS_68 + MAX_MODELS)
#define CS_LOCATIONS_68			(CS_PLAYERS_68 + MAX_CLIENTS)
#define CS_PARTICLES_68			(CS_LOCATIONS_68 + MAX_LOCATIONS)
#define CS_PAST_LAST_INDEX_68	(CS_PARTICLES_68 + MAX_LOCATIONS)

#define	CS_GAME_VERSION_73p		12
#define CS_LEVEL_START_TIME_73p 13
#define	CS_INTERMISSION_73p		14
#define CS_ITEMS_73p			15
#define	CS_MODELS_73p			17
#define CS_SOUNDS_73p           274
#define	CS_PLAYERS_73p          529
#define CS_LOCATIONS_73p		593
#define CS_FLAGSTATUS_73p		658
#define CS_SHADERSTATE_73p		665
#define CS_PARTICLES_73p		(CS_LOCATIONS_73p + MAX_LOCATIONS)
// Quake Live 688 689 ???
// CPMA
#define CS_CPMA_GAME_INFO       672
#define CS_CPMA_ROUND_INFO      710

#define	RESERVED_CONFIGSTRINGS	2	// game can't modify below this, only the system can

#define	MAX_GAMESTATE_CHARS	16000
typedef struct {
	s32			stringOffsets[MAX_CONFIGSTRINGS];
	s8		stringData[MAX_GAMESTATE_CHARS];
	s32			dataCount;
} gameState_t;

//=========================================================

// bit field limits
#define	MAX_STATS				16
#define	MAX_PERSISTANT			16
#define	MAX_POWERUPS			16
#define	MAX_WEAPONS				16

#define	MAX_PS_EVENTS			2

#define PS_PMOVEFRAMECOUNTBITS	6

// playerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c

// playerState_t is a full superset of entityState_t as it is used by players,
// so if a playerState_t is transmitted, the entityState_t can be fully derived
// from it.
struct idPlayerStateBase
{
	s32			commandTime;	// cmd->serverTime of last executed command
	s32			pm_type;
	s32			bobCycle;		// for view bobbing and footstep generation
	s32			pm_flags;		// ducked, jump_held, etc
	s32			pm_time;

	vec3_t		origin;
	vec3_t		velocity;
	s32			weaponTime;
	s32			gravity;
	s32			speed;
	s32			delta_angles[3];	// add to command angles to get view direction
									// changed by spawns, rotating objects, and teleporters

	s32			groundEntityNum;// ENTITYNUM_NONE = in air

	s32			legsTimer;		// don't change low priority animations until this runs out
	s32			legsAnim;		// mask off ANIM_TOGGLEBIT

	s32			torsoTimer;		// don't change low priority animations until this runs out
	s32			torsoAnim;		// mask off ANIM_TOGGLEBIT

	s32			movementDir;	// a number 0 to 7 that represents the reletive angle
								// of movement to the view angle (axial and diagonals)
								// when at rest, the value will remain unchanged
								// used to twist the legs during strafing

	vec3_t		grapplePoint;	// location of grapple to pull towards if PMF_GRAPPLE_PULL

	s32			eFlags;			// copied to entityState_t->eFlags

	s32			eventSequence;	// pmove generated events
	s32			events[MAX_PS_EVENTS];
	s32			eventParms[MAX_PS_EVENTS];

	s32			externalEvent;	// events set on player from another source
	s32			externalEventParm;
	s32			externalEventTime;

	s32			clientNum;		// ranges from 0 to MAX_CLIENTS-1
	s32			weapon;			// copied to entityState_t->weapon
	s32			weaponstate;

	vec3_t		viewangles;		// for fixed views
	s32			viewheight;

	// damage feedback
	s32			damageEvent;	// when it changes, latch the other parms
	s32			damageYaw;
	s32			damagePitch;
	s32			damageCount;

	s32			stats[MAX_STATS];
	s32			persistant[MAX_PERSISTANT];	// stats that aren't cleared on death
	s32			powerups[MAX_POWERUPS];	// level.time that the powerup runs out
	s32			ammo[MAX_WEAPONS];

	s32			generic1;
	s32			loopSound;
	s32			jumppad_ent;	// jumppad entity hit this frame

	// not communicated over the net at all
	s32			ping;			// server to game info for scoreboard
	s32			pmove_framecount;	// FIXME: don't transmit over the network
	s32			jumppad_frame;
	s32			entityEventSequence;
};

struct idPlayerState68 : idPlayerStateBase
{
};

struct idPlayerState73 : idPlayerStateBase
{
};

struct idPlayerState90 : idPlayerStateBase
{
	qboolean doubleJumped;
	int jumpTime;
	int unknown1;
	int unknown2;
	int unknown3;
};

typedef idPlayerState90 idLargestPlayerState;

//====================================================================

s32 Q_isprint( s32 c );
s32 Q_islower( s32 c );
s32 Q_isupper( s32 c );
s32 Q_isalpha( s32 c );

// portable case insensitive compare
s32			Q_stricmp( const char *s1, const char *s2 );
s32			Q_strncmp( const char *s1, const char *s2, s32 n );
s32			Q_stricmpn( const char *s1, const char *s2, s32 n );
const char	*Q_strrchr( const char* string, s32 c );

// buffer size safe library replacements
void	Q_strncpyz( char *dest, const char *src, s32 destsize );
void	Q_strcat( char *dest, s32 size, const char *src );

/*
==============================================================

NET

==============================================================
*/

#define	PACKET_BACKUP	32	// number of old messages that must be kept on client and
							// server for delta comrpession and ping estimation
#define	PACKET_MASK		(PACKET_BACKUP-1)

#define	MAX_PACKET_USERCMDS		32		// max number of usercmd_t in a packet

#define	PORT_ANY			-1

#define	MAX_RELIABLE_COMMANDS	64			// max string commands buffered for restransmit

typedef enum {
	NA_BOT,
	NA_BAD,					// an address lookup failed
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
} netadrtype_t;

typedef enum {
	NS_CLIENT,
	NS_SERVER
} netsrc_t;

typedef struct {
	netadrtype_t type;
	u8 ip[4];
	unsigned short port;
} netadr_t;

#define MAX_MSGLEN 16384 // max length of a message, which may be fragmented s32o multiple packets

/*
==============================================================

PROTOCOL

==============================================================
*/

#define	PROTOCOL_VERSION	68

#define	UPDATE_SERVER_NAME	"update.quake3arena.com"
// override on command line, config files etc.
#ifndef MASTER_SERVER_NAME
#define MASTER_SERVER_NAME	"master.quake3arena.com"
#endif
#ifndef AUTHORIZE_SERVER_NAME
#define	AUTHORIZE_SERVER_NAME	"authorize.quake3arena.com"
#endif

#define	PORT_MASTER			27950
#define	PORT_UPDATE			27951
#ifndef PORT_AUTHORIZE
#define	PORT_AUTHORIZE		27952
#endif
#define	PORT_SERVER			27960
#define	NUM_SERVER_PORTS	4		// broadcast scan this many ports after
									// PORT_SERVER so a single machine can
									// run multiple servers


// the svc_strings[] array in cl_parse.c should mirror this
//
// server to client
//
enum svc_ops_e
{
	svc_bad,
	svc_nop,
	svc_gamestate,
	svc_configstring,			// [short] [string] only in gamestate messages
	svc_baseline,				// only in gamestate messages
	svc_serverCommand,			// [string] to be executed by client game module
	svc_download,				// [short] size [size bytes]
	svc_snapshot,
	svc_EOF,
	// svc_extension follows a svc_EOF, followed by another svc_* ...
	// this keeps legacy clients compatible.
	svc_extension,
	svc_voip,     // not wrapped in USE_VOIP, so this value is reserved.
};

//
// client to server
//
enum clc_ops_e {
	clc_bad,
	clc_nop, 		
	clc_move,				// [[usercmd_t]
	clc_moveNoDelta,		// [[usercmd_t]
	clc_clientCommand,		// [string] message
	clc_EOF
};

// snapshots are a view of the server at a given time
struct idClientSnapshotBase
{
	qbool		valid;			// cleared if delta parsing was invalid
	s32				snapFlags;		// rate delayed and dropped commands

	s32				serverTime;		// server time the message is valid for (in msec)

	s32				messageNum;		// copied from netchan->incoming_sequence
	s32				deltaNum;		// messageNum the delta is from
	s32				ping;			// time from when cmdNum-1 was sent to time packet was reeceived
	u8			areamask[MAX_MAP_AREA_BYTES];		// portalarea visibility bits

	s32				cmdNum;			// the next cmdNum the server is expecting

	s32				numEntities;			// all of the entities that need to be presented
	s32				parseEntitiesNum;		// at the time of this snapshot

	s32				serverCommandNum;		// execute all commands up to this before
	// making the snapshot current

	//PlayerStateType	ps;						// complete information about the current player at this time
};

struct idClientSnapshot68 : idClientSnapshotBase
{
	idPlayerState68 ps; // complete information about the current player at this time
};

struct idClientSnapshot73 : idClientSnapshotBase
{
	idPlayerState73 ps; // complete information about the current player at this time
};

struct idClientSnapshot90 : idClientSnapshotBase
{
	idPlayerState90 ps; // complete information about the current player at this time
};

typedef idClientSnapshot90 idLargestClientSnapshot;

inline idPlayerStateBase* GetPlayerState(idClientSnapshotBase* snap, udtProtocol::Id protocol)
{
	if(protocol == udtProtocol::Dm68)
	{
		return &((idClientSnapshot68*)snap)->ps;
	}

	if(protocol == udtProtocol::Dm73)
	{
		return &((idClientSnapshot73*)snap)->ps;
	}

	if(protocol == udtProtocol::Dm90)
	{
		return &((idClientSnapshot90*)snap)->ps;
	}

	return NULL;
}

// allow a lot of command backups for very fast systems
// multiple commands may be combined s32o a single packet, so this
// needs to be larger than PACKET_BACKUP
#define	CMD_BACKUP		64
#define	CMD_MASK		(CMD_BACKUP - 1)

/*
=============================================================================

the clientActive_t structure is wiped completely at every
new gamestate_t, potentially several times during an established connection

=============================================================================
*/

// the parseEntities array must be large enough to hold PACKET_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original 
#define	MAX_PARSE_ENTITIES	2048

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define	EV_EVENT_BIT1		0x00000100
#define	EV_EVENT_BIT2		0x00000200
#define	EV_EVENT_BITS		(EV_EVENT_BIT1|EV_EVENT_BIT2)

#define	EVENT_VALID_MSEC	300

typedef enum {
	EV_NONE,

	EV_FOOTSTEP,
	EV_FOOTSTEP_METAL,
	EV_FOOTSPLASH,
	EV_FOOTWADE,
	EV_SWIM,

	EV_STEP_4,
	EV_STEP_8,
	EV_STEP_12,
	EV_STEP_16,

	EV_FALL_SHORT_68,
	EV_FALL_MEDIUM_68,
	EV_FALL_FAR_68,

	EV_JUMP_PAD_68,			// boing sound at origin, jump sound on player

	EV_JUMP_68,
	EV_WATER_TOUCH_68,	// foot touches
	EV_WATER_LEAVE_68,	// foot leaves
	EV_WATER_UNDER_68,	// head touches
	EV_WATER_CLEAR_68,	// head leaves

	EV_ITEM_PICKUP_68,			// normal item pickups are predictable
	EV_GLOBAL_ITEM_PICKUP_68,	// powerup / team sounds are broadcast to everyone

	EV_NOAMMO_68,
	EV_CHANGE_WEAPON_68,
	EV_FIRE_WEAPON_68,

	EV_USE_ITEM0_68,
	EV_USE_ITEM1,
	EV_USE_ITEM2,
	EV_USE_ITEM3,
	EV_USE_ITEM4,
	EV_USE_ITEM5,
	EV_USE_ITEM6,
	EV_USE_ITEM7,
	EV_USE_ITEM8,
	EV_USE_ITEM9,
	EV_USE_ITEM10,
	EV_USE_ITEM11,
	EV_USE_ITEM12,
	EV_USE_ITEM13,
	EV_USE_ITEM14,
	EV_USE_ITEM15,

	EV_ITEM_RESPAWN,
	EV_ITEM_POP,
	EV_PLAYER_TELEPORT_IN,
	EV_PLAYER_TELEPORT_OUT,

	EV_GRENADE_BOUNCE,		// eventParm will be the soundindex

	EV_GENERAL_SOUND,
	EV_GLOBAL_SOUND,		// no attenuation
	EV_GLOBAL_TEAM_SOUND,

	EV_BULLET_HIT_FLESH,
	EV_BULLET_HIT_WALL,

	EV_MISSILE_HIT,
	EV_MISSILE_MISS,
	EV_MISSILE_MISS_METAL,
	EV_RAILTRAIL,
	EV_SHOTGUN,
	EV_BULLET,				// otherEntity is the shooter

	EV_PAIN,
	EV_DEATH1,
	EV_DEATH2,
	EV_DEATH3,
	EV_OBITUARY_68,

	EV_POWERUP_QUAD,
	EV_POWERUP_BATTLESUIT,
	EV_POWERUP_REGEN,

	EV_GIB_PLAYER,			// gib a previously living player
	EV_SCOREPLUM,			// score plum

	//#ifdef MISSIONPACK
	EV_PROXIMITY_MINE_STICK,
	EV_PROXIMITY_MINE_TRIGGER,
	EV_KAMIKAZE,			// kamikaze explodes
	EV_OBELISKEXPLODE,		// obelisk explodes
	EV_OBELISKPAIN,			// obelisk is in pain
	EV_INVUL_IMPACT,		// invulnerability sphere impact
	EV_JUICED,				// invulnerability juiced effect
	EV_LIGHTNINGBOLT,		// lightning bolt bounced of invulnerability sphere
	//#endif

	EV_DEBUG_LINE,
	EV_STOPLOOPINGSOUND,
	EV_TAUNT,
	EV_TAUNT_YES,
	EV_TAUNT_NO,
	EV_TAUNT_FOLLOWME,
	EV_TAUNT_GETFLAG,
	EV_TAUNT_GUARDBASE,
	EV_TAUNT_PATROL
} entity_event_t;

typedef enum {
	EV_FOOTSTEP_73p = 1,
	EV_FOOTSTEP_METAL_73p = 2,
	EV_FOOTSPLASH_73p = 3,
	EV_FOOTWADE_73p = 4, // guess
	EV_SWIM_73p = 5,     // guess
	EV_FALL_SHORT_73p = 6,
	EV_FALL_MEDIUM_73p = 7,
	EV_FALL_FAR_73p = 8,
	EV_JUMP_PAD_73p = 9,
	EV_JUMP_73p = 10,
	EV_WATER_TOUCH_73p = 11,
	EV_WATER_LEAVE_73p = 12,
	EV_WATER_UNDER_73p = 13,
	EV_WATER_CLEAR_73p = 14,
	EV_ITEM_PICKUP_73p = 15,
	EV_GLOBAL_ITEM_PICKUP_73p = 16,
	EV_NOAMMO_73p = 17,
	EV_CHANGE_WEAPON_73p = 18,
	EV_DROP_WEAPON_73p = 19,
	EV_FIRE_WEAPON_73p = 20,
	EV_USE_ITEM0_73p = 21,

	EV_BULLET_HIT_FLESH_73p = 45,

	EV_DEATH1_73p = 54,
	EV_DEATH2_73p = 55,
	EV_DEATH3_73p = 56,
	EV_DROWN_73p = 57,
	EV_OBITUARY_73p = 58,

	EV_POWERUP_QUAD_73p,  // guess
	EV_POWERUP_BATTLESUIT_73p = 60,
	EV_POWERUP_REGEN_73p = 61,  // 62 in older demo zero4 vs cl0ck
	EV_POWERUP_ARMOR_REGEN_73p = 62,  // ctf silentnight (2010-12-26)  armor regen?
	EV_GIB_PLAYER_73p = 63,
	EV_SCOREPLUM_73p = 64,

	EV_PROXIMITY_MINE_STICK_73p = 65,
	EV_PROXIMITY_MINE_TRIGGER_73p = 66,

	EV_KAMIKAZE_73p = 67,			// kamikaze explodes
	EV_OBELISKEXPLODE_73p = 68,
	EV_OBELISKPAIN_73p = 69,
	EV_INVUL_IMPACT_73p = 70,		// invulnerability sphere impact

	EV_DEBUG_LINE_73p = 72,  // guess
	EV_STOPLOOPINGSOUND_73p = 73, // guess
	EV_TAUNT_73p = 74,
	EV_TAUNT_YES_73p,  // guess
	EV_TAUNT_NO_73p,  // guess
	EV_TAUNT_FOLLOWME_73p,  // guess
	EV_TAUNT_GETFLAG_73p,  // guess
	EV_TAUNT_GUARDBASE_73p,  // guess
	EV_TAUNT_PATROL_73p,  // guess

	EV_FOOTSTEP_SNOW_73p = 81,
	EV_FOOTSTEP_WOOD_73p = 82,
	EV_ITEM_PICKUP_SPEC_73p = 83,
	EV_OVERTIME_73p = 84,
	EV_GAMEOVER_73p = 85,

	EV_THAW_PLAYER_73p = 87,
	EV_THAW_TICK_73p = 88,
	EV_HEADSHOT_73p = 89,
	EV_POI_73p = 90,

	EV_RACE_START_73p = 93,
	EV_RACE_CHECKPOINT_73p = 94,
	EV_RACE_END_73p = 95,

	EV_DAMAGEPLUM_73p = 96,
	EV_AWARD_73p = 97,
} entity_event_73p_t;

// Means of Death
struct idMeansOfDeath68
{
	enum Id
	{
		Unknown,
		Shotgun,
		Gauntlet,
		MachineGun,
		Grenade,
		GrenadeSplash,
		Rocket,
		RocketSplash,
		Plasma,
		PlasmaSplash,
		RailGun,
		Lightning,
		BFG,
		BFGSplash,
		Water,
		Slime,
		Lava,
		Crush,
		TeleFrag,
		Fall,
		Suicide,
		TargetLaser,
		HurtTrigger,
		Grapple,
		Count
	};
};

struct idMeansOfDeath73p
{
	enum Id
	{
		Unknown,
		Shotgun,
		Gauntlet,
		MachineGun,
		Grenade,
		GrenadeSplash,
		Rocket,
		RocketSplash,
		Plasma,
		PlasmaSplash,
		RailGun,
		Lightning,
		BFG,
		BFGSplash,
		Water,
		Slime,
		Lava,
		Crush,
		TeleFrag,
		Fall,
		Suicide,
		TargetLaser,
		HurtTrigger,
		// Mission pack start
		NailGun,
		ChainGun,
		ProximityMine,
		Kamikaze,
		Juiced,
		// Mission pack end
		Grapple,
		// QL start
		TeamSwitch,
		Thaw,
		UnknownQlMod1,
		HeavyMachineGun,
		// QL end
		Count
	};
};

//
// entityState_t->eType
//
typedef enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_MOVER,
	ET_BEAM,
	ET_PORTAL,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_INVISIBLE,
	ET_GRAPPLE,				// grapple hooked on wall
	ET_TEAM,

	ET_EVENTS				// any of the EV_* events can be added freestanding
							// by setting eType to ET_EVENTS + eventNum
							// this avoids having to set eFlags and eventNum
} entityType_t;

// NOTE: may not have more than 16
typedef enum {
	PW_NONE,

	PW_QUAD,
	PW_BATTLESUIT,
	PW_HASTE,
	PW_INVIS,
	PW_REGEN,
	PW_FLIGHT,

	PW_REDFLAG,
	PW_BLUEFLAG,
	PW_NEUTRALFLAG,

	PW_SCOUT,
	PW_GUARD,
	PW_DOUBLER,
	PW_AMMOREGEN,
	PW_INVULNERABILITY,

	PW_NUM_POWERUPS,

	PW_FIRST = PW_QUAD,
	PW_LAST = PW_INVULNERABILITY

} powerup_t;


// player_state->stats[] indexes
// NOTE: may not have more than 16
typedef enum {
	STAT_HEALTH_68,
	STAT_HOLDABLE_ITEM_68,
	STAT_WEAPONS_68,					// 16 bit fields
	STAT_ARMOR_68,				
	STAT_DEAD_YAW_68,					// look this direction when dead (FIXME: get rid of?)
	STAT_CLIENTS_READY_68,				// bit mask of clients wishing to exit the s32ermission (FIXME: configstring?)
	STAT_MAX_HEALTH_68					// health / armor limit, changable by handicap
} statIndex_68_t;

typedef enum {
	STAT_HEALTH_73p,
	STAT_HOLDABLE_ITEM_73p,
	STAT_PERSISTANT_POWERUP_73p,
	STAT_WEAPONS_73p,					// 16 bit fields
	STAT_ARMOR_73p,				
	STAT_BATTLE_SUIT_KILL_COUNT_73p,
	STAT_CLIENTS_READY_73p,				// bit mask of clients wishing to exit the s32ermission (FIXME: configstring?)
	STAT_MAX_HEALTH_73p,				// health / armor limit, changable by handicap
	STAT_POWERUP_REMAINING_73p = 11,
	STAT_QUAD_KILL_COUNT_73p = 13,
	STAT_ARMOR_TIER_73p = 14
} statIndex_73p_t;

// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
// NOTE: may not have more than 16
typedef enum {
	PERS_SCORE_68,					// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_HITS_68,					// total pos32s damage inflicted so damage beeps can sound on change
	PERS_RANK_68,					// player rank or team rank
	PERS_TEAM_68,					// player team
	PERS_SPAWN_COUNT_68,			// incremented every respawn
	PERS_PLAYEREVENTS_68,			// 16 bits that can be flipped for events
	PERS_ATTACKER_68,				// clientnum of last damage inflicter
	PERS_ATTACKEE_ARMOR_68,			// health/armor of last person we attacked
	PERS_KILLED_68,					// count of the number of times you died
	// player awards tracking
	PERS_IMPRESSIVE_COUNT_68,		// two railgun hits in a row
	PERS_EXCELLENT_COUNT_68,		// two successive kills in a short amount of time
	PERS_DEFEND_COUNT_68,			// defend awards
	PERS_ASSIST_COUNT_68,			// assist awards
	PERS_GAUNTLET_FRAG_COUNT_68,	// kills with the guantlet
	PERS_CAPTURES_68				// captures
} persEnum_68_t;

typedef enum
{
	PERS_SCORE_73p = 0,
	PERS_HITS_73p = 1,
	PERS_RANK_73p = 2,
	PERS_TEAM_73p = 3,
	PERS_SPAWN_COUNT_73p = 4,
	PERS_PLAYEREVENTS_73p = 5,
	PERS_ATTACKER_73p = 6,
	PERS_KILLED_73p = 7,
	PERS_IMPRESSIVE_COUNT_73p = 8,
	PERS_EXCELLENT_COUNT_73p = 9,
	PERS_DEFEND_COUNT_73p = 10,
	PERS_ASSIST_COUNT_73p = 11,
	PERS_GAUNTLET_FRAG_COUNT_73p = 12,
	PERS_CAPTURES_73p = 13,
	PERS_ATTACKEE_ARMOR_73p = 14
} persEnum_73_t;

// entityState_t->eFlags
#define	EF_DEAD				0x00000001		// don't draw a foe marker over players with EF_DEAD
#define EF_TICKING_73		0x00000002		// used to make players play the prox mine ticking sound
#define	EF_TELEPORT_BIT		0x00000004		// toggled every time the origin abruptly changes
#define	EF_AWARD_EXCELLENT	0x00000008		// draw an excellent sprite
#define EF_PLAYER_EVENT		0x00000010
#define	EF_BOUNCE			0x00000010		// for missiles
#define	EF_BOUNCE_HALF		0x00000020		// for missiles
#define	EF_AWARD_GAUNTLET	0x00000040		// draw a gauntlet sprite
#define	EF_NODRAW			0x00000080		// may have an event, but no model (unspawned items)
#define	EF_FIRING			0x00000100		// for lightning gun
#define	EF_KAMIKAZE			0x00000200
#define	EF_MOVER_STOP		0x00000400		// will push otherwise
#define EF_AWARD_CAP		0x00000800		// draw the capture sprite
#define	EF_TALK				0x00001000		// draw a talk balloon
#define	EF_CONNECTION		0x00002000		// draw a connection trouble sprite
#define	EF_VOTED			0x00004000		// already cast a vote
#define	EF_AWARD_IMPRESSIVE	0x00008000		// draw an impressive sprite
#define	EF_AWARD_DEFEND		0x00010000		// draw a defend sprite
#define	EF_AWARD_ASSIST		0x00020000		// draw a assist sprite
#define EF_AWARD_DENIED		0x00040000		// denied
#define EF_TEAMVOTED		0x00080000		// already cast a team votetypedef enum 

typedef enum
{
	// -------
	// Quake 3
	// -------
	ITEM_INVALID,
	//ITEM_INVALID2,
	ITEM_ARMOR_SHARD,
	ITEM_ARMOR_COMBAT,

	ITEM_ARMOR_BODY,
	ITEM_ARMOR_GREEN,// MM
	ITEM_HEALTH_SMALL,
	ITEM_HEALTH,
	ITEM_HEALTH_LARGE,
	ITEM_HEALTH_MEGA,
	WEAPON_GAUNTLET,
	WEAPON_SHOTGUN,
	WEAPON_MACHINEGUN,
	WEAPON_GRENADELAUNCHER,
	WEAPON_ROCKETLAUNCHER,
	WEAPON_LIGHTNING,
	WEAPON_RAILGUN,	 
	WEAPON_PLASMAGUN,
	WEAPON_BFG,
	WEAPON_GRAPPLINGHOOK,
	AMMO_SHELLS, // shotgun
	AMMO_BULLETS,
	AMMO_GRENADES,
	AMMO_CELLS, // plasma gun
	AMMO_LIGHTNING,
	AMMO_ROCKETS,
	AMMO_SLUGS, // rail gun
	AMMO_BFG,
	HOLDABLE_TELEPORTER,
	HOLDABLE_MEDKIT,
	ITEM_QUAD,
	ITEM_ENVIRO, // battle suit
	ITEM_HASTE,
	ITEM_INVIS,
	ITEM_REGEN,
	ITEM_FLIGHT,
	TEAM_CTF_REDFLAG,
	TEAM_CTF_BLUEFLAG,
	// -----------------------
	// Team Arena / Quake Live
	// -----------------------
	HOLDABLE_KAMIKAZE,
	HOLDABLE_PORTAL,
	HOLDABLE_INVULNERABILITY,
	AMMO_NAILS,
	AMMO_MINES,
	AMMO_BELT, // chain gun
	ITEM_SCOUT,
	ITEM_GUARD,
	ITEM_DOUBLER,
	ITEM_AMMOREGEN,
	TEAM_CTF_NEUTRALFLAG,
	ITEM_REDCUBE,
	ITEM_BLUECUBE,
	WEAPON_NAILGUN,
	WEAPON_PROX_LAUNCHER,
	WEAPON_CHAINGUN
} itemList_t;

struct idWeapon68
{
	enum Id
	{
		None,
		Gauntlet,
		MachineGun,
		Shotgun,
		GrenadeLauncher,
		RocketLauncher,
		LightningGun,
		Railgun,
		PlasmaGun,
		BFG,
		GrapplingHook,
		AfterLast
	};
};

struct idWeapon73p
{
	enum Id
	{
		None,
		Gauntlet,
		MachineGun,
		Shotgun,
		GrenadeLauncher,
		RocketLauncher,
		LightningGun,
		Railgun,
		PlasmaGun,
		BFG,
		GrapplingHook,
		// Mission Pack start
		NailGun,
		ProximityMineLauncher,
		ChainGun,
		// Mission Pack end
		HeavyMachineGun, // @TODO: Correct value?
		AfterLast
	};
};

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum
{
	PM_NORMAL,		// can accelerate and turn
	PM_NOCLIP,		// noclip movement
	PM_SPECTATOR,	// still run into walls
	PM_DEAD,		// no acceleration or turning, but free falling
	PM_FREEZE,		// stuck in place with no control
	PM_INTERMISSION,	// no movement or status bar
	PM_SPINTERMISSION	// no movement or status bar
} pmtype_t;

#define	DEFAULT_GRAVITY		800
#define	GIB_HEALTH			-40

struct idItem68_baseq3
{
	enum Id
	{
		Null,
		ItemArmorShard,
		ItemArmorCombat,
		ItemArmorBody,
		ItemHealthSmall,
		ItemHealth,
		ItemHealthLarge,
		ItemHealthMega,
		WeaponGauntlet,
		WeaponShotgun,
		WeaponMachinegun,
		WeaponGrenadelauncher,
		WeaponRocketlauncher,
		WeaponLightning,
		WeaponRailgun,
		WeaponPlasmagun,
		WeaponBFG,
		WeaponGrapplinghook,
		AmmoShells,
		AmmoBullets,
		AmmoGrenades,
		AmmoCells,
		AmmoLightning,
		AmmoRockets,
		AmmoSlugs,
		AmmoBFG,
		HoldableTeleporter,
		HoldableMedkit,
		ItemQuad,
		ItemEnviro,
		ItemHaste,
		ItemInvis,
		ItemRegen,
		ItemFlight,
		TeamCTFRedflag,
		TeamCTFBlueflag,
		Count
	};
};

struct idItem68_CPMA
{
	enum Id
	{
		Null,
		ItemArmorShard,
		ItemArmorCombat,
		ItemArmorBody,
		ItemHealthSmall,
		ItemHealth,
		ItemHealthLarge,
		ItemHealthMega,
		WeaponGauntlet,
		WeaponShotgun,
		WeaponMachinegun,
		WeaponGrenadelauncher,
		WeaponRocketlauncher,
		WeaponLightning,
		WeaponRailgun,
		WeaponPlasmagun,
		WeaponBFG,
		WeaponGrapplinghook,
		AmmoShells,
		AmmoBullets,
		AmmoGrenades,
		AmmoCells,
		AmmoLightning,
		AmmoRockets,
		AmmoSlugs,
		AmmoBFG,
		HoldableTeleporter,
		HoldableMedkit,
		ItemQuad,
		ItemEnviro,
		ItemHaste,
		ItemInvis,
		ItemRegen,
		ItemFlight,
		TeamCTFRedflag,
		TeamCTFBlueflag,
		ItemArmorJacket,
		ItemBackpack,
		TeamCTFNeutralflag,
		Count
	};
};

struct idItem73
{
	enum Id
	{
		Null,
		ItemArmorShard,
		ItemArmorCombat,
		ItemArmorBody,
		ItemArmorJacket,
		ItemHealthSmall,
		ItemHealth,
		ItemHealthLarge,
		ItemHealthMega,
		WeaponGauntlet,
		WeaponShotgun,
		WeaponMachinegun,
		WeaponGrenadelauncher,
		WeaponRocketlauncher,
		WeaponLightning,
		WeaponRailgun,
		WeaponPlasmagun,
		WeaponBFG,
		WeaponGrapplinghook,
		AmmoShells,
		AmmoBullets,
		AmmoGrenades,
		AmmoCells,
		AmmoLightning,
		AmmoRockets,
		AmmoSlugs,
		AmmoBFG,
		HoldableTeleporter,
		HoldableMedkit,
		ItemQuad,
		ItemEnviro,
		ItemHaste,
		ItemInvis,
		ItemRegen,
		ItemFlight,
		TeamCTFRedflag,
		TeamCTFBlueflag,
		HoldableKamikaze,
		HoldablePortal,
		HoldableInvulnerability,
		AmmoNails,
		AmmoMines,
		AmmoBelt,
		ItemScout,
		ItemGuard,
		ItemDoubler,
		ItemAmmoregen,
		TeamCTFNeutralflag,
		ItemRedcube,
		ItemBluecube,
		WeaponNailgun,
		WeaponProxLauncher,
		WeaponChaingun,
		Count
	};
};

struct idItem90
{
	enum Id
	{
		Null,
		ItemArmorShard,
		ItemArmorCombat,
		ItemArmorBody,
		ItemArmorJacket,
		ItemHealthSmall,
		ItemHealth,
		ItemHealthLarge,
		ItemHealthMega,
		WeaponGauntlet,
		WeaponShotgun,
		WeaponMachinegun,
		WeaponGrenadelauncher,
		WeaponRocketlauncher,
		WeaponLightning,
		WeaponRailgun,
		WeaponPlasmagun,
		WeaponBFG,
		WeaponGrapplinghook,
		AmmoShells,
		AmmoBullets,
		AmmoGrenades,
		AmmoCells,
		AmmoLightning,
		AmmoRockets,
		AmmoSlugs,
		AmmoBFG,
		HoldableTeleporter,
		HoldableMedkit,
		ItemQuad,
		ItemEnviro,
		ItemHaste,
		ItemInvis,
		ItemRegen,
		ItemFlight,
		TeamCTFRedflag,
		TeamCTFBlueflag,
		HoldableKamikaze,
		HoldablePortal,
		HoldableInvulnerability,
		AmmoNails,
		AmmoMines,
		AmmoBelt,
		ItemScout,
		ItemGuard,
		ItemDoubler,
		ItemAmmoregen,
		TeamCTFNeutralflag,
		ItemRedcube,
		ItemBluecube,
		WeaponNailgun,
		WeaponProxLauncher,
		WeaponChaingun,
		ItemSpawnarmor,
		WeaponHMG,
		AmmoHMG,
		AmmoPack,
		Count
	};
};
