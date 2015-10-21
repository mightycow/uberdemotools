#include "message.hpp"
#include "huffman.hpp"


// if(s32)f == f and (s32)f + (1<<(FLOAT_INT_BITS-1)) < (1 << FLOAT_INT_BITS)
// the float value will be sent with FLOAT_INT_BITS, otherwise all 32 bits will be sent
#define	FLOAT_INT_BITS	13
#define	FLOAT_INT_BIAS	(1<<(FLOAT_INT_BITS-1))


// Offset from the start of the structure == absolute address when the struct is at address 0.
#if defined(UDT_GCC)
#	define	OFFSET_OF(type, member)		__builtin_offsetof(type, member)
#elif defined(UDT_MSVC)
#	include <stddef.h>
#	define	 OFFSET_OF(type, member)	offsetof(type, member)
#endif

//
// 3
//

#define ESF(field, bits) { #field, (s32)OFFSET_OF(idEntityState3, field), bits }

// Each field's index is the corresponding index into the field bit mask.
const idNetField EntityStateFields3[]
{
	ESF(eType, 8),
	ESF(eFlags, 16),
	ESF(pos.trType, 8),
	ESF(pos.trTime, 32),
	ESF(pos.trDuration, 32),
	ESF(pos.trBase[0], 0),
	ESF(pos.trBase[1], 0),
	ESF(pos.trBase[2], 0),
	ESF(pos.trDelta[0], 0),
	ESF(pos.trDelta[1], 0),
	ESF(pos.trDelta[2], 0),
	ESF(apos.trType, 8),
	ESF(apos.trTime, 32),
	ESF(apos.trDuration, 32),
	ESF(apos.trBase[0], 0),
	ESF(apos.trBase[1], 0),
	ESF(apos.trBase[2], 0),
	ESF(apos.trDelta[0], 0),
	ESF(apos.trDelta[1], 0),
	ESF(apos.trDelta[2], 0),
	ESF(time, 32),
	ESF(time2, 32),
	ESF(origin[0], 0),
	ESF(origin[1], 0),
	ESF(origin[2], 0),
	ESF(origin2[0], 0),
	ESF(origin2[1], 0),
	ESF(origin2[2], 0),
	ESF(angles[0], 0),
	ESF(angles[1], 0),
	ESF(angles[2], 0),
	ESF(angles2[0], 0),
	ESF(angles2[1], 0),
	ESF(angles2[2], 0),
	ESF(otherEntityNum, 10),
	ESF(otherEntityNum2, 10),
	ESF(groundEntityNum, 10),
	ESF(loopSound, 8),
	ESF(constantLight, 32),
	ESF(modelindex, 8),
	ESF(modelindex2, 8),
	ESF(frame, 16),
	ESF(clientNum, 8),
	ESF(solid, 24),
	ESF(event, 10),
	ESF(eventParm, 8),
	ESF(powerups, 16),
	ESF(weapon, 8),
	ESF(legsAnim, 8),
	ESF(torsoAnim, 8)
};

#undef ESF

const s32 EntityStateFieldCount3 = sizeof(EntityStateFields3) / sizeof(EntityStateFields3[0]);
static_assert(EntityStateFieldCount3 == 50, "dm3 network entity states have 50 fields!");

//
// 48
//

#define ESF(field, bits) { #field, (s32)OFFSET_OF(idEntityState48, field), bits }

// Each field's index is the corresponding index into the field bit mask.
const idNetField EntityStateFields48[]
{
	ESF(eType, 8),
	ESF(eFlags, 19), // Changed from 16 to 19...
	ESF(pos.trType, 8),
	ESF(pos.trTime, 32),
	ESF(pos.trDuration, 32),
	ESF(pos.trBase[0], 0),
	ESF(pos.trBase[1], 0),
	ESF(pos.trBase[2], 0),
	ESF(pos.trDelta[0], 0),
	ESF(pos.trDelta[1], 0),
	ESF(pos.trDelta[2], 0),
	ESF(apos.trType, 8),
	ESF(apos.trTime, 32),
	ESF(apos.trDuration, 32),
	ESF(apos.trBase[0], 0),
	ESF(apos.trBase[1], 0),
	ESF(apos.trBase[2], 0),
	ESF(apos.trDelta[0], 0),
	ESF(apos.trDelta[1], 0),
	ESF(apos.trDelta[2], 0),
	ESF(time, 32),
	ESF(time2, 32),
	ESF(origin[0], 0),
	ESF(origin[1], 0),
	ESF(origin[2], 0),
	ESF(origin2[0], 0),
	ESF(origin2[1], 0),
	ESF(origin2[2], 0),
	ESF(angles[0], 0),
	ESF(angles[1], 0),
	ESF(angles[2], 0),
	ESF(angles2[0], 0),
	ESF(angles2[1], 0),
	ESF(angles2[2], 0),
	ESF(otherEntityNum, 10),
	ESF(otherEntityNum2, 10),
	ESF(groundEntityNum, 10),
	ESF(loopSound, 8),
	ESF(constantLight, 32),
	ESF(modelindex, 8),
	ESF(modelindex2, 8),
	ESF(frame, 16),
	ESF(clientNum, 8),
	ESF(solid, 24),
	ESF(event, 10),
	ESF(eventParm, 8),
	ESF(powerups, 16),
	ESF(weapon, 8),
	ESF(legsAnim, 8),
	ESF(torsoAnim, 8),
	ESF(generic1, 8)
};

#undef ESF

const s32 EntityStateFieldCount48 = sizeof(EntityStateFields48) / sizeof(EntityStateFields48[0]);
static_assert(EntityStateFieldCount48 == 51, "dm_48 network entity states have 51 fields!");

//
// 68
//

#define ESF(x) #x, (s32)OFFSET_OF(idEntityState68, x)

const idNetField EntityStateFields68[] =
{
	{ ESF(pos.trTime), 32 },
	{ ESF(pos.trBase[0]), 0 },
	{ ESF(pos.trBase[1]), 0 },
	{ ESF(pos.trDelta[0]), 0 },
	{ ESF(pos.trDelta[1]), 0 },
	{ ESF(pos.trBase[2]), 0 },
	{ ESF(apos.trBase[1]), 0 },
	{ ESF(pos.trDelta[2]), 0 },
	{ ESF(apos.trBase[0]), 0 },
	{ ESF(event), 10 },
	{ ESF(angles2[1]), 0 },
	{ ESF(eType), 8 },
	{ ESF(torsoAnim), 8 },
	{ ESF(eventParm), 8 },
	{ ESF(legsAnim), 8 },
	{ ESF(groundEntityNum), GENTITYNUM_BITS },
	{ ESF(pos.trType), 8 },
	{ ESF(eFlags), 19 },
	{ ESF(otherEntityNum), GENTITYNUM_BITS },
	{ ESF(weapon), 8 },
	{ ESF(clientNum), 8 },
	{ ESF(angles[1]), 0 },
	{ ESF(pos.trDuration), 32 },
	{ ESF(apos.trType), 8 },
	{ ESF(origin[0]), 0 },
	{ ESF(origin[1]), 0 },
	{ ESF(origin[2]), 0 },
	{ ESF(solid), 24 },
	{ ESF(powerups), MAX_POWERUPS },
	{ ESF(modelindex), 8 },
	{ ESF(otherEntityNum2), GENTITYNUM_BITS },
	{ ESF(loopSound), 8 },
	{ ESF(generic1), 8 },
	{ ESF(origin2[2]), 0 },
	{ ESF(origin2[0]), 0 },
	{ ESF(origin2[1]), 0 },
	{ ESF(modelindex2), 8 },
	{ ESF(angles[0]), 0 },
	{ ESF(time), 32 },
	{ ESF(apos.trTime), 32 },
	{ ESF(apos.trDuration), 32 },
	{ ESF(apos.trBase[2]), 0 },
	{ ESF(apos.trDelta[0]), 0 },
	{ ESF(apos.trDelta[1]), 0 },
	{ ESF(apos.trDelta[2]), 0 },
	{ ESF(time2), 32 },
	{ ESF(angles[2]), 0 },
	{ ESF(angles2[0]), 0 },
	{ ESF(angles2[2]), 0 },
	{ ESF(constantLight), 32 },
	{ ESF(frame), 16 }
};

#undef ESF

const s32 EntityStateFieldCount68 = sizeof(EntityStateFields68) / sizeof(EntityStateFields68[0]);

