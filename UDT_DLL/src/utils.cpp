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


#define UDT_MEAN_OF_DEATH_ITEM(Enum, Desc) Desc,
static const char* MeansOfDeathNames[udtMeanOfDeath::Count + 1]
{
	UDT_MEAN_OF_DEATH_LIST(UDT_MEAN_OF_DEATH_ITEM)
	"unknown"
};
#undef UDT_MEAN_OF_DEATH_ITEM


udtString CallbackCutDemoFileNameCreation(const udtDemoStreamCreatorArg& arg)
{
	udtBaseParser* const parser = arg.Parser;
	udtVMLinearAllocator& tempAllocator = *arg.TempAllocator;
	CallbackCutDemoFileStreamCreationInfo* const info = (CallbackCutDemoFileStreamCreationInfo*)arg.UserData;

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

	const udtString startTime = FormatTimeForFileName(tempAllocator, arg.StartTimeMs);
	const udtString endTime = FormatTimeForFileName(tempAllocator, arg.EndTimeMs);

	const int gsIndex = parser->_inGameStateIndex;
	const bool outputGsIndex = gsIndex > 0;
	char gsIndexStr[16];
	if(gsIndex > 0)
	{
		sprintf(gsIndexStr, "gs%d_", gsIndex);
	}

	char shortDesc[16];
	if(arg.VeryShortDesc != NULL)
	{
		sprintf(shortDesc, "_%s", arg.VeryShortDesc);
	}

	const udtString cut = udtString::NewConstRef("_CUT_");
	const udtString us = udtString::NewConstRef("_");
	const udtString gsIdx = udtString::NewConstRef(outputGsIndex ? gsIndexStr : "");
	const udtString desc = udtString::NewConstRef(arg.VeryShortDesc != NULL ? shortDesc : "");
	const udtString proto = udtString::NewConstRef(udtGetFileExtensionByProtocol(parser->_inProtocol));
	const udtString* outputFilePathParts[] =
	{
		&outputFilePathStart,
		&cut,
		&gsIdx,
		&startTime,
		&us,
		&endTime,
		&desc,
		&proto
	};
	const udtString filePath = udtString::NewFromConcatenatingMultiple(*arg.FilePathAllocator, outputFilePathParts, (u32)UDT_COUNT_OF(outputFilePathParts));

	parser->_context->LogInfo("Writing cut demo: %s", filePath.GetPtr());

	return filePath;
}

udtString CallbackConvertedDemoFileNameCreation(const udtDemoStreamCreatorArg& arg)
{
	udtBaseParser* const parser = arg.Parser;
	udtVMLinearAllocator& tempAllocator = *arg.TempAllocator;
	CallbackCutDemoFileStreamCreationInfo* const info = (CallbackCutDemoFileStreamCreationInfo*)arg.UserData;

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

	const udtString proto = udtString::NewConstRef(udtGetFileExtensionByProtocol(parser->_outProtocol));
	const udtString* outputFilePathParts[] =
	{
		&outputFilePathStart,
		&proto
	};
	const udtString filePath = udtString::NewFromConcatenatingMultiple(*arg.FilePathAllocator, outputFilePathParts, (u32)UDT_COUNT_OF(outputFilePathParts));

	parser->_context->LogInfo("Writing converted demo: %s", filePath.GetPtr());

	return filePath;
}

bool StringParseInt(s32& output, const char* string)
{
	return sscanf(string, "%d", &output) == 1;
}

bool StringMatchesCutByChatRule(const udtString& string, const udtChatPatternRule& rule, udtVMLinearAllocator& allocator, udtProtocol::Id procotol)
{
	if(!string.IsValid() || rule.Pattern == NULL)
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

bool StringSplitLines(udtVMArray<udtString>& lines, udtString& inOutText)
{
	const u32 length = inOutText.GetLength();
	char* const inOutTextPtr = inOutText.GetWritePtr();

	u32 lastStart = 0;
	for(u32 i = 0; i < length; ++i)
	{
		if(inOutTextPtr[i] == '\r' || inOutTextPtr[i] == '\n')
		{
			inOutTextPtr[i] = '\0';
			if(i - lastStart > 0)
			{
				lines.Add(udtString::NewConstRef(inOutTextPtr + lastStart));
			}
			lastStart = i + 1;
		}
	}

	if(lastStart < length)
	{
		lines.Add(udtString::NewConstRef(inOutTextPtr + lastStart));
	}

	// Fix line lengths.
	for(u32 i = 0, end = lines.GetSize(); i < end; ++i)
	{
		lines[i] = udtString::NewConstRef(lines[i].GetPtr());
	}

	inOutText = udtString::NewEmptyConstant();

	return true;
}

udtString FormatTimeForFileName(udtVMLinearAllocator& allocator, s32 timeMs)
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

	const u32 length = minutesDigits + 2 + (negative ? 1 : 0);
	udtString formattedTime = udtString::NewEmpty(allocator, length + 1);
	sprintf(formattedTime.GetWritePtr(), "%s%d%02d", negative ? "-" : "", minutes, seconds);
	formattedTime.SetLength(length);

	return formattedTime;
}

