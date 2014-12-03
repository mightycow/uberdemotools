#include "utils.hpp"
#include "common.hpp"
#include "timer.hpp"
#include "scoped_stack_allocator.hpp"

#include <cstdlib>
#include <cstdio>
#include <cctype>


static const char* LogLevels[4] =
{
	"",
	"Warning: ",
	"Error: ",
	"Fatal: "
};


void CallbackConsoleMessage(s32 logLevel, const char* message)
{
	if(logLevel < 0 || logLevel >= 3)
	{
		logLevel = 3;
	}

	FILE* const file = (logLevel == 0 || logLevel == 1) ? stdout : stderr;
	fprintf(file, LogLevels[logLevel]);
	fprintf(file, message);
	fprintf(file, "\n");
}

static void PrintProgress(const char* label, int step, int total)
{
	const int pwidth = 72;
	const int width = pwidth - (int)strlen(label);
	const int pos = (step * width) / total;
	const int percent = (step * 100) / total;

	printf("%s[", label);

	for(int i = 0; i < pos; ++i)
	{
		printf("=");
	}

	for(int i = 0, end = width - pos + 1; i < end; ++i)
	{
		printf(" ");
	}

	printf("] %3d%%\r", percent);

	fflush(stdout);
}

void CallbackConsoleProgress(f32 progress, void*)
{
	PrintProgress("Progress: ", (int)(100.0f * progress), 100);
}

udtStream* CallbackCutDemoFileStreamCreation(s32 startTimeMs, s32 endTimeMs, udtBaseParser* parser, void* userData)
{
	udtVMLinearAllocator& tempAllocator = parser->_context->TempAllocator;
	udtVMScopedStackAllocator scopedTempAllocator(tempAllocator);
	CallbackCutDemoFileStreamCreationInfo* const info = (CallbackCutDemoFileStreamCreationInfo*)userData;

	char* inputFileName = NULL;
	if(parser->_inFilePath != NULL)
	{
		GetFileNameWithoutExtension(inputFileName, tempAllocator, parser->_inFilePath);
	}
	else
	{
		inputFileName = AllocateString(tempAllocator, "NEW_UDT_DEMO");
	}

	char* outputFilePathStart = NULL;
	if(info != NULL && info->OutputFolderPath != NULL)
	{
		StringPathCombine(outputFilePathStart, tempAllocator, info->OutputFolderPath, inputFileName);
	}
	else
	{
		char* inputFolderPath = NULL;
		GetFolderPath(inputFolderPath, tempAllocator, parser->_inFilePath);
		StringPathCombine(outputFilePathStart, tempAllocator, inputFolderPath, inputFileName);
	}

	char* startTime = NULL;
	char* endTime = NULL;
	FormatTimeForFileName(startTime, tempAllocator, startTimeMs);
	FormatTimeForFileName(endTime, tempAllocator, endTimeMs);

	const int gsIndex = parser->_inGameStateIndex;
	const bool outputGsIndex = gsIndex > 0;
	char gsIndexStr[16];
	if(gsIndex > 0)
	{
		sprintf(gsIndexStr, "gs%d_", gsIndex);
	}

	const char* outputFilePathParts[] = 
	{ 
		outputFilePathStart, 
		"_CUT_", 
		outputGsIndex ? gsIndexStr : "",
		startTime, 
		"_", 
		endTime, 
		udtGetFileExtensionByProtocol(parser->_protocol) 
	};
	char* outputFilePath = NULL;
	StringConcatenate(outputFilePath, tempAllocator, outputFilePathParts, UDT_COUNT_OF(outputFilePathParts));

	udtFileStream* const stream = parser->CreatePersistentObject<udtFileStream>();
	if(stream == NULL || !stream->Open(outputFilePath, udtFileOpenMode::Write))
	{
		return NULL;
	}

	parser->_context->LogInfo("Writing cut demo: %s", outputFilePath);

	return stream;
}

