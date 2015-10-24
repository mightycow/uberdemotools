#include "uberdemotools.h"

#include <stdlib.h>


int main(int argc, char** argv)
{
	u32 crashType = 0;
	if(argc == 2)
	{
		/* The return value in case of errors is 0, which suits our scenario. */
		const long arg = strtol(argv[1], NULL, 10);
		if(arg >= 0 && arg <= 2)
		{
			crashType = (u32)arg;
		}
	}

	udtCrash(crashType);

	return 0;
}
