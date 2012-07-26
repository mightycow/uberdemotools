#include "utils.hpp"
#include "common.hpp"

#include <cstdlib>


int FindVariableValueOffset(const std::string& input, const std::string& varName)
{
	const std::string searchFor = varName + "\\";
	size_t idx = input.find(searchFor);
	if(idx == 0)
	{
		return 0;
	}

	while(idx != std::string::npos)
	{
		// Can't be wrong since we started the search at offset >= 1.
		const char c = input[idx - 1];
		if(c == '"' || c == '\\')
		{
			break;
		}

		idx = input.find(searchFor, idx + 1);
	}

	if(idx == std::string::npos)
	{
		// Didn't find your variable. Sorry.
		return -1;
	}

	return (int)idx;
}

int GetVariable(const std::string& input, const std::string& varName)
{
	const int idx = FindVariableValueOffset(input, varName);
	if(idx == -1)
	{
		return -9999;
	}

	const int offset = idx + 1 + (int)varName.length();
	if(offset >= (int)input.length())
	{
		// Was about to go bad places.
		return false;
	}

	return atoi(input.c_str() + offset);
}

int GetCpmaConfigStringInt(const char* var, const char* str)
{
	return GetVariable(str, var);
}

bool GetVariable(const std::string& input, const std::string& varName, int* varValue)
{
	*varValue = -9999;

	const int idx = FindVariableValueOffset(input, varName);
	if(idx == -1)
	{
		return false;
	}

	const int offset = idx + 1 + (int)varName.length();
	if(offset >= (int)input.length())
	{
		// Was about to go bad places.
		return false;
	}

	*varValue = atoi(input.c_str() + offset);

	return true;
}

bool TryGetVariable(int* varValue, const std::string& input, const std::string& varName)
{
	return GetVariable(input, varName, varValue);
}

void GetVariable(std::string& varValue, const std::string& input, const std::string& varName)
{
	varValue = "N/A";

	const int idx1 = FindVariableValueOffset(input, varName);
	if(idx1 == -1)
	{
		return;
	}

	const size_t searchForLength = varName.length() + 1;
	size_t idx2 = input.find("\\", (size_t)idx1 + searchForLength);
	if(idx2 == std::string::npos)
	{
		idx2 = (int)input.length();
	}

	const size_t startIdx = (size_t)idx1 + searchForLength;
	varValue = input.substr(startIdx, idx2 - startIdx);
}

void ChangeVariable(const std::string& input, const std::string& varName, const std::string newVal, std::string& output)
{
	int val = -1;
	if(!GetVariable(input, varName, &val))
	{
		output = input;
		return;
	}

	const std::string searchFor = varName + "\\";
	const int idx1 = FindVariableValueOffset(input, varName);
	if(idx1 == -1)
	{
		output = input;
		return;
	}

	const size_t idx2 = input.find("\\", (size_t)idx1 + varName.length() + 1);
	if(idx2 == std::string::npos)
	{
		output = input;
		return;
	}

	const std::string a = input.substr(0, idx1);
	const std::string b = input.substr(idx2);

	output = a + searchFor + newVal + b;
}

void ChangeVariable(const std::string& input, const std::string& var, int val, std::string& output)
{
	char newVal[64];
	sprintf(newVal, "%d", val);
	ChangeVariable(input, var, newVal, output);
}

int ConvertPowerUpFlagsToValue(int flags)
{
	int result = PW_NONE;
	for(int i = PW_FIRST; i <= PW_LAST; ++i)
	{
		int mask = 1 << i;
		if(flags & mask)
		{
			result = i;
			break;
		}
	}

	return result;
}

void ReadScore(const char* scoreString, int* scoreValue)
{
	if(sscanf(scoreString, "%d", scoreValue) != 1)
	{
		*scoreValue = -9999;
	}
}