const char* GetFolderSeparator()
{
#if defined(_WIN32)
	return "\\";
#else
	return "/";
#endif
}

u32 GetFolderSeparatorLength()
{
	return 1;
}

bool StringMakeLowerCase(char* string)
{
	Q_strlwr(string);

	return true;
}

bool StringMakeUpperCase(char* string)
{
	Q_strupr(string);

	return true;
}

bool StringCloneLowerCase(char*& lowerCase, udtVMLinearAllocator& allocator, const char* input)
{
	lowerCase = AllocateString(allocator, input);
	if(lowerCase == NULL)
	{
		return false;
	}

	StringMakeLowerCase(lowerCase);

	return true;
}

bool StringCloneUpperCase(char*& upperCase, udtVMLinearAllocator& allocator, const char* input)
{
	upperCase = AllocateString(allocator, input);
	if(upperCase == NULL)
	{
		return false;
	}

	StringMakeUpperCase(upperCase);

	return true;
}

bool StringParseInt(s32& output, const char* string)
{
	return sscanf(string, "%d", &output) == 1;
}

bool StringContains_NoCase(const char* string, const char* pattern, u32* charIndex)
{
	const u32 stringLength = (u32)strlen(string);
	const u32 patternLength = (u32)strlen(pattern);
	if(patternLength > stringLength)
	{
		return false;
	}

	const u32 iend = stringLength - patternLength;
	for(u32 i = 0; i <= iend; ++i)
	{
		u32 j = 0;
		for(; pattern[j]; ++j)
		{
			if(::tolower(string[i + j]) != ::tolower(pattern[j]))
			{
				break;
			}
		}
	
		if(!pattern[j])
		{
			if(charIndex != NULL)
			{
				*charIndex = i;
			}

			return true;
		}
	}

	return false;
}

bool StringStartsWith_NoCase(const char* string, const char* pattern)
{
	const u32 stringLength = (u32)strlen(string);
	const u32 patternLength = (u32)strlen(pattern);
	if(patternLength > stringLength)
	{
		return false;
	}

	u32 j = 0;
	for(; pattern[j]; ++j)
	{
		if(::tolower(string[j]) != ::tolower(pattern[j]))
		{
			break;
		}
	}

	if(!pattern[j])
	{
		return true;
	}

	return false;
}

bool StringEndsWith_NoCase(const char* string, const char* pattern)
{
	const u32 stringLength = (u32)strlen(string);
	const u32 patternLength = (u32)strlen(pattern);
	if(patternLength > stringLength)
	{
		return false;
	}

	u32 i = stringLength - patternLength;
	u32 j = 0;
	for(; pattern[j]; ++j)
	{
		if(::tolower(string[i + j]) != ::tolower(pattern[j]))
		{
			break;
		}
	}

	if(!pattern[j])
	{
		return true;
	}

	return false;
}

bool StringEquals_NoCase(const char* a, const char* b)
{
	return Q_stricmp(a, b) == 0;
}

bool StringContains(const char* string, const char* pattern, u32* charIndex)
{
	const char* const patternAddress = strstr(string, pattern);
	if(patternAddress != NULL && charIndex != NULL)
	{
		*charIndex = (u32)(patternAddress - string);
	}

	return patternAddress != NULL;
}

bool StringStartsWith(const char* string, const char* pattern)
{
	const u32 stringLength = (u32)strlen(string);
	const u32 patternLength = (u32)strlen(pattern);
	if(patternLength > stringLength)
	{
		return false;
	}

	u32 j = 0;
	for(; pattern[j]; ++j)
	{
		if(string[j] != pattern[j])
		{
			break;
		}
	}

	if(!pattern[j])
	{
		return true;
	}

	return false;
}

bool StringEndsWith(const char* string, const char* pattern)
{
	const u32 stringLength = (u32)strlen(string);
	const u32 patternLength = (u32)strlen(pattern);
	if(patternLength > stringLength)
	{
		return false;
	}

	u32 i = stringLength - patternLength;
	u32 j = 0;
	for(; pattern[j]; ++j)
	{
		if(string[i + j] != pattern[j])
		{
			break;
		}
	}

	if(!pattern[j])
	{
		return true;
	}

	return false;
}

