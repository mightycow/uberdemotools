#include "config.hpp"
#include "file_stream.hpp"
#include "string.hpp"
#include "array.hpp"
#include "utils.hpp"


#if defined(UDT_GCC)
#	define  OFFSET_OF(type, member)  __builtin_offsetof(type, member)
#elif defined(UDT_MSVC)
#	include <stddef.h>
#	define  OFFSET_OF(type, member)  offsetof(type, member)
#endif


struct VarType
{
	enum Id
	{
		f32_,
		u32_,
		s32_,
		bool_,
		Count
	};
};

struct Var
{
	const char* Name;
	u32 Offset;
	VarType::Id Type;
};

#define ITEM(Type, Name, Value) 1 + 
static const u32 VariableCount = CONFIG_VARS(ITEM) 0;
#undef ITEM

#define ITEM(Type, Name, Value) { #Name, (u32)OFFSET_OF(Config, Name), VarType::Type##_ },
static const Var Variables[VariableCount + 1] =
{
	CONFIG_VARS(ITEM)
	{ "", 0, VarType::Count }
};
#undef ITEM


bool LoadConfig(Config& config, const char* filePath)
{
	udtFileStream file;
	if(!file.Open(filePath, udtFileOpenMode::Read))
	{
		return false;
	}

	udtVMLinearAllocator fileAllocator("LoadConfig::File");
	udtString fileString = file.ReadAllAsString(fileAllocator);
	if(fileString.GetLength() == 0 || !fileString.IsValid())
	{
		return false;
	}
	file.Close();

	udtVMArray<udtString> lines("LoadConfig::LinesArray");
	if(!StringSplitLines(lines, fileString))
	{
		return false;
	}

	u8* const configPtr = (u8*)&config;
	idTokenizer& tokenizer = *(idTokenizer*)fileAllocator.AllocateAndGetAddress((uptr)sizeof(idTokenizer));
	for(u32 l = 0, count = lines.GetSize(); l < count; ++l)
	{
		const udtString line = lines[l];
		if(udtString::IsNullOrEmpty(line) || udtString::StartsWith(line, "//"))
		{
			continue;
		}

		tokenizer.Tokenize(line.GetPtr());
		if(tokenizer.GetArgCount() != 2)
		{
			continue;
		}

		const char* const name = tokenizer.GetArgString(0);
		const char* const value = tokenizer.GetArgString(1);
		int temps = 0;
		unsigned int tempu = 0;
		for(u32 v = 0; v < VariableCount; ++v)
		{
			const Var& var = Variables[v];
			if(strcmp(var.Name, name) != 0)
			{
				continue;
			}

			switch(var.Type)
			{
				case VarType::f32_:
					sscanf(value, "%f", (f32*)(configPtr + var.Offset));
					break;

				case VarType::bool_:
					sscanf(value, "%d", &temps);
					*(bool*)(configPtr + var.Offset) = temps != 0;
					break;

				case VarType::u32_:
					sscanf(value, "%u", &tempu);
					*(u32*)(configPtr + var.Offset) = (u32)tempu;
					break;

				case VarType::s32_:
					sscanf(value, "%d", &temps);
					*(s32*)(configPtr + var.Offset) = (s32)temps;
					break;

				default:
					break;
			}

			break;
		}
	}

	return true;
}

bool SaveConfig(const Config& config, const char* filePath)
{
	udtFileStream file;
	if(!file.Open(filePath, udtFileOpenMode::Write))
	{
		return false;
	}

	const u8* const configPtr = (const u8*)&config;

	char line[256];
	for(u32 i = 0; i < VariableCount; ++i)
	{
		const Var& var = Variables[i];
		switch(var.Type)
		{
			case VarType::f32_:
				sprintf(line, "%s %f\n", var.Name, *(const float*)(configPtr + var.Offset));
				break;

			case VarType::bool_:
				sprintf(line, "%s %d\n", var.Name, *(const bool*)(configPtr + var.Offset) ? 1 : 0);
				break;

			case VarType::u32_:
				sprintf(line, "%s %u\n", var.Name, (unsigned int)*(const u32*)(configPtr + var.Offset));
				break;

			case VarType::s32_:
				sprintf(line, "%s %d\n", var.Name, (int)*(const s32*)(configPtr + var.Offset));
				break;

			default:
				continue;
		}
		
		file.Write(line, (u32)strlen(line), 1);
	}

	return true;
}
