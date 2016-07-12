#include "plug_in_chat.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"


/*
Q3/OSP chat format ("chat"):
name: message

QL chat format ("chat"):
playerIndex [clan] name: message
where playerIndex is 0-based and is formatted as 2 digits (printf-style: %02d)
and clan is optional

Q3/OSP team chat format ("tchat"):
(name) [(location)]: message
where location is the offset from the first location config string's index
not sure if the location is optional in Q3/OSP

QL team chat format ("tchat"):
playerIndex ([clan] name) [(location)]: message
where playerIndex is 0-based and is formatted as 2 digits (printf-style: %02d)
and location is the offset from the first location config string's index
and clan is optional
the location isn't stored in some QL demos I have

CPMA team chat format ("mm2"):
playerIndex locationIndex message

@NOTE: In Quake 3, player names can contain and end with spaces.

@NOTE: In the new QL, in both "chat" and "tchat", the clan and name separator 
can sometimes be a '.' instead of ' '...
*/


udtParserPlugInChat::udtParserPlugInChat()
{
	_gameStateIndex = -1;
}

udtParserPlugInChat::~udtParserPlugInChat()
{
}

void udtParserPlugInChat::InitAllocators(u32)
{
}

void udtParserPlugInChat::CopyBuffersStruct(void* buffersStruct) const
{
	*(udtParseDataChatBuffers*)buffersStruct = _buffers;
}

void udtParserPlugInChat::UpdateBufferStruct()
{
	_buffers.ChatMessageCount = ChatEvents.GetSize();
	_buffers.ChatMessageRanges = BufferRanges.GetStartAddress();
	_buffers.ChatMessages = ChatEvents.GetStartAddress();
	_buffers.StringBuffer = _stringAllocator.GetStartAddress();
	_buffers.StringBufferSize = (u32)_stringAllocator.GetCurrentByteCount();
}

u32 udtParserPlugInChat::GetItemCount() const
{
	return ChatEvents.GetSize();
}

void udtParserPlugInChat::StartDemoAnalysis()
{
	_gameStateIndex = -1;
}

void udtParserPlugInChat::ProcessCommandMessage(const udtCommandCallbackArg& /*info*/, udtBaseParser& parser)
{
	const idTokenizer& tokenizer = parser.GetTokenizer();
	if(tokenizer.GetArgCount() < 2)
	{
		return;
	}

	const udtString command = tokenizer.GetArg(0);
	if(parser._inProtocol <= udtProtocol::Dm68 &&
	   tokenizer.GetArgCount() == 3 &&
	   udtString::Equals(command, "cs"))
	{
		s32 csIndex = -1;
		const s32 firstPlayerCsIndex = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, parser._inProtocol);
		if(StringParseInt(csIndex, tokenizer.GetArgString(1)) &&
		   csIndex >= firstPlayerCsIndex &&
		   csIndex < firstPlayerCsIndex + 64)
		{
			ProcessPlayerConfigString(parser, tokenizer.GetArg(2), csIndex - firstPlayerCsIndex);
		}
	}
	else if(tokenizer.GetArgCount() == 2 && 
			udtString::Equals(command, "chat"))
	{
		ProcessChatCommand(parser);
	}
	else if(tokenizer.GetArgCount() == 2 && 
			udtString::Equals(command, "tchat"))
	{
		ProcessTeamChatCommand(parser);
	}
	else if(tokenizer.GetArgCount() == 4 && 
			udtString::Equals(command, "mm2"))
	{
		ProcessCPMATeamChatCommand(parser);
	}
}

void udtParserPlugInChat::ProcessGamestateMessage(const udtGamestateCallbackArg&, udtBaseParser& parser)
{
	++_gameStateIndex;

	if(parser._inProtocol <= udtProtocol::Dm68)
	{
		const s32 firstPlayerCsIndex = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, parser._inProtocol);
		for(s32 i = 0; i < 64; ++i)
		{
			ProcessPlayerConfigString(parser, parser.GetConfigString(firstPlayerCsIndex + i), i);
		}
	}
}

