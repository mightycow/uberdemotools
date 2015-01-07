#include "message.hpp"

#include <stdio.h>
#include <stdlib.h>


#pragma warning(disable: 4996)


void ReadFile(u8*& data, s32& byteCount, const char* filePath)
{
	FILE* file = fopen(filePath, "rb");
	fseek(file, 0, SEEK_END);
	byteCount = ftell(file);
	fseek(file, 0, SEEK_SET);
	data = (u8*)malloc(byteCount);
	fread(data, byteCount, 1, file);
	fclose(file);
}

void idHuffman(u8* data, s32 byteCount)
{
	// File start:
	// a) message sequence (4 bytes signed int)
	// b) message length (4 bytes signed int)
	// c) Huffman coded sequence acknowledge

	const s32 messageSequence = *(s32*)(data + 0);
	printf("Message Sequence: %d\n", (int)messageSequence);

	udtMessage message;
	//message.PrintDecoderTree();
	message.Init(data + 8, byteCount - 8);
	message.Buffer.cursize = *(s32*)(data + 4);
	message.Buffer.readcount = 0;
	printf("Encoded message length: %d bytes\n", (int)message.Buffer.cursize);

	message.Bitstream();
	//__debugbreak();
	const s32 sequenceAcknowledge = message.ReadLong();
	printf("Sequence Acknowledge: %d\n", (int)sequenceAcknowledge);

	system("pause");
}


int main()
{
	s32 byteCount = 0;
	u8* data = NULL;
	ReadFile(data, byteCount, "11785_151.dm_68");
	idHuffman(data, byteCount);

	return 0;
}