//
// 73
//

#define ESF(x) #x, (s32)OFFSET_OF(idEntityState73, x)

const idNetField EntityStateFields73[] =
{
	{ ESF(pos.trTime), 32 },
	{ ESF(pos.trBase[0]), 0 },
	{ ESF(pos.trBase[1]), 0 },
	{ ESF(pos.trDelta[0]), 0 },
	{ ESF(pos.trDelta[1]), 0 },
	{ ESF(pos.trBase[2]), 0 },
	{ ESF(apos.trBase[1]), 0 },
	{ ESF(pos.trDelta[2]), 0 },
	{ ESF(apos.trBase[0]), 0 },
	{ ESF(pos_gravity), 32 }, // New in dm_73
	{ ESF(event), 10 },
	{ ESF(angles2[1]), 0 },
	{ ESF(eType), 8 },
	{ ESF(torsoAnim), 8 },
	{ ESF(eventParm), 8 },
	{ ESF(legsAnim), 8 },
	{ ESF(groundEntityNum), GENTITYNUM_BITS },
	{ ESF(pos.trType), 8 },
	{ ESF(eFlags), 19 },
	{ ESF(otherEntityNum), GENTITYNUM_BITS },
	{ ESF(weapon), 8 },
	{ ESF(clientNum), 8 },
	{ ESF(angles[1]), 0 },
	{ ESF(pos.trDuration), 32 },
	{ ESF(apos.trType), 8 },
	{ ESF(origin[0]), 0 },
	{ ESF(origin[1]), 0 },
	{ ESF(origin[2]), 0 },
	{ ESF(solid), 24 },
	{ ESF(powerups), 16 },
	{ ESF(modelindex), 8 },
	{ ESF(otherEntityNum2), GENTITYNUM_BITS },
	{ ESF(loopSound), 8 },
	{ ESF(generic1), 8 },
	{ ESF(origin2[2]), 0 },
	{ ESF(origin2[0]), 0 },
	{ ESF(origin2[1]), 0 },
	{ ESF(modelindex2), 8 },
	{ ESF(angles[0]), 0 },
	{ ESF(time), 32 },
	{ ESF(apos.trTime), 32 },
	{ ESF(apos.trDuration), 32 },
	{ ESF(apos.trBase[2]), 0 },
	{ ESF(apos.trDelta[0]), 0 },
	{ ESF(apos.trDelta[1]), 0 },
	{ ESF(apos.trDelta[2]), 0 },
	{ ESF(apos_gravity), 32 }, // New in dm_73
	{ ESF(time2), 32 },
	{ ESF(angles[2]), 0 },
	{ ESF(angles2[0]), 0 },
	{ ESF(angles2[2]), 0 },
	{ ESF(constantLight), 32 },
	{ ESF(frame), 16 }
};

#undef ESF

const s32 EntityStateFieldCount73 = sizeof(EntityStateFields73) / sizeof(EntityStateFields73[0]);

//
// 90
//

#define ESF(x) #x, (s32)OFFSET_OF(idEntityState90, x)

const idNetField EntityStateFields90[] =
{
	{ ESF(pos.trTime), 32 },
	{ ESF(pos.trBase[0]), 0 },
	{ ESF(pos.trBase[1]), 0 },
	{ ESF(pos.trDelta[0]), 0 },
	{ ESF(pos.trDelta[1]), 0 },
	{ ESF(pos.trBase[2]), 0 },
	{ ESF(apos.trBase[1]), 0 },
	{ ESF(pos.trDelta[2]), 0 },
	{ ESF(apos.trBase[0]), 0 },
	{ ESF(pos_gravity), 32 }, // New in dm_73
	{ ESF(event), 10 },
	{ ESF(angles2[1]), 0 },
	{ ESF(eType), 8 },
	{ ESF(torsoAnim), 8 },
	{ ESF(eventParm), 8 },
	{ ESF(legsAnim), 8 },
	{ ESF(groundEntityNum), GENTITYNUM_BITS },
	{ ESF(pos.trType), 8 },
	{ ESF(eFlags), 19 },
	{ ESF(otherEntityNum), GENTITYNUM_BITS },
	{ ESF(weapon), 8 },
	{ ESF(clientNum), 8 },
	{ ESF(angles[1]), 0 },
	{ ESF(pos.trDuration), 32 },
	{ ESF(apos.trType), 8 },
	{ ESF(origin[0]), 0 },
	{ ESF(origin[1]), 0 },
	{ ESF(origin[2]), 0 },
	{ ESF(solid), 24 },
	{ ESF(powerups), 16 },
	{ ESF(modelindex), 8 },
	{ ESF(otherEntityNum2), GENTITYNUM_BITS },
	{ ESF(loopSound), 8 },
	{ ESF(generic1), 8 },
	{ ESF(origin2[2]), 0 },
	{ ESF(origin2[0]), 0 },
	{ ESF(origin2[1]), 0 },
	{ ESF(modelindex2), 8 },
	{ ESF(angles[0]), 0 },
	{ ESF(time), 32 },
	{ ESF(apos.trTime), 32 },
	{ ESF(apos.trDuration), 32 },
	{ ESF(apos.trBase[2]), 0 },
	{ ESF(apos.trDelta[0]), 0 },
	{ ESF(apos.trDelta[1]), 0 },
	{ ESF(apos.trDelta[2]), 0 },
	{ ESF(apos_gravity), 32 }, // New in dm_73
	{ ESF(time2), 32 },
	{ ESF(angles[2]), 0 },
	{ ESF(angles2[0]), 0 },
	{ ESF(angles2[2]), 0 },
	{ ESF(constantLight), 32 },
	{ ESF(frame), 16 },
	{ ESF(jumpTime), 32 }, // New in dm_90
	{ ESF(doubleJumped), 1 } // New in dm_90
};

#undef ESF

const s32 EntityStateFieldCount90 = sizeof(EntityStateFields90) / sizeof(EntityStateFields90[0]);

//
// 91
//

#define ESF(x) #x, (s32)OFFSET_OF(idEntityState91, x)

const idNetField EntityStateFields91[] =
{
	{ ESF(pos.trTime), 32 },
	{ ESF(pos.trBase[0]), 0 },
	{ ESF(pos.trBase[1]), 0 },
	{ ESF(pos.trDelta[0]), 0 },
	{ ESF(pos.trDelta[1]), 0 },
	{ ESF(pos.trBase[2]), 0 },
	{ ESF(apos.trBase[1]), 0 },
	{ ESF(pos.trDelta[2]), 0 },
	{ ESF(apos.trBase[0]), 0 },
	{ ESF(pos_gravity), 32 },
	{ ESF(event), 10 },
	{ ESF(angles2[1]), 0 },
	{ ESF(eType), 8 },
	{ ESF(torsoAnim), 8 },
	{ ESF(eventParm), 8 },
	{ ESF(legsAnim), 8 },
	{ ESF(groundEntityNum), GENTITYNUM_BITS },
	{ ESF(pos.trType), 8 },
	{ ESF(eFlags), 19 },
	{ ESF(otherEntityNum), GENTITYNUM_BITS },
	{ ESF(weapon), 8 },
	{ ESF(clientNum), 8 },
	{ ESF(angles[1]), 0 },
	{ ESF(pos.trDuration), 32 },
	{ ESF(apos.trType), 8 },
	{ ESF(origin[0]), 0 },
	{ ESF(origin[1]), 0 },
	{ ESF(origin[2]), 0 },
	{ ESF(solid), 24 },
	{ ESF(powerups), 16 },
	{ ESF(modelindex), 8 },
	{ ESF(otherEntityNum2), GENTITYNUM_BITS },
	{ ESF(loopSound), 8 },
	{ ESF(generic1), 8 },
	{ ESF(origin2[2]), 0 },
	{ ESF(origin2[0]), 0 },
	{ ESF(origin2[1]), 0 },
	{ ESF(modelindex2), 8 },
	{ ESF(angles[0]), 0 },
	{ ESF(time), 32 },
	{ ESF(apos.trTime), 32 },
	{ ESF(apos.trDuration), 32 },
	{ ESF(apos.trBase[2]), 0 },
	{ ESF(apos.trDelta[0]), 0 },
	{ ESF(apos.trDelta[1]), 0 },
	{ ESF(apos.trDelta[2]), 0 },
	{ ESF(apos_gravity), 32 },
	{ ESF(time2), 32 },
	{ ESF(angles[2]), 0 },
	{ ESF(angles2[0]), 0 },
	{ ESF(angles2[2]), 0 },
	{ ESF(constantLight), 32 },
	{ ESF(frame), 16 },
	{ ESF(jumpTime), 32 },
	{ ESF(doubleJumped), 1 },
	{ ESF(health), 16 }, // New in protocol 91.
	{ ESF(armor), 16 },  // New in protocol 91.
	{ ESF(location), 8 } // New in protocol 91.
};