bool StringEquals(const char* a, const char* b)
{
	return strcmp(a, b) == 0;
}

bool StringFindFirstCharacterInList(u32& index, const char* string, const char* charList)
{
	const u32 stringLength = (u32)strlen(string);
	const u32 charListLength = (u32)strlen(charList);

	for(u32 i = 0; i < stringLength; ++i)
	{
		for(u32 j = 0; j < charListLength; ++j)
		{
			if(string[i] == charList[j])
			{
				index = i;
				return true;
			}
		}
	}

	return false;
}

bool StringFindLastCharacterInList(u32& index, const char* string, const char* charList)
{
	const u32 stringLength = (u32)strlen(string);
	const u32 charListLength = (u32)strlen(charList);

	for(s32 i = (s32)stringLength - 1; i >= 0; --i)
	{
		for(u32 j = 0; j < charListLength; ++j)
		{
			if(string[i] == charList[j])
			{
				index = (u32)i;
				return true;
			}
		}
	}

	return false;
}

bool StringMatchesCutByChatRule(const char* string, const udtCutByChatRule& rule, udtVMLinearAllocator& allocator)
{
	if(string == NULL || rule.Pattern == NULL)
	{
		return false;
	}

	char* const input = AllocateString(allocator, string);
	char* const pattern = AllocateString(allocator, rule.Pattern);

	if(rule.IgnoreColorCodes)
	{
		Q_CleanStr(input);
	}

	if(!rule.CaseSensitive)
	{
		StringMakeLowerCase(input);
		StringMakeLowerCase(pattern);
	}

	if(rule.ChatOperator == (u32)udtChatOperator::Contains)
	{
		return StringContains(input, pattern);
	}
	else if(rule.ChatOperator == (u32)udtChatOperator::StartsWith)
	{
		return StringStartsWith(input, pattern);
	}
	else if(rule.ChatOperator == (u32)udtChatOperator::EndsWith)
	{
		return StringEndsWith(input, pattern);
	}

	return false;
}

bool StringPathCombine(char*& combinedPath, udtVMLinearAllocator& allocator, const char* folderPath, const char* extra)
{
	const bool isSeparatorNeeded = !StringHasTrailingFolderSeparator(folderPath);
	const char* strings[] = 
	{ 
		(folderPath == NULL || *folderPath == '\0') ? "." : folderPath, 
		isSeparatorNeeded ? GetFolderSeparator() : "", 
		extra 
	};
	
	return StringConcatenate(combinedPath, allocator, strings, UDT_COUNT_OF(strings));
}

bool StringHasTrailingFolderSeparator(const char* folderPath)
{
#if defined(_WIN32)
	return StringEndsWith(folderPath, "\\") || StringEndsWith(folderPath, "/");
#else
	return StringEndsWith(folderPath, "/");
#endif
}

bool GetFileName(char*& fileName, udtVMLinearAllocator& allocator, const char* filePath)
{
	u32 lastFolderSeparatorIndex = (u32)-1;
	const char* fileNameStart = filePath;
	if(StringFindLastCharacterInList(lastFolderSeparatorIndex, filePath, "/\\"))
	{
		fileNameStart = filePath + (lastFolderSeparatorIndex + 1);
	}

	fileName = AllocateString(allocator, fileNameStart);

	return true;
}

bool StringHasValidDemoFileExtension(const char* filePath)
{
	for(u32 i = (u32)udtProtocol::FirstProtocol; i < (u32)udtProtocol::AfterLastProtocol; ++i)
	{
		const char* const extension = udtGetFileExtensionByProtocol((udtProtocol::Id)i);
		if(StringEndsWith_NoCase(filePath, extension))
		{
			return true;
		}
	}

	return false;
}

