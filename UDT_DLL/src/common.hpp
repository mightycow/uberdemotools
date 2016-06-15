#pragma once


#include "uberdemotools.h"
#include "macros.hpp"

#include <string.h>
#include <stdio.h>


// safe strncpy that ensures a trailing zero
extern void Q_strncpyz(char* dest, const char* src, s32 destsize);


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

#define Com_Memset memset
#define Com_Memcpy memcpy

// if entityState->solid == SOLID_BMODEL, modelindex is an inline model number
#define	SOLID_BMODEL	0xffffff

//
// per-level limits
//
#define MAX_LOCATIONS		64

#define	GENTITYNUM_BITS		10		// don't need to send any more
#define	MAX_GENTITIES		(1<<GENTITYNUM_BITS)

// Entity numbers are communicated with GENTITY_BITS bits, so any reserved
// values that are going to be communicated over the net need to
// also be in this range.
#define	ENTITYNUM_NONE       (MAX_GENTITIES-1)
#define	ENTITYNUM_WORLD	     (MAX_GENTITIES-2)
#define	ENTITYNUM_MAX_NORMAL (MAX_GENTITIES-2)


#define	MAX_MODELS			256		// these are sent over the net as 8 bits
#define	MAX_SOUNDS			256		// so they cannot be blindly increased



#define	MAX_CONFIGSTRINGS	1024

#define CS_SERVERINFO             0 // an info string with all the serverinfo cvars
#define CS_SYSTEMINFO             1 // an info string for server system to client system configuration (timescale, etc)
#define CS_CPMA_GAME_INFO       672
#define CS_CPMA_ROUND_INFO      710
#define CS_OSP_GAMEPLAY         806

/*
==============================================================

NET

==============================================================
*/

#define	PACKET_BACKUP	32	// number of old messages that must be kept on client and
							// server for delta comrpession and ping estimation
#define	PACKET_MASK		(PACKET_BACKUP-1)

#define	MAX_PACKET_USERCMDS		32		// max number of usercmd_t in a packet

#define	MAX_RELIABLE_COMMANDS	64			// max string commands buffered for restransmit

// server to client
// the svc_strings[] array in cl_parse.c should mirror this
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


// snapshots are a view of the server at a given time
struct idClientSnapshotBase
{
	u8   areamask[MAX_MAP_AREA_BYTES]; // portalarea visibility bits
	s32  snapFlags;                    // rate delayed and dropped commands
	s32  serverTime;                   // server time the message is valid for (in msec)
	s32  messageNum;                   // copied from netchan->incoming_sequence
	s32  deltaNum;                     // messageNum the delta is from
	//s32  cmdNum;                     // the next cmdNum the server is expecting
	s32  numEntities;                  // all of the entities that need to be presented
	s32  parseEntitiesNum;             // at the time of this snapshot
	s32  serverCommandNum;             // execute all commands up to this before making the snapshot current
	bool valid;                        // cleared if delta parsing was invalid
};

struct idClientSnapshot3 : idClientSnapshotBase
{
	idPlayerState3 ps; // complete information about the current player at this time
};

struct idClientSnapshot48 : idClientSnapshotBase
{
	idPlayerState48 ps; // complete information about the current player at this time
};

struct idClientSnapshot66 : idClientSnapshotBase
{
	idPlayerState66 ps; // complete information about the current player at this time
};

struct idClientSnapshot67 : idClientSnapshotBase
{
	idPlayerState67 ps; // complete information about the current player at this time
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

struct idClientSnapshot91 : idClientSnapshotBase
{
	idPlayerState91 ps; // complete information about the current player at this time
};

typedef idClientSnapshot91 idLargestClientSnapshot;

inline idPlayerStateBase* GetPlayerState(idClientSnapshotBase* snap, udtProtocol::Id protocol)
{
	switch(protocol)
	{
		case udtProtocol::Dm3: return &((idClientSnapshot3*)snap)->ps;
		case udtProtocol::Dm48: return &((idClientSnapshot48*)snap)->ps;
		case udtProtocol::Dm66: return &((idClientSnapshot66*)snap)->ps;
		case udtProtocol::Dm67: return &((idClientSnapshot67*)snap)->ps;
		case udtProtocol::Dm68: return &((idClientSnapshot68*)snap)->ps;
		case udtProtocol::Dm73: return &((idClientSnapshot73*)snap)->ps;
		case udtProtocol::Dm90: return &((idClientSnapshot90*)snap)->ps;
		case udtProtocol::Dm91: return &((idClientSnapshot91*)snap)->ps;
		default: return NULL;
	}
}

// allow a lot of command backups for very fast systems
// multiple commands may be combined s32o a single packet, so this
// needs to be larger than PACKET_BACKUP
#define	CMD_BACKUP		64
#define	CMD_MASK		(CMD_BACKUP - 1)

// Two bits at the top of the idEntityStateBase::event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.
// And off the value with (~EV_EVENT_BITS) to retrieve the actual event number.
#define	ID_ES_EVENT_BIT_1    0x00000100
#define	ID_ES_EVENT_BIT_2    0x00000200
#define	ID_ES_EVENT_BITS     (ID_ES_EVENT_BIT_1 | ID_ES_EVENT_BIT_2)

#define	EVENT_VALID_MSEC	300

#define EV_NONE 0

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

struct udtGame
{
	enum Id
	{
		Q3,
		QL,
		CPMA,
		OSP
	};
};

// @NOTE: This matches udtFlagStatus exactly. It was never changed.
struct idFlagStatus
{
	enum Id
	{
		InBase,  // In its spot in base.
		Carried, // Being carried by an enemy player.
		Missing, // Not being carried by anyone but not in its spot either.
		Count
	};
};
