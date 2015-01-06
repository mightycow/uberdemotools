#pragma once


#include "types.hpp"


/*
This is based on the Adaptive Huffman algorithm described in Sayood's Data
Compression book. The ranks are not actually stored, but implicitly defined
by the location of a node within a doubly-linked list.
*/

#define HUFF_MAX 256 // Maximum symbol

struct idHuffmanNode;
struct idHuffmanNode 
{
	idHuffmanNode *left, *right, *parent; // tree structure
	idHuffmanNode *next, *prev; // doubly-linked list
	idHuffmanNode **head; // highest ranked node in block
	s32		weight;
	s32		symbol;
};

struct idHuffmanTree
{
	s32			blocNode;
	s32			blocPtrs;

	idHuffmanNode*		tree;
	idHuffmanNode*		lhead;
	idHuffmanNode*		ltail;
	idHuffmanNode*		loc[HUFF_MAX+1];
	idHuffmanNode**		freelist;

	idHuffmanNode		nodeList[768];
	idHuffmanNode*		nodePtrs[768];
};

struct idHuffmanCodec
{
	idHuffmanTree		compressor;
	idHuffmanTree		decompressor;
};

struct udtHuffman
{
public:
	udtHuffman() { _bloc = 0; }

	void	Init(idHuffmanCodec *huff);
	void	AddRef(idHuffmanTree* huff, u8 ch);
	void	OffsetReceive (idHuffmanNode *node, s32 *ch, u8 *fin, s32 *offset);
	void	OffsetTransmit (idHuffmanTree *huff, s32 ch, u8 *fout, s32 *offset);
	void	PutBit( s32 bit, u8 *fout, s32 *offset);
	s32		GetBit( u8 *fout, s32 *offset);
	s32		GetBloc();
	void	SetBloc(s32 bloc);

private:
	s32		_bloc;
};