#undef ESF

const s32 EntityStateFieldCount91 = sizeof(EntityStateFields91) / sizeof(EntityStateFields91[0]);

//
// 3
//

#define PSF(field, bits) { #field, (s32)OFFSET_OF(idPlayerState3, field), bits }

static const idNetField PlayerStateFields3[] =
{
	PSF(commandTime, 32),
	PSF(pm_type, 8),
	PSF(bobCycle, 8),
	PSF(pm_flags, 16),
	PSF(pm_time, 16),
	PSF(origin[0], 0),
	PSF(origin[1], 0),
	PSF(origin[2], 0),
	PSF(velocity[0], 0),
	PSF(velocity[1], 0),
	PSF(velocity[2], 0),
	PSF(weaponTime, 16),
	PSF(gravity, 16),
	PSF(speed, 16),
	PSF(delta_angles[0], 16),
	PSF(delta_angles[1], 16),
	PSF(delta_angles[2], 16),
	PSF(groundEntityNum, 10),
	PSF(legsTimer, 8),
	PSF(torsoTimer, 12),
	PSF(legsAnim, 8),
	PSF(torsoAnim, 8),
	PSF(movementDir, 4),
	PSF(eFlags, 16),
	PSF(eventSequence, 16),
	PSF(events[0], 8),
	PSF(events[1], 8),
	PSF(eventParms[0], 8),
	PSF(eventParms[1], 8),
	PSF(externalEvent, 8),
	PSF(externalEventParm, 8),
	PSF(clientNum, 8),
	PSF(weapon, 5),
	PSF(weaponstate, 4),
	PSF(viewangles[0], 0),
	PSF(viewangles[1], 0),
	PSF(viewangles[2], 0),
	PSF(viewheight, 8),
	PSF(damageEvent, 8),
	PSF(damageYaw, 8),
	PSF(damagePitch, 8),
	PSF(damageCount, 8),
	PSF(grapplePoint[0], 0),
	PSF(grapplePoint[1], 0),
	PSF(grapplePoint[2], 0)
};

#undef PSF

static const s32 PlayerStateFieldCount3 = sizeof(PlayerStateFields3) / sizeof(PlayerStateFields3[0]);

//
// 48
//

#define PSF(field, bits) { #field, (s32)OFFSET_OF(idPlayerState48, field), bits }

static const idNetField PlayerStateFields48[] =
{
	PSF(commandTime, 32),
	PSF(pm_type, 8),
	PSF(bobCycle, 8),
	PSF(pm_flags, 16),
	PSF(pm_time, -16),
	PSF(origin[0], 0),
	PSF(origin[1], 0),
	PSF(origin[2], 0),
	PSF(velocity[0], 0),
	PSF(velocity[1], 0),
	PSF(velocity[2], 0),
	PSF(weaponTime, -16),
	PSF(gravity, 16),
	PSF(speed, 16),
	PSF(delta_angles[0], 16),
	PSF(delta_angles[1], 16),
	PSF(delta_angles[2], 16),
	PSF(groundEntityNum, 10),
	PSF(legsTimer, 8),
	PSF(torsoTimer, 12),
	PSF(legsAnim, 8),
	PSF(torsoAnim, 8),
	PSF(movementDir, 4),
	PSF(eFlags, 16),
	PSF(eventSequence, 16),
	PSF(events[0], 8),
	PSF(events[1], 8),
	PSF(eventParms[0], 8),
	PSF(eventParms[1], 8),
	PSF(externalEvent, 10), // Changed from 8 to 10...
	PSF(externalEventParm, 8),
	PSF(clientNum, 8),
	PSF(weapon, 5),
	PSF(weaponstate, 4),
	PSF(viewangles[0], 0),
	PSF(viewangles[1], 0),
	PSF(viewangles[2], 0),
	PSF(viewheight, -8),
	PSF(damageEvent, 8),
	PSF(damageYaw, 8),
	PSF(damagePitch, 8),
	PSF(damageCount, 8), 
	PSF(grapplePoint[0], 0),
	PSF(grapplePoint[1], 0),
	PSF(grapplePoint[2], 0),
	PSF(jumppad_ent, 10), // New in dm_48.
	PSF(loopSound, 16),   // New in dm_48.
	PSF(generic1, 8),     // New in dm_48.
};

#undef PSF

static const s32 PlayerStateFieldCount48 = sizeof(PlayerStateFields48) / sizeof(PlayerStateFields48[0]);

//
// 68
//

#define PSF(x) #x,(s32)(sptr)(&((idPlayerState68*)0)->x)

static const idNetField PlayerStateFields68[] =
{
	{ PSF(commandTime), 32 },
	{ PSF(origin[0]), 0 },
	{ PSF(origin[1]), 0 },
	{ PSF(bobCycle), 8 },
	{ PSF(velocity[0]), 0 },
	{ PSF(velocity[1]), 0 },
	{ PSF(viewangles[1]), 0 },
	{ PSF(viewangles[0]), 0 },
	{ PSF(weaponTime), -16 },
	{ PSF(origin[2]), 0 },
	{ PSF(velocity[2]), 0 },
	{ PSF(legsTimer), 8 },
	{ PSF(pm_time), -16 },
	{ PSF(eventSequence), 16 },
	{ PSF(torsoAnim), 8 },
	{ PSF(movementDir), 4 },
	{ PSF(events[0]), 8 },
	{ PSF(legsAnim), 8 },
	{ PSF(events[1]), 8 },
	{ PSF(pm_flags), 16 },
	{ PSF(groundEntityNum), GENTITYNUM_BITS },
	{ PSF(weaponstate), 4 },
	{ PSF(eFlags), 16 },
	{ PSF(externalEvent), 10 },
	{ PSF(gravity), 16 },
	{ PSF(speed), 16 },
	{ PSF(delta_angles[1]), 16 },
	{ PSF(externalEventParm), 8 },
	{ PSF(viewheight), -8 },
	{ PSF(damageEvent), 8 },
	{ PSF(damageYaw), 8 },
	{ PSF(damagePitch), 8 },
	{ PSF(damageCount), 8 },
	{ PSF(generic1), 8 },
	{ PSF(pm_type), 8 },
	{ PSF(delta_angles[0]), 16 },
	{ PSF(delta_angles[2]), 16 },
	{ PSF(torsoTimer), 12 },
	{ PSF(eventParms[0]), 8 },
	{ PSF(eventParms[1]), 8 },
	{ PSF(clientNum), 8 },
	{ PSF(weapon), 5 },
	{ PSF(viewangles[2]), 0 },
	{ PSF(grapplePoint[0]), 0 },
	{ PSF(grapplePoint[1]), 0 },
	{ PSF(grapplePoint[2]), 0 },
	{ PSF(jumppad_ent), 10 },
	{ PSF(loopSound), 16 }
};

#undef PSF

static const s32 PlayerStateFieldCount68 = sizeof(PlayerStateFields68) / sizeof(PlayerStateFields68[0]);

//
// 73
//

#define PSF(x) #x,(s32)(sptr)(&((idPlayerState73*)0)->x)

