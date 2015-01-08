#include "huffman.hpp"

#include <string.h>


/* This is based on the Adaptive Huffman algorithm described in Sayood's Data
 * Compression book.  The ranks are not actually stored, but implicitly defined
 * by the location of a node within a doubly-linked list */

#define NYT HUFF_MAX					/* NYT = Not Yet Transmitted */
#define INTERNAL_NODE (HUFF_MAX+1)


void	udtHuffman::PutBit( s32 bit, u8 *fout, s32 *offset) {
	_bloc = *offset;
	if ((_bloc&7) == 0) {
		fout[(_bloc>>3)] = 0;
	}
	fout[(_bloc>>3)] |= bit << (_bloc&7);
	_bloc++;
	*offset = _bloc;
}

s32		udtHuffman::GetBit( u8 *fin, s32 *offset) {
	s32 t;
	_bloc = *offset;
	t = (fin[(_bloc>>3)] >> (_bloc&7)) & 0x1;
	_bloc++;
	*offset = _bloc;
	return t;
}

s32	udtHuffman::GetBloc()
{
	return _bloc;
}

void udtHuffman::SetBloc(s32 bloc)
{
	_bloc = bloc;
}

/* Add a bit to the output file (buffered) */
static UDT_FORCE_INLINE void add_bit (s32& bloc, s8 bit, u8 *fout) {
	if ((bloc&7) == 0) {
		fout[(bloc>>3)] = 0;
	}
	fout[(bloc>>3)] |= bit << (bloc&7);
	bloc++;
}

/* Receive one bit from the input file (buffered) */
static UDT_FORCE_INLINE s32 get_bit(s32& bloc, u8 *fin)
{
	s32 t;
	t = (fin[(bloc>>3)] >> (bloc&7)) & 0x1;
	bloc++;
	return t;
}

static idHuffmanNode **get_ppnode(idHuffmanTree* huff) {
	idHuffmanNode **tppnode;
	if (!huff->freelist) {
		return &(huff->nodePtrs[huff->blocPtrs++]);
	} else {
		tppnode = huff->freelist;
		huff->freelist = (idHuffmanNode **)*tppnode;
		return tppnode;
	}
}

static void free_ppnode(idHuffmanTree* huff, idHuffmanNode **ppnode) {
	*ppnode = (idHuffmanNode *)huff->freelist;
	huff->freelist = ppnode;
}

/* Swap the location of these two nodes in the tree */
static void swap (idHuffmanTree* huff, idHuffmanNode *node1, idHuffmanNode *node2) { 
	idHuffmanNode *par1, *par2;

	par1 = node1->parent;
	par2 = node2->parent;

	if (par1) {
		if (par1->left == node1) {
			par1->left = node2;
		} else {
	      par1->right = node2;
		}
	} else {
		huff->tree = node2;
	}

	if (par2) {
		if (par2->left == node2) {
			par2->left = node1;
		} else {
			par2->right = node1;
		}
	} else {
		huff->tree = node1;
	}
  
	node1->parent = par2;
	node2->parent = par1;
}

/* Swap these two nodes in the linked list (update ranks) */
static void swaplist(idHuffmanNode *node1, idHuffmanNode *node2) {
	idHuffmanNode *par1;

	par1 = node1->next;
	node1->next = node2->next;
	node2->next = par1;

	par1 = node1->prev;
	node1->prev = node2->prev;
	node2->prev = par1;

	if (node1->next == node1) {
		node1->next = node2;
	}
	if (node2->next == node2) {
		node2->next = node1;
	}
	if (node1->next) {
		node1->next->prev = node1;
	}
	if (node2->next) {
		node2->next->prev = node2;
	}
	if (node1->prev) {
		node1->prev->next = node1;
	}
	if (node2->prev) {
		node2->prev->next = node2;
	}
}

/* Do the increments */
static void increment(idHuffmanTree* huff, idHuffmanNode *node) {
	idHuffmanNode *lnode;

	if (!node) {
		return;
	}

	if (node->next != NULL && node->next->weight == node->weight) {
	    lnode = *node->head;
		if (lnode != node->parent) {
			swap(huff, lnode, node);
		}
		swaplist(lnode, node);
	}
	if (node->prev && node->prev->weight == node->weight) {
		*node->head = node->prev;
	} else {
	    *node->head = NULL;
		free_ppnode(huff, node->head);
	}
	node->weight++;
	if (node->next && node->next->weight == node->weight) {
		node->head = node->next->head;
	} else { 
		node->head = get_ppnode(huff);
		*node->head = node;
	}
	if (node->parent) {
		increment(huff, node->parent);
		if (node->prev == node->parent) {
			swaplist(node, node->parent);
			if (*node->head == node) {
				*node->head = node->parent;
			}
		}
	}
}