udtString FormatBytes(udtVMLinearAllocator& allocator, u64 byteCount)
{
	if(byteCount == 0)
	{
		return udtString::NewClone(allocator, "0 byte");
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

	udtString formattedSize = udtString::NewEmpty(allocator, 64);
	sprintf(formattedSize.GetWritePtr(), "%.3f %s", number, units[unitIndex]);

	return formattedSize;
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
	u8* const chunk = allocator.AllocateAndGetAddress(chunkSize);

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

static const char* FindConfigStringValueAddress(udtVMLinearAllocator& allocator, const char* varName, const char* configString)
{
	// The format is the following: "key1\value1\key2\value2"
	// We work with no guarantee of a leading or trailing backslash.

	const udtString inputString = udtString::NewConstRef(configString);
	const udtString varNameString = udtString::NewConstRef(varName);
	if(inputString.GetLength() > varNameString.GetLength() && 
	   udtString::StartsWith(inputString, varNameString) &&
	   configString[varNameString.GetLength()] == '\\')
	{
		return configString + varNameString.GetLength() + 1;
	}

	const udtString separator = udtString::NewConstRef("\\");
	const udtString* strings[3] = { &separator, &varNameString, &separator };
	const udtString pattern = udtString::NewFromConcatenatingMultiple(allocator, strings, (u32)UDT_COUNT_OF(strings));

	u32 charIndex = 0;
	if(udtString::Contains(charIndex, inputString, pattern))
	{
		return configString + charIndex + pattern.GetLength();
	}

	return NULL;
}

bool ParseConfigStringValueInt(s32& varValue, udtVMLinearAllocator& allocator, const char* varName, const char* configString)
{
	if(configString == NULL)
	{
		return false;
	}

	const char* const valueString = FindConfigStringValueAddress(allocator, varName, configString);
	if(valueString == NULL || *valueString == '\0')
	{
		return false;
	}

	int result = 0;
	if(sscanf(valueString, "%d", &result) != 1)
	{
		return false;
	}

	varValue = (s32)result;

	return true;
}

bool ParseConfigStringValueString(udtString& varValue, udtVMLinearAllocator& allocator, const char* varName, const char* configString)
{
	if(configString == NULL)
	{
		return false;
	}

	// @NOTE: An empty string can be a valid value.
	const char* const valueStart = FindConfigStringValueAddress(allocator, varName, configString);
	if(valueStart == NULL)
	{
		return false;
	}

	const char* const separatorAfterValue = strchr(valueStart, '\\');
	const u32 length = separatorAfterValue == NULL ? u32(udtString::InvalidLength) : u32(separatorAfterValue - valueStart);
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

void LogLinearAllocatorDebugStats(udtContext& context, udtVMLinearAllocator& tempAllocator)
{
	u32 allocatorCount = 256;
	const uptr allocatorListOffset = tempAllocator.Allocate((uptr)sizeof(udtVMLinearAllocator*) * (uptr)allocatorCount);
	udtVMLinearAllocator** allocators = (udtVMLinearAllocator**)tempAllocator.GetAddressAt(allocatorListOffset);
	udtVMLinearAllocator::GetThreadAllocators(allocatorCount, allocators);

	udtString unusedMemory;
	udtString reservedMemory;
	uptr totalUnused = 0;
	uptr totalReserved = 0;
	uptr unusedPc = 0;
	context.LogInfo("Thread allocator count: %u", allocatorCount);
	for(u32 i = 0; i < allocatorCount; ++i)
	{
		allocators = (udtVMLinearAllocator**)tempAllocator.GetAddressAt(allocatorListOffset);
		udtVMLinearAllocator& allocator = *allocators[i];
		if(allocator.GetReservedByteCount() == 0)
		{
			continue;
		}

		const char* name = allocator.GetName();
		if(name == NULL)
		{
			name = "noname";
		}

		const uptr lowestUnusedByteCount = allocator.GetReservedByteCount() - allocator.GetPeakUsedByteCount();
		totalUnused += lowestUnusedByteCount;
		totalReserved += allocator.GetReservedByteCount();
		unusedPc = (100 * lowestUnusedByteCount) / allocator.GetReservedByteCount();

		// Use allocator before any new allocations calls before it could be relocated by FormatBytes.
		const u64 reservedByteCount = allocator.GetReservedByteCount();
		const u32 resizeCount = allocator.GetResizeCount();

		udtVMScopedStackAllocator tempAllocScope(tempAllocator);
		unusedMemory = FormatBytes(tempAllocator, (u64)lowestUnusedByteCount);
		reservedMemory = FormatBytes(tempAllocator, (u64)reservedByteCount);
		context.LogInfo("- %s: reserved %s - unused %s (%u%%) - resized %u", name, reservedMemory.GetPtr(), unusedMemory.GetPtr(), (u32)unusedPc, resizeCount);
	}

	unusedPc = (100 * totalUnused) / totalReserved;
	unusedMemory = FormatBytes(tempAllocator, (u64)totalUnused);
	reservedMemory = FormatBytes(tempAllocator, (u64)totalReserved);
	context.LogInfo("Thread unused byte count: %s (%u%%)", unusedMemory.GetPtr(), (u32)unusedPc);
	context.LogInfo("Thread reserved byte count: %s", reservedMemory.GetPtr());
}

bool IsObituaryEvent(udtObituaryEvent& info, const idEntityStateBase& entity, udtProtocol::Id protocol)
{
	const s32 obituaryEvtId = idEntityEvent::Obituary(protocol);
	const s32 eventTypeId = idEntityType::Event(protocol);
	const s32 eventType = entity.eType & (~ID_ES_EVENT_BITS);
	if(eventType != eventTypeId + obituaryEvtId)
	{
		return false;
	}

	// The target must always be a player.
	const s32 targetIdx = entity.otherEntityNum;
	if(targetIdx < 0 || targetIdx >= ID_MAX_CLIENTS)
	{
		return false;
	}

	// The attacker can be the world, though.
	s32 attackerIdx = entity.otherEntityNum2;
	if(attackerIdx < 0 || attackerIdx >= ID_MAX_CLIENTS)
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

s32 GetUDTItemFromIdItem(s32 idItem, udtProtocol::Id protocol)
{
	if(udtIsValidProtocol(protocol) == 0)
	{
		return -1;
	}

	// @NOTE: idItem90p is a superset of idItem73.
	if(protocol == udtProtocol::Dm73 &&
	   idItem >= (s32)idItem73::Count)
	{
		return -1;
	}

	// @NOTE: idItem68_CPMA is a superset of idItem68_baseq3.
	if(protocol <= udtProtocol::Dm68 &&
	   idItem >= (s32)idItem68_baseq3::Count)
	{
		return -1;
	}
	
	if(protocol >= udtProtocol::Dm73)
	{
		switch((idItem90p::Id)idItem)
		{
			case idItem90p::ItemArmorShard: return (s32)udtItem::ItemArmorShard;
			case idItem90p::ItemArmorCombat: return (s32)udtItem::ItemArmorCombat;
			case idItem90p::ItemArmorBody: return (s32)udtItem::ItemArmorBody;
			case idItem90p::ItemArmorJacket: return (s32)udtItem::ItemArmorJacket;
			case idItem90p::ItemHealthSmall: return (s32)udtItem::ItemHealthSmall;
			case idItem90p::ItemHealth: return (s32)udtItem::ItemHealth;
			case idItem90p::ItemHealthLarge: return (s32)udtItem::ItemHealthLarge;
			case idItem90p::ItemHealthMega: return (s32)udtItem::ItemHealthMega;
			case idItem90p::WeaponGauntlet: return (s32)udtItem::WeaponGauntlet;
			case idItem90p::WeaponShotgun: return (s32)udtItem::WeaponShotgun;
			case idItem90p::WeaponMachinegun: return (s32)udtItem::WeaponMachinegun;
			case idItem90p::WeaponGrenadelauncher: return (s32)udtItem::WeaponGrenadeLauncher;
			case idItem90p::WeaponRocketlauncher: return (s32)udtItem::WeaponRocketLauncher;
			case idItem90p::WeaponLightning: return (s32)udtItem::WeaponLightningGun;
			case idItem90p::WeaponRailgun: return (s32)udtItem::WeaponRailgun;
			case idItem90p::WeaponPlasmagun: return (s32)udtItem::WeaponPlasmaGun;
			case idItem90p::WeaponBFG: return (s32)udtItem::WeaponBFG;
			case idItem90p::WeaponGrapplinghook: return (s32)udtItem::WeaponGrapplingHook;
			case idItem90p::AmmoShells: return (s32)udtItem::AmmoShells;
			case idItem90p::AmmoBullets: return (s32)udtItem::AmmoBullets;
			case idItem90p::AmmoGrenades: return (s32)udtItem::AmmoGrenades;
			case idItem90p::AmmoCells: return (s32)udtItem::AmmoCells;
			case idItem90p::AmmoLightning: return (s32)udtItem::AmmoLightning;
			case idItem90p::AmmoRockets: return (s32)udtItem::AmmoRockets;
			case idItem90p::AmmoSlugs: return (s32)udtItem::AmmoSlugs;
			case idItem90p::AmmoBFG: return (s32)udtItem::AmmoBFG;
			case idItem90p::HoldableTeleporter: return (s32)udtItem::HoldableTeleporter;
			case idItem90p::HoldableMedkit: return (s32)udtItem::HoldableMedkit;
			case idItem90p::ItemQuad: return (s32)udtItem::ItemQuad;
			case idItem90p::ItemEnviro: return (s32)udtItem::ItemEnviro;
			case idItem90p::ItemHaste: return (s32)udtItem::ItemHaste;
			case idItem90p::ItemInvis: return (s32)udtItem::ItemInvis;
			case idItem90p::ItemRegen: return (s32)udtItem::ItemRegen;
			case idItem90p::ItemFlight: return (s32)udtItem::ItemFlight;
			case idItem90p::TeamCTFRedflag: return (s32)udtItem::FlagRed;
			case idItem90p::TeamCTFBlueflag: return (s32)udtItem::FlagBlue;
			case idItem90p::HoldableKamikaze: return (s32)udtItem::HoldableKamikaze;
			case idItem90p::HoldablePortal: return (s32)udtItem::HoldablePortal;
			case idItem90p::HoldableInvulnerability: return (s32)udtItem::HoldableInvulnerability;
			case idItem90p::AmmoNails: return (s32)udtItem::AmmoNails;
			case idItem90p::AmmoMines: return (s32)udtItem::AmmoMines;
			case idItem90p::AmmoBelt: return (s32)udtItem::AmmoBelt;
			case idItem90p::ItemScout: return (s32)udtItem::ItemScout;
			case idItem90p::ItemGuard: return (s32)udtItem::ItemGuard;
			case idItem90p::ItemDoubler: return (s32)udtItem::ItemDoubler;
			case idItem90p::ItemAmmoregen: return (s32)udtItem::ItemAmmoRegen;
			case idItem90p::TeamCTFNeutralflag: return (s32)udtItem::FlagNeutral;
			case idItem90p::ItemRedcube: return (s32)udtItem::ItemRedCube;
			case idItem90p::ItemBluecube: return (s32)udtItem::ItemBlueCube;
			case idItem90p::WeaponNailgun: return (s32)udtItem::WeaponNailgun;
			case idItem90p::WeaponProxLauncher: return (s32)udtItem::WeaponProxLauncher;
			case idItem90p::WeaponChaingun: return (s32)udtItem::WeaponChaingun;
			case idItem90p::ItemSpawnArmor: return (s32)udtItem::ItemSpawnArmor;
			case idItem90p::WeaponHMG: return (s32)udtItem::WeaponHMG;
			case idItem90p::AmmoHMG: return (s32)udtItem::AmmoHMG;
			case idItem90p::AmmoPack: return (s32)udtItem::AmmoPack;
			case idItem90p::ItemKeySilver: return (s32)udtItem::ItemKeySilver;
			case idItem90p::ItemKeyGold: return (s32)udtItem::ItemKeyGold;
			case idItem90p::ItemKeyMaster: return (s32)udtItem::ItemKeyMaster;
			default: return -1;
		}
	}
	else
	{
		switch((idItem68_CPMA::Id)idItem)
		{
			case idItem68_CPMA::ItemArmorShard: return (s32)udtItem::ItemArmorShard;
			case idItem68_CPMA::ItemArmorCombat: return (s32)udtItem::ItemArmorCombat;
			case idItem68_CPMA::ItemArmorBody: return (s32)udtItem::ItemArmorBody;
			case idItem68_CPMA::ItemHealthSmall: return (s32)udtItem::ItemHealthSmall;
			case idItem68_CPMA::ItemHealth: return (s32)udtItem::ItemHealth;
			case idItem68_CPMA::ItemHealthLarge: return (s32)udtItem::ItemHealthLarge;
			case idItem68_CPMA::ItemHealthMega: return (s32)udtItem::ItemHealthMega;
			case idItem68_CPMA::WeaponGauntlet: return (s32)udtItem::WeaponGauntlet;
			case idItem68_CPMA::WeaponShotgun: return (s32)udtItem::WeaponShotgun;
			case idItem68_CPMA::WeaponMachinegun: return (s32)udtItem::WeaponMachinegun;
			case idItem68_CPMA::WeaponGrenadelauncher: return (s32)udtItem::WeaponGrenadeLauncher;
			case idItem68_CPMA::WeaponRocketlauncher: return (s32)udtItem::WeaponRocketLauncher;
			case idItem68_CPMA::WeaponLightning: return (s32)udtItem::WeaponLightningGun;
			case idItem68_CPMA::WeaponRailgun: return (s32)udtItem::WeaponRailgun;
			case idItem68_CPMA::WeaponPlasmagun: return (s32)udtItem::WeaponPlasmaGun;
			case idItem68_CPMA::WeaponBFG: return (s32)udtItem::WeaponBFG;
			case idItem68_CPMA::WeaponGrapplinghook: return (s32)udtItem::WeaponGrapplingHook;
			case idItem68_CPMA::AmmoShells: return (s32)udtItem::AmmoShells;
			case idItem68_CPMA::AmmoBullets: return (s32)udtItem::AmmoBullets;
			case idItem68_CPMA::AmmoGrenades: return (s32)udtItem::AmmoGrenades;
			case idItem68_CPMA::AmmoCells: return (s32)udtItem::AmmoCells;
			case idItem68_CPMA::AmmoLightning: return (s32)udtItem::AmmoLightning;
			case idItem68_CPMA::AmmoRockets: return (s32)udtItem::AmmoRockets;
			case idItem68_CPMA::AmmoSlugs: return (s32)udtItem::AmmoSlugs;
			case idItem68_CPMA::AmmoBFG: return (s32)udtItem::AmmoBFG;
			case idItem68_CPMA::HoldableTeleporter: return (s32)udtItem::HoldableTeleporter;
			case idItem68_CPMA::HoldableMedkit: return (s32)udtItem::HoldableMedkit;
			case idItem68_CPMA::ItemQuad: return (s32)udtItem::ItemQuad;
			case idItem68_CPMA::ItemEnviro: return (s32)udtItem::ItemEnviro;
			case idItem68_CPMA::ItemHaste: return (s32)udtItem::ItemHaste;
			case idItem68_CPMA::ItemInvis: return (s32)udtItem::ItemInvis;
			case idItem68_CPMA::ItemRegen: return (s32)udtItem::ItemRegen;
			case idItem68_CPMA::ItemFlight: return (s32)udtItem::ItemFlight;
			case idItem68_CPMA::TeamCTFRedflag: return (s32)udtItem::FlagRed;
			case idItem68_CPMA::TeamCTFBlueflag: return (s32)udtItem::FlagBlue;
			case idItem68_CPMA::ItemArmorJacket: return (s32)udtItem::ItemArmorJacket;
			case idItem68_CPMA::ItemBackpack: return (s32)udtItem::ItemBackpack;
			case idItem68_CPMA::TeamCTFNeutralflag: return (s32)udtItem::FlagNeutral;
			default: return -1;
		}
	}
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

uptr ComputeReservedByteCount(uptr smallByteCount, uptr bigByteCount, u32 demoCountThreshold, u32 demoCount)
{
	if(demoCount < demoCountThreshold)
	{
		return bigByteCount * (uptr)demoCount;
	}

	const uptr byteCount1 = bigByteCount * (uptr)demoCountThreshold;
	const uptr byteCount2 = smallByteCount * (uptr)demoCount;
	const uptr byteCount = udt_max(byteCount1, byteCount2);

	return byteCount;
}

bool IsTeamMode(udtGameType::Id gameType)
{
	const u8* gameTypeFlags = NULL;
	u32 gameTypeCount = 0;
	if(udtGetByteArray(udtByteArray::GameTypeFlags, &gameTypeFlags, &gameTypeCount) != udtErrorCode::None ||
	   (u32)gameType >= gameTypeCount)
	{
		return false;
	}

	return (gameTypeFlags[gameType] & (u8)udtGameTypeFlags::Team) != 0;
}

bool IsRoundBasedMode(udtGameType::Id gameType)
{
	const u8* gameTypeFlags = NULL;
	u32 gameTypeCount = 0;
	if(udtGetByteArray(udtByteArray::GameTypeFlags, &gameTypeFlags, &gameTypeCount) != udtErrorCode::None ||
	   (u32)gameType >= gameTypeCount)
	{
		return false;
	}

	return (gameTypeFlags[gameType] & (u8)udtGameTypeFlags::RoundBased) != 0;
}

void PerfStatsInit(u64* perfStats)
{
	memset(perfStats, 0, sizeof(u64) * (size_t)udtPerfStatsField::Count);
}

void PerfStatsAddCurrentThread(u64* perfStats, u64 totalDemoByteCount)
{
	udtVMLinearAllocator::Stats allocStats;
	udtVMLinearAllocator::GetThreadStats(allocStats);
	perfStats[udtPerfStatsField::MemoryReserved] += (u64)allocStats.ReservedByteCount;
	perfStats[udtPerfStatsField::MemoryCommitted] += (u64)allocStats.CommittedByteCount;
	perfStats[udtPerfStatsField::MemoryUsed] += (u64)allocStats.UsedByteCount;
	perfStats[udtPerfStatsField::AllocatorCount] += allocStats.AllocatorCount;
	perfStats[udtPerfStatsField::DataProcessed] += totalDemoByteCount;
	perfStats[udtPerfStatsField::ResizeCount] += (u64)allocStats.ResizeCount;
}

void PerfStatsFinalize(u64* perfStats, u32 threadCount, u64 durationMs)
{
	const u64 extraByteCount = (u64)sizeof(udtParserContext) * (u64)threadCount;
	perfStats[udtPerfStatsField::MemoryReserved] += extraByteCount;
	perfStats[udtPerfStatsField::MemoryCommitted] += extraByteCount;
	perfStats[udtPerfStatsField::MemoryUsed] += extraByteCount;
	perfStats[udtPerfStatsField::Duration] = durationMs;
	perfStats[udtPerfStatsField::ThreadCount] = (u64)threadCount;
	perfStats[udtPerfStatsField::MemoryEfficiency] = 0;

	if(durationMs > 0)
	{
		perfStats[udtPerfStatsField::DataThroughput] = (1000 * perfStats[udtPerfStatsField::DataProcessed]) / durationMs;
	}
	else
	{
		perfStats[udtPerfStatsField::DataThroughput] = 0;
	}

	if(perfStats[udtPerfStatsField::MemoryCommitted] > 0)
	{
		perfStats[udtPerfStatsField::MemoryEfficiency] = (1000 * perfStats[udtPerfStatsField::MemoryUsed]) / perfStats[udtPerfStatsField::MemoryCommitted];
	}
	else
	{
		perfStats[udtPerfStatsField::MemoryEfficiency] = 0;
	}
}

void WriteStringToApiStruct(u32& offset, const udtString& string)
{
	u32* const offsetAndLength = &offset;
	offsetAndLength[0] = string.GetOffset();
	offsetAndLength[1] = string.GetLength();
}

void WriteNullStringToApiStruct(u32& offset)
{
	u32* const offsetAndLength = &offset;
	offsetAndLength[0] = U32_MAX;
	offsetAndLength[1] = 0;
}

namespace idEntityEvent
{
	s32 Obituary(udtProtocol::Id protocol)
	{
		switch(protocol)
		{
			case udtProtocol::Dm3:  return EV_OBITUARY_3;
			case udtProtocol::Dm48: return EV_OBITUARY_48;
			case udtProtocol::Dm66:
			case udtProtocol::Dm67:
			case udtProtocol::Dm68: return EV_OBITUARY_68;
			case udtProtocol::Dm73:
			case udtProtocol::Dm90:
			case udtProtocol::Dm91: return EV_OBITUARY_73p;
			default: return -1;
		}
	}

	s32 WeaponFired(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm68) ? (s32)EV_FIRE_WEAPON_68 : (s32)EV_FIRE_WEAPON_73p;
	}

	s32 ItemPickup(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm68) ? (s32)EV_ITEM_PICKUP_68 : (s32)EV_ITEM_PICKUP_73p;
	}

	s32 GlobalItemPickup(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm68) ? (s32)EV_GLOBAL_ITEM_PICKUP_68 : (s32)EV_GLOBAL_ITEM_PICKUP_73p;
	}

	s32 GlobalSound(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm68) ? (s32)EV_GLOBAL_SOUND_68 : (s32)EV_GLOBAL_SOUND_73p;
	}

	s32 GlobalTeamSound(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm68) ? (s32)EV_GLOBAL_TEAM_SOUND_68 : (s32)EV_GLOBAL_TEAM_SOUND_73p;
	}

	s32 QL_Overtime(udtProtocol::Id protocol)
	{
		return (protocol >= udtProtocol::Dm73) ? EV_OVERTIME_73p : -1;
	}

	s32 QL_GameOver(udtProtocol::Id protocol)
	{
		return (protocol >= udtProtocol::Dm73) ? EV_GAMEOVER_73p : -1;
	}
};