void udtParserPlugInChat::ProcessPlayerConfigString(udtBaseParser& parser, const udtString& cs, s32 playerIndex)
{
	if(udtString::IsNullOrEmpty(cs))
	{
		_cleanPlayerNames[playerIndex] = udtString::NewEmptyConstant();
		return;
	}

	udtVMScopedStackAllocator allocScope(*TempAllocator);

	udtString playerName;
	if(!ParseConfigStringValueString(playerName, *TempAllocator, "n", cs.GetPtr()))
	{
		_cleanPlayerNames[playerIndex] = udtString::NewEmptyConstant();
		return;
	}

	_cleanPlayerNames[playerIndex] = udtString::NewCleanCloneFromRef(_stringAllocator, parser._inProtocol, playerName);
}

void udtParserPlugInChat::ProcessChatCommand(udtBaseParser& parser)
{
	udtVMScopedStackAllocator allocScope(*TempAllocator);

	udtParseDataChat chatEvent;
	InitChatEvent(chatEvent, parser._inServerTime);

	const idTokenizer& tokenizer = parser.GetTokenizer();
	const udtString originalCommand = udtString::NewClone(_stringAllocator, tokenizer.GetOriginalCommand());
	const udtString cleanCommand = udtString::NewCleanCloneFromRef(_stringAllocator, parser._inProtocol, originalCommand);
	WriteStringToApiStruct(chatEvent.Strings[0].OriginalCommand, originalCommand);
	WriteStringToApiStruct(chatEvent.Strings[1].OriginalCommand, cleanCommand);

	udtString argument1[2];
	argument1[0] = udtString::NewCloneFromRef(*TempAllocator, tokenizer.GetArg(1));
	argument1[1] = udtString::NewCleanCloneFromRef(*TempAllocator, parser._inProtocol, argument1[0]);
	udtString::RemoveEmCharacter(argument1[0]);
	udtString::RemoveEmCharacter(argument1[1]);

	const bool qlFormat = parser._inProtocol >= udtProtocol::Dm73;
	if(qlFormat)
	{
		ExtractPlayerIndexRelatedData(chatEvent, argument1[1], parser);

		for(u32 i = 0; i < 2; ++i)
		{
			u32 colon;
			if(!udtString::FindFirstCharacterMatch(colon, argument1[i], ':') ||
			   colon + 2 >= argument1[i].GetLength())
			{
				continue;
			}

			WriteStringToApiStruct(chatEvent.Strings[i].Message, udtString::NewSubstringClone(_stringAllocator, argument1[i], colon + 2));
		}
	}
	else
	{
		ProcessQ3GlobalChat(chatEvent, argument1);
	}

	ChatEvents.Add(chatEvent);
}

