#include "common.hpp"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <string>
#include <stdexcept>

#if defined(_MSC_VER) && defined(_WIN32) && defined(_DEBUG)
#	include <Windows.h> // IsDebuggerPresent, __debugbreak
#endif


ProgressCallback _progressCallback = NULL;
MessageCallback _messageCallback = NULL;


void LogInfo(const char* format, ...)
{
	if(!_messageCallback)
	{
		return;
	}

	char msg[MAXPRINTMSG];

	va_list argptr;
	va_start(argptr, format);
	Q_vsnprintf(msg, sizeof(msg) - 1, format, argptr);
	va_end(argptr);

	(*_messageCallback)(0, msg);
}

void LogWarning(const char* format, ...)
{
	if(!_messageCallback)
	{
		return;
	}

	char msg[MAXPRINTMSG];

	va_list argptr;
	va_start(argptr, format);
	Q_vsnprintf(msg, sizeof(msg) - 1, format, argptr);
	va_end(argptr);

	(*_messageCallback)(1, msg);
}

void LogError(const char* format, ...)
{
	if(!_messageCallback)
	{
		return;
	}

	char msg[MAXPRINTMSG];

	va_list argptr;
	va_start(argptr, format);
	Q_vsnprintf(msg, sizeof(msg) - 1, format, argptr);
	va_end(argptr);

	(*_messageCallback)(2, msg);
}

void LogErrorAndCrash(const char* format, ...)
{
	char msg[MAXPRINTMSG];

	va_list argptr;
	va_start(argptr, format);
	Q_vsnprintf(msg, sizeof(msg) - 1, format, argptr);
	va_end(argptr);

	if(_messageCallback)
	{
		(*_messageCallback)(3, msg);
	}

#if defined(_MSC_VER) && defined(_WIN32) && defined(_DEBUG)
	if(IsDebuggerPresent()) __debugbreak();
#endif

	const std::string errorMsg = std::string("UDT critical error: ") + msg;
	throw std::runtime_error(errorMsg.c_str());
}

/*
============================================================================

BYTE ORDER FUNCTIONS
these don't belong in here and should never be used by a VM

============================================================================
*/

short   ShortSwap (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

int    LongSwap (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

typedef union {
    float	f;
    unsigned int i;
} _FloatByteUnion;

float FloatSwap (const float *f) {
	_FloatByteUnion out;

	out.f = *f;
	out.i = LongSwap(out.i);

	return out.f;
}

/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

int Q_isprint( int c )
{
	if ( c >= 0x20 && c <= 0x7E )
		return ( 1 );
	return ( 0 );
}

int Q_islower( int c )
{
	if (c >= 'a' && c <= 'z')
		return ( 1 );
	return ( 0 );
}

int Q_isupper( int c )
{
	if (c >= 'A' && c <= 'Z')
		return ( 1 );
	return ( 0 );
}

int Q_isalpha( int c )
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
		return ( 1 );
	return ( 0 );
}

const char* Q_strrchr( const char* string, int c )
{
	char cc = (char)c;
	const char *s;
	const char *sp = (const char*)0;

	s = string;

	while (*s)
	{
		if (*s == cc)
			sp = s;
		s++;
	}
	if (cc == 0)
		sp = s;

	return sp;
}


// safe strncpy that ensures a trailing zero

void Q_strncpyz( char *dest, const char *src, int destsize )
{
	if ( !dest ) {
		LogErrorAndCrash("Q_strncpyz: NULL dest");
		return;
	}
	if ( !src ) {
		LogErrorAndCrash("Q_strncpyz: NULL src");
		return;
	}
	if ( destsize < 1 ) {
		LogErrorAndCrash("Q_strncpyz: destsize < 1");
		return;
	}

	strncpy( dest, src, destsize-1 );
	dest[destsize-1] = 0;
}

int Q_stricmpn( const char *s1, const char *s2, int n )
{
	int		c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}

		if (c1 != c2) {
			if (c1 >= 'a' && c1 <= 'z') {
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z') {
				c2 -= ('a' - 'A');
			}
			if (c1 != c2) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while (c1);

	return 0;		// strings are equal
}

int Q_strncmp( const char *s1, const char *s2, int n )
{
	int		c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}

		if (c1 != c2) {
			return c1 < c2 ? -1 : 1;
		}
	} while (c1);

	return 0;		// strings are equal
}

int Q_stricmp( const char *s1, const char *s2 )
{
	return (s1 && s2) ? Q_stricmpn (s1, s2, 99999) : -1;
}


char *Q_strlwr( char *s1 )
{
	char* s = s1;
	while ( *s ) {
		*s = (char)tolower(*s);
		s++;
	}
	return s1;
}

char *Q_strupr( char *s1 )
{
	char* s = s1;
	while ( *s ) {
		*s = (char)toupper(*s);
		s++;
	}
	return s1;
}


// never goes past bounds or leaves without a terminating 0
void Q_strcat( char *dest, int size, const char *src )
{
	int		l1;

	l1 = strlen( dest );
	if ( l1 >= size ) {
		LogErrorAndCrash("Q_strcat: already overflowed");
	}
	Q_strncpyz( dest + l1, src, size - l1 );
}


int Q_PrintStrlen( const char *string )
{
	int len = 0;
	const char* p = string;

	if (!p)
		return 0;

	while ( *p ) {
		if( Q_IsColorString( p ) ) {
			p += 2;
			continue;
		}
		p++;
		len++;
	}

	return len;
}


char *Q_CleanStr( char *string ) {
	char*	d;
	char*	s;
	int		c;

	s = string;
	d = string;
	while ((c = *s) != 0 ) {
		if ( Q_IsColorString( s ) ) {
			s++;
		}		
		else if ( c >= 0x20 && c <= 0x7E ) {
			*d++ = (char)c;
		}
		s++;
	}
	*d = '\0';

	return string;
}