namespace idEntityType
{
	s32 General(udtProtocol::Id)
	{
		return ET_GENERAL;
	}

	s32 Player(udtProtocol::Id)
	{
		return ET_PLAYER;
	}

	s32 Item(udtProtocol::Id)
	{
		return ET_ITEM;
	}

	s32 Missile(udtProtocol::Id)
	{
		return ET_MISSILE;
	}

	s32 Mover(udtProtocol::Id)
	{
		return ET_MOVER;
	}

	s32 Beam(udtProtocol::Id)
	{
		return ET_BEAM;
	}

	s32 Portal(udtProtocol::Id)
	{
		return ET_PORTAL;
	}

	s32 Speaker(udtProtocol::Id)
	{
		return ET_SPEAKER;
	}

	s32 PushTrigger(udtProtocol::Id)
	{
		return ET_PUSH_TRIGGER;
	}

	s32 TeleportTrigger(udtProtocol::Id)
	{
		return ET_TELEPORT_TRIGGER;
	}

	s32 Invisible(udtProtocol::Id)
	{
		return ET_INVISIBLE;
	}

	s32 Grapple(udtProtocol::Id)
	{
		return ET_GRAPPLE;
	}

	s32 Team(udtProtocol::Id protocol)
	{
		return (protocol >= udtProtocol::Dm48) ? ET_TEAM : -1;
	}