static const idNetField PlayerStateFields73[] =
{
	{ PSF(commandTime), 32 },
	{ PSF(origin[0]), 0 },
	{ PSF(origin[1]), 0 },
	{ PSF(bobCycle), 8 },
	{ PSF(velocity[0]), 0 },
	{ PSF(velocity[1]), 0 },
	{ PSF(viewangles[1]), 0 },
	{ PSF(viewangles[0]), 0 },
	{ PSF(weaponTime), -16 },
	{ PSF(origin[2]), 0 },
	{ PSF(velocity[2]), 0 },
	{ PSF(legsTimer), 8 },
	{ PSF(pm_time), -16 },
	{ PSF(eventSequence), 16 },
	{ PSF(torsoAnim), 8 },
	{ PSF(movementDir), 4 },
	{ PSF(events[0]), 8 },
	{ PSF(legsAnim), 8 },
	{ PSF(events[1]), 8 },
	{ PSF(pm_flags), 16 },
	{ PSF(groundEntityNum), GENTITYNUM_BITS },
	{ PSF(weaponstate), 4 },
	{ PSF(eFlags), 16 },
	{ PSF(externalEvent), 10 },
	{ PSF(gravity), 16 },
	{ PSF(speed), 16 },
	{ PSF(delta_angles[1]), 16 },
	{ PSF(externalEventParm), 8 },
	{ PSF(viewheight), -8 },
	{ PSF(damageEvent), 8 },
	{ PSF(damageYaw), 8 },
	{ PSF(damagePitch), 8 },
	{ PSF(damageCount), 8 },
	{ PSF(generic1), 8 },
	{ PSF(pm_type), 8 },
	{ PSF(delta_angles[0]), 16 },
	{ PSF(delta_angles[2]), 16 },
	{ PSF(torsoTimer), 12 },
	{ PSF(eventParms[0]), 8 },
	{ PSF(eventParms[1]), 8 },
	{ PSF(clientNum), 8 },
	{ PSF(weapon), 5 },
	{ PSF(viewangles[2]), 0 },
	{ PSF(grapplePoint[0]), 0 },
	{ PSF(grapplePoint[1]), 0 },
	{ PSF(grapplePoint[2]), 0 },
	{ PSF(jumppad_ent), 10 },
	{ PSF(loopSound), 16 }
};

#undef PSF

static const s32 PlayerStateFieldCount73 = sizeof(PlayerStateFields73) / sizeof(PlayerStateFields73[0]);

//
// 90
//

#define PSF(x) #x,(s32)(sptr)(&((idPlayerState90*)0)->x)

static const idNetField PlayerStateFields90[] =
{
	{ PSF(commandTime), 32 },
	{ PSF(origin[0]), 0 },
	{ PSF(origin[1]), 0 },
	{ PSF(bobCycle), 8 },
	{ PSF(velocity[0]), 0 },
	{ PSF(velocity[1]), 0 },
	{ PSF(viewangles[1]), 0 },
	{ PSF(viewangles[0]), 0 },
	{ PSF(weaponTime), -16 },
	{ PSF(origin[2]), 0 },
	{ PSF(velocity[2]), 0 },
	{ PSF(legsTimer), 8 },
	{ PSF(pm_time), -16 },
	{ PSF(eventSequence), 16 },
	{ PSF(torsoAnim), 8 },
	{ PSF(movementDir), 4 },
	{ PSF(events[0]), 8 },
	{ PSF(legsAnim), 8 },
	{ PSF(events[1]), 8 },
	{ PSF(pm_flags), 24 },
	{ PSF(groundEntityNum), GENTITYNUM_BITS },
	{ PSF(weaponstate), 4 },
	{ PSF(eFlags), 16 },
	{ PSF(externalEvent), 10 },
	{ PSF(gravity), 16 },
	{ PSF(speed), 16 },
	{ PSF(delta_angles[1]), 16 },
	{ PSF(externalEventParm), 8 },
	{ PSF(viewheight), -8 },
	{ PSF(damageEvent), 8 },
	{ PSF(damageYaw), 8 },
	{ PSF(damagePitch), 8 },
	{ PSF(damageCount), 8 },
	{ PSF(generic1), 8 },
	{ PSF(pm_type), 8 },
	{ PSF(delta_angles[0]), 16 },
	{ PSF(delta_angles[2]), 16 },
	{ PSF(torsoTimer), 12 },
	{ PSF(eventParms[0]), 8 },
	{ PSF(eventParms[1]), 8 },
	{ PSF(clientNum), 8 },
	{ PSF(weapon), 5 },
	{ PSF(viewangles[2]), 0 },
	{ PSF(grapplePoint[0]), 0 }, // New in protocol 90.
	{ PSF(grapplePoint[1]), 0 }, // New in protocol 90.
	{ PSF(grapplePoint[2]), 0 }, // New in protocol 90.
	{ PSF(jumppad_ent), 10 },
	{ PSF(loopSound), 16 },
	{ PSF(jumpTime), 32 },   // New in protocol 90.
	{ PSF(doubleJumped), 1 } // New in protocol 90.
};

#undef PSF

static const s32 PlayerStateFieldCount90 = sizeof(PlayerStateFields90) / sizeof(PlayerStateFields90[0]);

//
// 91
//

#define PSF(x) #x,(s32)(sptr)(&((idPlayerState91*)0)->x)

static const idNetField PlayerStateFields91[] =
{
	{ PSF(commandTime), 32 },
	{ PSF(origin[0]), 0 },
	{ PSF(origin[1]), 0 },
	{ PSF(bobCycle), 8 },
	{ PSF(velocity[0]), 0 },
	{ PSF(velocity[1]), 0 },
	{ PSF(viewangles[1]), 0 },
	{ PSF(viewangles[0]), 0 },
	{ PSF(weaponTime), -16 },
	{ PSF(origin[2]), 0 },
	{ PSF(velocity[2]), 0 },
	{ PSF(legsTimer), 8 },
	{ PSF(pm_time), -16 },
	{ PSF(eventSequence), 16 },
	{ PSF(torsoAnim), 8 },
	{ PSF(movementDir), 4 },
	{ PSF(events[0]), 8 },
	{ PSF(legsAnim), 8 },
	{ PSF(events[1]), 8 },
	{ PSF(pm_flags), 24 },
	{ PSF(groundEntityNum), GENTITYNUM_BITS },
	{ PSF(weaponstate), 4 },
	{ PSF(eFlags), 16 },
	{ PSF(externalEvent), 10 },
	{ PSF(gravity), 16 },
	{ PSF(speed), 16 },
	{ PSF(delta_angles[1]), 16 },
	{ PSF(externalEventParm), 8 },
	{ PSF(viewheight), -8 },
	{ PSF(damageEvent), 8 },
	{ PSF(damageYaw), 8 },
	{ PSF(damagePitch), 8 },
	{ PSF(damageCount), 8 },
	{ PSF(generic1), 8 },
	{ PSF(pm_type), 8 },
	{ PSF(delta_angles[0]), 16 },
	{ PSF(delta_angles[2]), 16 },
	{ PSF(torsoTimer), 12 },
	{ PSF(eventParms[0]), 8 },
	{ PSF(eventParms[1]), 8 },
	{ PSF(clientNum), 8 },
	{ PSF(weapon), 5 },
	{ PSF(weaponPrimary), 8 },
	{ PSF(viewangles[2]), 0 },
	{ PSF(grapplePoint[0]), 0 },
	{ PSF(grapplePoint[1]), 0 },
	{ PSF(grapplePoint[2]), 0 },
	{ PSF(jumppad_ent), 10 },
	{ PSF(loopSound), 16 },
	{ PSF(jumpTime), 32 },
	{ PSF(doubleJumped), 1 },
	{ PSF(crouchTime), 32 },      // New in protocol 91.
	{ PSF(crouchSlideTime), 32 }, // New in protocol 91.
	{ PSF(location), 8 },         // New in protocol 91.
	{ PSF(fov), 8 },              // New in protocol 91.
	{ PSF(forwardmove), 8 },      // New in protocol 91.
	{ PSF(rightmove), 8 },        // New in protocol 91.
	{ PSF(upmove), 8 }            // New in protocol 91.
};

#undef PSF

static const s32 PlayerStateFieldCount91 = sizeof(PlayerStateFields91) / sizeof(PlayerStateFields91[0]);


udtMessage::udtMessage()
{
	_protocol = udtProtocol::Dm68;
	_protocolSizeOfEntityState = sizeof(idEntityState68);
	_protocolSizeOfPlayerState = sizeof(idPlayerState68);
	_entityStateFields = EntityStateFields68;
	_entityStateFieldCount = EntityStateFieldCount68;
	_playerStateFields = PlayerStateFields68;
	_playerStateFieldCount = PlayerStateFieldCount68;
}

void udtMessage::InitContext(udtContext* context)
{
	Context = context; 
}

