#include "utils.hpp"
#include "common.hpp"
#include "timer.hpp"
#include "scoped_stack_allocator.hpp"
#include "parser_context.hpp"
#include "path.hpp"
#include "parser_runner.hpp"

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

#define UDT_MEAN_OF_DEATH_ITEM(Enum, Desc) Desc,
static const char* MeansOfDeathNames[udtMeanOfDeath::Count + 1]
{
	UDT_MEAN_OF_DEATH_LIST(UDT_MEAN_OF_DEATH_ITEM)
	"unknown"
};
#undef UDT_MEAN_OF_DEATH_ITEM


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

bool StringMatchesCutByChatRule(const udtString& string, const udtCutByChatRule& rule, udtVMLinearAllocator& allocator, udtProtocol::Id procotol)
{
	if(string.String == NULL || rule.Pattern == NULL)
	{
		return false;
	}

	udtString input = udtString::NewCloneFromRef(allocator, string);
	udtString pattern = udtString::NewClone(allocator, rule.Pattern);

	if(rule.IgnoreColorCodes)
	{
		udtString::CleanUp(input, procotol);
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
	bool negative = false;
	if(timeMs < 0)
	{
		timeMs = -timeMs;
		negative = true;
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

	formattedTime = AllocateSpaceForString(allocator, minutesDigits + 2 + (negative ? 1 : 0));
	if(formattedTime == NULL)
	{
		return false;
	}

	sprintf(formattedTime, "%s%d%02d", negative ? "-" : "", minutes, seconds);

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

s32 GetErrorCode(bool success, const s32* cancel)
{
	if(success)
	{
		return (s32)udtErrorCode::None;
	}

	return (s32)((cancel != NULL && *cancel != 0) ? udtErrorCode::OperationCanceled : udtErrorCode::OperationFailed);
}

bool RunParser(udtBaseParser& parser, udtStream& file, const s32* cancelOperation)
{
	udtParserRunner runner;
	if(!runner.Init(parser, file, cancelOperation))
	{
		return false;
	}

	while(runner.ParseNextMessage())
	{
	}

	runner.FinishParsing();

	return runner.WasSuccess();
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

s32 GetUDTPlayerMODBitFromUDTMod(s32 udtMod)
{
	switch((udtMeanOfDeath::Id)udtMod)
	{
		case udtMeanOfDeath::Shotgun: return (s32)udtPlayerMeansOfDeathBits::Shotgun;
		case udtMeanOfDeath::Gauntlet: return (s32)udtPlayerMeansOfDeathBits::Gauntlet;
		case udtMeanOfDeath::MachineGun: return (s32)udtPlayerMeansOfDeathBits::MachineGun;
		case udtMeanOfDeath::Grenade: return (s32)udtPlayerMeansOfDeathBits::Grenade;
		case udtMeanOfDeath::GrenadeSplash: return (s32)udtPlayerMeansOfDeathBits::GrenadeSplash;
		case udtMeanOfDeath::Rocket: return (s32)udtPlayerMeansOfDeathBits::Rocket;
		case udtMeanOfDeath::RocketSplash: return (s32)udtPlayerMeansOfDeathBits::RocketSplash;
		case udtMeanOfDeath::Plasma: return (s32)udtPlayerMeansOfDeathBits::Plasma;
		case udtMeanOfDeath::PlasmaSplash: return (s32)udtPlayerMeansOfDeathBits::PlasmaSplash;
		case udtMeanOfDeath::Railgun: return (s32)udtPlayerMeansOfDeathBits::Railgun;
		case udtMeanOfDeath::Lightning: return (s32)udtPlayerMeansOfDeathBits::Lightning;
		case udtMeanOfDeath::BFG: return (s32)udtPlayerMeansOfDeathBits::BFG;
		case udtMeanOfDeath::BFGSplash: return (s32)udtPlayerMeansOfDeathBits::BFGSplash;
		case udtMeanOfDeath::TeleFrag: return (s32)udtPlayerMeansOfDeathBits::TeleFrag;
		case udtMeanOfDeath::NailGun: return (s32)udtPlayerMeansOfDeathBits::NailGun;
		case udtMeanOfDeath::ChainGun: return (s32)udtPlayerMeansOfDeathBits::ChainGun;
		case udtMeanOfDeath::ProximityMine: return (s32)udtPlayerMeansOfDeathBits::ProximityMine;
		case udtMeanOfDeath::Kamikaze: return (s32)udtPlayerMeansOfDeathBits::Kamikaze;
		case udtMeanOfDeath::Grapple: return (s32)udtPlayerMeansOfDeathBits::Grapple;
		case udtMeanOfDeath::Thaw: return (s32)udtPlayerMeansOfDeathBits::Thaw;
		case udtMeanOfDeath::HeavyMachineGun: return (s32)udtPlayerMeansOfDeathBits::HeavyMachineGun;
		default: return -1;
	}
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

s32 GetUDTGameTypeFromIdGameType(s32 gt, udtProtocol::Id protocol, udtGame::Id game)
{
	if(udtIsValidProtocol(protocol) == 0)
	{
		return -1;
	}

	if(protocol <= udtProtocol::Dm68 &&
	   game == udtGame::OSP && 
	   gt == 2)
	{
		// OSP replaced "Single Player" with "ClanBase TDM".
		return (s32)udtGameType::CBTDM;
	}

	if(protocol == udtProtocol::Dm3)
	{
		switch((idGameType3::Id)gt)
		{
			case idGameType3::FFA: return (s32)udtGameType::FFA;
			case idGameType3::Duel: return (s32)udtGameType::Duel;
			case idGameType3::SP: return (s32)udtGameType::SP;
			case idGameType3::TDM: return (s32)udtGameType::TDM;
			case idGameType3::CTF: return (s32)udtGameType::CTF;
			default: return -1;
		}
	}
	else if(protocol >= udtProtocol::Dm48 && protocol <= udtProtocol::Dm68 && game != udtGame::CPMA)
	{
		switch((idGameType48p::Id)gt)
		{
			case idGameType48p::FFA: return (s32)udtGameType::FFA;
			case idGameType48p::Duel: return (s32)udtGameType::Duel;
			case idGameType48p::SP: return (s32)udtGameType::SP;
			case idGameType48p::TDM: return (s32)udtGameType::TDM;
			case idGameType48p::CTF: return (s32)udtGameType::CTF;
			case idGameType48p::OneFlagCTF: return (s32)udtGameType::OneFlagCTF;
			case idGameType48p::Obelisk: return (s32)udtGameType::Obelisk;
			case idGameType48p::Harvester: return (s32)udtGameType::Harvester;
			default: return -1;
		}
	}
	else if(protocol >= udtProtocol::Dm73)
	{
		switch((idGameType73p::Id)gt)
		{
			case idGameType73p::FFA: return (s32)udtGameType::FFA;
			case idGameType73p::Duel: return (s32)udtGameType::Duel;
			case idGameType73p::Race: return (s32)udtGameType::Race;
			case idGameType73p::TDM: return (s32)udtGameType::TDM;
			case idGameType73p::CA: return (s32)udtGameType::CA;
			case idGameType73p::CTF: return (s32)udtGameType::CTF;
			case idGameType73p::OneFlagCTF: return (s32)udtGameType::OneFlagCTF;
			case idGameType73p::Obelisk: return (s32)udtGameType::Obelisk;
			case idGameType73p::Harvester: return (s32)udtGameType::Harvester;
			case idGameType73p::FT: return (s32)udtGameType::FT;
			case idGameType73p::Domination: return (s32)udtGameType::Domination;
			case idGameType73p::CTFS: return (s32)udtGameType::CTFS;
			case idGameType73p::RedRover: return (s32)udtGameType::RedRover;
			default: return -1;
		}
	}
	else if(protocol <= udtProtocol::Dm68 && game == udtGame::CPMA)
	{
		switch((idGameType68_CPMA::Id)gt)
		{
			case idGameType68_CPMA::HM: return (s32)udtGameType::HM;
			case idGameType68_CPMA::FFA: return (s32)udtGameType::FFA;
			case idGameType68_CPMA::Duel: return (s32)udtGameType::Duel;
			case idGameType68_CPMA::SP: return (s32)udtGameType::SP;
			case idGameType68_CPMA::TDM: return (s32)udtGameType::TDM;
			case idGameType68_CPMA::CTF: return (s32)udtGameType::CTF;
			case idGameType68_CPMA::CA: return (s32)udtGameType::CA;
			case idGameType68_CPMA::FT: return (s32)udtGameType::FT;
			case idGameType68_CPMA::CTFS: return (s32)udtGameType::CTFS;
			case idGameType68_CPMA::NTF: return (s32)udtGameType::NTF;
			case idGameType68_CPMA::TwoVsTwo: return (s32)udtGameType::TwoVsTwo;
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

bool IsObituaryEvent(udtObituaryEvent& info, const idEntityStateBase& entity, udtProtocol::Id protocol)
{
	const s32 obituaryEvtId = idEntityEvent::Obituary(protocol);
	const s32 eventTypeId = idEntityType::Event(protocol);
	const s32 eventType = entity.eType & (~EV_EVENT_BITS);
	if(eventType != eventTypeId + obituaryEvtId)
	{
		return false;
	}

	// The target must always be a player.
	const s32 targetIdx = entity.otherEntityNum;
	if(targetIdx < 0 || targetIdx >= MAX_CLIENTS)
	{
		return false;
	}

	// The attacker can be the world, though.
	s32 attackerIdx = entity.otherEntityNum2;
	if(attackerIdx < 0 || attackerIdx >= MAX_CLIENTS)
	{
		attackerIdx = -1;
	}

	info.AttackerIndex = attackerIdx;
	info.TargetIndex = targetIdx;
	info.MeanOfDeath = GetUDTModFromIdMod(entity.eventParm, protocol);

	return true;
}

s32 GetUDTModFromIdMod(s32 idMod, udtProtocol::Id protocol)
{
	if(udtIsValidProtocol(protocol) == 0)
	{
		return -1;
	}

	if(protocol <= udtProtocol::Dm68)
	{
		switch((idMeansOfDeath68::Id)idMod)
		{
			case idMeansOfDeath68::Shotgun: return (s32)udtMeanOfDeath::Shotgun;
			case idMeansOfDeath68::Gauntlet: return (s32)udtMeanOfDeath::Gauntlet;
			case idMeansOfDeath68::MachineGun: return (s32)udtMeanOfDeath::MachineGun;
			case idMeansOfDeath68::Grenade: return (s32)udtMeanOfDeath::Grenade;
			case idMeansOfDeath68::GrenadeSplash: return (s32)udtMeanOfDeath::GrenadeSplash;
			case idMeansOfDeath68::Rocket: return (s32)udtMeanOfDeath::Rocket;
			case idMeansOfDeath68::RocketSplash: return (s32)udtMeanOfDeath::RocketSplash;
			case idMeansOfDeath68::Plasma: return (s32)udtMeanOfDeath::Plasma;
			case idMeansOfDeath68::PlasmaSplash: return (s32)udtMeanOfDeath::PlasmaSplash;
			case idMeansOfDeath68::RailGun: return (s32)udtMeanOfDeath::Railgun;
			case idMeansOfDeath68::Lightning: return (s32)udtMeanOfDeath::Lightning;
			case idMeansOfDeath68::BFG: return (s32)udtMeanOfDeath::BFG;
			case idMeansOfDeath68::BFGSplash: return (s32)udtMeanOfDeath::BFGSplash;
			case idMeansOfDeath68::Water: return (s32)udtMeanOfDeath::Water;
			case idMeansOfDeath68::Slime: return (s32)udtMeanOfDeath::Slime;
			case idMeansOfDeath68::Lava: return (s32)udtMeanOfDeath::Lava;
			case idMeansOfDeath68::Crush: return (s32)udtMeanOfDeath::Crush;
			case idMeansOfDeath68::TeleFrag: return (s32)udtMeanOfDeath::TeleFrag;
			case idMeansOfDeath68::Fall: return (s32)udtMeanOfDeath::Fall;
			case idMeansOfDeath68::Suicide: return (s32)udtMeanOfDeath::Suicide;
			case idMeansOfDeath68::TargetLaser: return (s32)udtMeanOfDeath::TargetLaser;
			case idMeansOfDeath68::HurtTrigger: return (s32)udtMeanOfDeath::TriggerHurt;
			case idMeansOfDeath68::Grapple: return (s32)udtMeanOfDeath::Grapple;
			default: return -1;
		}
	}
	else if(protocol >= udtProtocol::Dm73)
	{
		switch((idMeansOfDeath73p::Id)idMod)
		{
			case idMeansOfDeath73p::Shotgun: return (s32)udtMeanOfDeath::Shotgun;
			case idMeansOfDeath73p::Gauntlet: return (s32)udtMeanOfDeath::Gauntlet;
			case idMeansOfDeath73p::MachineGun: return (s32)udtMeanOfDeath::MachineGun;
			case idMeansOfDeath73p::Grenade: return (s32)udtMeanOfDeath::Grenade;
			case idMeansOfDeath73p::GrenadeSplash: return (s32)udtMeanOfDeath::GrenadeSplash;
			case idMeansOfDeath73p::Rocket: return (s32)udtMeanOfDeath::Rocket;
			case idMeansOfDeath73p::RocketSplash: return (s32)udtMeanOfDeath::RocketSplash;
			case idMeansOfDeath73p::Plasma: return (s32)udtMeanOfDeath::Plasma;
			case idMeansOfDeath73p::PlasmaSplash: return (s32)udtMeanOfDeath::PlasmaSplash;
			case idMeansOfDeath73p::RailGun: return (s32)udtMeanOfDeath::Railgun;
			case idMeansOfDeath73p::Lightning: return (s32)udtMeanOfDeath::Lightning;
			case idMeansOfDeath73p::BFG: return (s32)udtMeanOfDeath::BFG;
			case idMeansOfDeath73p::BFGSplash: return (s32)udtMeanOfDeath::BFGSplash;
			case idMeansOfDeath73p::Water: return (s32)udtMeanOfDeath::Water;
			case idMeansOfDeath73p::Slime: return (s32)udtMeanOfDeath::Slime;
			case idMeansOfDeath73p::Lava: return (s32)udtMeanOfDeath::Lava;
			case idMeansOfDeath73p::Crush: return (s32)udtMeanOfDeath::Crush;
			case idMeansOfDeath73p::TeleFrag: return (s32)udtMeanOfDeath::TeleFrag;
			case idMeansOfDeath73p::Fall: return (s32)udtMeanOfDeath::Fall;
			case idMeansOfDeath73p::Suicide: return (s32)udtMeanOfDeath::Suicide;
			case idMeansOfDeath73p::TargetLaser: return (s32)udtMeanOfDeath::TargetLaser;
			case idMeansOfDeath73p::HurtTrigger: return (s32)udtMeanOfDeath::TriggerHurt;
			case idMeansOfDeath73p::NailGun: return (s32)udtMeanOfDeath::NailGun;
			case idMeansOfDeath73p::ChainGun: return (s32)udtMeanOfDeath::ChainGun;
			case idMeansOfDeath73p::ProximityMine: return (s32)udtMeanOfDeath::ProximityMine;
			case idMeansOfDeath73p::Kamikaze: return (s32)udtMeanOfDeath::Kamikaze;
			case idMeansOfDeath73p::Juiced: return (s32)udtMeanOfDeath::Juiced;
			case idMeansOfDeath73p::Grapple: return (s32)udtMeanOfDeath::Grapple;
			case idMeansOfDeath73p::TeamSwitch: return (s32)udtMeanOfDeath::TeamSwitch;
			case idMeansOfDeath73p::Thaw: return (s32)udtMeanOfDeath::Thaw;
			case idMeansOfDeath73p::HeavyMachineGun: return (s32)udtMeanOfDeath::HeavyMachineGun;
			default: return -1;
		}
	}

	return -1;
}

const char* GetUDTModName(s32 mod)
{
	if(mod < 0 || mod >= (s32)udtMeanOfDeath::Count)
	{
		mod = (s32)udtMeanOfDeath::Count;
	}

	return MeansOfDeathNames[mod];
}

bool GetClanAndPlayerName(udtString& clan, udtString& player, bool& hasClan, udtVMLinearAllocator& allocator, udtProtocol::Id protocol, const char* configString)
{
	if(configString == NULL)
	{
		return false;
	}

	hasClan = false;

	udtString clanAndPlayer;
	if(!ParseConfigStringValueString(clanAndPlayer, allocator, "n", configString))
	{
		return false;
	}

	// "xcn" was for the full clan name, "c" for the country.
	if(protocol <= udtProtocol::Dm90 &&
	   ParseConfigStringValueString(clan, allocator, "cn", configString))
	{
		hasClan = true;
		player = clanAndPlayer;
		return true;
	}

	// Some QuakeCon 2015 demos have a '.' between the clan tag and player name.
	// Some of them have no separator at all and I don't see how the ambiguity can be resolved. :-(
	u32 firstSeparatorIdx = 0;
	if(protocol <= udtProtocol::Dm90 ||
	   !udtString::FindFirstCharacterListMatch(firstSeparatorIdx, clanAndPlayer, udtString::NewConstRef(" .")))
	{
		player = clanAndPlayer;
		return true;
	}

	// There can be multiple spaces...
	u32 lastSeparatorIdx = 0;
	udtString::FindLastCharacterListMatch(lastSeparatorIdx, clanAndPlayer, udtString::NewConstRef(" ."));

	hasClan = true;
	clan = udtString::NewSubstringClone(allocator, clanAndPlayer, 0, firstSeparatorIdx);
	player = udtString::NewSubstringClone(allocator, clanAndPlayer, lastSeparatorIdx + 1);

	return true;
}

namespace idEntityEvent
{
	s32 Obituary(udtProtocol::Id protocol)
	{
		switch(protocol)
		{
			case udtProtocol::Dm3:  return (s32)EV_OBITUARY_3;
			case udtProtocol::Dm48: return (s32)EV_OBITUARY_48;
			case udtProtocol::Dm66:
			case udtProtocol::Dm67:
			case udtProtocol::Dm68: return (s32)EV_OBITUARY_68;
			case udtProtocol::Dm73:
			case udtProtocol::Dm90:
			case udtProtocol::Dm91: return (s32)EV_OBITUARY_73p;
			default: return -1;
		}
	}

	s32 WeaponFired(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm68) ? (s32)EV_FIRE_WEAPON_68 : (s32)EV_FIRE_WEAPON_73p;
	}
};

namespace idEntityType
{
	s32 Event(udtProtocol::Id protocol)
	{
		return (protocol == udtProtocol::Dm3) ? (s32)ET_EVENTS_3 : (s32)ET_EVENTS;
	}
}

namespace idConfigStringIndex
{
	s32 FirstPlayer(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm68) ? (s32)CS_PLAYERS_68 : (s32)CS_PLAYERS_73p;
	}

	s32 Intermission(udtProtocol::Id protocol)
	{
		switch(protocol)
		{
			case udtProtocol::Dm3:  return (s32)CS_INTERMISSION_3;
			case udtProtocol::Dm48:
			case udtProtocol::Dm66:
			case udtProtocol::Dm67:
			case udtProtocol::Dm68: return (s32)CS_INTERMISSION_68;
			case udtProtocol::Dm73:
			case udtProtocol::Dm90:
			case udtProtocol::Dm91: return (s32)CS_INTERMISSION_73p;
			default: return -1;
		}
	}

	s32 LevelStartTime(udtProtocol::Id protocol)
	{
		switch(protocol)
		{
			case udtProtocol::Dm3:  return (s32)CS_LEVEL_START_TIME_3;
			case udtProtocol::Dm48:
			case udtProtocol::Dm66:
			case udtProtocol::Dm67:
			case udtProtocol::Dm68: return (s32)CS_LEVEL_START_TIME_68;
			case udtProtocol::Dm73:
			case udtProtocol::Dm90:
			case udtProtocol::Dm91: return (s32)CS_LEVEL_START_TIME_73p;
			default: return -1;
		}
	}

	s32 WarmUpEndTime(udtProtocol::Id /*protocol*/)
	{
		return (s32)CS_WARMUP;
	}

	s32 FirstPlacePlayerName(udtProtocol::Id protocol)
	{
		return protocol >= udtProtocol::Dm91 ? (s32)CS_SCORES1PLAYER_91p : -1;
	}

	s32 SecondPlacePlayerName(udtProtocol::Id protocol)
	{
		return protocol >= udtProtocol::Dm91 ? (s32)CS_SCORES2PLAYER_91p : -1;
	}

	s32 PauseStart(udtProtocol::Id protocol)
	{
		return protocol >= udtProtocol::Dm73 ? (s32)CS_PAUSE_START_73p : -1;
	}

	s32 PauseEnd(udtProtocol::Id protocol)
	{
		return protocol >= udtProtocol::Dm73 ? (s32)CS_PAUSE_COUNTDOWN_73p : -1;
	}
}

namespace idPowerUpIndex
{
	s32 RedFlag(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm90) ? (s32)PW_REDFLAG : (s32)PW_REDFLAG_91;
	}

	s32 BlueFlag(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm90) ? (s32)PW_BLUEFLAG : (s32)PW_BLUEFLAG_91;
	}

	s32 NeutralFlag(udtProtocol::Id protocol)
	{
		if(protocol == udtProtocol::Dm3) return -1; // @NOTE: dm3 doesn't have a neutral flag slot.

		return (protocol <= udtProtocol::Dm90) ? (s32)PW_NEUTRALFLAG : (s32)PW_NEUTRALFLAG_91;
	}
}

namespace idPersStatsIndex
{
	s32 FlagCaptures(udtProtocol::Id protocol)
	{
		if(protocol == udtProtocol::Dm3) return -1; // @NOTE: dm3 doesn't have a flag captures slot.

		return (protocol <= udtProtocol::Dm68) ? (s32)PERS_CAPTURES_68 : (s32)PERS_CAPTURES_73p;
	}
}