	s32 Event(udtProtocol::Id protocol)
	{
		return (protocol == udtProtocol::Dm3) ? (s32)ET_EVENTS_3 : (s32)ET_EVENTS;
	}
}

namespace idConfigStringIndex
{
	s32 FirstPlayer(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm68) ? CS_PLAYERS_68 : CS_PLAYERS_73p;
	}

	s32 Intermission(udtProtocol::Id protocol)
	{
		switch(protocol)
		{
			case udtProtocol::Dm3:  return CS_INTERMISSION_3;
			case udtProtocol::Dm48:
			case udtProtocol::Dm66:
			case udtProtocol::Dm67:
			case udtProtocol::Dm68: return CS_INTERMISSION_48_68;
			case udtProtocol::Dm73:
			case udtProtocol::Dm90:
			case udtProtocol::Dm91: return CS_INTERMISSION_73p;
			default: return -1;
		}
	}

	s32 LevelStartTime(udtProtocol::Id protocol)
	{
		switch(protocol)
		{
			case udtProtocol::Dm3:  return CS_LEVEL_START_TIME_3;
			case udtProtocol::Dm48:
			case udtProtocol::Dm66:
			case udtProtocol::Dm67:
			case udtProtocol::Dm68: return CS_LEVEL_START_TIME_48_68;
			case udtProtocol::Dm73:
			case udtProtocol::Dm90:
			case udtProtocol::Dm91: return CS_LEVEL_START_TIME_73p;
			default: return -1;
		}
	}

	s32 WarmUpEndTime(udtProtocol::Id)
	{
		return CS_WARMUP;
	}

	s32 FirstPlacePlayerName(udtProtocol::Id protocol)
	{
		return protocol >= udtProtocol::Dm91 ? CS_SCORES1PLAYER_91 : -1;
	}

	s32 SecondPlacePlayerName(udtProtocol::Id protocol)
	{
		return protocol >= udtProtocol::Dm91 ? CS_SCORES2PLAYER_91 : -1;
	}

	s32 PauseStart(udtProtocol::Id protocol)
	{
		return protocol >= udtProtocol::Dm73 ? CS_PAUSE_START_73p : -1;
	}

	s32 PauseEnd(udtProtocol::Id protocol)
	{
		return protocol >= udtProtocol::Dm73 ? CS_PAUSE_COUNTDOWN_73p : -1;
	}

	s32 FlagStatus(udtProtocol::Id protocol)
	{
		if(protocol == udtProtocol::Dm3)
		{
			return CS_FLAGSTATUS_3;
		}

		return protocol >= udtProtocol::Dm73 ? CS_FLAGSTATUS_73p : CS_FLAGSTATUS_48_68;
	}

	s32 ServerInfo(udtProtocol::Id)
	{
		return CS_SERVERINFO;
	}

	s32 SystemInfo(udtProtocol::Id)
	{
		return CS_SYSTEMINFO;
	}

	s32 Scores1(udtProtocol::Id)
	{
		return CS_SCORES1;
	}

	s32 Scores2(udtProtocol::Id)
	{
		return CS_SCORES2;
	}

	s32 VoteTime(udtProtocol::Id)
	{
		return CS_VOTE_TIME;
	}

	s32 VoteString(udtProtocol::Id)
	{
		return CS_VOTE_STRING;
	}

	s32 VoteYes(udtProtocol::Id)
	{
		return CS_VOTE_YES;
	}

	s32 VoteNo(udtProtocol::Id)
	{
		return CS_VOTE_NO;
	}

	s32 TeamVoteTime(udtProtocol::Id protocol)
	{
		return (protocol >= udtProtocol::Dm48 && protocol <= udtProtocol::Dm68) ? CS_TEAMVOTE_TIME_48_68 : -1;
	}

	s32 TeamVoteString(udtProtocol::Id protocol)
	{
		return (protocol >= udtProtocol::Dm48 && protocol <= udtProtocol::Dm68) ? CS_TEAMVOTE_STRING_48_68 : -1;
	}

	s32 TeamVoteYes(udtProtocol::Id protocol)
	{
		return (protocol >= udtProtocol::Dm48 && protocol <= udtProtocol::Dm68) ? CS_TEAMVOTE_YES_48_68 : -1;
	}

	s32 TeamVoteNo(udtProtocol::Id protocol)
	{
		return (protocol >= udtProtocol::Dm48 && protocol <= udtProtocol::Dm68) ? CS_TEAMVOTE_NO_48_68 : -1;
	}

	s32 GameVersion(udtProtocol::Id protocol)
	{
		switch(protocol)
		{
			case udtProtocol::Dm3:  return CS_GAME_VERSION_3;
			case udtProtocol::Dm48:
			case udtProtocol::Dm66:
			case udtProtocol::Dm67:
			case udtProtocol::Dm68: return CS_GAME_VERSION_48_68;
			case udtProtocol::Dm73:
			case udtProtocol::Dm90:
			case udtProtocol::Dm91: return CS_GAME_VERSION_73p;
			default: return -1;
		}
	}

	s32 ItemFlags(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm68) ? CS_ITEMS_68 : CS_ITEMS_73p;
	}

	s32 QL_TimeoutStartTime(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm90) ? CS_TIMEOUT_BEGIN_TIME_73p : -1;
	}

	s32 QL_TimeoutEndTime(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm90) ? CS_TIMEOUT_END_TIME_73p : -1;
	}

	s32 QL_RedTeamTimeoutsLeft(udtProtocol::Id protocol)
	{
		return (protocol == udtProtocol::Dm91) ? CS_TIMEOUTS_RED_91 : -1;
	}

	s32 QL_BlueTeamTimeoutsLeft(udtProtocol::Id protocol)
	{
		return (protocol == udtProtocol::Dm91) ? CS_TIMEOUTS_BLUE_91 : -1;
	}

	s32 QL_ReadTeamClanName(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm90) ? CS_RED_TEAM_CLAN_NAME_73p : -1;
	}

	s32 QL_BlueTeamClanName(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm90) ? CS_BLUE_TEAM_CLAN_NAME_73p : -1;
	}

	s32 QL_RedTeamClanTag(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm90) ? CS_RED_TEAM_CLAN_TAG_73p : -1;
	}

	s32 QL_BlueTeamClanTag(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm90) ? CS_BLUE_TEAM_CLAN_TAG_73p : -1;
	}

	s32 CPMA_GameInfo(udtProtocol::Id)
	{
		return CS_CPMA_GAME_INFO;
	}

	s32 CPMA_RoundInfo(udtProtocol::Id)
	{
		return CS_CPMA_ROUND_INFO;
	}

	s32 OSP_GamePlay(udtProtocol::Id)
	{
		return CS_OSP_GAMEPLAY;
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

	s32 QuadDamage(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm90) ? (s32)PW_QUAD : (s32)PW_QUAD_91;
	}

	s32 BattleSuit(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm90) ? (s32)PW_BATTLESUIT : (s32)PW_BATTLESUIT_91;
	}

	s32 Haste(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm90) ? (s32)PW_HASTE : (s32)PW_HASTE_91;
	}

	s32 Invisibility(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm90) ? (s32)PW_INVIS : (s32)PW_INVIS_91;
	}

	s32 Regeneration(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm90) ? (s32)PW_REGEN : (s32)PW_REGEN_91;
	}

	s32 Flight(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm90) ? (s32)PW_FLIGHT : (s32)PW_FLIGHT_91;
	}

	s32 Scout(udtProtocol::Id protocol)
	{
		if(protocol == udtProtocol::Dm3) return -1;

		return (protocol <= udtProtocol::Dm90) ? (s32)PW_SCOUT : (s32)NOTPW_SCOUT_91;
	}

	s32 Guard(udtProtocol::Id protocol)
	{
		if(protocol == udtProtocol::Dm3) return -1;

		return (protocol <= udtProtocol::Dm90) ? (s32)PW_GUARD : (s32)NOTPW_GUARD_91;
	}

	s32 Doubler(udtProtocol::Id protocol)
	{
		if(protocol == udtProtocol::Dm3) return -1;

		return (protocol <= udtProtocol::Dm90) ? (s32)PW_DOUBLER : (s32)NOTPW_DOUBLER_91;
	}

	s32 ArmorRegeneration(udtProtocol::Id protocol)
	{
		if(protocol <= udtProtocol::Dm3) return -1;

		return (protocol <= udtProtocol::Dm90) ? (s32)PW_AMMOREGEN : (s32)NOTPW_ARMORREGEN_91;
	}

	s32 Invulnerability(udtProtocol::Id protocol)
	{
		if(protocol == udtProtocol::Dm3) return -1;

		return (protocol <= udtProtocol::Dm90) ? (s32)PW_INVULNERABILITY : (s32)PW_INVULNERABILITY_91;
	}
}