void udtParserPlugInChat::ProcessTeamChatCommand(udtBaseParser& parser)
{
	udtVMScopedStackAllocator allocScope(*TempAllocator);

	udtParseDataChat chatEvent;
	InitChatEvent(chatEvent, parser._inServerTime);
	chatEvent.TeamMessage = 1;

	const idTokenizer& tokenizer = parser.GetTokenizer();
	const udtString originalCommand = udtString::NewClone(_stringAllocator, tokenizer.GetOriginalCommand());
	udtString cleanedUpCommand = udtString::NewCloneFromRef(_stringAllocator, originalCommand);
	udtString::CleanUp(cleanedUpCommand, parser._inProtocol);
	WriteStringToApiStruct(chatEvent.Strings[0].OriginalCommand, originalCommand);
	WriteStringToApiStruct(chatEvent.Strings[1].OriginalCommand, cleanedUpCommand);

	udtString argument1[2];
	argument1[0] = udtString::NewCloneFromRef(*TempAllocator, tokenizer.GetArg(1));
	argument1[1] = udtString::NewCleanCloneFromRef(*TempAllocator, parser._inProtocol, argument1[0]);
	udtString::RemoveEmCharacter(argument1[0]);
	udtString::RemoveEmCharacter(argument1[1]);

	const bool qlFormat = parser._inProtocol >= udtProtocol::Dm73;
	if(qlFormat)
	{
		ExtractPlayerIndexRelatedData(chatEvent, argument1[1], parser);

		for(u32 i = 0; i < 2; ++i)
		{
			const udtString& arg1 = argument1[i];
			u32 leftParen1, rightParen1, colon;
			if(!udtString::FindFirstCharacterMatch(leftParen1, arg1, '(') ||
			   !udtString::FindFirstCharacterMatch(rightParen1, arg1, ')', leftParen1 + 1) ||
			   !udtString::FindFirstCharacterMatch(colon, arg1, ':', rightParen1 + 1))
			{
				continue;
			}

			u32 leftParen2, rightParen2;
			if(udtString::FindFirstCharacterMatch(leftParen2, arg1, '(', rightParen1 + 1) &&
			   udtString::FindFirstCharacterMatch(rightParen2, arg1, ')', leftParen2 + 1) &&
			   rightParen2 < colon)
			{
				const udtString location = udtString::NewSubstringClone(_stringAllocator, arg1, leftParen2 + 1, rightParen2 - leftParen2 - 1);
				WriteStringToApiStruct(chatEvent.Strings[i].Location, location);
			}

			const udtString message = udtString::NewSubstringClone(_stringAllocator, arg1, colon + 2);
			WriteStringToApiStruct(chatEvent.Strings[i].Message, message);
		}
	}
	else
	{
		for(u32 i = 0; i < 2; ++i)
		{
			const udtString& arg1 = argument1[i];
			u32 leftParen1, rightParen1, colon;
			if(!udtString::FindFirstCharacterMatch(leftParen1, arg1, '(') ||
			   !udtString::FindFirstCharacterMatch(rightParen1, arg1, ')', leftParen1 + 1) ||
			   !udtString::FindFirstCharacterMatch(colon, arg1, ':', rightParen1 + 1))
			{
				continue;
			}

			u32 leftParen2, rightParen2;
			if(udtString::FindFirstCharacterMatch(leftParen2, arg1, '(', rightParen1 + 1) &&
			   udtString::FindFirstCharacterMatch(rightParen2, arg1, ')', leftParen2 + 1) &&
			   rightParen2 < colon)
			{
				const udtString location = udtString::NewSubstringClone(_stringAllocator, arg1, leftParen2 + 1, rightParen2 - leftParen2 - 1);
				WriteStringToApiStruct(chatEvent.Strings[i].Location, location);
			}

			const udtString playerName = udtString::NewSubstringClone(_stringAllocator, arg1, leftParen1 + 1, rightParen1 - leftParen1 - 1);
			const udtString message = udtString::NewSubstringClone(_stringAllocator, arg1, colon + 2);
			WriteStringToApiStruct(chatEvent.Strings[i].PlayerName, playerName);
			WriteStringToApiStruct(chatEvent.Strings[i].Message, message);
		}
	}

	ChatEvents.Add(chatEvent);
}

