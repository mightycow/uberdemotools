#include "utils.hpp"
#include "common.hpp"
#include "timer.hpp"
#include "scoped_stack_allocator.hpp"
#include "parser_context.hpp"
#include "path.hpp"

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

void CallbackConsoleProgress(f32, void*)
{
}

udtStream* CallbackCutDemoFileStreamCreation(s32 startTimeMs, s32 endTimeMs, const char* veryShortDesc, udtBaseParser* parser, void* userData)
{
	udtVMLinearAllocator& tempAllocator = parser->_tempAllocator;
	udtVMScopedStackAllocator scopedTempAllocator(tempAllocator);
	CallbackCutDemoFileStreamCreationInfo* const info = (CallbackCutDemoFileStreamCreationInfo*)userData;

	udtString inputFileName;
	if(udtString::IsNullOrEmpty(parser->_inFilePath) ||
	   !udtPath::GetFileNameWithoutExtension(inputFileName, tempAllocator, parser->_inFilePath))
	{	
		inputFileName = udtString::NewConstRef("NEW_UDT_DEMO");
	}

	udtString outputFilePathStart;
	if(info != NULL && info->OutputFolderPath != NULL)
	{
		udtPath::Combine(outputFilePathStart, tempAllocator, udtString::NewConstRef(info->OutputFolderPath), inputFileName);
	}
	else
	{
		udtString inputFolderPath;
		udtPath::GetFolderPath(inputFolderPath, tempAllocator, parser->_inFilePath);
		udtPath::Combine(outputFilePathStart, tempAllocator, inputFolderPath, inputFileName);
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

	char shortDesc[16];
	if(veryShortDesc)
	{
		sprintf(shortDesc, "_%s", veryShortDesc);
	}

	const char* outputFilePathParts[] = 
	{ 
		outputFilePathStart.String, 
		"_CUT_", 
		outputGsIndex ? gsIndexStr : "",
		startTime, 
		"_", 
		endTime, 
		veryShortDesc ? shortDesc : "",
		udtGetFileExtensionByProtocol(parser->_inProtocol) 
	};
	const udtString outputFilePath = udtString::NewFromConcatenatingMultiple(tempAllocator, outputFilePathParts, (u32)UDT_COUNT_OF(outputFilePathParts));

	udtFileStream* const stream = parser->CreatePersistentObject<udtFileStream>();
	if(stream == NULL || !stream->Open(outputFilePath.String, udtFileOpenMode::Write))
	{
		return NULL;
	}

	parser->_context->LogInfo("Writing cut demo: %s", outputFilePath.String);

	return stream;
}

udtStream* CallbackConvertedDemoFileStreamCreation(s32 /*startTimeMs*/, s32 /*endTimeMs*/, const char* /*veryShortDesc*/, udtBaseParser* parser, void* userData)
{
	udtVMLinearAllocator& tempAllocator = parser->_tempAllocator;
	udtVMScopedStackAllocator scopedTempAllocator(tempAllocator);
	CallbackCutDemoFileStreamCreationInfo* const info = (CallbackCutDemoFileStreamCreationInfo*)userData;

	udtString inputFileName;
	if(udtString::IsNullOrEmpty(parser->_inFilePath) ||
	   !udtPath::GetFileNameWithoutExtension(inputFileName, tempAllocator, parser->_inFilePath))
	{
		inputFileName = udtString::NewConstRef("NEW_UDT_DEMO");
	}

	udtString outputFilePathStart;
	if(info != NULL && info->OutputFolderPath != NULL)
	{
		udtPath::Combine(outputFilePathStart, tempAllocator, udtString::NewConstRef(info->OutputFolderPath), inputFileName);
	}
	else
	{
		udtString inputFolderPath;
		udtPath::GetFolderPath(inputFolderPath, tempAllocator, parser->_inFilePath);
		udtPath::Combine(outputFilePathStart, tempAllocator, inputFolderPath, inputFileName);
	}

	const char* outputFilePathParts[] =
	{
		outputFilePathStart.String,
		udtGetFileExtensionByProtocol(parser->_outProtocol)
	};
	const udtString outputFilePath = udtString::NewFromConcatenatingMultiple(tempAllocator, outputFilePathParts, (u32)UDT_COUNT_OF(outputFilePathParts));

	udtFileStream* const stream = parser->CreatePersistentObject<udtFileStream>();
	if(stream == NULL || !stream->Open(outputFilePath.String, udtFileOpenMode::Write))
	{
		return NULL;
	}

	parser->_context->LogInfo("Writing converted demo: %s", outputFilePath.String);

	return stream;
}

bool StringParseInt(s32& output, const char* string)
{
	return sscanf(string, "%d", &output) == 1;
}

bool StringMatchesCutByChatRule(const udtString& string, const udtCutByChatRule& rule, udtVMLinearAllocator& allocator)
{
	if(string.String == NULL || rule.Pattern == NULL)
	{
		return false;
	}

	udtString input = udtString::NewCloneFromRef(allocator, string);
	udtString pattern = udtString::NewClone(allocator, rule.Pattern);

	if(rule.IgnoreColorCodes)
	{
		udtString::CleanUp(input);
	}

	if(!rule.CaseSensitive)
	{
		udtString::MakeLowerCase(input);
		udtString::MakeLowerCase(pattern);
	}

	if(rule.ChatOperator == (u32)udtChatOperator::Contains)
	{
		u32 charIndex = 0;
		return udtString::Contains(charIndex, input, pattern);
	}
	else if(rule.ChatOperator == (u32)udtChatOperator::StartsWith)
	{
		return udtString::StartsWith(input, pattern);
	}
	else if(rule.ChatOperator == (u32)udtChatOperator::EndsWith)
	{
		return udtString::EndsWith(input, pattern);
	}

	return false;
}

bool StringSplitLines(udtVMArrayWithAlloc<udtString>& lines, udtString& inOutText)
{
	const u32 length = (s32)inOutText.Length;

	udtString tempString;
	tempString.ReservedBytes = 0;
	tempString.Length = 0;

	u32 lastStart = 0;
	for(u32 i = 0; i < length; ++i)
	{
		if(inOutText.String[i] == '\r' || inOutText.String[i] == '\n')
		{
			inOutText.String[i] = '\0';
			if(i - lastStart > 0)
			{
				tempString.String = inOutText.String + lastStart;
				lines.Add(tempString);
			}
			lastStart = i + 1;
		}
	}

	if(lastStart < length)
	{
		tempString.String = inOutText.String + lastStart;
		lines.Add(tempString);
	}

	// Fix line lengths.
	for(u32 i = 0, end = lines.GetSize(); i < end; ++i)
	{
		lines[i] = udtString::NewConstRef(lines[i].String);
	}

	inOutText = udtString::NewEmptyConstant();

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

bool FormatBytes(char*& formattedSize, udtVMLinearAllocator& allocator, u64 byteCount)
{
	if(byteCount == 0)
	{
		formattedSize = AllocateString(allocator, "0 byte");
		return true;
	}

	const char* const units[] = { "bytes", "KB", "MB", "GB", "TB" };

	s32 unitIndex = 0;
	u64 prev = 0;
	u64 temp = byteCount;
	while(temp >= 1024)
	{
		++unitIndex;
		prev = temp;
		temp >>= 10;
	}

	const f64 number = (f64)prev / 1024.0f;

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

	size_t elementsRead;
	udtMessage inMsg;
	u8* const inMsgData = parser._persistentAllocator.Allocate(MAX_MSGLEN); // Avoid allocating 16 KB on the stack...
	s32 inServerMessageSequence;

	inMsg.InitContext(context);
	inMsg.InitProtocol(parser._inProtocol);

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

static const char* FindConfigStringValueAddress(udtVMLinearAllocator& allocator, const char* varName, const char* configString)
{
	// The format is the following: "key1\value1\key2\value2"
	// We work with no guarantee of a leading or trailing backslash.
	// @NOTE: Config strings 0 and 1 have a leading backslash, but player config strings don't.

	const udtString inputString = udtString::NewConstRef(configString);
	const udtString varNameString = udtString::NewConstRef(varName);
	if(udtString::StartsWith(inputString, varNameString) && configString[varNameString.Length] == '\\')
	{
		return configString + varNameString.Length + 1;
	}

	const udtString separator = udtString::NewConstRef("\\");
	const udtString* strings[3] = { &separator, &varNameString, &separator };
	const udtString pattern = udtString::NewFromConcatenatingMultiple(allocator, strings, (u32)UDT_COUNT_OF(strings));

	u32 charIndex = 0;
	if(udtString::Contains(charIndex, inputString, pattern))
	{
		return configString + charIndex + pattern.Length;
	}

	return NULL;
}

bool ParseConfigStringValueInt(s32& varValue, udtVMLinearAllocator& allocator, const char* varName, const char* configString)
{
	const char* const valueString = FindConfigStringValueAddress(allocator, varName, configString);
	if(valueString == NULL)
	{
		return false;
	}

	return sscanf(valueString, "%d", &varValue) == 1;
}

bool ParseConfigStringValueString(udtString& varValue, udtVMLinearAllocator& allocator, const char* varName, const char* configString)
{
	const char* const valueStart = FindConfigStringValueAddress(allocator, varName, configString);
	if(valueStart == NULL)
	{
		return false;
	}

	const char* const separatorAfterValue = strchr(valueStart, '\\');
	const u32 length = (separatorAfterValue == NULL) ? 0 : (u32)(separatorAfterValue - valueStart);
	varValue = udtString::NewClone(allocator, valueStart, length);

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

static s32 GetUDTPlayerMODBitFromIdMod68(s32 idMod)
{
	switch((idMeansOfDeath68::Id)idMod)
	{
		case idMeansOfDeath68::MachineGun: return (s32)udtPlayerMeansOfDeathBits::MachineGun;
		case idMeansOfDeath68::Shotgun: return (s32)udtPlayerMeansOfDeathBits::Shotgun;
		case idMeansOfDeath68::Gauntlet: return (s32)udtPlayerMeansOfDeathBits::Gauntlet;
		case idMeansOfDeath68::Grenade: return (s32)udtPlayerMeansOfDeathBits::Grenade;
		case idMeansOfDeath68::GrenadeSplash: return (s32)udtPlayerMeansOfDeathBits::Grenade;
		case idMeansOfDeath68::Rocket: return (s32)udtPlayerMeansOfDeathBits::Rocket;
		case idMeansOfDeath68::RocketSplash: return (s32)udtPlayerMeansOfDeathBits::RocketSplash;
		case idMeansOfDeath68::Plasma: return (s32)udtPlayerMeansOfDeathBits::Plasma;
		case idMeansOfDeath68::PlasmaSplash: return (s32)udtPlayerMeansOfDeathBits::PlasmaSplash;
		case idMeansOfDeath68::RailGun: return (s32)udtPlayerMeansOfDeathBits::Railgun;
		case idMeansOfDeath68::Lightning: return (s32)udtPlayerMeansOfDeathBits::Lightning;
		case idMeansOfDeath68::BFG: return (s32)udtPlayerMeansOfDeathBits::BFG;
		case idMeansOfDeath68::BFGSplash: return (s32)udtPlayerMeansOfDeathBits::BFGSplash;
		case idMeansOfDeath68::TeleFrag: return (s32)udtPlayerMeansOfDeathBits::TeleFrag;
		default: return 0;
	}
}

static s32 GetUDTPlayerMODBitFromIdMod73p(s32 idMod)
{
	switch((idMeansOfDeath73p::Id)idMod)
	{
		case idMeansOfDeath73p::MachineGun: return (s32)udtPlayerMeansOfDeathBits::MachineGun;
		case idMeansOfDeath73p::Shotgun: return (s32)udtPlayerMeansOfDeathBits::Shotgun;
		case idMeansOfDeath73p::Gauntlet: return (s32)udtPlayerMeansOfDeathBits::Gauntlet;
		case idMeansOfDeath73p::Grenade: return (s32)udtPlayerMeansOfDeathBits::Grenade;
		case idMeansOfDeath73p::GrenadeSplash: return (s32)udtPlayerMeansOfDeathBits::Grenade;
		case idMeansOfDeath73p::Rocket: return (s32)udtPlayerMeansOfDeathBits::Rocket;
		case idMeansOfDeath73p::RocketSplash: return (s32)udtPlayerMeansOfDeathBits::RocketSplash;
		case idMeansOfDeath73p::Plasma: return (s32)udtPlayerMeansOfDeathBits::Plasma;
		case idMeansOfDeath73p::PlasmaSplash: return (s32)udtPlayerMeansOfDeathBits::PlasmaSplash;
		case idMeansOfDeath73p::RailGun: return (s32)udtPlayerMeansOfDeathBits::Railgun;
		case idMeansOfDeath73p::Lightning: return (s32)udtPlayerMeansOfDeathBits::Lightning;
		case idMeansOfDeath73p::BFG: return (s32)udtPlayerMeansOfDeathBits::BFG;
		case idMeansOfDeath73p::BFGSplash: return (s32)udtPlayerMeansOfDeathBits::BFGSplash;
		case idMeansOfDeath73p::TeleFrag: return (s32)udtPlayerMeansOfDeathBits::TeleFrag;
		case idMeansOfDeath73p::NailGun: return (s32)udtPlayerMeansOfDeathBits::NailGun;
		case idMeansOfDeath73p::ChainGun: return (s32)udtPlayerMeansOfDeathBits::ChainGun;
		case idMeansOfDeath73p::ProximityMine: return (s32)udtPlayerMeansOfDeathBits::ProximityMine;
		case idMeansOfDeath73p::Kamikaze: return (s32)udtPlayerMeansOfDeathBits::Kamikaze;
		case idMeansOfDeath73p::Thaw: return (s32)udtPlayerMeansOfDeathBits::Thaw;
		case idMeansOfDeath73p::HeavyMachineGun: return (s32)udtPlayerMeansOfDeathBits::HeavyMachineGun;
		default: return 0;
	}
}

s32 GetUDTPlayerMODBitFromIdMod(s32 idMod, udtProtocol::Id protocol)
{
	if(udtIsValidProtocol(protocol) == 0)
	{
		return 0;
	}

	if(protocol == udtProtocol::Dm68)
	{
		return GetUDTPlayerMODBitFromIdMod68(idMod);
	}

	return GetUDTPlayerMODBitFromIdMod73p(idMod);
}

s32 GetUDTWeaponFromIdWeapon(s32 idWeapon, udtProtocol::Id protocol)
{
	if(udtIsValidProtocol(protocol) == 0)
	{
		return -1;
	}

	if(protocol == udtProtocol::Dm68)
	{
		switch((idWeapon68::Id)idWeapon)
		{
			case idWeapon68::Gauntlet: return (s32)udtWeapon::Gauntlet;
			case idWeapon68::MachineGun: return (s32)udtWeapon::MachineGun;
			case idWeapon68::Shotgun: return (s32)udtWeapon::Shotgun;
			case idWeapon68::GrenadeLauncher: return (s32)udtWeapon::GrenadeLauncher;
			case idWeapon68::RocketLauncher: return (s32)udtWeapon::RocketLauncher;
			case idWeapon68::LightningGun: return (s32)udtWeapon::LightningGun;
			case idWeapon68::Railgun: return (s32)udtWeapon::Railgun;
			case idWeapon68::PlasmaGun: return (s32)udtWeapon::PlasmaGun;
			case idWeapon68::BFG: return (s32)udtWeapon::BFG;
			case idWeapon68::GrapplingHook: return (s32)udtWeapon::GrapplingHook;
			default: return -1;
		}
	}
	else
	{
		switch((idWeapon73p::Id)idWeapon)
		{
			case idWeapon73p::Gauntlet: return (s32)udtWeapon::Gauntlet;
			case idWeapon73p::MachineGun: return (s32)udtWeapon::MachineGun;
			case idWeapon73p::Shotgun: return (s32)udtWeapon::Shotgun;
			case idWeapon73p::GrenadeLauncher: return (s32)udtWeapon::GrenadeLauncher;
			case idWeapon73p::RocketLauncher: return (s32)udtWeapon::RocketLauncher;
			case idWeapon73p::LightningGun: return (s32)udtWeapon::LightningGun;
			case idWeapon73p::Railgun: return (s32)udtWeapon::Railgun;
			case idWeapon73p::PlasmaGun: return (s32)udtWeapon::PlasmaGun;
			case idWeapon73p::BFG: return (s32)udtWeapon::BFG;
			case idWeapon73p::GrapplingHook: return (s32)udtWeapon::GrapplingHook;
			case idWeapon73p::NailGun: return (s32)udtWeapon::NailGun;
			case idWeapon73p::ChainGun: return (s32)udtWeapon::ChainGun;
			case idWeapon73p::ProximityMineLauncher: return (s32)udtWeapon::ProximityMineLauncher;
			case idWeapon73p::HeavyMachineGun: return (s32)udtWeapon::HeavyMachineGun;
			default: return -1;
		}
	}
}

s32 GetUDTWeaponFromIdMod(s32 idMod, udtProtocol::Id protocol)
{
	if(udtIsValidProtocol(protocol) == 0)
	{
		return -1;
	}

	if(protocol == udtProtocol::Dm68)
	{
		switch((idMeansOfDeath68::Id)idMod)
		{
			case idMeansOfDeath68::Gauntlet: return (s32)udtWeapon::Gauntlet;
			case idMeansOfDeath68::MachineGun: return (s32)udtWeapon::MachineGun;
			case idMeansOfDeath68::Shotgun: return (s32)udtWeapon::Shotgun;
			case idMeansOfDeath68::Grenade:
			case idMeansOfDeath68::GrenadeSplash: return (s32)udtWeapon::GrenadeLauncher;
			case idMeansOfDeath68::Rocket:
			case idMeansOfDeath68::RocketSplash: return (s32)udtWeapon::RocketLauncher;
			case idMeansOfDeath68::Lightning: return (s32)udtWeapon::LightningGun;
			case idMeansOfDeath68::RailGun: return (s32)udtWeapon::Railgun;
			case idMeansOfDeath68::Plasma:
			case idMeansOfDeath68::PlasmaSplash: return (s32)udtWeapon::PlasmaGun;
			case idMeansOfDeath68::BFG: return (s32)udtWeapon::BFG;
			case idMeansOfDeath68::Grapple: return (s32)udtWeapon::GrapplingHook;
			default: return -1;
		}
	}
	else if(protocol >= udtProtocol::Dm73)
	{
		switch((idMeansOfDeath73p::Id)idMod)
		{
			case idMeansOfDeath73p::Gauntlet: return (s32)udtWeapon::Gauntlet;
			case idMeansOfDeath73p::MachineGun: return (s32)udtWeapon::MachineGun;
			case idMeansOfDeath73p::Shotgun: return (s32)udtWeapon::Shotgun;
			case idMeansOfDeath73p::Grenade:
			case idMeansOfDeath73p::GrenadeSplash: return (s32)udtWeapon::GrenadeLauncher;
			case idMeansOfDeath73p::Rocket:
			case idMeansOfDeath73p::RocketSplash: return (s32)udtWeapon::RocketLauncher;
			case idMeansOfDeath73p::Lightning: return (s32)udtWeapon::LightningGun;
			case idMeansOfDeath73p::RailGun: return (s32)udtWeapon::Railgun;
			case idMeansOfDeath73p::Plasma:
			case idMeansOfDeath73p::PlasmaSplash: return (s32)udtWeapon::PlasmaGun;
			case idMeansOfDeath73p::BFG: return (s32)udtWeapon::BFG;
			case idMeansOfDeath73p::Grapple: return (s32)udtWeapon::GrapplingHook;
			case idMeansOfDeath73p::HeavyMachineGun: return (s32)udtWeapon::HeavyMachineGun;
			default: return -1;
		}
	}

	return -1;
}

void LogLinearAllocatorStats(u32 threadCount, u32 fileCount, udtContext& context, udtVMLinearAllocator& allocator, const udtVMLinearAllocator::Stats& stats)
{
	char* bytes;
	context.LogInfo("File count: %u", fileCount);
	context.LogInfo("Thread count: %u", threadCount);
	context.LogInfo("Allocator count: %u", stats.AllocatorCount);
	FormatBytes(bytes, allocator, stats.ReservedByteCount);
	context.LogInfo("Reserved memory: %s", bytes);
	FormatBytes(bytes, allocator, stats.CommittedByteCount);
	context.LogInfo("Committed memory: %s", bytes);
	FormatBytes(bytes, allocator, stats.UsedByteCount);
	context.LogInfo("Used memory: %s", bytes);
	const f64 efficiency = 100.0 * ((f64)stats.UsedByteCount / (f64)stats.CommittedByteCount);
	context.LogInfo("Physical memory pages usage: %.1f%%", (f32)efficiency);
}
