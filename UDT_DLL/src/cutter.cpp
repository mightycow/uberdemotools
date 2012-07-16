#include "cutter.hpp"
#include "demo.hpp"
#include "demo68.hpp"
#include "demo73.hpp"

#include <sstream>
#include <limits.h>


template<typename T>
static std::string ToString(const T& x)
{
	std::ostringstream stream;
	stream << x;

	return stream.str();
}

static bool Find(const std::string& inside, const std::vector<std::string>& entries)
{
	for(size_t i = 0; i < entries.size(); ++i)
	{
		if(inside.find(entries[0]) != std::string::npos)
		{
			return true;
		}
	}

	return false;
}

static void MergeRanges(const std::vector<std::pair<int, int> >& ranges, std::vector<std::pair<int, int> >& result)
{
	std::vector<std::pair<int, int> >::const_iterator it = ranges.begin();
	std::pair<int,int> current = *(it)++;

	while(it != ranges.end())
	{
		if(current.second >= it->first)
		{
			current.second = std::max(current.second, it->second); 
		} 
		else 
		{
			result.push_back(current);
			current = *(it);
		}

		it++;
	}

	result.push_back(current);
}

static Demo* CreateDemo(const std::string& inFilePath)
{
	Protocol::Id protocol = Protocol::Invalid;
	if(inFilePath.find_last_of(".dm_68") != std::string::npos)
	{
		protocol = Protocol::Dm68;
	}
	else if(inFilePath.find_last_of(".dm_73") != std::string::npos)
	{
		protocol = Protocol::Dm73;
	}

	Demo* demo = NULL;
	switch(protocol)
	{
	case Protocol::Dm68:
		demo = new Demo68;
		break;

	case Protocol::Dm73:
		demo = new Demo73;
		break;

	default:
		break;
	}

	return demo;
}


void CutDemoTime(const std::string& inFilePath, const std::string& outFilePath, int startTime, int endTime, ProgressCallback progressCb, MessageCallback messageCb)
{
	_progressCallback = progressCb;
	_messageCallback = messageCb;

	Demo* demo = CreateDemo(inFilePath);
	if(demo == NULL)
	{
		return;
	}

	demo->_inFilePath = inFilePath;
	demo->_outFilePath = outFilePath;
	demo->_demoRecordStartTime = startTime;
	demo->_demoRecordEndTime = endTime;
	demo->Do();
}

void CutDemoChat(const std::string& inFilePath, const std::string& /*outFilePath*/, const std::vector<std::string>& chatEntries, int startOffsetSecs, int endOffsetSecs)
{
	Demo* demo = CreateDemo(inFilePath);
	if(demo == NULL)
	{
		return;
	}

	demo->_inFilePath = inFilePath;
	demo->_outFilePath = "[invalid]";
	demo->_demoRecordStartTime = INT_MIN;
	demo->_demoRecordEndTime = INT_MIN;
	demo->Do();

	const Demo::ChatMap& chatMessages = demo->_chatMessages;
	Demo::ChatMap::const_iterator it = chatMessages.begin();
	Demo::ChatMap::const_iterator end = chatMessages.end();
	std::vector<std::pair<int, int> > cutSections;

	for(; it != end; ++it)
	{
		if(!Find(it->second, chatEntries))
		{
			continue;
		}

		int msgTime = it->first;
		int startTime = msgTime - startOffsetSecs * 1000;
		int endTime = msgTime + endOffsetSecs * 1000;
		cutSections.push_back(std::make_pair(startTime, endTime));
	}

	std::vector<std::pair<int, int> > mergedCutSections;
	MergeRanges(cutSections, mergedCutSections);

	for(size_t i = 0; i < mergedCutSections.size(); ++i)
	{
		std::string::size_type lastDotIdx = inFilePath.rfind('.');
		std::string outFilePath = inFilePath.substr(0, lastDotIdx) + "_cut_" + ToString(i + 1) + ".dm_68";

		Demo* innerDemo = CreateDemo(inFilePath);
		if(innerDemo == NULL)
		{
			continue;
		}

		innerDemo->_inFilePath = inFilePath;
		innerDemo->_outFilePath = outFilePath;
		innerDemo->_demoRecordStartTime = mergedCutSections[i].first;
		innerDemo->_demoRecordEndTime = mergedCutSections[i].second;
		innerDemo->Do();
	}
}