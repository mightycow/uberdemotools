#include "common.hpp"

#include <string.h>


void Q_strncpyz(char* dest, const char* src, s32 destsize)
{
	if(dest == NULL || src == NULL || destsize < 1)
	{
		return;
	}

	strncpy(dest, src, destsize - 1);
	dest[destsize - 1] = 0;
}
