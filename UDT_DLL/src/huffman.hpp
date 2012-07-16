#pragma once


#include "message.hpp"


/* This is based on the Adaptive Huffman algorithm described in Sayood's Data
 * Compression book.  The ranks are not actually stored, but implicitly defined
 * by the location of a node within a doubly-linked list */

#define HUFF_MAX 256 /* Maximum symbol */

typedef struct nodetype {
	struct	nodetype *left, *right, *parent; /* tree structure */ 
	struct	nodetype *next, *prev; /* doubly-linked list */
	struct	nodetype **head; /* highest ranked node in block */
	int		weight;
	int		symbol;
} node_t;

typedef struct {
	int			blocNode;
	int			blocPtrs;

	node_t*		tree;
	node_t*		lhead;
	node_t*		ltail;
	node_t*		loc[HUFF_MAX+1];
	node_t**	freelist;

	node_t		nodeList[768];
	node_t*		nodePtrs[768];
} huff_t;

typedef struct {
	huff_t		compressor;
	huff_t		decompressor;
} huffman_t;

void	Huff_Compress(msg_t *buf, int offset);
void	Huff_Decompress(msg_t *buf, int offset);
void	Huff_Init(huffman_t *huff);
void	Huff_addRef(huff_t* huff, byte ch);
int		Huff_Receive (node_t *node, int *ch, byte *fin);
void	Huff_transmit (huff_t *huff, int ch, byte *fout);
void	Huff_offsetReceive (node_t *node, int *ch, byte *fin, int *offset);
void	Huff_offsetTransmit (huff_t *huff, int ch, byte *fout, int *offset);
void	Huff_putBit( int bit, byte *fout, int *offset);
int		Huff_getBit( byte *fout, int *offset);

#define SV_ENCODE_START		4
#define CL_ENCODE_START		12
#define SV_DECODE_START		CL_ENCODE_START
#define CL_DECODE_START		SV_ENCODE_START