void udtHuffman::AddRef(idHuffmanTree* huff, u8 ch) {
	idHuffmanNode *tnode, *tnode2;
	if (huff->loc[ch] == NULL) { /* if this is the first transmission of this node */
		tnode = &(huff->nodeList[huff->blocNode++]);
		tnode2 = &(huff->nodeList[huff->blocNode++]);

		tnode2->symbol = INTERNAL_NODE;
		tnode2->weight = 1;
		tnode2->next = huff->lhead->next;
		if (huff->lhead->next) {
			huff->lhead->next->prev = tnode2;
			if (huff->lhead->next->weight == 1) {
				tnode2->head = huff->lhead->next->head;
			} else {
				tnode2->head = get_ppnode(huff);
				*tnode2->head = tnode2;
			}
		} else {
			tnode2->head = get_ppnode(huff);
			*tnode2->head = tnode2;
		}
		huff->lhead->next = tnode2;
		tnode2->prev = huff->lhead;
 
		tnode->symbol = ch;
		tnode->weight = 1;
		tnode->next = huff->lhead->next;
		if (huff->lhead->next) {
			huff->lhead->next->prev = tnode;
			if (huff->lhead->next->weight == 1) {
				tnode->head = huff->lhead->next->head;
			} else {
				/* this should never happen */
				tnode->head = get_ppnode(huff);
				*tnode->head = tnode2;
		    }
		} else {
			/* this should never happen */
			tnode->head = get_ppnode(huff);
			*tnode->head = tnode;
		}
		huff->lhead->next = tnode;
		tnode->prev = huff->lhead;
		tnode->left = tnode->right = NULL;
 
		if (huff->lhead->parent) {
			if (huff->lhead->parent->left == huff->lhead) { /* lhead is guaranteed to by the NYT */
				huff->lhead->parent->left = tnode2;
			} else {
				huff->lhead->parent->right = tnode2;
			}
		} else {
			huff->tree = tnode2; 
		}
 
		tnode2->right = tnode;
		tnode2->left = huff->lhead;
 
		tnode2->parent = huff->lhead->parent;
		huff->lhead->parent = tnode->parent = tnode2;
     
		huff->loc[ch] = tnode;
 
		increment(huff, tnode2->parent);
	} else {
		increment(huff, huff->loc[ch]);
	}
}

/* Get a symbol */
void udtHuffman::OffsetReceive(idHuffmanNode *node, s32 *ch, u8 *fin, s32 *offset)
{
	_bloc = *offset;
	while(node && node->symbol == INTERNAL_NODE)
	{
		// myT: The following is equivalent to: node = get_bit(_bloc, fin) ? node->right : node->left
		// myT: It kills any possibility that the compiler might generate a conditional branch.
		const sptr bit = (sptr)get_bit(_bloc, fin); 
		const sptr mask = bit | (-bit);
		const sptr result = (((sptr)node->right ^ (sptr)node->left) & mask) ^ (sptr)node->left;
		node = (idHuffmanNode*)result;
	}

	if(!node)
	{
		*ch = 0;
		return;
	}

	*ch = node->symbol;
	*offset = _bloc;
}

/* Send the prefix code for this node */

static void send(s32& bloc, idHuffmanNode *node, idHuffmanNode *child, u8 *fout) 
{
	if(node->parent) 
	{
		send(bloc, node->parent, node, fout);
	}

	if(child)
	{
		// @myT: The ternary op helps the compiler avoid the conditional branch.
		add_bit(bloc, node->right == child ? 1 : 0, fout);
	}
}

void udtHuffman::OffsetTransmit (idHuffmanTree *huff, s32 ch, u8 *fout, s32 *offset) {
	_bloc = *offset;
	send(_bloc, huff->loc[ch], NULL, fout);
	*offset = _bloc;
}

void udtHuffman::Init(idHuffmanCodec *huff) 
{
	memset(&huff->compressor, 0, sizeof(idHuffmanTree));
	memset(&huff->decompressor, 0, sizeof(idHuffmanTree));

	// Initialize the tree & list with the NYT node 
	huff->decompressor.tree = huff->decompressor.lhead = huff->decompressor.ltail = huff->decompressor.loc[NYT] = &(huff->decompressor.nodeList[huff->decompressor.blocNode++]);
	huff->decompressor.tree->symbol = NYT;
	huff->decompressor.tree->weight = 0;
	huff->decompressor.lhead->next = huff->decompressor.lhead->prev = NULL;
	huff->decompressor.tree->parent = huff->decompressor.tree->left = huff->decompressor.tree->right = NULL;

	// Add the NYT (not yet transmitted) node s32o the tree/list */
	huff->compressor.tree = huff->compressor.lhead = huff->compressor.loc[NYT] =  &(huff->compressor.nodeList[huff->compressor.blocNode++]);
	huff->compressor.tree->symbol = NYT;
	huff->compressor.tree->weight = 0;
	huff->compressor.lhead->next = huff->compressor.lhead->prev = NULL;
	huff->compressor.tree->parent = huff->compressor.tree->left = huff->compressor.tree->right = NULL;
	huff->compressor.loc[NYT] = huff->compressor.tree;
}
