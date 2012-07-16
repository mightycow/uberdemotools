#pragma once


#include "api.h"

#include <string>
#include <vector>


extern void CutDemoTime(const std::string& inFilePath, const std::string& outFilePath, int startTime, int endTime, ProgressCallback progressCb, MessageCallback messageCb);
extern void CutDemoChat(const std::string& inFilePath, const std::string& outFilePath, const std::vector<std::string>& chatEntries, int startOffsetSecs, int endOffsetSecs);