namespace idPersStatsIndex
{
	s32 FlagCaptures(udtProtocol::Id protocol)
	{
		if(protocol == udtProtocol::Dm3) return -1; // @NOTE: dm3 doesn't have a flag captures slot.

		return (protocol <= udtProtocol::Dm68) ? (s32)PERS_CAPTURES_68 : (s32)PERS_CAPTURES_73p;
	}

	s32 Score(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm68) ? (s32)PERS_SCORE_68 : (s32)PERS_SCORE_73p;
	}

	s32 Hits(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm68) ? (s32)PERS_HITS_68 : (s32)PERS_HITS_73p;
	}

	s32 Rank(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm68) ? (s32)PERS_RANK_68 : (s32)PERS_RANK_73p;
	}

	s32 Team(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm68) ? (s32)PERS_TEAM_68 : (s32)PERS_TEAM_73p;
	}

	s32 SpawnCount(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm68) ? (s32)PERS_SPAWN_COUNT_68 : (s32)PERS_SPAWN_COUNT_73p;
	}

	s32 Deaths(udtProtocol::Id protocol)
	{
		if(protocol == udtProtocol::Dm3) return PERS_KILLED_3;

		return (protocol <= udtProtocol::Dm68) ? (s32)PERS_KILLED_68 : (s32)PERS_KILLED_73p;
	}

	s32 LastAttacker(udtProtocol::Id protocol)
	{
		if(protocol == udtProtocol::Dm3) return PERS_ATTACKER_3;

		return (protocol <= udtProtocol::Dm68) ? (s32)PERS_ATTACKER_68 : (s32)PERS_ATTACKER_73p;
	}

	s32 DamageGiven(udtProtocol::Id protocol)
	{
		if(protocol == udtProtocol::Dm3) return PERS_HITS_3;

		return (protocol <= udtProtocol::Dm68) ? (s32)PERS_HITS_68 : (s32)PERS_HITS_73p;
	}

	s32 LastTargetHealthAndArmor(udtProtocol::Id protocol)
	{
		if(protocol == udtProtocol::Dm3) return -1;

		return (protocol <= udtProtocol::Dm68) ? (s32)PERS_ATTACKEE_ARMOR_68 : (s32)PERS_ATTACKEE_ARMOR_73p;
	}

	s32 Impressives(udtProtocol::Id protocol)
	{
		if(protocol == udtProtocol::Dm3) return PERS_IMPRESSIVE_COUNT_3;

		return (protocol <= udtProtocol::Dm68) ? (s32)PERS_IMPRESSIVE_COUNT_68 : (s32)PERS_IMPRESSIVE_COUNT_73p;
	}

	s32 Excellents(udtProtocol::Id protocol)
	{
		if(protocol == udtProtocol::Dm3) return PERS_EXCELLENT_COUNT_3;

		return (protocol <= udtProtocol::Dm68) ? (s32)PERS_EXCELLENT_COUNT_68 : (s32)PERS_EXCELLENT_COUNT_73p;
	}

	s32 Defends(udtProtocol::Id protocol)
	{
		if(protocol == udtProtocol::Dm3) return -1;

		return (protocol <= udtProtocol::Dm68) ? (s32)PERS_DEFEND_COUNT_68 : (s32)PERS_DEFEND_COUNT_73p;
	}

	s32 Assists(udtProtocol::Id protocol)
	{
		if(protocol == udtProtocol::Dm3) return -1;

		return (protocol <= udtProtocol::Dm68) ? (s32)PERS_ASSIST_COUNT_68 : (s32)PERS_ASSIST_COUNT_73p;
	}

	s32 Humiliations(udtProtocol::Id protocol)
	{
		if(protocol == udtProtocol::Dm3) return PERS_GAUNTLET_FRAG_COUNT_3;

		return (protocol <= udtProtocol::Dm68) ? (s32)PERS_GAUNTLET_FRAG_COUNT_68 : (s32)PERS_GAUNTLET_FRAG_COUNT_73p;
	}
}