void udtParserPlugInChat::ProcessCPMATeamChatCommand(udtBaseParser& parser)
{
	s32 clientNumber = -1;
	const idTokenizer& tokenizer = parser.GetTokenizer();
	if(!StringParseInt(clientNumber, tokenizer.GetArgString(1)) ||
	   clientNumber < 0 ||
	   clientNumber >= 64)
	{
		return;
	}

	udtParseDataChat chatEvent;
	InitChatEvent(chatEvent, parser._inServerTime);
	chatEvent.PlayerIndex = clientNumber;
	chatEvent.TeamMessage = 1;

	const udtProtocol::Id protocol = parser._inProtocol;
	
	s32 locationIdx = -1;
	if(StringParseInt(locationIdx, tokenizer.GetArgString(2)) &&
	   locationIdx >= 0 &&
	   locationIdx < MAX_LOCATIONS)
	{
		const udtString& cs = parser.GetConfigString(608 + locationIdx);
		if(!udtString::IsNull(cs))
		{
			const udtString location = udtString::NewCloneFromRef(_stringAllocator, cs);
			const udtString cleanLocation = udtString::NewCleanCloneFromRef(_stringAllocator, protocol, location);
			WriteStringToApiStruct(chatEvent.Strings[0].Location, location);
			WriteStringToApiStruct(chatEvent.Strings[1].Location, cleanLocation);
		}
	}

	const udtString message = udtString::NewCloneFromRef(_stringAllocator, tokenizer.GetArg(3));
	const udtString cleanMessage = udtString::NewCleanCloneFromRef(_stringAllocator, protocol, message);
	WriteStringToApiStruct(chatEvent.Strings[0].Message, message);
	WriteStringToApiStruct(chatEvent.Strings[1].Message, cleanMessage);

	udtString playerName;
	const s32 firstPlayerIndex = GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, protocol);
	if(firstPlayerIndex != -1 &&
	   ParseConfigStringValueString(playerName, _stringAllocator, "n", parser._inConfigStrings[firstPlayerIndex + clientNumber].GetPtr()))
	{
		const udtString cleanPlayerName = udtString::NewCleanCloneFromRef(_stringAllocator, protocol, playerName);
		WriteStringToApiStruct(chatEvent.Strings[0].PlayerName, playerName);
		WriteStringToApiStruct(chatEvent.Strings[1].PlayerName, cleanPlayerName);
	}
	
	const udtString command = udtString::NewClone(_stringAllocator, tokenizer.GetOriginalCommand());
	const udtString cleanCommand = udtString::NewCleanCloneFromRef(_stringAllocator, protocol, command);
	WriteStringToApiStruct(chatEvent.Strings[0].OriginalCommand, command);
	WriteStringToApiStruct(chatEvent.Strings[1].OriginalCommand, cleanCommand);

	ChatEvents.Add(chatEvent);
}

void udtParserPlugInChat::ExtractPlayerIndexRelatedData(udtParseDataChat& chatEvent, const udtString& argument1, udtBaseParser& parser)
{
	// @NOTE: I have seen QL demo data where a chat message from a player kicked for excess flood is received right after he disconnected,
	// which would mean looking up the player's config string will yield a blank name but the chat commands itself has a valid name.

	s32 playerIndex = -1;
	if(!StringParseInt(playerIndex, argument1.GetPtr()) ||
	   playerIndex < 0 ||
	   playerIndex >= 64)
	{
		return;
	}

	chatEvent.PlayerIndex = playerIndex;

	const char* const cs = parser._inConfigStrings[GetIdNumber(udtMagicNumberType::ConfigStringIndex, udtConfigStringIndex::FirstPlayer, parser._inProtocol) + playerIndex].GetPtr();
	udtString clan, player;
	bool hasClan;
	if(!GetClanAndPlayerName(clan, player, hasClan, *TempAllocator, parser._inProtocol, cs))
	{
		return;
	}

	WriteStringToApiStruct(chatEvent.Strings[0].PlayerName, udtString::NewCloneFromRef(_stringAllocator, player));
	WriteStringToApiStruct(chatEvent.Strings[1].PlayerName, udtString::NewCleanCloneFromRef(_stringAllocator, parser._inProtocol, player));
	if(hasClan)
	{
		const udtString raw = udtString::NewCloneFromRef(_stringAllocator, clan);
		const udtString clean = udtString::NewCleanCloneFromRef(_stringAllocator, parser._inProtocol, clan);
		WriteStringToApiStruct(chatEvent.Strings[0].ClanName, raw);
		WriteStringToApiStruct(chatEvent.Strings[1].ClanName, clean);
	}
}