void udtMessage::InitProtocol(udtProtocol::Id protocol)
{
	_protocol = protocol;

	switch(protocol)
	{
		case udtProtocol::Dm91:
			_protocolSizeOfEntityState = sizeof(idEntityState91);
			_protocolSizeOfPlayerState = sizeof(idPlayerState91);
			_entityStateFields = EntityStateFields91;
			_entityStateFieldCount = EntityStateFieldCount91;
			_playerStateFields = PlayerStateFields91;
			_playerStateFieldCount = PlayerStateFieldCount91;
			break;

		case udtProtocol::Dm90:
			_protocolSizeOfEntityState = sizeof(idEntityState90);
			_protocolSizeOfPlayerState = sizeof(idPlayerState90);
			_entityStateFields = EntityStateFields90;
			_entityStateFieldCount = EntityStateFieldCount90;
			_playerStateFields = PlayerStateFields90;
			_playerStateFieldCount = PlayerStateFieldCount90;
			break;

		case udtProtocol::Dm73:
			_protocolSizeOfEntityState = sizeof(idEntityState73);
			_protocolSizeOfPlayerState = sizeof(idPlayerState73);
			_entityStateFields = EntityStateFields73;
			_entityStateFieldCount = EntityStateFieldCount73;
			_playerStateFields = PlayerStateFields73;
			_playerStateFieldCount = PlayerStateFieldCount73;
			break;

		case udtProtocol::Dm3:
			_protocolSizeOfEntityState = sizeof(idEntityState3);
			_protocolSizeOfPlayerState = sizeof(idPlayerState3);
			_entityStateFields = EntityStateFields3;
			_entityStateFieldCount = EntityStateFieldCount3;
			_playerStateFields = PlayerStateFields3;
			_playerStateFieldCount = PlayerStateFieldCount3;
			break;

		case udtProtocol::Dm48:
			_protocolSizeOfEntityState = sizeof(idEntityState48);
			_protocolSizeOfPlayerState = sizeof(idPlayerState48);
			_entityStateFields = EntityStateFields48;
			_entityStateFieldCount = EntityStateFieldCount48;
			_playerStateFields = PlayerStateFields48;
			_playerStateFieldCount = PlayerStateFieldCount48;
			break;

		case udtProtocol::Dm66:
			_protocolSizeOfEntityState = sizeof(idEntityState66);
			_protocolSizeOfPlayerState = sizeof(idPlayerState66);
			_entityStateFields = EntityStateFields68;
			_entityStateFieldCount = EntityStateFieldCount68;
			_playerStateFields = PlayerStateFields68;
			_playerStateFieldCount = PlayerStateFieldCount68;
			break;

		case udtProtocol::Dm67:
			_protocolSizeOfEntityState = sizeof(idEntityState67);
			_protocolSizeOfPlayerState = sizeof(idPlayerState67);
			_entityStateFields = EntityStateFields68;
			_entityStateFieldCount = EntityStateFieldCount68;
			_playerStateFields = PlayerStateFields68;
			_playerStateFieldCount = PlayerStateFieldCount68;
			break;

		case udtProtocol::Dm68:
		default:
			_protocolSizeOfEntityState = sizeof(idEntityState68);
			_protocolSizeOfPlayerState = sizeof(idPlayerState68);
			_entityStateFields = EntityStateFields68;
			_entityStateFieldCount = EntityStateFieldCount68;
			_playerStateFields = PlayerStateFields68;
			_playerStateFieldCount = PlayerStateFieldCount68;
			break;
	}
}

void udtMessage::Init(u8* data, s32 length) 
{
	Com_Memset(&Buffer, 0, sizeof(idMessage));
	Buffer.data = data;
	Buffer.maxsize = length;
	SetValid(true);
}

void udtMessage::InitOOB(u8* data, s32 length) 
{
	Com_Memset(&Buffer, 0, sizeof(idMessage));
	Buffer.data = data;
	Buffer.maxsize = length;
	Buffer.oob = true;
	SetValid(true);
}

void udtMessage::Bitstream() 
{
	Buffer.oob = false;
}

void udtMessage::SetHuffman(bool huffman)
{
	Buffer.oob = huffman ? false : true;
}

void udtMessage::GoToNextByte()
{
	if((Buffer.bit & 7) != 0)
	{
		++Buffer.readcount;
		Buffer.bit = Buffer.readcount << 3;
	}
}

// negative bit values include signs
void udtMessage::RealWriteBits(s32 value, s32 bits) 
{
	if(Buffer.bit + bits > Buffer.maxsize * 8)
	{
		Context->LogError("udtMessage::RealWriteBits: Overflowed!");
		SetValid(false);
		return;
	}

	if(bits == 0 || bits < -31 || bits > 32) 
	{
		Context->LogError("udtMessage::RealWriteBits: Invalid bit count: %d", bits);
		SetValid(false);
		return;
	}

	if(bits < 0) 
	{
		bits = -bits;
	}

	if(Buffer.oob) 
	{
		if(bits == 8)
		{
			Buffer.data[Buffer.cursize] = (u8)value;
			Buffer.cursize += 1;
			Buffer.bit += 8;
		} 
		else if(bits == 16) 
		{
			unsigned short* sp = (unsigned short*)&Buffer.data[Buffer.cursize];
			*sp = (unsigned short)value;
			Buffer.cursize += 2;
			Buffer.bit += 16;
		} 
		else if(bits == 32) 
		{
			u32* ip = (u32*)&Buffer.data[Buffer.cursize];
			*ip = value;
			Buffer.cursize += 4;
			Buffer.bit += 32;
		} 
		else 
		{
			Context->LogError("udtMessage::RealWriteBits: Can't write %d bits", bits);
			SetValid(false);
			return;
		}
	} 
	else 
	{
		value &= (0xffffffff >> (32-bits));

		if(bits & 7) 
		{
			s32 nbits = bits&7;
			for(s32 i = 0; i < nbits; ++i) 
			{
				udtHuffman::PutBit(Buffer.data, Buffer.bit, value & 1);
				value = value >> 1;
				++Buffer.bit;
			}
			bits = bits - nbits;
		}

		if(bits) 
		{
			for(s32 i = 0; i < bits; i += 8) 
			{
				udtHuffman::OffsetTransmit(Buffer.data, &Buffer.bit, value & 0xFF);
				value = value >> 8;
			}
		}

		Buffer.cursize = (Buffer.bit >> 3) + 1;
	}
}

s32 udtMessage::RealReadBits(s32 bits) 
{
	if(Buffer.bit + bits > (Buffer.cursize + 1) * 8)
	{
		Context->LogError("udtMessage::RealReadBits: Overflowed!");
		SetValid(false);
		return -1;
	}

	bool sgn;
	if(bits < 0) 
	{
		bits = -bits;
		sgn = true;
	}
	else
	{
		sgn = false;
	}

	s32 value = 0;
	if(Buffer.oob) 
	{
		if(_protocol >= udtProtocol::Dm68)
		{
			if(bits == 8)
			{
				value = Buffer.data[Buffer.readcount];
				Buffer.readcount += 1;
				Buffer.bit += 8;
			}
			else if(bits == 16)
			{
				unsigned short* sp = (unsigned short*)&Buffer.data[Buffer.readcount];
				value = *sp;
				Buffer.readcount += 2;
				Buffer.bit += 16;
			}
			else if(bits == 32)
			{
				u32* ip = (u32*)&Buffer.data[Buffer.readcount];
				value = *ip;
				Buffer.readcount += 4;
				Buffer.bit += 32;
			}
			else
			{
				Context->LogError("udtMessage::RealReadBits: Can't read %d bits", bits);
				SetValid(false);
				return -1;
			}
		}
		else
		{
			if(bits > 32)
			{
				Context->LogError("udtMessage::RealReadBits: Can't read %d bits (more than 32)", bits);
				SetValid(false);
				return -1;
			}

			u64 readBits = *(u64*)&Buffer.data[Buffer.readcount];
			const u64 bitPosition = (u64)Buffer.bit & 7;
			const u64 diff = 64 - (u64)bits;
			readBits >>= bitPosition;
			readBits <<= diff;
			readBits >>= diff;
			value = (s32)readBits;
			Buffer.bit += bits;
			Buffer.readcount = Buffer.bit >> 3;
		}
	} 
	else
	{
		int nbits = 0;
		if(bits & 7)
		{
			nbits = bits & 7;
			const s16 allBits = *(s16*)(Buffer.data + (Buffer.bit >> 3)) >> (Buffer.bit & 7);
			value = allBits & ((1 << nbits) - 1);
			Buffer.bit += (s32)nbits;
			bits = bits - nbits;
		}

		if(bits)
		{
			for(s32 i = 0; i < bits; i += 8)
			{
				s32	get;
				udtHuffman::OffsetReceive(&get, Buffer.data, &Buffer.bit);
				value |= (get << (i + nbits));
			}
		}

		Buffer.readcount = (Buffer.bit >> 3) + 1;
	}

	if(sgn) 
	{
		if(value & (1 << (bits - 1)))
		{
			value |= -1 ^ ((1 << bits) - 1);
		}
	}

	return value;
}

