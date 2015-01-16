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
	_chatStringAllocator.Init((uptr)(1 << 16) * (uptr)demoCount);
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
	if(strcmp(tokenizer.argv(0), "chat") != 0 || tokenizer.argc() != 2)
	{
		return;
	}

	udtParseDataChat chatEvent;
	memset(&chatEvent, 0, sizeof(chatEvent));
	chatEvent.GameStateIndex = _gameStateIndex;
	chatEvent.ServerTimeMs = parser._inServerTime;
	chatEvent.PlayerIndex = S32_MIN;

	chatEvent.Strings[0].OriginalCommand = AllocateString(_chatStringAllocator, tokenizer.argv(1));
	chatEvent.Strings[1].OriginalCommand = Q_CleanStr(AllocateString(_chatStringAllocator, tokenizer.argv(1)));

	const bool qlFormat = parser._protocol >= udtProtocol::Dm73;
	if(qlFormat)
	{
		for(u32 i = 0; i < 2; ++i)
		{
			udtChatEventData& data = chatEvent.Strings[i];
			tokenizer.Tokenize(data.OriginalCommand);
			if(tokenizer.argc() < 3)
			{
				continue;
			}

			const bool hasClanName = tokenizer.argc() >= 4 && strchr(tokenizer.argv(1), ':') == NULL;
			const int playerIdx = hasClanName ? 2 : 1;
			const char* const colon = strchr(data.OriginalCommand, ':');
			const size_t originalCommandLength = strlen(data.OriginalCommand);
			data.ClanName = hasClanName ? AllocateString(_chatStringAllocator, tokenizer.argv(1)) : nullString;
			data.PlayerName = AllocateString(_chatStringAllocator, tokenizer.argv(playerIdx), (u32)strlen(tokenizer.argv(playerIdx)) - 1);
			data.Message = (colon + 2 < data.OriginalCommand + originalCommandLength) ? AllocateString(_chatStringAllocator, colon + 2) : nullString;
		}

		s32 playerIndex = -1;
		if(StringParseInt(playerIndex, tokenizer.argv(0)) && playerIndex >= 0 && playerIndex < 64)
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
			data.PlayerName = AllocateString(_chatStringAllocator, data.OriginalCommand, (u32)(colon - data.OriginalCommand));
			data.Message = (colon + 2 < data.OriginalCommand + originalCommandLength) ? AllocateString(_chatStringAllocator, colon + 2) : nullString;
		}
	}
	
	ChatEvents.Add(chatEvent);
}

void udtParserPlugInChat::ProcessGamestateMessage(const udtGamestateCallbackArg&, udtBaseParser&)
{
	++_gameStateIndex;
}
