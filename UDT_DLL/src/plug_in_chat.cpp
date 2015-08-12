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


static const char* nullString = "";


static void StringRemoveEmCharacter(udtString& string)
{
	if(string.ReservedBytes == 0)
	{
		return;
	}

	char* d = string.String;
	char* s = string.String;
	while(*s != '\0')
	{
		const char c = *s++;
		if(c == '\x19')
		{
			continue;
		}

		*d++ = c;
	}
	*d = '\0';
	string.Length = (u32)(d - string.String);
}

static void FixQLClanSeparator(udtString& string)
{
	char* s = string.String;
	while(*s != '\0' && *s != ':')
	{
		if(*s == '.')
		{
			*s = ' ';
		}

		++s;
	}
}


udtParserPlugInChat::udtParserPlugInChat()
{
	_gameStateIndex = -1;
}

udtParserPlugInChat::~udtParserPlugInChat()
{
}

void udtParserPlugInChat::InitAllocators(u32 demoCount)
{
	_chatStringAllocator.Init((uptr)(1 << 18) * (uptr)demoCount);
	FinalAllocator.Init((uptr)(1 << 16) * (uptr)demoCount);
	ChatEvents.SetAllocator(FinalAllocator);
}

void udtParserPlugInChat::StartDemoAnalysis()
{
	_gameStateIndex = -1;
}

void udtParserPlugInChat::ProcessCommandMessage(const udtCommandCallbackArg& /*info*/, udtBaseParser& parser)
{
	CommandLineTokenizer& tokenizer = parser._context->Tokenizer;
	if(tokenizer.GetArgCount() < 2)
	{
		return;
	}

	const udtString command = tokenizer.GetArg(0);
	if(tokenizer.GetArgCount() == 2 && udtString::Equals(command, "chat"))
	{
		ProcessChatCommand(parser);
	}
	else if(tokenizer.GetArgCount() == 2 && udtString::Equals(command, "tchat"))
	{
		ProcessTeamChatCommand(parser);
	}
	else if(tokenizer.GetArgCount() == 4 && udtString::Equals(command, "mm2"))
	{
		ProcessCPMATeamChatCommand(parser);
	}
}

void udtParserPlugInChat::ProcessGamestateMessage(const udtGamestateCallbackArg&, udtBaseParser&)
{
	++_gameStateIndex;
}

void udtParserPlugInChat::ProcessChatCommand(udtBaseParser& parser)
{
	udtVMScopedStackAllocator allocScope(*TempAllocator);

	udtParseDataChat chatEvent;
	memset(&chatEvent, 0, sizeof(chatEvent));
	chatEvent.GameStateIndex = _gameStateIndex;
	chatEvent.ServerTimeMs = parser._inServerTime;
	chatEvent.PlayerIndex = -1;

	CommandLineTokenizer& tokenizer = parser._context->Tokenizer;
	const udtString originalCommand = udtString::NewClone(_chatStringAllocator, tokenizer.GetOriginalCommand());
	const udtString cleanCommand = udtString::NewCleanCloneFromRef(_chatStringAllocator, parser._inProtocol, originalCommand);
	chatEvent.Strings[0].OriginalCommand = originalCommand.String;
	chatEvent.Strings[1].OriginalCommand = cleanCommand.String;

	udtString argument1[2];
	argument1[0] = udtString::NewCloneFromRef(*TempAllocator, tokenizer.GetArg(1));
	argument1[1] = udtString::NewCleanCloneFromRef(*TempAllocator, parser._inProtocol, argument1[0]);
	StringRemoveEmCharacter(argument1[0]);
	StringRemoveEmCharacter(argument1[1]);

	const bool qlFormat = parser._inProtocol >= udtProtocol::Dm73;
	if(qlFormat)
	{
		for(u32 i = 0; i < 2; ++i)
		{
			FixQLClanSeparator(argument1[i]);
			const char* const commandArg1 = argument1[i].String;
			udtChatEventData& data = chatEvent.Strings[i];
			tokenizer.Tokenize(commandArg1);
			if(tokenizer.GetArgCount() < 3)
			{
				continue;
			}

			const bool hasClanName = tokenizer.GetArgCount() >= 4 && strchr(tokenizer.GetArgString(1), ':') == NULL;
			const u32 playerIdx = hasClanName ? 2 : 1;
			const char* const colon = strchr(commandArg1, ':');
			const u32 originalCommandLength = argument1[i].Length;
			data.ClanName = hasClanName ? AllocateString(_chatStringAllocator, tokenizer.GetArgString(1)) : nullString;
			data.PlayerName = AllocateString(_chatStringAllocator, tokenizer.GetArgString(playerIdx), tokenizer.GetArgLength(playerIdx) - 1);
			data.Message = (colon + 2 < commandArg1 + originalCommandLength) ? AllocateString(_chatStringAllocator, colon + 2) : nullString;
		}

		s32 playerIndex = -1;
		if(StringParseInt(playerIndex, tokenizer.GetArgString(0)) && playerIndex >= 0 && playerIndex < 64)
		{
			chatEvent.PlayerIndex = playerIndex;
		}
	}
	else
	{
		for(u32 i = 0; i < 2; ++i)
		{
			const char* const commandArg1 = argument1[i].String;
			udtChatEventData& data = chatEvent.Strings[i];
			const char* const colon = strchr(commandArg1, ':');
			const u32 originalCommandLength = argument1[i].Length;
			data.PlayerName = AllocateString(_chatStringAllocator, commandArg1, (u32)(colon - commandArg1 - 1));
			data.Message = (colon + 2 < commandArg1 + originalCommandLength) ? AllocateString(_chatStringAllocator, colon + 2) : nullString;
		}
	}

	ChatEvents.Add(chatEvent);
}