void udtMessage::WriteData(const void* data, s32 length) 
{
	for(s32 i = 0; i < length; ++i) 
	{
		WriteByte(((u8*)data)[i]);
	}
}

void udtMessage::RealWriteFloat(s32 c)
{
	// Sneaking around the strict aliasing rules.
	union IntAndFloat
	{
		IntAndFloat(s32 i) : AsInt(i) {}

		s32 AsInt;
		f32 AsFloat;
	};

	const IntAndFloat fullFloatUnion(c);
	const f32 fullFloat = fullFloatUnion.AsFloat;
	const s32 truncatedFloat = (s32)fullFloat;
	if(truncatedFloat == fullFloat && 
	   truncatedFloat + FLOAT_INT_BIAS >= 0 &&
	   truncatedFloat + FLOAT_INT_BIAS < (1 << FLOAT_INT_BITS))
	{
		// Small integer case.
		WriteBits(0, 1);
		WriteBits(truncatedFloat + FLOAT_INT_BIAS, FLOAT_INT_BITS);
	}
	else
	{
		// Full floating-point value.
		WriteBits(1, 1);
		WriteBits(c, 32);
	}
}

void udtMessage::RealWriteString(const char* s, s32 length) 
{
	if(!s) 
	{
		WriteData("", 1);
		return;
	} 
	
	if(length >= MAX_STRING_CHARS)
	{
		Context->LogError("udtMessage::WriteString: The string is too long");
		SetValid(false);
		return;
	}

	if(_protocol >= udtProtocol::Dm91)
	{
		WriteData(s, length + 1);
		return;
	}

	udtContext::ReadStringBufferRef string = Context->ReadStringBuffer;
	Q_strncpyz(string, s, sizeof(string));

	// get rid of 0xff bytes, because old clients don't like them
	for(s32 i = 0; i < length; ++i)
	{
		if(((u8*)string)[i] > 127)
		{
			string[i] = '.';
		}
	}

	WriteData(string, length + 1);
}

void udtMessage::RealWriteBigString(const char* s, s32 length) 
{
	if(!s) 
	{
		WriteData("", 1);
	}

	if(length >= BIG_INFO_STRING) 
	{
		Context->LogError("udtMessage::WriteBigString: The string is too long");
		SetValid(false);
		return;
	}

	if(_protocol >= udtProtocol::Dm91)
	{
		WriteData(s, length + 1);
		return;
	}

	udtContext::ReadBigStringBufferRef string = Context->ReadBigStringBuffer;
	Q_strncpyz(string, s, sizeof(string));

	// get rid of 0xff bytes, because old clients don't like them
	for(s32 i = 0; i < length; ++i)
	{
		if(((u8*)string)[i] > 127)
		{
			string[i] = '.';
		}
	}

	WriteData(string, length + 1);
}

s32 udtMessage::RealReadFloat()
{
	if(ReadBits(1))
	{
		return ReadBits(32);
	}

	// Sneaking around the strict aliasing rules.
	union FloatAndInt
	{
		FloatAndInt(f32 f) : AsFloat(f) {}

		f32 AsFloat;
		s32 AsInt;
	};

	const s32 intValue = ReadBits(FLOAT_INT_BITS) - FLOAT_INT_BIAS;
	const FloatAndInt realValue((f32)intValue);
	
	return realValue.AsInt;
}

char* udtMessage::RealReadString(s32& length) 
{
	udtContext::ReadStringBufferRef stringBuffer = Context->ReadStringBuffer;

	s32 stringLength = 0;
	do 
	{
		// use ReadByte so -1 is out of bounds
		s32 c = ReadByte(); 
		if(c == -1 || c == 0) 
		{
			break;
		}

		// translate all fmt spec to avoid crash bugs
		if(c == '%')
		{
			c = '.';
		}
		
		// don't allow higher ascii values
		if(_protocol <= udtProtocol::Dm90 && c > 127)
		{
			c = '.';
		}
		
		stringBuffer[stringLength] = (s8)c;
		stringLength++;
	} 
	while(stringLength < (s32)sizeof(stringBuffer) - 1);

	stringBuffer[stringLength] = 0;
	length = stringLength;

	return stringBuffer;
}

char* udtMessage::RealReadBigString(s32& length)
{
	udtContext::ReadBigStringBufferRef stringBuffer = Context->ReadBigStringBuffer;

	s32 stringLength = 0;
	do 
	{
		// use ReadByte so -1 is out of bounds
		s32 c = ReadByte(); 
		if(c == -1 || c == 0)
		{
			break;
		}

		// translate all fmt spec to avoid crash bugs
		if(c == '%') 
		{
			c = '.';
		}

		// don't allow higher ascii values
		if(_protocol <= udtProtocol::Dm90 && c > 127)
		{
			c = '.';
		}

		stringBuffer[stringLength] = (s8)c;
		stringLength++;
	} 
	while(stringLength < (s32)sizeof(stringBuffer) - 1);

	stringBuffer[stringLength] = 0;
	length = stringLength;

	return stringBuffer;
}

void udtMessage::RealReadData(void* data, s32 len) 
{
	for(s32 i = 0; i < len; ++i) 
	{
		((u8*)data)[i] = (u8)ReadByte();
	}
}

s32 udtMessage::RealPeekByte()
{
	if(Buffer.bit + 8 > (Buffer.cursize + 1) * 8)
	{
		Context->LogError("udtMessage::RealPeekByte: Overflowed!");
		SetValid(false);
		return -1;
	}

	const s32 readcount = Buffer.readcount;
	const s32 bit = Buffer.bit;
	const s32 c = ReadByte();
	Buffer.readcount = readcount;
	Buffer.bit = bit;

	return c;
}

bool udtMessage::RealWriteDeltaPlayer(const idPlayerStateBase* from, idPlayerStateBase* to)
{
	s32				i;
	idLargestPlayerState dummy;
	s32				*fromF, *toF;
	s32				lc;

	if(!from) 
	{
		from = &dummy;
		Com_Memset (&dummy, 0, sizeof(dummy));
	}

	lc = 0;
	const idNetField* field;
	for(i = 0, field = _playerStateFields ; i < _playerStateFieldCount ; i++, field++) 
	{
		fromF = (s32 *)((u8 *)from + field->offset);
		toF = (s32 *)((u8 *)to + field->offset);
		if(*fromF != *toF) 
		{
			lc = i+1;
		}
	}

	WriteByte(lc);	// # of changes

	for(i = 0, field = _playerStateFields ; i < lc ; i++, field++) 
	{
		fromF = (s32 *)((u8 *)from + field->offset);
		toF = (s32 *)((u8 *)to + field->offset);

		if(*fromF == *toF) 
		{
			WriteBits(0, 1);
			continue;
		}

		WriteBits(1, 1);
		WriteField(*toF, field->bits);
	}

	//
	// send the arrays
	//
	s32 statsbits = 0;
	for(i=0 ; i<MAX_STATS ; i++) 
	{
		if(to->stats[i] != from->stats[i])
		{
			statsbits |= 1<<i;
		}
	}
	s32 persistantbits = 0;
	for(i=0 ; i<MAX_PERSISTANT ; i++) 
	{
		if(to->persistant[i] != from->persistant[i])
		{
			persistantbits |= 1<<i;
		}
	}
	s32 ammobits = 0;
	for(i=0 ; i<MAX_WEAPONS ; i++)
	{
		if(to->ammo[i] != from->ammo[i]) 
		{
			ammobits |= 1<<i;
		}
	}
	s32 powerupbits = 0;
	for(i=0 ; i<MAX_POWERUPS ; i++) 
	{
		if(to->powerups[i] != from->powerups[i]) 
		{
			powerupbits |= 1<<i;
		}
	}

	if(!statsbits && !persistantbits && !ammobits && !powerupbits) 
	{
		WriteBits(0, 1);	// no change
		return ValidState();
	}
	WriteBits(1, 1);	// changed

	if(statsbits) 
	{
		WriteBits(1, 1);	// changed
		WriteBits(statsbits, MAX_STATS);
		for(i=0 ; i<MAX_STATS ; i++)
		{
			if(statsbits & (1<<i))
			{
				WriteShort(to->stats[i]);
			}
		}
	} 
	else 
	{
		WriteBits(0, 1);	// no change
	}

	if(persistantbits)
	{
		WriteBits(1, 1);	// changed
		WriteBits(persistantbits, MAX_PERSISTANT);
		for(i=0 ; i<MAX_PERSISTANT ; i++)
		{
			if(persistantbits & (1<<i))
			{
				WriteShort(to->persistant[i]);
			}
		}		
	} 
	else 
	{
		WriteBits(0, 1);	// no change
	}

	if(ammobits) 
	{
		WriteBits(1, 1);	// changed
		WriteBits(ammobits, MAX_WEAPONS);
		for(i=0 ; i<MAX_WEAPONS ; i++)
		{
			if(ammobits & (1<<i))
			{
				WriteShort(to->ammo[i]);
			}
		}	
	} 
	else 
	{
		WriteBits(0, 1);	// no change
	}

	if(powerupbits) 
	{
		WriteBits(1, 1);	// changed
		WriteBits(powerupbits, MAX_POWERUPS);
		for(i=0 ; i<MAX_POWERUPS ; i++)
		{
			if(powerupbits & (1<<i))
			{
				WriteLong(to->powerups[i]);
			}
		}
	} 
	else 
	{
		WriteBits(0, 1);	// no change
	}

	return ValidState();
}