namespace idEntityStateFlag
{
	s32 Dead(udtProtocol::Id)
	{
		return EF_DEAD;
	}

	s32 TeleportBit(udtProtocol::Id)
	{
		return EF_TELEPORT_BIT;
	}

	s32 AwardExcellent(udtProtocol::Id)
	{
		return EF_AWARD_EXCELLENT;
	}

	s32 PlayerEvent(udtProtocol::Id protocol)
	{
		return (protocol >= udtProtocol::Dm66) ? EF_PLAYER_EVENT_66p : 0;
	}

	s32 AwardHumiliation(udtProtocol::Id)
	{
		return EF_AWARD_GAUNTLET;
	}

	s32 NoDraw(udtProtocol::Id)
	{
		return EF_NODRAW;
	}

	s32 Firing(udtProtocol::Id)
	{
		return EF_FIRING;
	}

	s32 AwardCapture(udtProtocol::Id)
	{
		return EF_AWARD_CAP;
	}

	s32 Chatting(udtProtocol::Id)
	{
		return EF_TALK;
	}

	s32 ConnectionInterrupted(udtProtocol::Id)
	{
		return EF_CONNECTION;
	}

	s32 HasVoted(udtProtocol::Id protocol)
	{
		return (protocol <= udtProtocol::Dm90) ? EF_VOTED_3_90 : 0;
	}

	s32 AwardImpressive(udtProtocol::Id)
	{
		return EF_AWARD_IMPRESSIVE;
	}

	s32 AwardDefense(udtProtocol::Id protocol)
	{
		return (protocol >= udtProtocol::Dm48) ? EF_AWARD_DEFEND_48p : 0;
	}

	s32 AwardAssist(udtProtocol::Id protocol)
	{
		return (protocol >= udtProtocol::Dm48) ? EF_AWARD_ASSIST_48p : 0;
	}

	s32 AwardDenied(udtProtocol::Id protocol)
	{
		return (protocol >= udtProtocol::Dm48) ? EF_AWARD_DENIED_48p : 0;
	}

	s32 HasTeamVoted(udtProtocol::Id protocol)
	{
		return (protocol >= udtProtocol::Dm48 && protocol <= udtProtocol::Dm90) ? EF_TEAMVOTED_48_90 : 0;
	}
}