void udtParserPlugInChat::ProcessTeamChatCommand(udtBaseParser& parser)
{
	udtVMScopedStackAllocator allocScope(*TempAllocator);

	udtParseDataChat chatEvent;
	memset(&chatEvent, 0, sizeof(chatEvent));
	chatEvent.GameStateIndex = _gameStateIndex;
	chatEvent.ServerTimeMs = parser._inServerTime;
	chatEvent.PlayerIndex = -1;
	chatEvent.TeamMessage = 1;

	CommandLineTokenizer& tokenizer = parser._context->Tokenizer;
	const udtString originalCommand = udtString::NewClone(_chatStringAllocator, tokenizer.GetOriginalCommand());
	udtString cleanedUpCommand = udtString::NewCloneFromRef(_chatStringAllocator, originalCommand);
	udtString::CleanUp(cleanedUpCommand, parser._inProtocol);
	chatEvent.Strings[0].OriginalCommand = originalCommand.String;
	chatEvent.Strings[1].OriginalCommand = cleanedUpCommand.String;

	udtString argument1[2];
	argument1[0] = udtString::NewCloneFromRef(*TempAllocator, tokenizer.GetArg(1));
	argument1[1] = udtString::NewCleanCloneFromRef(*TempAllocator, parser._inProtocol, argument1[0]);
	StringRemoveEmCharacter(argument1[0]);
	StringRemoveEmCharacter(argument1[1]);

	const bool qlFormat = parser._inProtocol >= udtProtocol::Dm73;
	if(qlFormat)
	{
		for(u32 i = 0; i < 2; ++i)
		{
			const udtString& arg1 = argument1[i];
			s32 clientNumber = -1;
			u32 leftParen1, rightParen1, colon;
			if(!StringParseInt(clientNumber, arg1.String) ||
			   clientNumber < 0 || 
			   clientNumber >= 64 ||
			   !udtString::FindFirstCharacterMatch(leftParen1, arg1, '(') ||
			   !udtString::FindFirstCharacterMatch(rightParen1, arg1, ')', leftParen1 + 1) ||
			   !udtString::FindFirstCharacterMatch(colon, arg1, ':', rightParen1 + 1))
			{
				continue;
			}
			chatEvent.PlayerIndex = clientNumber;

			const char clanNameSeparator = parser._inProtocol >= udtProtocol::Dm91 ? '.' : ' ';

			u32 spaceOrDot;
			if(udtString::FindFirstCharacterMatch(spaceOrDot, arg1, clanNameSeparator, leftParen1 + 1) &&
			   spaceOrDot < rightParen1)
			{
				const udtString clanName = udtString::NewSubstringClone(_chatStringAllocator, arg1, leftParen1 + 1, spaceOrDot - leftParen1 - 1);
				const udtString playerName = udtString::NewSubstringClone(_chatStringAllocator, arg1, spaceOrDot + 1, rightParen1 - spaceOrDot - 1);
				chatEvent.Strings[i].ClanName = clanName.String;
				chatEvent.Strings[i].PlayerName = playerName.String;
			}
			else
			{
				const udtString playerName = udtString::NewSubstringClone(_chatStringAllocator, arg1, leftParen1 + 1, rightParen1 - leftParen1 - 1);
				chatEvent.Strings[i].PlayerName = playerName.String;
			}

			u32 leftParen2, rightParen2;
			if(udtString::FindFirstCharacterMatch(leftParen2, arg1, '(', rightParen1 + 1) &&
			   udtString::FindFirstCharacterMatch(rightParen2, arg1, ')', leftParen2 + 1) &&
			   rightParen2 < colon)
			{
				const udtString location = udtString::NewSubstringClone(_chatStringAllocator, arg1, leftParen2 + 1, rightParen2 - leftParen2 - 1);
				chatEvent.Strings[i].Location = location.String;
			}

			const udtString message = udtString::NewSubstringClone(_chatStringAllocator, arg1, colon + 2);
			chatEvent.Strings[i].Message = message.String;
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
				const udtString location = udtString::NewSubstringClone(_chatStringAllocator, arg1, leftParen2 + 1, rightParen2 - leftParen2 - 1);
				chatEvent.Strings[i].Location = location.String;
			}

			const udtString playerName = udtString::NewSubstringClone(_chatStringAllocator, arg1, leftParen1 + 1, rightParen1 - leftParen1 - 1);
			const udtString message = udtString::NewSubstringClone(_chatStringAllocator, arg1, colon + 2);
			chatEvent.Strings[i].PlayerName = playerName.String;
			chatEvent.Strings[i].Message = message.String;
		}
	}

	ChatEvents.Add(chatEvent);
}

