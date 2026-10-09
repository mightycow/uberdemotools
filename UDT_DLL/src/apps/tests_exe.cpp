#include "tests.hpp"
#include "uberdemotools.h"

UDT_TEST_FILE("Executables");

UDT_TEST("@TODO:")
{
	return true;
}

#if 0

// @TODO: (re)move
void TestTimedCutMinqlx(int* argc, char*** argv)
{
	// t -g=0 -s=42 -e=653 -o=cut tenbit/20260914-183300_slot03_tenbit.dm_91
	static char* args[] =
	{
		(*argv)[0],
		"t",
		"-g=0",
		"-s=42",
		"-e=653",
		"-o=cut",
		"tenbit/20260914-183300_slot03_tenbit.dm_91"
	};

	*argc = UDT_ARRAY_LENGTH(args);
	*argv = args;
}

#endif
