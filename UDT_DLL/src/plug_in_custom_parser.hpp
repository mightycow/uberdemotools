#pragma once


#include "parser_plug_in.hpp"


struct udtCuContext_s;


struct udtCustomParsingPlugIn : public udtBaseParserPlugIn
{
	udtCustomParsingPlugIn();

	void SetContext(udtCuContext_s* context);

	void InitAllocators(u32) override;
	void ProcessMessageBundleStart(const udtMessageBundleCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessMessageBundleEnd(const udtMessageBundleCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessCommandMessage(const udtCommandCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtCustomParsingPlugIn);

	udtCuContext_s* _context;
};
