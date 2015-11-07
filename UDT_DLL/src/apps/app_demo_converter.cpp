#include "parser.hpp"
#include "shared.hpp"
#include "stack_trace.hpp"
#include "utils.hpp"

#include <stdio.h>
#include <stdlib.h>


void PrintHelp()
{
	printf("Converts demos from one protocol to another.\n");
	printf("\n");
	printf("UDT_converter -p=protocol [-q] filepath\n");
	printf("\n");
	printf("-q    quiet mode: no logging to stdout      (default: off)\n");
	printf("-p=N  set the output protocol version to N\n");
	printf("        N=68  output to a .dm_68 file\n");
	printf("              supported input: .dm3 and .dm_48");
	printf("        N=91  output to a .dm_91 file\n");
	printf("              supported input: .dm_73 and .dm_90");
}

static bool IsValidConversion(udtProtocol::Id input, udtProtocol::Id output)
{
	if((output == udtProtocol::Dm91 && input == udtProtocol::Dm73) ||
	   (output == udtProtocol::Dm91 && input == udtProtocol::Dm90) ||
	   (output == udtProtocol::Dm68 && input == udtProtocol::Dm3) ||
	   (output == udtProtocol::Dm68 && input == udtProtocol::Dm48))
	{
		return true;
	}

	return false;
}

static bool ConvertDemos(const char* filePath, udtProtocol::Id outputProtocol)
{
	if(!IsValidConversion((udtProtocol::Id)udtGetProtocolByFilePath(filePath), outputProtocol))
	{
		fprintf(stderr, "Unsupported conversion.\n");
		return false;
	}

	s32 outputErrorCode = 0;
	s32 cancel = 0;

	udtParseArg info;
	memset(&info, 0, sizeof(info));
	info.CancelOperation = &cancel;
	info.MessageCb = &CallbackConsoleMessage;
	info.ProgressCb = &CallbackConsoleProgress;

	udtMultiParseArg extraInfo;
	memset(&extraInfo, 0, sizeof(extraInfo));
	extraInfo.FileCount = 1;
	extraInfo.FilePaths = &filePath;
	extraInfo.OutputErrorCodes = &outputErrorCode;

	udtProtocolConversionArg conversionArg;
	memset(&conversionArg, 0, sizeof(conversionArg));
	conversionArg.OutputProtocol = (u32)outputProtocol;

	const s32 errorCode = udtConvertDemoFiles(&info, &extraInfo, &conversionArg);
	if(errorCode != (s32)udtErrorCode::None)
	{
		fprintf(stderr, "udtConvertDemoFiles failed with error: '%s'.\n", udtGetErrorCodeString(errorCode));
		return false;
	}

	return true;
}

int udt_main(int argc, char** argv)
{
	if(argc == 1)
	{
		PrintHelp();
		return 0;
	}

	const char* const inputPath = argv[argc - 1];
	udtProtocol::Id outputProtocol = udtProtocol::Invalid;
	for(int i = 1; i < argc - 1; ++i)
	{
		s32 localProtocol = (s32)udtProtocol::Invalid;
		const udtString arg = udtString::NewConstRef(argv[i]);
		if(udtString::StartsWith(arg, "-p=") &&
		   arg.Length >= 4 &&
		   StringParseInt(localProtocol, arg.String + 3))
		{
			if(localProtocol == 68)
			{
				outputProtocol = udtProtocol::Dm68;
			}
			else if(localProtocol == 91)
			{
				outputProtocol = udtProtocol::Dm91;
			}
		}
	}

	if(outputProtocol == udtProtocol::Invalid)
	{
		fprintf(stderr, "Invalid or unspecified output protocol number.\n");
		return 1;
	}

	if(!ConvertDemos(inputPath, outputProtocol))
	{
		return 1;
	}

	return 0;
}
