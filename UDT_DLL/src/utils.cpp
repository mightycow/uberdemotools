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


#define ITEM(Enum, Desc, Bit) Desc,
static const char* MeansOfDeathNames[udtMeanOfDeath::Count + 1]
{
	UDT_MEAN_OF_DEATH_LIST(ITEM)
	"unknown"
};
#undef ITEM


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
	const s32 obituaryEvtId = GetIdNumber(udtMagicNumberType::EntityEvent, udtEntityEvent::Obituary, protocol);
	const s32 eventTypeId = GetIdNumber(udtMagicNumberType::EntityType, udtEntityType::Event, protocol);
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

	u32 udtMod;
	if(!GetUDTNumber(udtMod, udtMagicNumberType::MeanOfDeath, entity.eventParm, protocol))
	{
		return false;
	}

	info.AttackerIndex = attackerIdx;
	info.TargetIndex = targetIdx;
	info.MeanOfDeath = udtMod;

	return true;
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

bool IsTeamMode(udtGameType::Id gameType)
{
	const u8* gameTypeFlags = NULL;
	u32 gameTypeCount = 0;
	if(udtGetByteArray(udtByteArray::GameTypeFlags, &gameTypeFlags, &gameTypeCount) != udtErrorCode::None ||
	   (u32)gameType >= gameTypeCount)
	{
		return false;
	}

	return (gameTypeFlags[gameType] & (u8)udtGameTypeMask::Team) != 0;
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

	return (gameTypeFlags[gameType] & (u8)udtGameTypeMask::RoundBased) != 0;
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

void PerfStatsFinalize(u64* perfStats, u32 threadCount, u64 durationUs)
{
	const u64 extraByteCount = (u64)sizeof(udtParserContext) * (u64)threadCount;
	perfStats[udtPerfStatsField::MemoryReserved] += extraByteCount;
	perfStats[udtPerfStatsField::MemoryCommitted] += extraByteCount;
	perfStats[udtPerfStatsField::MemoryUsed] += extraByteCount;
	perfStats[udtPerfStatsField::Duration] = durationUs;
	perfStats[udtPerfStatsField::ThreadCount] = (u64)threadCount;
	perfStats[udtPerfStatsField::MemoryEfficiency] = 0;
	perfStats[udtPerfStatsField::DataThroughput] = (durationUs > 0) ? 
		((1000000 * perfStats[udtPerfStatsField::DataProcessed]) / durationUs) : 0;
	perfStats[udtPerfStatsField::MemoryEfficiency] = (perfStats[udtPerfStatsField::MemoryCommitted] > 0) ?
		((1000 * perfStats[udtPerfStatsField::MemoryUsed]) / perfStats[udtPerfStatsField::MemoryCommitted]) : 0;
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
	offsetAndLength[0] = UDT_U32_MAX;
	offsetAndLength[1] = 0;
}

void PlayerStateToEntityState(idEntityStateBase& es, s32& lastEventSequence, const idPlayerStateBase& ps, bool extrapolate, s32 serverTimeMs, udtProtocol::Id protocol)
{
	s32 healthStatIdx = GetIdNumber(udtMagicNumberType::LifeStatsIndex, udtLifeStatsIndex::Health, protocol, udtMod::None);
	if(healthStatIdx < 0 || healthStatIdx > 16)
	{
		healthStatIdx = 0;
	}

	const bool isPlayerInvisible = 
		ps.pm_type == GetIdNumber(udtMagicNumberType::PlayerMovementType, udtPlayerMovementType::Intermission, protocol, udtMod::None) ||
		ps.pm_type == GetIdNumber(udtMagicNumberType::PlayerMovementType, udtPlayerMovementType::SPIntermission, protocol, udtMod::None) ||
		ps.stats[healthStatIdx] <= GIB_HEALTH;
	es.eType = GetIdNumber(udtMagicNumberType::EntityType, isPlayerInvisible ? udtEntityType::Invisible : udtEntityType::Player, protocol);
	es.number = ps.clientNum;

	Float3::Copy(es.pos.trBase, ps.origin);
	Float3::Copy(es.pos.trDelta, ps.velocity); // set the trDelta for flag direction
	if(extrapolate)
	{
		es.pos.trType = ID_TR_LINEAR_STOP;
		es.pos.trTime = serverTimeMs; // set the time for linear prediction
		es.pos.trDuration = 50; // set maximum extrapolation time: 1000 / sv_fps (default = 20)
	}
	else
	{
		es.pos.trType = ID_TR_INTERPOLATE;
	}

	es.apos.trType = ID_TR_INTERPOLATE;
	Float3::Copy(es.apos.trBase, ps.viewangles);
	es.angles2[YAW] = (f32)ps.movementDir;
	es.legsAnim = ps.legsAnim;
	es.torsoAnim = ps.torsoAnim;
	es.clientNum = ps.clientNum;

	const s32 entityFlagDead = GetIdEntityStateFlagMask(udtEntityFlag::Dead, protocol);
	es.eFlags = ps.eFlags;
	if(ps.stats[healthStatIdx] <= 0)
	{
		es.eFlags |= entityFlagDead;
	}
	else
	{
		es.eFlags &= ~entityFlagDead;
	}

	if(ps.eventSequence != lastEventSequence)
	{
		const s32 seq = (ps.eventSequence & 1) ^ 1;
		es.event = ps.events[seq];
		es.eventParm = ps.eventParms[seq];
		lastEventSequence = ps.eventSequence;
	}
	else
	{
		es.event = 0;
		es.eventParm = 0;
	}

	es.weapon = ps.weapon;
	es.groundEntityNum = ps.groundEntityNum;

	es.powerups = 0;
	for(s32 i = 0; i < ID_MAX_PS_POWERUPS; i++)
	{
		if(ps.powerups[i])
		{
			es.powerups |= 1 << i;
		}
	}

	es.loopSound = ps.loopSound;
	es.generic1 = ps.generic1;
}
