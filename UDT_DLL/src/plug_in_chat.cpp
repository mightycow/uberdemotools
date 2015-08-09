#include "plug_in_chat.hpp"
#include "utils.hpp"


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
	if(tokenizer.GetArgCount() != 2 || !udtString::Equals(tokenizer.GetArg(0), "chat"))
	{
		return;
	}

	udtParseDataChat chatEvent;
	memset(&chatEvent, 0, sizeof(chatEvent));
	chatEvent.GameStateIndex = _gameStateIndex;
	chatEvent.ServerTimeMs = parser._inServerTime;
	chatEvent.PlayerIndex = S32_MIN;

	const udtString arg1 = tokenizer.GetArg(1);
	const udtString originalCommand = udtString::NewCloneFromRef(_chatStringAllocator, arg1);
	udtString cleanedUpCommand = udtString::NewCloneFromRef(_chatStringAllocator, arg1);
	udtString::CleanUp(cleanedUpCommand, parser._inProtocol);
	chatEvent.Strings[0].OriginalCommand = originalCommand.String;
	chatEvent.Strings[1].OriginalCommand = cleanedUpCommand.String;

	const bool qlFormat = parser._inProtocol >= udtProtocol::Dm73;
	if(qlFormat)
	{
		for(u32 i = 0; i < 2; ++i)
		{
			udtChatEventData& data = chatEvent.Strings[i];
			tokenizer.Tokenize(data.OriginalCommand);
			if(tokenizer.GetArgCount() < 3)
			{
				continue;
			}
			
			const bool hasClanName = tokenizer.GetArgCount() >= 4 && strchr(tokenizer.GetArgString(1), ':') == NULL;
			const u32 playerIdx = hasClanName ? 2 : 1;
			const char* const colon = strchr(data.OriginalCommand, ':');
			const size_t originalCommandLength = strlen(data.OriginalCommand);
			const u32 playerCharsToRemove = (tokenizer.GetArgCount() == 5 && udtString::Equals(tokenizer.GetArg(3), ":")) ? 0 : 1;
			data.ClanName = hasClanName ? AllocateString(_chatStringAllocator, tokenizer.GetArgString(1)) : nullString;
			data.PlayerName = AllocateString(_chatStringAllocator, tokenizer.GetArgString(playerIdx), tokenizer.GetArgLength(playerIdx) - playerCharsToRemove);
			data.Message = (colon + 2 < data.OriginalCommand + originalCommandLength) ? AllocateString(_chatStringAllocator, colon + 2) : nullString;
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
			udtChatEventData& data = chatEvent.Strings[i];
			const char* const colon = strchr(data.OriginalCommand, ':');
			const size_t originalCommandLength = strlen(data.OriginalCommand);
			const char* const whiteEm = strstr(data.OriginalCommand, "^7\x19");
			const char* const em = strstr(data.OriginalCommand, "\x19");
			u32 playerNameLength = (u32)(colon - data.OriginalCommand);
			if(whiteEm != NULL && whiteEm < colon && playerNameLength > 3)
			{
				playerNameLength -= 3;
			}
			else if(em != NULL && em < colon && playerNameLength > 1)
			{
				playerNameLength -= 1;
			}

			data.PlayerName = AllocateString(_chatStringAllocator, data.OriginalCommand, playerNameLength);
			data.Message = (colon + 2 < data.OriginalCommand + originalCommandLength) ? AllocateString(_chatStringAllocator, colon + 2) : nullString;
		}
	}
	
	ChatEvents.Add(chatEvent);
}

void udtParserPlugInChat::ProcessGamestateMessage(const udtGamestateCallbackArg&, udtBaseParser&)
{
	++_gameStateIndex;
}