void udtMessage::ReadDeltaPlayerDM3(idPlayerStateBase* to)
{
	const idNetField* field = _playerStateFields;
	const s32 fieldCount = _playerStateFieldCount;
	for(s32 i = 0; i < fieldCount; i++, field++)
	{
		if(!ReadBits(1))
		{
			continue;
		}

		s32* const toF = (s32*)((u8*)to + field->offset);
		*toF = ReadField(field->bits);
	}

	// Stats array.
	if(ReadBits(1))
	{
		const s32 mask = ReadShort();
		for(s32 i = 0; i < MAX_STATS; ++i)
		{
			if((mask & (1 << i)) != 0) // bit set?
			{
				// Stats can be negative.
				to->stats[i] = ReadSignedShort();
			}
		}
	}

	// Persistent array.
	if(ReadBits(1))
	{
		const s32 mask = ReadShort();
		for(s32 i = 0; i < MAX_PERSISTANT; ++i)
		{
			if((mask & (1 << i)) != 0) // bit set?
			{
				// @TODO: Can those be negative too?
				to->persistant[i] = ReadShort();
			}
		}
	}

	// Ammo array.
	if(ReadBits(1))
	{
		const s32 mask = ReadShort();
		for(s32 i = 0; i < MAX_WEAPONS; ++i)
		{
			if((mask & (1 << i)) != 0) // bit set?
			{
				to->ammo[i] = ReadShort();
			}
		}
	}

	// Power-ups array.
	if(ReadBits(1))
	{
		const s32 mask = ReadShort();
		for(s32 i = 0; i < MAX_POWERUPS; ++i)
		{
			if((mask & (1 << i)) != 0) // bit set?
			{
				// Yep, we read 32 bits.
				to->powerups[i] = ReadLong();
			}
		}
	}
}

bool udtMessage::RealReadDeltaPlayer(const idPlayerStateBase* from, idPlayerStateBase* to)
{
	s32			i, lc;
	s32			bits;
	s32			*fromF, *toF;
	idLargestPlayerState dummy;

	if(!from) 
	{
		from = &dummy;
		memset(&dummy, 0, sizeof(dummy));
	}
	memcpy(to, from, _protocolSizeOfPlayerState);
	
	if(_protocol <= udtProtocol::Dm48)
	{
		ReadDeltaPlayerDM3(to);
		return ValidState();
	}

	lc = ReadByte();
	if(lc > _playerStateFieldCount || lc < 0)
	{
		Context->LogError("udtMessage::ReadDeltaPlayerstate: Invalid playerState field count: %d (max is %d)", lc, _playerStateFieldCount);
		SetValid(false);
		return false;
	}

	const idNetField* field;
	for(i = 0, field = _playerStateFields; i < lc; i++, field++)
	{
		fromF = (s32 *)((u8 *)from + field->offset);
		toF = (s32 *)((u8 *)to + field->offset);

		if(ReadBits(1) == 0) 
		{
			*toF = *fromF;
			continue;
		} 

		*toF = ReadField(field->bits);
	}
	for(i = lc, field = &_playerStateFields[lc]; i < _playerStateFieldCount; i++, field++)
	{
		fromF = (s32 *)((u8 *)from + field->offset);
		toF = (s32 *)((u8 *)to + field->offset);
		*toF = *fromF;
	}

	// read the arrays
	if(ReadBits(1)) 
	{
		// parse stats
		if(ReadBits(1)) 
		{
			bits = ReadBits(MAX_STATS);
			for(i=0 ; i<MAX_STATS ; i++) 
			{
				if(bits & (1<<i))
				{
					to->stats[i] = ReadShort();
				}
			}
		}

		// parse persistant stats
		if(ReadBits(1)) 
		{
			bits = ReadBits(MAX_PERSISTANT);
			for(i=0 ; i<MAX_PERSISTANT ; i++) 
			{
				if(bits & (1<<i)) 
				{
					to->persistant[i] = ReadShort();
				}
			}
		}

		// parse ammo
		if(ReadBits(1)) 
		{
			bits = ReadBits(MAX_WEAPONS);
			for(i=0 ; i<MAX_WEAPONS ; i++) 
			{
				if(bits & (1<<i)) 
				{
					to->ammo[i] = ReadShort();
				}
			}
		}

		// parse powerups
		if(ReadBits(1)) 
		{
			bits = ReadBits(MAX_POWERUPS);
			for(i=0 ; i<MAX_POWERUPS ; i++) 
			{
				if(bits & (1<<i)) 
				{
					to->powerups[i] = ReadLong();
				}
			}
		}
	}

	return ValidState();
}

/*
Writes part of a packet entities message, including the entity number.
Can delta from either a baseline or a previous packet_entity
If to is NULL, a remove entity update will be sent
If force is not set, then nothing at all will be generated if the entity is
identical, under the assumption that the in-order delta code will catch it.
*/
bool udtMessage::RealWriteDeltaEntity(const idEntityStateBase* from, const idEntityStateBase* to, bool force)
{
	s32			i, lc;
	s32			*fromF, *toF;

	idLargestEntityState dummy;
	if(from == NULL)
	{
		from = &dummy;
		Com_Memset (&dummy, 0, sizeof(dummy));
	}

	// all fields should be 32 bits to avoid any compiler packing issues
	// the "number" field is not part of the field list
	// if this assert fails, someone added a field to the entityState_t
	// struct without updating the message fields
#define STATIC_ASSERT(x) static_assert(x, #x)
	STATIC_ASSERT(EntityStateFieldCount68 + 1 == sizeof(idEntityState68) / 4);
	STATIC_ASSERT(EntityStateFieldCount73 + 1 == sizeof(idEntityState73) / 4);
	STATIC_ASSERT(EntityStateFieldCount90 + 1 == sizeof(idEntityState90) / 4);
	STATIC_ASSERT(EntityStateFieldCount91 + 1 == sizeof(idEntityState91) / 4);
#undef STATIC_ASSERT

	// a NULL to is a delta remove message
	if(to == NULL) 
	{
		if(from == NULL) 
		{
			return ValidState();
		}
		WriteBits(from->number, GENTITYNUM_BITS);
		WriteBits(1, 1);
		return ValidState();
	}

	if(to->number < 0 || to->number >= MAX_GENTITIES) 
	{
		Context->LogError("udtMessage::WriteDeltaEntity: Bad entity number: %d (max is %d)", to->number, MAX_GENTITIES - 1);
		SetValid(false);
		return false;
	}

	lc = 0;
	const idNetField* field;
	// build the change vector as bytes so it is endian independent
	for(i = 0, field = _entityStateFields ; i < _entityStateFieldCount ; i++, field++) 
	{
		fromF = (s32 *)((u8 *)from + field->offset);
		toF = (s32 *)((u8 *)to + field->offset);
		if(*fromF != *toF) 
		{
			lc = i+1;
		}
	}

	if(lc == 0) 
	{
		// nothing at all changed
		if(!force) 
		{
			return ValidState(); // nothing at all
		}
		// write two bits for no change
		WriteBits(to->number, GENTITYNUM_BITS);
		WriteBits(0, 1);		// not removed
		WriteBits(0, 1);		// no delta
		return ValidState();
	}

	WriteBits(to->number, GENTITYNUM_BITS);
	WriteBits(0, 1);			// not removed
	WriteBits(1, 1);			// we have a delta

	WriteByte(lc);	// # of changes

	for(i = 0, field = _entityStateFields ; i < lc ; i++, field++) 
	{
		fromF = (s32 *)((u8 *)from + field->offset);
		toF = (s32 *)((u8 *)to + field->offset);

		if(*fromF == *toF) 
		{
			WriteBits(0, 1);
			continue;
		}

		WriteBits(1, 1);
		if(*toF == 0)
		{
			WriteBits(0, 1);
			continue;
		}

		WriteBits(1, 1);
		WriteField(*toF, field->bits);
	}

	return ValidState();
}

