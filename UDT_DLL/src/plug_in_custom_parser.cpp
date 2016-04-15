#include "plug_in_custom_parser.hpp"
#include "custom_context.hpp"


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
	_context->StringAllocator.Clear();

	udtCuMessageOutput& msg = _context->Message;
	msg.Commands = NULL;
	msg.ServerCommandCount = 0;
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
	msg.ServerCommandCount = commandCount;
	msg.Commands = _context->Commands.GetStartAddress();

	// Patch the addresses.
	for(u32 i = 0; i < commandCount; ++i)
	{
		_context->Commands[i].CommandString = _context->CommandStrings[i].GetPtr();
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

void udtCustomParsingPlugIn::ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser&)
{
	const udtString string = udtString::NewClone(_context->StringAllocator, arg.String, arg.StringLength);
	_context->CommandStrings.Add(string);

	udtCuCommandMessage cmd;
	cmd.CommandSequence = arg.CommandSequence;
	cmd.CommandString = NULL; // We'll patch the address later in ProcessMessageBundleEnd.
	cmd.CommandStringLength = string.GetLength();
	cmd.ConfigStringIndex = arg.ConfigStringIndex;
	cmd.IsConfigString = arg.IsConfigString;
	_context->Commands.Add(cmd);
}

void udtCustomParsingPlugIn::ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser&)
{
	const u32 entityCount = arg.EntityCount;
	udtVMArray<const idEntityStateBase*>& changedEntities = _context->ChangedEntities;
	udtVMArray<u32>& changedEntityFlags = _context->ChangedEntityFlags;
	changedEntities.Clear();
	changedEntityFlags.Clear();
	for(u32 i = 0; i < entityCount; ++i)
	{
		changedEntities.Add(arg.Entities[i].Entity);

		u32 flags = 0;
		if(arg.Entities[i].IsNewEvent)
		{
			flags |= (u32)udtEntityFlags::IsNewEvent;
		}
		changedEntityFlags.Add(flags);
	}

	udtCuSnapshotMessage& snap = _context->Snapshot;
	memcpy(snap.AreaMask, arg.Snapshot->areamask, 32);
	snap.ChangedEntities = changedEntities.GetStartAddress();
	snap.ChangedEntityFlags = changedEntityFlags.GetStartAddress();
	snap.CommandNumber = arg.CommandNumber;
	snap.EntityCount = entityCount;
	snap.MessageNumber = arg.MessageNumber;
	snap.PlayerState = GetPlayerState(arg.Snapshot, _context->Context.Parser._inProtocol);
	snap.RemovedEntities = arg.RemovedEntities;
	snap.RemovedEntityCount = arg.RemovedEntityCount;
	snap.ServerTimeMs = arg.ServerTime;

	udtCuMessageOutput& msg = _context->Message;
	msg.GameStateOrSnapshot.Snapshot = &_context->Snapshot;
}