bool StringConcatenate(char*& output, udtVMLinearAllocator& allocator, const char** strings, u32 stringCount)
{
	if(strings == NULL || stringCount == 0)
	{
		return false;
	}

	u32 newLength = 0;
	for(u32 i = 0; i < stringCount; ++i)
	{
		if(strings[i] == NULL)
		{
			return false;
		}

		newLength += (u32)strlen(strings[i]);
	}

	output = AllocateSpaceForString(allocator, newLength);

	strcpy(output, strings[0]);
	for(u32 i = 1; i < stringCount; ++i)
	{
		strcat(output, strings[i]);
	}

	return true;
}

bool StringConcatenate(char*& output, udtVMLinearAllocator& allocator, const char* a, const char* b)
{
	const char* strings[] = { a, b };

	return StringConcatenate(output, allocator, strings, UDT_COUNT_OF(strings));
}

bool StringConcatenate(char*& output, udtVMLinearAllocator& allocator, const char* a, const char* b, const char* c)
{
	const char* strings[] = { a, b, c };

	return StringConcatenate(output, allocator, strings, UDT_COUNT_OF(strings));
}

bool StringSplitLines(udtVMArray<const char*>& lines, char* inOutText)
{
	const s32 length = (s32)strlen(inOutText);

	s32 lastStart = 0;
	for(s32 i = 0; i < length; ++i)
	{
		if(inOutText[i] == '\r' || inOutText[i] == '\n')
		{
			inOutText[i] = '\0';
			if(i - lastStart > 0)
			{
				lines.Add(inOutText + lastStart);
			}
			lastStart = i + 1;
		}
	}

	if(lastStart < length)
	{
		lines.Add(inOutText + lastStart);
	}

	return true;
}

bool StringIsNullOrEmpty(const char* string)
{
	return string == NULL || *string == '\0';
}

bool GetFileNameWithoutExtension(char*& fileNameNoExt, udtVMLinearAllocator& allocator, const char* filePath)
{
	u32 lastFolderSeparatorIndex = (u32)-1;
	const char* fileNameStart = filePath;
	if(StringFindLastCharacterInList(lastFolderSeparatorIndex, filePath, "/\\"))
	{
		fileNameStart = filePath + (lastFolderSeparatorIndex + 1);
	}

	const char* const extensionDot = strrchr(fileNameStart, '.');
	if(extensionDot == NULL)
	{
		fileNameNoExt = AllocateString(allocator, fileNameStart);
		return true;
	}

	fileNameNoExt = AllocateString(allocator, fileNameStart, (u32)(extensionDot - fileNameStart));

	return true;
}

bool GetFolderPath(char*& folderPath, udtVMLinearAllocator& allocator, const char* filePath)
{
	u32 lastFolderSeparatorIndex = (u32)-1;
	if(!StringFindLastCharacterInList(lastFolderSeparatorIndex, filePath, "/\\"))
	{
		folderPath = AllocateString(allocator, "");

		return true;
	}

	folderPath = AllocateSpaceForString(allocator, (u32)lastFolderSeparatorIndex);
	strncpy(folderPath, filePath, (size_t)lastFolderSeparatorIndex);
	folderPath[lastFolderSeparatorIndex] = '\0';

	return true;
}

bool GetFileExtension(char* buffer, u32 bufferLength, const char* filePath)
{
	const char* const extensionDot = strrchr(filePath, '.');
	if(extensionDot == NULL)
	{
		if(bufferLength > 0)
		{
			buffer[0] = '\0';
		}

		return false;
	}

	const size_t maxByteCount = (size_t)bufferLength - 1;
	strncpy(buffer, extensionDot + 1, (size_t)bufferLength - 1);
	buffer[maxByteCount] = '\0';

	return true;
}

