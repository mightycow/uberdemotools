#include "tests.hpp"
#include "uberdemotools.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

UDT_TEST_FILE("CLI");

static void Run(const char* format, ...)
{
	char cmd[1024];
	va_list argList;
	va_start(argList, format);
	vsprintf(cmd, format, argList);
	printf("> %s\n", cmd);
	system(cmd);
	va_end(argList);
}

UDT_TEST("timed_cut/minqlx_time_rewind")
{
	ctx.OutTempDir.ListFiles();
	Run("UDT_cutter t -g=0 -s=42 -e=653 -o=%s %s/wrong_cut_time_42_653.dm_91", ctx.OutTempDir.Path, ctx.DemoDir);
	ctx.OutTempDir.ListFiles();
	const auto& newFiles = ctx.OutTempDir.GetNewFiles();
	UDT_ENSURE(newFiles.GetSize() == 1);
	printf("yay %s\n", newFiles[0].Path.GetPtr());

	return true;
}