void udtParserPlugInChat::ProcessQ3GlobalChat(udtParseDataChat& chatEvent, const udtString* argument1)
{
	//
	// If we have a message of the form: "A: B: C", there is an ambiguity.
	// We could have name="A" message="B: C" or name="A: B" message="C".
	// To try to resolve that, we'll check against current player names.
	// If not possible: name="A" message="B: C" will be the default.
	//

	const char* firstColonPtr1 = strstr(argument1[1].GetPtr(), ": ");
	u32 firstColon1 = (u32)(firstColonPtr1 - argument1[1].GetPtr());
	if(firstColonPtr1 == NULL || firstColon1 + 2 >= argument1[1].GetLength())
	{
		return;
	}

	const char* colonPtr1 = firstColonPtr1;
	u32 colon1 = firstColon1;
	for(;;)
	{
		const udtString name = udtString::NewSubstringClone(*TempAllocator, argument1[1], 0, colon1);
		if(IsPlayerCleanName(name))
		{
			WriteStringToApiStruct(chatEvent.Strings[1].PlayerName, udtString::NewSubstringClone(_stringAllocator, argument1[1], 0, colon1));
			WriteStringToApiStruct(chatEvent.Strings[1].Message, udtString::NewSubstringClone(_stringAllocator, argument1[1], colon1 + 2));
			break;
		}

		colonPtr1 = strstr(colonPtr1 + 2, ": ");
		colon1 = (u32)(colonPtr1 - argument1[1].GetPtr());
		if(colonPtr1 == NULL || colon1 + 2 >= argument1[1].GetLength())
		{
			// The search ended unsuccessfully, so roll back to the default scenario.
			WriteStringToApiStruct(chatEvent.Strings[1].PlayerName, udtString::NewSubstringClone(_stringAllocator, argument1[1], 0, firstColon1));
			WriteStringToApiStruct(chatEvent.Strings[1].Message, udtString::NewSubstringClone(_stringAllocator, argument1[1], firstColon1 + 2));
			colon1 = firstColon1;
			break;
		}
	}

	//
	// It's probably not guaranteed that the player name ends with ":" and without a trailing color code
	// in the raw message.
	// Therefore, we count the number of colons in the clean message that lead up to the player name 
	// to locate the name's end in the raw message.
	//

	u32 colonCount = 1;
	for(u32 i = 0; i < colon1; ++i)
	{
		if(argument1[1].GetPtr()[i] == ':')
		{
			++colonCount;
		}
	}

	const char* colonPtr0 = argument1[0].GetPtr() - 1;
	for(u32 i = 0; i < colonCount; ++i)
	{
		colonPtr0 = strchr(colonPtr0 + 1, ':');
		if(colonPtr0 == NULL)
		{
			return;
		}
	}

	const u32 colon0 = (u32)(colonPtr0 - argument1[0].GetPtr());
	if(colon0 + 2 >= argument1[0].GetLength())
	{
		return;
	}

	 WriteStringToApiStruct(chatEvent.Strings[0].PlayerName, udtString::NewSubstringClone(_stringAllocator, argument1[0], 0, colon0));
	 WriteStringToApiStruct(chatEvent.Strings[0].Message, udtString::NewSubstringClone(_stringAllocator, argument1[0], colon0 + 2));
}

bool udtParserPlugInChat::IsPlayerCleanName(const udtString& cleanName)
{
	for(s32 i = 0; i < 64; ++i)
	{
		if(udtString::Equals(cleanName, _cleanPlayerNames[i]))
		{
			return true;
		}
	}

	return false;
}

void udtParserPlugInChat::InitChatEvent(udtParseDataChat& chatEvent, s32 serverTimeMs)
{
	memset(&chatEvent, 0, sizeof(chatEvent));
	chatEvent.GameStateIndex = _gameStateIndex;
	chatEvent.ServerTimeMs = serverTimeMs;
	chatEvent.PlayerIndex = -1;
	memset(chatEvent.Strings, -1, sizeof(chatEvent.Strings));
}
