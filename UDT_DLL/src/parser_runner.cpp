#include "parser_runner.hpp"
#include "utils.hpp"


udtParserRunner::udtParserRunner()
{
	_fileStartOffset = 0;
	_fileOffset = 0;
	_maxByteCount = 0;
	_parser = NULL;
	_file = NULL;
	_cancelOperation = NULL;
	_success = false;
}

bool udtParserRunner::Init(udtBaseParser& parser, udtStream& file, const s32* cancelOperation)
{
	_parser = &parser;
	_file = &file;
	_cancelOperation = cancelOperation;

	_inMsg.InitContext(parser._context);
	_inMsg.InitProtocol(parser._inProtocol);

	_fileStartOffset = (u64)file.Offset();
	_maxByteCount = file.Length() - _fileStartOffset;

	_timer.Start();

	return true;
}

bool udtParserRunner::ParseNextMessage()
{
	if(_cancelOperation != NULL && *_cancelOperation != 0)
	{
		SetSuccess(false);
		return false;
	}

	const u64 fileOffset = _fileOffset;

	s32 inServerMessageSequence = 0;
	u32 elementsRead = _file->Read(&inServerMessageSequence, 4, 1);
	if(elementsRead != 1)
	{
		_parser->_context->LogWarning("Demo file %s is truncated", _parser->GetFileNamePtr());
		SetSuccess(true);
		return false;
	}

	_inMsg.Init(_parser->_inMsgData, ID_MAX_MSG_LENGTH);

	elementsRead = _file->Read(&_inMsg.Buffer.cursize, 4, 1);
	if(elementsRead != 1)
	{
		_parser->_context->LogWarning("Demo file %s is truncated", _parser->GetFileNamePtr());
		SetSuccess(true);
		return false;
	}

	if(_inMsg.Buffer.cursize == -1)
	{
		SetSuccess(true);
		return false;
	}

	if((u32)_inMsg.Buffer.cursize > (u32)_inMsg.Buffer.maxsize)
	{
		_parser->_context->LogError("Demo file %s has a message length greater than MAX_SIZE", _parser->GetFileNamePtr());
		SetSuccess(false);
		return false;
	}

	elementsRead = _file->Read(_inMsg.Buffer.data, _inMsg.Buffer.cursize, 1);
	if(elementsRead != 1)
	{
		_parser->_context->LogWarning("Demo file %s is truncated", _parser->GetFileNamePtr());
		SetSuccess(true);
		return false;
	}

	_inMsg.Buffer.readcount = 0;
	if(!_parser->ParseNextMessage(_inMsg, inServerMessageSequence, (u32)fileOffset))
	{
		SetSuccess(true);
		return false;
	}

	const u64 currentByteCount = fileOffset - _fileStartOffset;
	const f32 currentProgress = (f32)currentByteCount / (f32)_maxByteCount;
	_parser->_context->NotifyProgress(currentProgress);
	_fileOffset += (u64)_inMsg.Buffer.cursize + 8;

	SetSuccess(true);

	return true;
}

void udtParserRunner::FinishParsing()
{
	_parser->FinishParsing(_success);
}

bool udtParserRunner::WasSuccess() const
{
	return _success;
}

void udtParserRunner::SetSuccess(bool success)
{
	_success = success;
}
