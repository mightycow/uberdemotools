#pragma once


#include "common.hpp"


#define MAX_MSGLEN 16384 // max length of a message, which may be fragmented into multiple packets

typedef struct {
	qbool	allowoverflow;	// if qfalse, do a Com_Error
	qbool	overflowed;		// set to qtrue if the buffer size failed (with allowoverflow set)
	qbool	oob;
	byte	*data;
	int		maxsize;
	int		cursize;
	int		readcount;
	int		bit;				// for bitwise reads and writes
} msg_t;

void MSG_Init (msg_t *buf, byte *data, int length);
void MSG_InitOOB( msg_t *buf, byte *data, int length );
void MSG_Clear (msg_t *buf);
void MSG_WriteData (msg_t *buf, const void *data, int length);
void MSG_Bitstream( msg_t *buf );

// TTimo
// copy a msg_t in case we need to store it as is for a bit
// (as I needed this to keep an msg_t from a static var for later use)
// sets data buffer as MSG_Init does prior to do the copy
void MSG_Copy( msg_t* buf, byte* data, int length, const msg_t* src );

void MSG_WriteBits( msg_t *msg, int value, int bits );

void MSG_WriteByte (msg_t *sb, int c);
void MSG_WriteShort (msg_t *sb, int c);
void MSG_WriteLong (msg_t *sb, int c);
void MSG_WriteString (msg_t *sb, const char *s);
void MSG_WriteBigString (msg_t *sb, const char *s);

void	MSG_BeginReading (msg_t *sb);
void	MSG_BeginReadingOOB(msg_t *sb);

int		MSG_ReadBits( msg_t *msg, int bits );

int		MSG_ReadByte (msg_t *sb);
int		MSG_ReadShort (msg_t *sb);
int		MSG_ReadLong (msg_t *sb);
char	*MSG_ReadString (msg_t *sb);
char	*MSG_ReadBigString (msg_t *sb);
char	*MSG_ReadStringLine (msg_t *sb);
void	MSG_ReadData (msg_t *sb, void *buffer, int size);

void MSG_WriteDeltaUsercmdKey( msg_t* msg, int key, const usercmd_t* from, usercmd_t* to );
void MSG_ReadDeltaUsercmdKey( msg_t* msg, int key, const usercmd_t* from, usercmd_t* to );

void MSG_WriteDeltaPlayerstate( msg_t* msg, const playerState_t* from, playerState_t* to );
void MSG_ReadDeltaPlayerstate( msg_t* msg, const playerState_t* from, playerState_t* to );

// if (int)f == f and (int)f + ( 1<<(FLOAT_INT_BITS-1) ) < ( 1 << FLOAT_INT_BITS )
// the float will be sent with FLOAT_INT_BITS, otherwise all 32 bits will be sent
#define	FLOAT_INT_BITS	13
#define	FLOAT_INT_BIAS	(1<<(FLOAT_INT_BITS-1))

typedef struct {
	const char* name;
	int		offset;
	int		bits;		// 0 = float
} netField_t;

/*
==================
MSG_WriteDeltaEntity

Writes part of a packetentities message, including the entity number.
Can delta from either a baseline or a previous packet_entity
If to is NULL, a remove entity update will be sent
If force is not set, then nothing at all will be generated if the entity is
identical, under the assumption that the in-order delta code will catch it.
==================
*/

