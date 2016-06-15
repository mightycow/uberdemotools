#include "plug_in_custom_parser.hpp"
#include "custom_context.hpp"
#include "utils.hpp"


udtCustomParsingPlugIn::udtCustomParsingPlugIn()
{
	_context = NULL;
}

void udtCustomParsingPlugIn::SetContext(udtCuContext_s* context)
{
	_context = context;
}

void udtCustomParsingPlugIn::InitAllocators(u32)
{
}

void udtCustomParsingPlugIn::ProcessMessageBundleStart(const udtMessageBundleCallbackArg&, udtBaseParser&)
{
	_context->Commands.Clear();
	_context->CommandStrings.Clear();
	_context->CommandTokens.Clear();
	_context->CommandTokenAddresses.Clear();
	_context->StringAllocator.Clear();

	udtCuMessageOutput& msg = _context->Message;
	msg.Commands = NULL;
	msg.CommandCount = 0;
	msg.GameStateOrSnapshot.GameState = NULL;
	msg.IsGameState = 0;
}

void udtCustomParsingPlugIn::ProcessMessageBundleEnd(const udtMessageBundleCallbackArg&, udtBaseParser&)
{
	const u32 commandCount = _context->Commands.GetSize();
	if(commandCount == 0)
	{
		return;
	}

	udtCuMessageOutput& msg = _context->Message;
	msg.CommandCount = commandCount;
	msg.Commands = _context->Commands.GetStartAddress();

	// Patch the command string addresses.
	for(u32 i = 0; i < commandCount; ++i)
	{
		_context->Commands[i].CommandString = _context->CommandStrings[i].GetPtr();
	}

	// Build the array of command token addresses.
	const u32 tokenCount = _context->CommandTokens.GetSize();
	_context->CommandTokenAddresses.Clear();
	for(u32 i = 0; i < tokenCount; ++i)
	{
		_context->CommandTokenAddresses.Add(_context->CommandTokens[i].GetPtr());
	}

	// Patch the command token addresses.
	u32 firstTokenIdx = 0;
	for(u32 i = 0; i < commandCount; ++i)
	{
		const u32 count = _context->Commands[i].TokenCount;
		_context->Commands[i].CommandTokens = &_context->CommandTokenAddresses[firstTokenIdx];
		firstTokenIdx += count;
	}
}

void udtCustomParsingPlugIn::ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser&)
{
	udtCuGamestateMessage& gs = _context->GameState;
	gs.ChecksumFeed = arg.ChecksumFeed;
	gs.ClientNumber = arg.ClientNum;
	gs.ServerCommandSequence = arg.ServerCommandSequence;

	udtCuMessageOutput& msg = _context->Message;
	msg.IsGameState = 1;
	msg.GameStateOrSnapshot.GameState = &_context->GameState;
}

void udtCustomParsingPlugIn::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser)
{
	const udtString string = udtString::NewClone(_context->StringAllocator, arg.String, arg.StringLength);
	_context->CommandStrings.Add(string);

	const idTokenizer& tokenizer = parser.GetTokenizer();
	const u32 tokenCount = tokenizer.GetArgCount();
	for(u32 i = 0; i < tokenCount; ++i)
	{
		const udtString token = udtString::NewCloneFromRef(_context->StringAllocator, tokenizer.GetArg(i));
		_context->CommandTokens.Add(token);
	}

	udtCuCommandMessage cmd;
	cmd.CommandSequence = arg.CommandSequence;
	cmd.CommandString = NULL; // We'll patch the address later in ProcessMessageBundleEnd.
	cmd.CommandStringLength = string.GetLength();
	cmd.CommandTokens = NULL; // We'll patch the address later in ProcessMessageBundleEnd.
	cmd.ConfigStringIndex = arg.ConfigStringIndex;
	cmd.IsConfigString = arg.IsConfigString;
	cmd.TokenCount = tokenCount;
	_context->Commands.Add(cmd);
}

void udtCustomParsingPlugIn::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser)
{
	const u32 entityCount = arg.ChangedEntityCount;
	const s32 entityTypeEventId = GetIdNumber(udtMagicNumberType::EntityType, udtEntityType::Event, parser._inProtocol);
	udtVMArray<const idEntityStateBase*>& changedEntities = _context->ChangedEntities;
	changedEntities.Clear();
	for(u32 i = 0; i < entityCount; ++i)
	{
		idEntityStateBase* const ent = arg.ChangedEntities[i].Entity;
		if(ent->eType >= entityTypeEventId)
		{
			if(!arg.ChangedEntities[i].IsNewEvent)
			{
				// Don't give our user any duplicate event.
				continue;
			}

			// Simplify stuff for our user a bit.
			ent->event = (ent->eType - entityTypeEventId) & (~ID_ES_EVENT_BITS);
			ent->eType = entityTypeEventId;
		}

		changedEntities.Add(arg.ChangedEntities[i].Entity);
	}

	udtCuSnapshotMessage& snap = _context->Snapshot;
	memcpy(snap.AreaMask, arg.Snapshot->areamask, 32);
	snap.ChangedEntities = changedEntities.GetStartAddress();
	snap.CommandNumber = arg.CommandNumber;
	snap.ChangedEntityCount = changedEntities.GetSize();
	snap.Entities = (const idEntityStateBase**)arg.Entities;
	snap.EntityCount = arg.EntityCount;
	snap.EntityFlags = arg.EntityFlags;
	snap.MessageNumber = arg.MessageNumber;
	snap.PlayerState = GetPlayerState(arg.Snapshot, _context->Context.Parser._inProtocol);
	snap.RemovedEntities = arg.RemovedEntities;
	snap.RemovedEntityCount = arg.RemovedEntityCount;
	snap.ServerTimeMs = arg.ServerTime;

	udtCuMessageOutput& msg = _context->Message;
	msg.GameStateOrSnapshot.Snapshot = &_context->Snapshot;
}