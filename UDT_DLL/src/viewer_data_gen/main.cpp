#include "sprites.hpp"
#include "string.hpp"
#include "path.hpp"
#include "array.hpp"
#include "file_stream.hpp"
#include "utils.hpp"
#include "file_system.hpp"
#include "stb_image.h"
#include "stb_image_write.h"

#include <stdlib.h>


struct SpriteInfo
{
	u8* Pixels;
	u32 Width;
	u32 Height;
};

static void PreMultiplyByAlpha(u8* pixels, u32 width, u32 height)
{
	for(u32 y = 0; y < height; ++y)
	{
		for(u32 x = 0; x < width; ++x)
		{
			pixels[0] = ((u16)pixels[0] * (u16)pixels[3]) / 255;
			pixels[1] = ((u16)pixels[1] * (u16)pixels[3]) / 255;
			pixels[2] = ((u16)pixels[2] * (u16)pixels[3]) / 255;
			pixels += 4;
		}
	}
}

static const char* ImageFileExtensions[] =
{
	".png",
	".jpg",
	".jpeg",
	".tga",
	".bmp"
};

struct SpritePackGenerator
{
	void ProcessFolder(const udtString& dataPath, const udtString& outputPath)
	{
		printf("processing sprites\n");

#define ITEM(Enum, FileNameNoExt) LoadSprite(dataPath, udtString::NewConstRef(FileNameNoExt));
		SPRITE_LIST(ITEM)
#undef ITEM

		WriteFile(outputPath);
	}

private:
	void LoadSprite(const udtString& dataFolderPath, const udtString& fileNameNoExt)
	{
		udtString filePathNoExt;
		udtPath::Combine(filePathNoExt, _alloc, dataFolderPath, fileNameNoExt);
		udtString filePath;
		bool fileFound = false;
		for(u32 i = 0; i < UDT_COUNT_OF(ImageFileExtensions); ++i)
		{
			filePath = udtString::NewFromConcatenating(_alloc, filePathNoExt, udtString::NewConstRef(ImageFileExtensions[i]));
			if(udtFileStream::Exists(filePath.GetPtr()))
			{
				fileFound = true;
				break;
			}
		}

		if(!fileFound)
		{
			printf("failed to load sprite %s:\n", fileNameNoExt.GetPtr());
			return;
		}

		int width, height, channels;
		stbi_uc* const pixels = stbi_load(filePath.GetPtr(), &width, &height, &channels, 4);
		PreMultiplyByAlpha(pixels, width, height);

		SpriteInfo info;
		info.Width = width;
		info.Height = height;
		info.Pixels = pixels;
		_sprites.Add(info);
	}

	void WriteFile(const udtString& outputPath)
	{
		if(_sprites.GetSize() != Sprite::Count)
		{
			return;
		}

		udtFileStream file;
		if(!file.Open(outputPath.GetPtr(), udtFileOpenMode::Write))
		{
			return;
		}

		const u32 version = 1;
		file.Write(&version, 4, 1);

		const u32 spriteCount = _sprites.GetSize();
		file.Write(&spriteCount, 4, 1);

		for(u32 s = 0; s < spriteCount; ++s)
		{
			const SpriteInfo& sprite = _sprites[s];
			file.Write(&sprite.Width, 4, 1);
			file.Write(&sprite.Height, 4, 1);
			file.Write(sprite.Pixels, sprite.Width * sprite.Height * 4, 1);
		}
	}

	udtVMArray<SpriteInfo> _sprites { "SpritePackGenerator::SpritesArray" };
	udtVMLinearAllocator _alloc { "SpritePackGenerator::Temp" };
};

struct MapTextConverter
{
	void ProcessFolder(const udtString& folderPath, const udtString& outputPath)
	{
		udtFileListQuery query;
		query.FolderPath = folderPath;
		query.UserData = nullptr;
		query.Recursive = false;
		query.FileFilter = nullptr;
		if(!GetDirectoryFileList(query))
		{
			return;
		}

		for(u32 i = 0; i < query.Files.GetSize(); ++i)
		{
			const udtFileInfo& file = query.Files[i];
			if(!udtString::EndsWith(file.Name, ".txt"))
			{
				continue;
			}

			udtString fileNameNoExt;
			udtPath::GetFileNameWithoutExtension(fileNameNoExt, _tempAlloc, file.Name);
			printf("processing map %s\n", fileNameNoExt.GetPtr());
			ProcessMap(folderPath, fileNameNoExt, outputPath);
		}
	}

private:
	void ProcessMap(const udtString& folderPath, const udtString& name, const udtString& outputPath)
	{
		udtString textFilePath;
		udtString textFileName = udtString::NewFromConcatenating(_tempAlloc, name, udtString::NewConstRef(".txt"));
		udtPath::Combine(textFilePath, _tempAlloc, folderPath, textFileName);
		udtFileStream infoFile;
		if(!infoFile.Open(textFilePath.GetPtr(), udtFileOpenMode::Read))
		{
			return;
		}

		udtString textFileData = infoFile.ReadAllAsString(_tempAlloc);
		f32 min[3];
		f32 max[3];
		if(sscanf(textFileData.GetPtr(), "origin = %f %f %f\r\nend = %f %f %f",
			&min[0], &min[1], &min[2], &max[0], &max[1], &max[2]) != 6)
		{
			return;
		}

		// Memento_Mori inverted the Y axis but to make sure any kind of error gets corrected,
		// we just min/max on all 3 axis.
		f32 realMin[3];
		f32 realMax[3];
		for(u32 i = 0; i < 3; ++i)
		{
			realMin[i] = udt_min(min[i], max[i]);
			realMax[i] = udt_max(min[i], max[i]);
		}

		const udtString mapFileName = udtString::NewFromConcatenating(_tempAlloc, name, udtString::NewConstRef(".mapinfo"));
		udtString mapFilePath;
		udtPath::Combine(mapFilePath, _tempAlloc, outputPath, mapFileName);
		udtFileStream mapFile;
		if(!mapFile.Open(mapFilePath.GetPtr(), udtFileOpenMode::Write))
		{
			return;
		}

		const u32 version = 1;
		mapFile.Write(&version, 4, 1);
		mapFile.Write(realMin, 12, 1);
		mapFile.Write(realMax, 12, 1);
	}

	udtVMLinearAllocator _tempAlloc { "MapTextConverter::Temp" };
};


void PrintHelp()
{
}

int udt_main(int argc, char** argv)
{
	udtInitLibrary();

	udtString inputPath = udtString::NewConstRef("viewer_data");
	udtString outputPath = udtString::NewConstRef(".");
	for(int i = 1; i < argc; ++i)
	{
		if(udtString::StartsWith(udtString::NewConstRef(argv[i]), "-o="))
		{
			outputPath = udtString::NewConstRef(argv[i] + 3);
		}
		else
		{
			inputPath = udtString::NewConstRef(argv[i]);
		}
	}

	udtVMLinearAllocator alloc("udt_main::Paths");
	udtString spritesFolderPath;
	udtString mapsFolderPath;
	udtString spritesOutputPath;
	udtPath::Combine(spritesFolderPath, alloc, inputPath, "sprites");
	udtPath::Combine(mapsFolderPath, alloc, inputPath, "maps");
	udtPath::Combine(spritesOutputPath, alloc, outputPath, "sprites.texturepack");

	SpritePackGenerator atlasGenerator;
	atlasGenerator.ProcessFolder(spritesFolderPath, spritesOutputPath);
	MapTextConverter mapPackager;
	mapPackager.ProcessFolder(mapsFolderPath, outputPath);

	udtShutDownLibrary();

	return 0;
}
