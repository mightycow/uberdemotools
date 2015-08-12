#include "plug_in_chat.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"


/*
Q3 chat format:
name: message

QL chat format:
playerIndex [clan] name: message
where playerIndex is 0-based and is formatted as 2 digits (printf-style: %02d)
and clan is optional

@NOTE: In Quake 3, player names can contain and end with spaces.
*/


static const char* nullString = "";


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
	if(tokenizer.GetArgCount() == 2 && udtString::Equals(tokenizer.GetArg(0), "chat"))
	{
		ProcessChatCommand(parser, false);
	}
	else if(tokenizer.GetArgCount() == 2 && udtString::Equals(tokenizer.GetArg(0), "tchat"))
	{
		ProcessChatCommand(parser, true);
	}
	else if(tokenizer.GetArgCount() == 4 && udtString::Equals(tokenizer.GetArg(0), "mm2"))
	{
		ProcessCPMATeamChatCommand(parser);
	}
}

void udtParserPlugInChat::ProcessGamestateMessage(const udtGamestateCallbackArg&, udtBaseParser&)
{
	++_gameStateIndex;
}

void udtParserPlugInChat::ProcessChatCommand(udtBaseParser& parser, bool teamChat)
{
	udtVMScopedStackAllocator allocScope(*TempAllocator);

	udtParseDataChat chatEvent;
	memset(&chatEvent, 0, sizeof(chatEvent));
	chatEvent.GameStateIndex = _gameStateIndex;
	chatEvent.ServerTimeMs = parser._inServerTime;
	chatEvent.PlayerIndex = -1;
	chatEvent.TeamMessage = teamChat ? 1 : 0;

	CommandLineTokenizer& tokenizer = parser._context->Tokenizer;
	const udtString originalCommand = udtString::NewClone(_chatStringAllocator, tokenizer.GetOriginalCommand());
	udtString cleanedUpCommand = udtString::NewCloneFromRef(_chatStringAllocator, originalCommand);
	udtString::CleanUp(cleanedUpCommand, parser._inProtocol);
	chatEvent.Strings[0].OriginalCommand = originalCommand.String;
	chatEvent.Strings[1].OriginalCommand = cleanedUpCommand.String;

	udtString argument1[2];
	argument1[0] = udtString::NewCloneFromRef(*TempAllocator, tokenizer.GetArg(1));
	argument1[1] = udtString::NewCloneFromRef(*TempAllocator, argument1[0]);
	udtString::CleanUp(argument1[1], parser._inProtocol);

	const bool qlFormat = parser._inProtocol >= udtProtocol::Dm73;
	if(qlFormat)
	{
		for(u32 i = 0; i < 2; ++i)
		{
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
			const u32 playerCharsToRemove = (tokenizer.GetArgCount() == 5 && udtString::Equals(tokenizer.GetArg(3), ":")) ? 0 : 1;
			data.ClanName = hasClanName ? AllocateString(_chatStringAllocator, tokenizer.GetArgString(1)) : nullString;
			data.PlayerName = AllocateString(_chatStringAllocator, tokenizer.GetArgString(playerIdx), tokenizer.GetArgLength(playerIdx) - playerCharsToRemove);
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
			const char* const whiteEm = strstr(commandArg1, "^7\x19");
			const char* const em = strstr(commandArg1, "\x19");
			u32 playerNameLength = (u32)(colon - commandArg1);
			if(whiteEm != NULL && whiteEm < colon && playerNameLength > 3)
			{
				playerNameLength -= 3;
			}
			else if(em != NULL && em < colon && playerNameLength > 1)
			{
				playerNameLength -= 1;
			}

			data.PlayerName = AllocateString(_chatStringAllocator, commandArg1, playerNameLength);
			data.Message = (colon + 2 < commandArg1 + originalCommandLength) ? AllocateString(_chatStringAllocator, colon + 2) : nullString;
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
