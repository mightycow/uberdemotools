#include "tests.hpp"
#include "uberdemotools.h"
#include <stdlib.h>
#include <stdio.h>

UDT_TEST_FILE("CLI");

static void Run(const char* cmd)
{
	printf("%s\n", cmd);
	system(cmd);
}

UDT_TEST("timed_cut/minqlx_time_rewind")
{
	char cmd[1024];
	sprintf(cmd, "UDT_cutter t -g=0 -s=42 -e=653 -o=%s %s/wrong_cut_time_42_653.dm_91", ctx.OutTempDir, ctx.DemoDir);
	Run(cmd);

	return true;
}
