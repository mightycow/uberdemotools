#include "api_helpers.hpp"
#include "parser_context.hpp"
#include "linear_allocator.hpp"
#include "scoped_stack_allocator.hpp"
#include "utils.hpp"
#include "analysis_cut_by_chat.hpp"
#include "analysis_cut_by_frag.hpp"
#include "analysis_cut_by_awards.hpp"


bool ParseDemoFile(udtParserContext* context, const udtParseArg* info, const char* demoFilePath, bool clearPlugInData)
{
	if(clearPlugInData)
	{
		context->Reset();
	}
	else
	{
		context->ResetButKeepPlugInData();
	}

	context->CreateAndAddPlugIns(info->PlugIns, info->PlugInCount);

	const udtProtocol::Id protocol = udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return false;
	}

	if(!context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext))
	{
		return false;
	}

	udtFileStream file;
	if(!file.Open(demoFilePath, udtFileOpenMode::Read))
	{
		return false;
	}

	if(!context->Parser.Init(&context->Context, protocol))
	{
		return false;
	}

	udtVMScopedStackAllocator tempAllocScope(context->Context.TempAllocator);

	context->Parser.SetFilePath(demoFilePath);
	if(!RunParser(context->Parser, file, info->CancelOperation))
	{
		return false;
	}

	return true;
}

template<class udtParserPlugInCutByWhatever, class udtCutByWhateverArg>
static bool CutByWhatever(udtParserContext* context, const udtParseArg* info, const udtCutByWhateverArg* whateverInfo, const char* demoFilePath, const char* analysisType)
{
	const udtProtocol::Id protocol = udtGetProtocolByFilePath(demoFilePath);
	if(protocol == udtProtocol::Invalid)
	{
		return false;
	}

	udtParserPlugInCutByWhatever plugIn(*whateverInfo);
	if(!RunParserWithPlugIn(context, plugIn, protocol, info, demoFilePath, analysisType))
	{
		return false;
	}

	if(plugIn.Analyzer.CutSections.IsEmpty())
	{
		return true;
	}

	context->Reset();
	if(!context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext))
	{
		return false;
	}

	udtFileStream file;
	if(!file.Open(demoFilePath, udtFileOpenMode::Read))
	{
		return false;
	}

	const s32 gsIndex = plugIn.Analyzer.CutSections[0].GameStateIndex;
	const u32 fileOffset = context->Parser._inGameStateFileOffsets[gsIndex];
	if(fileOffset > 0 && file.Seek((s32)fileOffset, udtSeekOrigin::Start) != 0)
	{
		return false;
	}

	if(!context->Parser.Init(&context->Context, protocol, gsIndex))
	{
		return false;
	}

	context->Parser.SetFilePath(demoFilePath);

	CallbackCutDemoFileStreamCreationInfo cutCbInfo;
	cutCbInfo.OutputFolderPath = info->OutputFolderPath;

	const udtCutAnalyzerBase::CutSectionVector& sections = plugIn.Analyzer.CutSections;
	for(u32 i = 0, count = sections.GetSize(); i < count; ++i)
	{
		const udtCutAnalyzerBase::CutSection& section = sections[i];
		context->Parser.AddCut(section.GameStateIndex, section.StartTimeMs, section.EndTimeMs, &CallbackCutDemoFileStreamCreation, &cutCbInfo);
	}

	context->Context.LogInfo("Processing for %s cut(s): %s", analysisType, demoFilePath);

	udtVMScopedStackAllocator tempAllocScope(context->Context.TempAllocator);

	context->Context.SetCallbacks(info->MessageCb, NULL, NULL);
	const bool result = RunParser(context->Parser, file, info->CancelOperation);
	context->Context.SetCallbacks(info->MessageCb, info->ProgressCb, info->ProgressContext);

	return result;
}

bool CutByChat(udtParserContext* context, const udtParseArg* info, const udtCutByChatArg* chatInfo, const char* demoFilePath)
{
	typedef udtCutPlugInBase<udtCutByChatAnalyzer, udtCutByChatArg> T;

	return CutByWhatever<T>(context, info, chatInfo, demoFilePath, "chat");
}

bool CutByFrag(udtParserContext* context, const udtParseArg* info, const udtCutByFragArg* fragInfo, const char* demoFilePath)
{
	typedef udtCutPlugInBase<udtCutByFragAnalyzer, udtCutByFragArg> T;

	return CutByWhatever<T>(context, info, fragInfo, demoFilePath, "frag");
}

bool CutByAward(udtParserContext* context, const udtParseArg* info, const udtCutByAwardArg* awardInfo, const char* demoFilePath)
{
	typedef udtCutPlugInBase<udtCutByAwardAnalyzer, udtCutByAwardArg> T;

	return CutByWhatever<T>(context, info, awardInfo, demoFilePath, "award");
}