/*
The entity number has already been read from the message, which
is how the from state is identified.

If the delta removes the entity, entityState_t->number will be set to MAX_GENTITIES-1

Can go from either a baseline or a previous packet_entity
*/

// @NOTE: Same values for dm3 and dm_48 confirmed.
static const u8 KnownBitMasks[32][7] =
{
	{ 0x60, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xE1, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00 },
	{ 0x60, 0x80, 0x00, 0x00, 0x00, 0x10, 0x00 },
	{ 0xE0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xE0, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00 },
	{ 0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x20, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0x80, 0x00, 0x00, 0x01, 0x00, 0x00 },
	{ 0xED, 0x07, 0x00, 0x00, 0x00, 0x80, 0x00 },
	{ 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xED, 0x07, 0x00, 0x00, 0x00, 0x30, 0x00 },
	{ 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xE0, 0xC0, 0x00, 0x00, 0x00, 0x10, 0x00 },
	{ 0x60, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00 },
	{ 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0xE1, 0x00, 0x00, 0x00, 0x04, 0x20, 0x00 },
	{ 0xE1, 0x00, 0xC0, 0x01, 0x20, 0x20, 0x00 },
	{ 0xE0, 0xC0, 0x00, 0x00, 0x01, 0x00, 0x00 },
	{ 0x60, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x40, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0xC0, 0x00, 0x00, 0x01, 0x00, 0x00 },
	{ 0x60, 0xC0, 0x00, 0x00, 0x00, 0x10, 0x00 },
	{ 0x60, 0x80, 0x00, 0x00, 0x01, 0x00, 0x01 },
	{ 0x60, 0x80, 0x00, 0x00, 0x00, 0x30, 0x00 },
	{ 0xE0, 0x80, 0x00, 0x00, 0x00, 0x10, 0x00 },
	{ 0x20, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0x80, 0x00, 0x00, 0x00, 0x00, 0x02 },
	{ 0xE0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

void udtMessage::ReadDeltaEntityDM3(const idEntityStateBase* from, idEntityStateBase* to, s32 number)
{
	to->number = number;

	u8 bitMask[7]; // 50-51 bits used only.
	const s32 maskIndex = ReadBits(5);
	if(maskIndex == 0x1F)
	{
		for(s32 i = 0; i < 6; ++i)
		{
			bitMask[i] = (u8)ReadBits(8);
		}
		bitMask[6] = (u8)ReadBits(_protocol == udtProtocol::Dm3 ? 2 : 3);
	}
	else
	{
		// Let's not use memcpy for 7 bytes...
		for(s32 i = 0; i < 7; ++i)
		{
			bitMask[i] = KnownBitMasks[maskIndex][i];
		}
	}

	const idNetField* field = _entityStateFields;
	const s32 fieldCount = _entityStateFieldCount;
	for(s32 i = 0; i < fieldCount; i++, field++)
	{
		const s32* const fromF = (s32*)((u8*)from + field->offset);
		s32* const toF = (s32*)((u8*)to + field->offset);

		const s32 byteIndex = i >> 3;
		const s32 bitIndex = i & 7;
		if((bitMask[byteIndex] & (1 << bitIndex)) == 0)
		{
			*toF = *fromF;
			continue;
		}

		*toF = ReadField(field->bits);
	}
}

bool udtMessage::RealReadDeltaEntity(bool& addedOrChanged, const idEntityStateBase* from, idEntityStateBase* to, s32 number)
{
	s32			i, lc;
	s32			*fromF, *toF;

	if(number < 0 || number >= MAX_GENTITIES) 
	{
		Context->LogError("udtMessage::ReadDeltaEntity: Bad delta entity number: %d (max is %d)", number, MAX_GENTITIES - 1);
		SetValid(false);
		return false;
	}

	// check for a remove
	if(ReadBits(1) == 1) 
	{
		Com_Memset(to, 0, _protocolSizeOfEntityState);
		to->number = MAX_GENTITIES - 1;
		addedOrChanged = false;
		return ValidState();
	}

	// check for no delta
	if(ReadBits(1) == 0) 
	{
		Com_Memcpy(to, from, _protocolSizeOfEntityState);
		to->number = number;
		addedOrChanged = false;
		return ValidState();
	}

	addedOrChanged = true;
	if(_protocol <= udtProtocol::Dm48)
	{
		ReadDeltaEntityDM3(from, to, number);
		return ValidState();
	}
	
	lc = ReadByte();
	if(lc > _entityStateFieldCount || lc < 0)
	{
		Context->LogError("udtMessage::ReadDeltaEntity: Invalid entityState field count: %d (max is %d)", lc, _entityStateFieldCount);
		SetValid(false);
		return false;
	}

	to->number = number;

	const idNetField* field;
	for(i = 0, field = _entityStateFields ; i < lc ; i++, field++) 
	{
		fromF = (s32 *)((u8 *)from + field->offset);
		toF = (s32 *)((u8 *)to + field->offset);

		if(ReadBits(1) == 0) 
		{
			*toF = *fromF;
			continue;
		} 

		if(ReadBits(1) == 0)
		{
			*toF = 0;
			continue;
		}

		*toF = ReadField(field->bits);
	}

	for(i = lc, field = &_entityStateFields[lc] ; i < _entityStateFieldCount ; i++, field++) 
	{
		fromF = (s32 *)((u8 *)from + field->offset);
		toF = (s32 *)((u8 *)to + field->offset);
		*toF = *fromF;
	}

	return ValidState();
}

void udtMessage::SetValid(bool valid)
{
	Buffer.valid = valid;
	if(valid)
	{
		_readBits = &udtMessage::RealReadBits;
		_readFloat = &udtMessage::RealReadFloat;
		_readString = &udtMessage::RealReadString;
		_readBigString = &udtMessage::RealReadBigString;
		_readData = &udtMessage::RealReadData;
		_peekByte = &udtMessage::RealPeekByte;
		_readDeltaEntity = &udtMessage::RealReadDeltaEntity;
		_readDeltaPlayer = &udtMessage::RealReadDeltaPlayer;
		_writeBits = &udtMessage::RealWriteBits;
		_writeFloat = &udtMessage::RealWriteFloat;
		_writeString = &udtMessage::RealWriteString;
		_writeBigString = &udtMessage::RealWriteBigString;
		_writeDeltaPlayer = &udtMessage::RealWriteDeltaPlayer;
		_writeDeltaEntity = &udtMessage::RealWriteDeltaEntity;
	}
	else
	{
		_readBits = &udtMessage::DummyReadCount;
		_readFloat = &udtMessage::DummyRead;
		_readString = &udtMessage::DummyReadString;
		_readBigString = &udtMessage::DummyReadString;
		_readData = &udtMessage::DummyReadData;
		_peekByte = &udtMessage::DummyRead;
		_readDeltaEntity = &udtMessage::DummyReadDeltaEntity;
		_readDeltaPlayer = &udtMessage::DummyReadDeltaPlayer;
		_writeBits = &udtMessage::DummyWriteCount;
		_writeFloat = &udtMessage::DummyWrite;
		_writeString = &udtMessage::DummyWriteString;
		_writeBigString = &udtMessage::DummyWriteString;
		_writeDeltaPlayer = &udtMessage::DummyWriteDeltaPlayer;
		_writeDeltaEntity = &udtMessage::DummyWriteDeltaEntity;
	}
}