bool FormatTimeForFileName(char*& formattedTime, udtVMLinearAllocator& allocator, s32 timeMs)
{
	if(timeMs < 0)
	{
		return false;
	}

	const s32 secondsTotal = timeMs / 1000;
	const s32 minutes = secondsTotal / 60;
	const s32 seconds = secondsTotal - (minutes * 60);

	s32 minutesDigits = 1;
	s32 minutesCopy = minutes;
	while(minutesCopy >= 10)
	{ 
		++minutesDigits;
		minutesCopy /= 10;
	}

	formattedTime = AllocateSpaceForString(allocator, minutesDigits + 2);
	if(formattedTime == NULL)
	{
		return false;
	}

	sprintf(formattedTime, "%d%02d", minutes, seconds);

	return true;
}

bool FormatBytes(char*& formattedSize, udtVMLinearAllocator& allocator, u32 byteCount)
{
	if(byteCount == 0)
	{
		formattedSize = AllocateString(allocator, "0 byte");
		return true;
	}

	const char* const units[] = { "bytes", "KB", "MB", "GB", "TB" };

	s32 unitIndex = 0;
	u32 prev = 0;
	u32 temp = byteCount;
	while(temp >= 1024)
	{
		++unitIndex;
		prev = temp;
		temp >>= 10;
	}

	const f32 number = (f32)prev / 1024.0f;

	formattedSize = (char*)allocator.Allocate(64);
	sprintf(formattedSize, "%.3f %s", number, units[unitIndex]);

	return true;
}

bool StringParseSeconds(s32& duration, const char* buffer)
{
	s32 minutes;
	s32 seconds;
	if(sscanf(buffer, "%d:%d", &minutes, &seconds) == 2)
	{
		duration = minutes * 60 + seconds;
		return true;
	}

	if(sscanf(buffer, "%d", &seconds) == 1)
	{
		duration = seconds;
		return true;
	}

	return false;
}

bool CopyFileRange(udtStream& input, udtStream& output, udtVMLinearAllocator& allocator, u32 startOffset, u32 endOffset)
{
	const u32 chunkSize = 64 * 1024;
	u8* const chunk = allocator.Allocate(chunkSize);

	const u32 fullChunkCount = (endOffset - startOffset) / chunkSize;
	const u32 lastChunkSize = (endOffset - startOffset) % chunkSize;
	for(u32 i = 0; i < fullChunkCount; ++i)
	{
		if(!input.Read(chunk, chunkSize, 1)) return false;
		if(!output.Write(chunk, chunkSize, 1)) return false;
	}

	if(lastChunkSize != 0)
	{
		if(!input.Read(chunk, lastChunkSize, 1)) return false;
		if(!output.Write(chunk, lastChunkSize, 1)) return false;
	}

	return true;
}

s32 GetErrorCode(bool success, s32* cancel)
{
	if(success)
	{
		return (s32)udtErrorCode::None;
	}

	return (s32)((cancel != NULL && *cancel != 0) ? udtErrorCode::OperationCanceled : udtErrorCode::OperationFailed);
}