void udtParserPlugInChat::ProcessCPMATeamChatCommand(udtBaseParser& parser)
{
	s32 clientNumber = -1;
	CommandLineTokenizer& tokenizer = parser._context->Tokenizer;
	if(!StringParseInt(clientNumber, tokenizer.GetArgString(1)) ||
	   clientNumber < 0 ||
	   clientNumber >= 64)
	{
		return;
	}

	udtParseDataChat chatEvent;
	memset(&chatEvent, 0, sizeof(chatEvent));
	chatEvent.GameStateIndex = _gameStateIndex;
	chatEvent.ServerTimeMs = parser._inServerTime;
	chatEvent.PlayerIndex = clientNumber;
	chatEvent.TeamMessage = 1;

	const udtProtocol::Id protocol = parser._inProtocol;
	
	s32 locationIdx = -1;
	if(StringParseInt(locationIdx, tokenizer.GetArgString(2)) &&
	   locationIdx >= 0 &&
	   locationIdx < MAX_LOCATIONS)
	{
		const udtBaseParser::udtConfigString& cs = parser._inConfigStrings[CS_LOCATIONS_68 + locationIdx];
		if(cs.String != NULL)
		{
			const udtString location = udtString::NewClone(_chatStringAllocator, cs.String, cs.StringLength);
			udtString cleanLocation = udtString::NewCloneFromRef(_chatStringAllocator, location);
			udtString::CleanUp(cleanLocation, protocol);
			chatEvent.Strings[0].Location = location.String;
			chatEvent.Strings[1].Location = cleanLocation.String;
		}
	}

	const udtString message = udtString::NewCloneFromRef(_chatStringAllocator, tokenizer.GetArg(3));
	udtString cleanMessage = udtString::NewCloneFromRef(_chatStringAllocator, message);
	udtString::CleanUp(cleanMessage, protocol);
	chatEvent.Strings[0].Message = message.String;
	chatEvent.Strings[1].Message = cleanMessage.String;

	udtString playerName;
	const s32 firstPlayerIndex = idConfigStringIndex::FirstPlayer(protocol);
	if(firstPlayerIndex != -1 &&
	   ParseConfigStringValueString(playerName, _chatStringAllocator, "n", parser._inConfigStrings[firstPlayerIndex + clientNumber].String))
	{
		udtString cleanPlayerName = udtString::NewCloneFromRef(_chatStringAllocator, playerName);
		udtString::CleanUp(cleanMessage, protocol);
		chatEvent.Strings[0].PlayerName = playerName.String;
		chatEvent.Strings[1].PlayerName = cleanPlayerName.String;
	}
	
	const udtString command = udtString::NewClone(_chatStringAllocator, tokenizer.GetOriginalCommand());
	udtString cleanCommand = udtString::NewCloneFromRef(_chatStringAllocator, command);
	udtString::CleanUp(cleanMessage, protocol);
	chatEvent.Strings[0].OriginalCommand = command.String;
	chatEvent.Strings[1].OriginalCommand = cleanCommand.String;

	ChatEvents.Add(chatEvent);
}