template<class DemoT, typename EntityStateT>
void MSG_WriteDeltaEntity( msg_t* msg, const EntityStateT* from, const EntityStateT* to, qbool force )
{
	int			i, lc;
	int			*fromF, *toF;

	EntityStateT dummy;
	if(from == NULL)
	{
		from = &dummy;
		Com_Memset (&dummy, 0, sizeof(dummy));
	}

	// all fields should be 32 bits to avoid any compiler packing issues
	// the "number" field is not part of the field list
	// if this assert fails, someone added a field to the entityState_t
	// struct without updating the message fields
	COMPILE_TIME_ASSERT( DemoT::numESF + 1 == sizeof(*from) / 4 );

	// a NULL to is a delta remove message
	if ( to == NULL ) {
		if ( from == NULL ) {
			return;
		}
		MSG_WriteBits( msg, from->number, GENTITYNUM_BITS );
		MSG_WriteBits( msg, 1, 1 );
		return;
	}

	if ( to->number < 0 || to->number >= MAX_GENTITIES ) {
		LogErrorAndCrash("MSG_WriteDeltaEntity: Bad entity number: %i", to->number);
	}

	lc = 0;
	const netField_t* field;
	// build the change vector as bytes so it is endien independent
	for ( i = 0, field = DemoT::EntityStateFields ; i < DemoT::EntityStateFieldCount ; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );
		if ( *fromF != *toF ) {
			lc = i+1;
		}
	}

	if ( lc == 0 ) {
		// nothing at all changed
		if ( !force ) {
			return;		// nothing at all
		}
		// write two bits for no change
		MSG_WriteBits( msg, to->number, GENTITYNUM_BITS );
		MSG_WriteBits( msg, 0, 1 );		// not removed
		MSG_WriteBits( msg, 0, 1 );		// no delta
		return;
	}

	MSG_WriteBits( msg, to->number, GENTITYNUM_BITS );
	MSG_WriteBits( msg, 0, 1 );			// not removed
	MSG_WriteBits( msg, 1, 1 );			// we have a delta

	MSG_WriteByte( msg, lc );	// # of changes

	for ( i = 0, field = DemoT::EntityStateFields ; i < lc ; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );

		if ( *fromF == *toF ) {
			MSG_WriteBits( msg, 0, 1 );	// no change
			continue;
		}

		MSG_WriteBits( msg, 1, 1 );	// changed

		if ( field->bits == 0 ) {
			float fullFloat = *(float *)toF;

			if (fullFloat == 0.0f) {
				MSG_WriteBits( msg, 0, 1 );
			} else {
				MSG_WriteBits( msg, 1, 1 );
				int trunc = (int)fullFloat;
				if ( trunc == fullFloat && trunc + FLOAT_INT_BIAS >= 0 && 
					trunc + FLOAT_INT_BIAS < ( 1 << FLOAT_INT_BITS ) ) {
					// send as small integer
					MSG_WriteBits( msg, 0, 1 );
					MSG_WriteBits( msg, trunc + FLOAT_INT_BIAS, FLOAT_INT_BITS );
				} else {
					// send as full floating point value
					MSG_WriteBits( msg, 1, 1 );
					MSG_WriteBits( msg, *toF, 32 );
				}
			}
		} else {
			if (*toF == 0) {
				MSG_WriteBits( msg, 0, 1 );
			} else {
				MSG_WriteBits( msg, 1, 1 );
				// integer
				MSG_WriteBits( msg, *toF, field->bits );
			}
		}
	}
}

/*
==================
MSG_ReadDeltaEntity

The entity number has already been read from the message, which
is how the from state is identified.

If the delta removes the entity, entityState_t->number will be set to MAX_GENTITIES-1

Can go from either a baseline or a previous packet_entity
==================
*/

template<class DemoT, typename EntityStateT>
void MSG_ReadDeltaEntity( msg_t* msg, const EntityStateT* from, EntityStateT* to, int number )
{
	int			i, lc;
	int			*fromF, *toF;

	if ( number < 0 || number >= MAX_GENTITIES ) {
		LogErrorAndCrash("MSG_ReadDeltaEntity: Bad delta entity number: %i", number);
	}

	// check for a remove
	if ( MSG_ReadBits( msg, 1 ) == 1 ) {
		Com_Memset( to, 0, sizeof( *to ) );
		to->number = MAX_GENTITIES - 1;
		return;
	}

	// check for no delta
	if ( MSG_ReadBits( msg, 1 ) == 0 ) {
		*to = *from;
		to->number = number;
		return;
	}

	lc = MSG_ReadByte(msg);

	to->number = number;

	const netField_t* field;
	for ( i = 0, field = DemoT::EntityStateFields ; i < lc ; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );

		if ( ! MSG_ReadBits( msg, 1 ) ) {
			// no change
			*toF = *fromF;
		} else {
			if ( field->bits == 0 ) {
				// float
				if ( MSG_ReadBits( msg, 1 ) == 0 ) {
					*(float *)toF = 0.0f;
				} else {
					if ( MSG_ReadBits( msg, 1 ) == 0 ) {
						// integral float
						int trunc = MSG_ReadBits( msg, FLOAT_INT_BITS );
						// bias to allow equal parts positive and negative
						trunc -= FLOAT_INT_BIAS;
						*(float *)toF = (float)trunc; 
					} else {
						// full floating point value
						*toF = MSG_ReadBits( msg, 32 );
					}
				}
			} else {
				if ( MSG_ReadBits( msg, 1 ) == 0 ) {
					*toF = 0;
				} else {
					// integer
					*toF = MSG_ReadBits( msg, field->bits );
				}
			}
//			pcount[i]++;
		}
	}
	for ( i = lc, field = &DemoT::EntityStateFields[lc] ; i < DemoT::EntityStateFieldCount ; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );
		// no change
		*toF = *fromF;
	}
}