bool RunParser(udtBaseParser& parser, udtStream& file, const s32* cancelOperation)
{
	udtContext* const context = parser._context;
	udtVMScopedStackAllocator tempAllocator(context->TempAllocator);

	size_t elementsRead;
	udtMessage inMsg;
	u8* const inMsgData = tempAllocator.Allocate(MAX_MSGLEN); // Avoid allocating 16 KB on the stack...
	s32 inServerMessageSequence;

	inMsg.InitContext(context);
	inMsg.InitProtocol(parser._protocol);

	udtTimer timer;
	timer.Start();

	const u64 fileStartOffset = (u64)file.Offset();
	const u64 fileEnd = file.Length();
	const u64 maxByteCount = fileEnd - fileStartOffset;
	for(;;)
	{
		if(cancelOperation != NULL && *cancelOperation != 0)
		{
			parser.FinishParsing(false);
			return false;
		}

		const u32 fileOffset = (u32)file.Offset();

		elementsRead = file.Read(&inServerMessageSequence, 4, 1);
		if(elementsRead != 1)
		{
			parser._context->LogWarning("Demo file %s is truncated", parser._inFileName);
			break;
		}

		inMsg.Init(&inMsgData[0], MAX_MSGLEN);

		elementsRead = file.Read(&inMsg.Buffer.cursize, 4, 1);
		if(elementsRead != 1)
		{
			parser._context->LogWarning("Demo file %s is truncated", parser._inFileName);
			break;
		}

		if(inMsg.Buffer.cursize == -1)
		{
			break;
		}

		if(inMsg.Buffer.cursize > inMsg.Buffer.maxsize)
		{
			context->LogError("Demo file %s has a message length greater than MAX_SIZE", parser._inFileName);
			parser.FinishParsing(false);
			return false;
		}

		elementsRead = file.Read(inMsg.Buffer.data, inMsg.Buffer.cursize, 1);
		if(elementsRead != 1)
		{
			parser._context->LogWarning("Demo file %s is truncated", parser._inFileName);
			break;
		}

		inMsg.Buffer.readcount = 0;
		if(!parser.ParseNextMessage(inMsg, inServerMessageSequence, fileOffset))
		{
			break;
		}

		if(timer.GetElapsedMs() >= UDT_MIN_PROGRESS_TIME_MS)
		{
			timer.Restart();
			const u64 currentByteCount = (u64)fileOffset - fileStartOffset;
			const f32 currentProgress = (f32)currentByteCount / (f32)maxByteCount;
			parser._context->NotifyProgress(currentProgress);
		}
	}

	parser.FinishParsing(true);

	return true;
}

char* AllocateString(udtVMLinearAllocator& allocator, const char* string, u32 stringLength)
{
	if(string == NULL)
	{
		return NULL;
	}

	if(stringLength == 0)
	{
		stringLength = (u32)strlen(string);
	}

	char* newString = (char*)allocator.Allocate(stringLength + 1);
	memcpy(newString, string, stringLength);
	newString[stringLength] = '\0';

	return newString;
}

char* AllocateSpaceForString(udtVMLinearAllocator& allocator, u32 stringLength)
{
	return (char*)allocator.Allocate(stringLength + 1);
}

bool ParseConfigStringValueInt(s32& varValue, const char* varName, const char* configString)
{
	char pattern[64];
	const size_t patternLength = strlen(varName) + 1;
	if(patternLength + 1 > sizeof(pattern))
	{
		return false;
	}

	strcpy(pattern, varName);
	strcat(pattern, "\\");

	u32 index = 0;
	if(!StringContains(configString, pattern, &index))
	{
		return false;
	}

	return sscanf(configString + index + patternLength, "%d", &varValue) == 1;
}

bool ParseConfigStringValueString(char*& varValue, udtVMLinearAllocator& allocator, const char* varName, const char* configString)
{
	char* pattern = NULL;
	if(!StringConcatenate(pattern, allocator, varName, "\\"))
	{
	}

	u32 index = 0;
	if(!StringContains(configString, pattern, &index))
	{
		return false;
	}

	const size_t patternLength = strlen(pattern);
	const char* const valueStart = configString + index + patternLength;
	const char* const separatorAfterValue = strchr(valueStart, '\\');
	if(separatorAfterValue == NULL)
	{
		return false;
	}

	varValue = AllocateString(allocator, configString + index + patternLength, (u32)(separatorAfterValue - valueStart));

	return true;
}

s32 ConvertPowerUpFlagsToValue(s32 flags)
{
	s32 result = PW_NONE;
	for(s32 i = PW_FIRST; i <= PW_LAST; ++i)
	{
		s32 mask = 1 << i;
		if(flags & mask)
		{
			result = i;
			break;
		}
	}

	return result;
}

const char* GetTeamName(s32 team)
{
	switch(team)
	{
		case TEAM_FREE: return "game";
		case TEAM_RED: return "red team";
		case TEAM_BLUE: return "blue team";
		case TEAM_SPECTATOR: return "spectators";
		default: return "unknown";
	}
}
