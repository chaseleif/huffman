#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// structure and public function definitions
#include "common.h"

// size of the byte buffer used for file IO, two buffers work are in use at the same time during compression and decompression
#define BYTEBUFSIZE 4096

// whether to consider new huffman trees as part of the min heap during construction of the hfc tree
#define INCLUDETREESINMINHEAP

// for curses utility, define this to not immediately free the tree
//#define _DONTFREETREES

// the node and 8 bit unsigned int
// the node has { node*left,node*right,ull count,byte val,char* strval }
typedef struct node node;
typedef uint8_t byte;

// global pointer for ease of access
node *hfcroot=NULL;

// Static functions ***********

// called to get the number of bits needed to represent a number. (max return is 8)
static inline byte bitlength(int num) {
	byte ret = 0;
	while (num) {
		num>>=1;
		if (++ret==8) return ret;
	}
	return ret;
}

// returns a string from a path value and number of steps, handles leading zeroes
static inline char *getpathstring(const int path,const int steps) {
	char *ret = (char*)malloc(sizeof(char)*(steps+1));
	ret[steps]='\0';
	for (int i=0;i<steps;++i) {
		ret[i] = ((path>>((steps-i)-1))&1)+'0';
	}
	return ret;
}

// generate encoding, called after a huffman tree is created, sets the strval of value nodes
// go left is zero, go right is one, the string is derived from path taken
// returns the maximum depth (longest encoding)
static byte generateencoding(node *root,const int path,const int steps) {
	if (!root) return 0;
	if (!root->left) { // this is a leaf node
		root->strval = getpathstring(path,steps);
		return steps;
	}
	// this is an internal node
	byte leftdepth = generateencoding(root->left,(path<<1),steps+1);
	byte rightdepth = generateencoding(root->right,(path<<1)+1,steps+1);
	if (leftdepth>rightdepth) return leftdepth;
	return rightdepth;
}

// called after a root is swapped with the end, need to push the root down
// this just swaps values down until they are at the end or the heap is ok
static inline void pushdownminheap(node **hfcheap,const int lasti) {
	int z=0; // push the root down the heap
	while ((z<<1)+1<=lasti) { //while this node has a child
		const int childpos=(z<<1)+1; // first child
		if (childpos+1<=lasti) { // have two children
			if (hfcheap[z]->count<=hfcheap[childpos]->count &&
				hfcheap[z]->count<=hfcheap[childpos+1]->count) break; // this node is in the right spot
			if (hfcheap[childpos]->count<=hfcheap[childpos+1]->count) { // the right child has a larger count.
				node *swap = hfcheap[childpos];
				hfcheap[childpos]=hfcheap[z];
				hfcheap[z]=swap;
				z=childpos;
			}
			else { // the right child is smaller than the left
				node *swap = hfcheap[childpos+1];
				hfcheap[childpos+1]=hfcheap[z];
				hfcheap[z]=swap;
				z=childpos+1;
			}
		}
		else { // only have one child
			if (hfcheap[z]->count<=hfcheap[childpos]->count) break; // the right spot
			node *swap=hfcheap[childpos];
			hfcheap[childpos]=hfcheap[z];
			hfcheap[z]=swap;
			z=childpos;
		}
	}
}

// takes two nodes (in pairs), if there were only one new value then finalizehuffmantree would be called
// finds the minimum of (val1->count+val2->count) or val1->count + (some subtree->count)
// either extends a subtree or creates a new subtree, rootslen is updated as needed
// returns the number of nodes consumed (2 or 1)
static byte addtorootlist(node **roots,int *rootslen,node *val1,node *val2) {
	int mini=-1;
	unsigned long long minval;
	// this define option skips readding new tree nodes to the min heap
#ifndef INCLUDETREESINMINHEAP
		mini=-1; // mini==-1 when we make a new tree
		minval=val1->count+val2->count; // minval will be the nodes sum
		// check if adding val1 to another subtree is less than adding val1 to val2, is so we do that
		for (int i=0;i<*rootslen;++i) {
			if (roots[i]->count+val1->count<minval) {
				mini=i;
				minval=roots[i]->count+val1->count;
			}
		}
// The logic below acts as if nodes were 'reinserted' into the min heap
// Nodes with smaller values become the next candidate to either merge with val1 or merge with another node.
#else // ifdef INCLUDETREESINMINHEAP
		// we have exactly one tree
		if (*rootslen==1) {
			// adding the two nodes is less than the adding to the existing tree
			if (val2->count<roots[0]->count) {
				mini=-1;
				minval=val1->count+val2->count;
			}
			// add val1 to the existing tree
			else {
				mini=0;
				minval=val1->count+roots[0]->count;
			}
		}
		// we have at least 2 trees
		else if (*rootslen) {
			// check the count of the existing roots until we merge val1 somewhere
			while (1) {
				// if we keep merging trees we end up with 1 root left
				// add val1 to tree or make a new tree with val1 and val2
				if (*rootslen==1) {
					if (val2->count<roots[0]->count) {
						mini=-1;
						minval=val1->count+val2->count;
					}
					else {
						mini=0;
						minval=val1->count+roots[0]->count;
					}
					break;
				}
				int min2i;
				unsigned long long min2val;
				// find lowest subtree value, first set the positions using the first two trees
				if (roots[0]->count<=roots[1]->count) {
					mini=0;
					minval=roots[0]->count;
					min2i = 1;
					min2val = roots[1]->count;
				}
				else {
					mini=1;
					minval=roots[1]->count;
					min2i = 0;
					min2val = roots[0]->count;
				}
				if (*rootslen>2) {
					// iterate from the third tree to the end
					for (int i=2;i<*rootslen;++i) {
						if (roots[i]->count<minval) {
							min2i = mini;
							min2val = minval;
							mini=i;
							minval=roots[i]->count;
						}
						else if (roots[i]->count<min2val) {
							min2val = roots[i]->count;
							min2i = i;
						}
					}
				}
				// a subtree's value is less than val1
				if (minval<val1->count) {
					// the second tree count is higher than the val1, add the val1 to the first min tree (mini is set)
					if (min2val>=val1->count) {
						minval+=val1->count; // minval was set to the first tree's count, add val1's count and break
						break;
					}
					// both of these trees are less than the first value's count, merge these trees and restart
					node *newroot = (node*)malloc(sizeof(node));
					const int lefti = (mini<min2i)?mini:min2i;
					const int righti = (lefti==mini)?min2i:mini;
					newroot->left = roots[mini];
					newroot->right = roots[min2i];
					newroot->strval=NULL;
					newroot->count = minval+min2val;
					roots[lefti] = newroot; // take over the left-most spot in the array
					*rootslen = *rootslen-1; // decrease the array length
					//if righti < rootslen there exists an unmerged node at [rootslen] that needs to swap with [righti]
					if (righti<*rootslen) roots[righti] = roots[*rootslen];
				}
				// a subtree's value is less than val2, add val1 to the subtree
				else if (minval<val2->count) {
					minval+=val1->count;
					break;
				}
				// no tree was less than val1
				else {
					mini=-1;
					minval=val1->count+val2->count;
					break;
				}
			}
		}
		else {
			mini=-1;
			minval=val1->count+val2->count;
		}
#endif // INCLUDETREESINMINHEAP
	// adding the next two values is less than adding to a tree
	if (mini<0) {
		// add a new subtree
		roots[*rootslen] = (node*)malloc(sizeof(node));
		roots[*rootslen]->left = val1;
		roots[*rootslen]->right = val2;
		roots[*rootslen]->strval=NULL;
		roots[*rootslen]->count = minval;
		*rootslen=*rootslen+1; //increase number of roots
		return 2; // added both vals as a new subtree
	}
	// extend the lowest root with val1, (val2 will not be used)
	node *newroot = (node*)malloc(sizeof(node));
	newroot->right = roots[mini];
	newroot->left = val1;
	newroot->strval=NULL;
	newroot->count = minval;
	roots[mini] = newroot;
	return 1; // added 1 val to an existing subtree
}

// finalizehuffmantree merges any subtrees into one tree, lastval may be NULL
// (optionally add lastval to smallest subtree)
// while there is more than one root -> merge the two smallest roots, all roots condense to roots[0]
static void finalizehuffmantree(node **roots,int *rootslen,node *lastval) {
	// if there is no final value then lastval==NULL
	if (lastval) {
		// insert lastval into the smallest subtree
		int mini=0;
		unsigned long long minval=roots[0]->count;
		for (int i=1;i<*rootslen;++i) {
			if (roots[i]->count<minval) {
				minval=roots[i]->count;
				mini=i;
			}
		}
		node *newroot = (node*)malloc(sizeof(node));
		newroot->right = roots[mini];
		newroot->left = lastval;
		newroot->strval=NULL;
		newroot->count = lastval->count + minval;
		roots[mini] = newroot;
	}
	// merge all subtrees
	while (*rootslen>1) {
		int mini, min2i;
		unsigned long long minval, min2val;
		//set first two roots as the initial minimums
		if (roots[0]->count > roots[1]->count) {
			mini=1; min2i=0;
			minval=roots[1]->count; min2val=roots[0]->count;
		}
		else {
			mini=0; min2i=1;
			minval=roots[0]->count; min2val=roots[1]->count;
		}
		//get the actual minimum positions if there are at least 3 roots
		for (int i=2;i<*rootslen;++i) {
			if (roots[i]->count<minval) {
				min2i=mini;
				min2val=minval;
				mini=i;
				minval=roots[i]->count;
			}
			else if (roots[i]->count<min2val) {
				min2i=i;
				min2val=roots[i]->count;
			}
		}
		//the merged tree takes the leftmost position
		const int lefti = (mini<min2i)?mini:min2i;
		const int righti = (lefti==mini)?min2i:mini;
		node *newroot = (node*)malloc(sizeof(node));
		newroot->left = roots[mini];
		newroot->right = roots[min2i];
		newroot->strval=NULL;
		newroot->count = roots[mini]->count+roots[min2i]->count;
		roots[lefti] = newroot;
		*rootslen = *rootslen-1;
		//if righti < rootslen there exists an unmerged node at [rootslen] that needs to swap with [righti]
		if (righti<*rootslen) roots[righti] = roots[*rootslen];
	}
}

// this takes an insertnode as input that has a strval
// the insertnode is inserted from the root using the path in its strval
static void recreatehuffmantree(node *insertnode) {
	node *trav = hfcroot;
	int i=0;
	// just find the node's spot, creating anything along the way
	while (insertnode->strval[i+1]!='\0') {
		if (insertnode->strval[i]=='1') {
			if (!trav->right) {
				node *newnode = (node*)malloc(sizeof(node));
				newnode->right=NULL;
				newnode->left=NULL;
				newnode->strval=NULL;
				newnode->count=0;
				trav->right=newnode;
			}
			trav=trav->right;
		}
		else {
			if (!trav->left) {
				node *newnode = (node*)malloc(sizeof(node));
				newnode->right=NULL;
				newnode->left=NULL;
				newnode->strval=NULL;
				newnode->count=0;
				trav->left=newnode;
			}
			trav=trav->left;
		}
		++i;
	}
	insertnode->right=NULL;
	insertnode->left=NULL;
	if (insertnode->strval[i]=='1')
		trav->right = insertnode;
	else
		trav->left = insertnode;
}

// Shared functions ***********

// destructor for the Huffman tree built using this file.
void freehuffmantree(node *root) {
	if (!root) return;
	if (root->strval) // this is a leaf
		free(root->strval);
	else { // this is an internal node
		freehuffmantree(root->left);
		freehuffmantree(root->right);
	}
	free(root);
}

// decompress infile and write outfile
void dorestore(FILE *infile,FILE *outfile) {
	// in buffer
	byte buffer[BYTEBUFSIZE];
	// fill the buffer to get initial values
	int ret = fread(buffer,sizeof(byte),BYTEBUFSIZE,infile);
	if (ret<8) {
		fprintf(stderr,"\n###\n# huffmantree.c: dorestore, initial input file read returned %d !\n###\nError reading from input file stream\n",ret);
		return;
	}
	// first byte = count of unique bytes, <=256 (count from zero as one for this first count)
	// (global int so it can be returned for visualizations)
	int uniquebytes = ((unsigned int)buffer[0])+1;
	if (uniquebytes>256 || uniquebytes < 1) {
		fprintf(stderr,"\n###\n# huffmantree.c: dorestore, bytes sanity check failed!\n###\nThe input file was not created using this utility or something bad happened\nCannot decompress this file.\n");
		return;
	}
	// second byte is maximum depth, it is the current depth of values we read
	unsigned int depth = buffer[1];
	if (!depth || depth>uniquebytes) {
		fprintf(stderr,"\n###\n# huffmantree.c: dorestore, depth sanity check failed!\n###\nThe input file was not created using this utility or something bad happened\nCannot decompress this file.\n");
		return;
	}
	// third byte is the final shmt, if this is 7 all bits will be the input data
	// if the final shmt is less than 7, we only have (7-finalshmt) bits in the last byte
	byte finalshmt = buffer[2];
	if (finalshmt>7) {
		fprintf(stderr,"\n###\n# huffmantree.c: dorestore, shmt sanity check failed!\n###\nThe input file was not created using this utility or something bad happened\nCannot decompress this file.\n");
		return;
	}
	// one iterator for byte and bit position for each buffer
	int bufferpos=3;
	int bitshmt=7;
	// make a new root
	hfcroot = (node*)malloc(sizeof(node));
	hfcroot->right=NULL;
	hfcroot->left=NULL;
	hfcroot->strval=NULL;
	hfcroot->count=0;
	// read the encodings from the file, creating /inserting nodes into the tree
	int bytesremaining=uniquebytes;
	while (bytesremaining>0) {
		// get the number of bytes in the next group, all of each group have the same length path
		int groupsize=0;
		int groupbits = bitlength(bytesremaining);
		for (int x=groupbits-1;x>=0;--x) {
			groupsize|=((buffer[bufferpos]>>bitshmt)&1)<<x;
			if (--bitshmt<0) {
				if (++bufferpos>=ret) {
					ret=fread(buffer,sizeof(byte),BYTEBUFSIZE,infile);
					bufferpos=0;
				}
				bitshmt=7;
			}
		}
		// for each byte in this depth group
		for (int x=0;x<groupsize;++x) {
			node *newnode=(node*)malloc(sizeof(node));
			newnode->val=0;
			newnode->count=0;
			// get the 8 bit value.
			for (int i=7;i>=0;--i) {
				newnode->val|=((buffer[bufferpos]>>bitshmt)&1)<<i;
				if (--bitshmt<0) {
					if (++bufferpos>=ret) {
						ret=fread(buffer,sizeof(byte),BYTEBUFSIZE,infile);
						bufferpos=0;
					}
					bitshmt=7;
				}
			}
			// get the path
			newnode->strval = (char*)malloc(sizeof(char)*(depth+1));
			newnode->strval[depth]='\0';
			for (int i=0;i<depth;++i) {
				newnode->strval[i]=((buffer[bufferpos]>>bitshmt)&1)+'0';
				if (--bitshmt<0) {
					if (++bufferpos>=ret) {
						ret=fread(buffer,sizeof(byte),BYTEBUFSIZE,infile);
						bufferpos=0;
					}
					bitshmt=7;
				}
			}
			// add the newnode to the hfcroot, uses the newnode->strval to place it in its path
			recreatehuffmantree(newnode);
		}
		// reduce the remaining bytes by number processed and reduce the depth
		// the bits needed for metadata decrease with the bytes remaining
		bytesremaining-=groupsize;
		--depth;
	}
	// hfcroot is now the same huffman tree the file was compressed with (minus the count)
	// the remainder of the file is data, read the file and traverse the tree to replace the values.
	byte writebuffer[BYTEBUFSIZE];
	int writepos=0;
	// while we read the file
	while (ret) {
		// set a pointer to the root
		node *trav = hfcroot;
		// this loop breaks when we are at a value node
		while (!trav->strval) {
			// move the pointer according to the input bit
			if ((buffer[bufferpos]>>bitshmt)&1) trav=trav->right;
			else trav=trav->left;
			if (--bitshmt<0) {
				if (++bufferpos>=ret) {
					ret=fread(buffer,sizeof(byte),BYTEBUFSIZE,infile);
					if (!ret)
						break;
					bufferpos=0;
				}
				bitshmt=7;
			}
		}
		// we have arrived at a leaf node, put its value into the write buffer
		if (!trav->left) {
			writebuffer[writepos++]=trav->val;
			if (writepos==BYTEBUFSIZE) {
				fwrite(writebuffer,sizeof(byte),writepos,outfile);
				fflush(outfile);
				writepos=0;
			}
		}
		// must handle bit alignment at the final byte
		if (bufferpos==ret-1) {
			if (finalshmt<7 && feof(infile)) { // this is the file's last byte and it isn't a 'full' byte
				if (bitshmt==finalshmt) break; // we have read all of the bits of the encoding
			}
		}
	}
	if (writepos) { // clear the write buffer
		fwrite(writebuffer,sizeof(byte),writepos,outfile);
		fflush(outfile);
	}
//for curses visualization
#ifndef _DONTFREETREES
	freehuffmantree(hfcroot);
#endif
}

// function to do the compression, pass in/out file pointers
void docompress(FILE *infile,FILE *outfile) {
	// buffer to read the file
	byte buffer[BYTEBUFSIZE];

	// count the frequency of each 8-bit byte
	unsigned long long frequencies[256]; //2^8, spot for each possible byte value
	memset(frequencies,0,sizeof(unsigned long long)*256);
	// read the file to get the frequency of bytes
	int i, x;
	int uniquebytes=0; // global uniquebytes, curses uses this

	//fill out the frequencies array
	while ((i=fread(buffer,sizeof(byte),BYTEBUFSIZE,infile))>0) {
		for (x=0;x<i;++x) {
			if (!frequencies[buffer[x]]) ++uniquebytes;
			++frequencies[buffer[x]];
		}
	}
	fseek(infile,0L,SEEK_SET); //rewind file

	// build the hfcheap
	node **hfcheap = (node**)malloc(sizeof(node*)*uniquebytes);
	// a side array to keep a static reference of every ullbyte so we don't need to search
	node *ullbytes[256];
	// for each element of frequency, add it to the hfcheap
	// creating nodes in reverse order so each addition is a new 'root' with the lowest index
	x=uniquebytes-1;
	i=0;
	while (i<256) {
		if (frequencies[i]) {
			// adding new elements from right to left in the heap, pushing each new index down as needed
			hfcheap[x] = (node*)malloc(sizeof(node));
			hfcheap[x]->count=frequencies[i];
			hfcheap[x]->val=i;
			// initialize tree node vals
			hfcheap[x]->left=NULL; hfcheap[x]->right=NULL; hfcheap[x]->strval=NULL;
			ullbytes[i] = hfcheap[x]; //ullbytes[] can be referenced by actual value, used for direct indexing in compression
			pushdownminheap(hfcheap+x,uniquebytes-1-x);
			if (--x<0) break; // no more unique values
		}
		++i;
	}
	// build a Huffman tree with a dynamic array of disjoint trees
	// we can have a maximum (if we only ever have 1 subtree) of ((uniquebytes+1)/2)+1 roots
	if (uniquebytes>1) {
		node **roots = (node**)malloc(sizeof(node*)*(((uniquebytes+1)>>1)+1));
		int numroots=0;
		i=0; // i is a processed element counter
		// popped nodes are swapped with the last element and the length of the array decremented
		// new roots are pushed down
		x=uniquebytes-1; // the last index, x moves toward zero
		while (i<uniquebytes) {
			if (i+1==uniquebytes) { // the last value
				finalizehuffmantree(roots,&numroots,hfcheap[0]);
				break;
			}
			if (i+2==uniquebytes) { // two values
				if (addtorootlist(roots,&numroots,hfcheap[0],hfcheap[1])==1)
					finalizehuffmantree(roots,&numroots,hfcheap[1]);
				else finalizehuffmantree(roots,&numroots,NULL);
				break;
			}
			node *val1=hfcheap[0]; // the first value is the root
			node *swap = hfcheap[0]; // swap the root with the end, reduce array size
			hfcheap[0]=hfcheap[x];
			hfcheap[x]=swap;
			pushdownminheap(hfcheap,--x); // check the new root, we've taken the old 'root' out of bounds
			node *val2=hfcheap[0]; // the new root may not be consumed
			// we made a new subtree
			if (addtorootlist(roots,&numroots,val1,val2)==2) {
				// used two values, get new root ready for the next iteration
				swap = hfcheap[0];
				hfcheap[0]=hfcheap[x];
				hfcheap[x]=swap;
				pushdownminheap(hfcheap,--x);
				i+=2;
			}
			else ++i; // only took the first root, the current root is ready for the next iteration
		}
		// finalizehuffmantree put all of the roots together in this tree
		hfcroot = roots[0];
		// dont need the root list anymore
		free(roots);
	}
	else {
		hfcroot = (node*)malloc(sizeof(node));
		hfcroot->count=hfcheap[0]->count;
		hfcroot->right=NULL; hfcroot->strval=NULL;
		hfcroot->left=hfcheap[0];
	}

	// actual encoding will be of length of the depth of the node, with leading zeroes filling this length
	// left is zero, right is one
	// the maximum depth is returned
	byte maxstringlength = generateencoding(hfcroot,0,0);
	// precalculate the number of bits needed for the final shmt for metadata
	// need the tree to have been created
	i=-1;
	int finalbitshmt=0; // we also need the metadata shmt below
	while (++i<256) {
		if (frequencies[i]) {
			finalbitshmt += 8 - (((int)(((unsigned long long)strlen(ullbytes[i]->strval))*ullbytes[i]->count)) & 7);
			if (finalbitshmt>7) finalbitshmt-=8;
		}
	}

	// write decoding information to front of output file
	byte writebuffer[BYTEBUFSIZE];
	// first byte = count of unique bytes, <=256 (count from zero as one so 256 can fit into 255)
	writebuffer[0] = uniquebytes-1;
	// second byte = the maxdepth (maximum encoding string length)
	writebuffer[1] = maxstringlength;
	// third byte set just below, before compression starts

	// for the rest of the metadata
	// the (byte+encoding) will now be grouped according to encoding length from maxlength, decrementing by 1
	// each group will use only the number of bits for remaining size to indicate group count
	// each val will have 1 byte followed by strlen bits
	int bytesremaining=uniquebytes;
	byte groupbitsneeded = bitlength(bytesremaining);
	// write group information in front of group data, keep a separate iterator and shmt for the group size spot
	// 7 is first bit shmt, 0 is last
	int groupbitshmt=7; // start shmt for groupsize. shmt is left shift with LOR into destination position. (decrementing to walk right) [7,6,5..,0]
	int groupposstart=3; // pos i for groupsize
	writebuffer[3]=0; // zero out buffers as we increment iterators
	// actual write position, starts (needed bits) behind the group size position
	int bufferpos=groupposstart; // track position for write buffer
	// push the bitshmt away from the groupshmt
	int bitshmt=groupbitshmt-groupbitsneeded; //bit position within bufferpos, where the group will be - the group's bits
	if (bitshmt<0) { bitshmt+=8; writebuffer[++bufferpos]=0; }
	// for all unique bytes
	while (bytesremaining>0) {
		int groupcount = 0; // worst case the group has 256
		//for each byte + encoding . . .
		for (i=0;i<uniquebytes;++i) {
			// group by lengths/level
			if (strlen(hfcheap[i]->strval)!=maxstringlength) continue;
			++groupcount;
			// store the original byte value
			for (x=7;x>=0;--x) { // x is val pos shmt
				writebuffer[bufferpos] |= (((hfcheap[i]->val)>>x)&1)<<bitshmt;
				if (--bitshmt<0) {
					bitshmt=7;
					writebuffer[++bufferpos]=0;
				}
			}
			// store the encoding string
			for (x=0;hfcheap[i]->strval[x]!='\0';++x) { // x is string i
				writebuffer[bufferpos] |= (hfcheap[i]->strval[x]-'0')<<bitshmt;
				if (--bitshmt<0) {
					bitshmt=7;
					writebuffer[++bufferpos]=0;
				}
			}
		}
		// store the group size before the group's data, prepare this step for the next group
		for (x=groupbitsneeded-1;x>=0;--x) { // x is groupsize shmt
			writebuffer[groupposstart] |= ((groupcount>>x)&1)<<groupbitshmt;
			if (--groupbitshmt<0) {
				groupbitshmt=7;
				++groupposstart;
			}
		}
		// get next maximum groupsize
		bytesremaining-=groupcount;
		// figure out maximum bits needed
		groupbitsneeded=bitlength(bytesremaining);
		// next group start is the current buffer position
		groupposstart=bufferpos;
		groupbitshmt=bitshmt;
		// shift the buffer position after the group start + group bits
		bufferpos=groupposstart;
		bitshmt-=groupbitsneeded;
		if (bitshmt<0) { bitshmt+=8; writebuffer[++bufferpos]=0; }
		--maxstringlength; // decrease next depth group
	}
	// third byte is reserved for recording the final shmt amount, this covers cases where the last byte is not full
	finalbitshmt += bitshmt;
	if (finalbitshmt>7) finalbitshmt-=8;
	writebuffer[2]=finalbitshmt;
	// don't need the heap anymore
	free(hfcheap);
	// read the input file, use the bytes as keys for new encodings and write the output
	while ((i=fread(buffer,sizeof(byte),BYTEBUFSIZE,infile))>0) {
		for (x=0;x<i;++x) {
			byte flushbuffer = (bufferpos+16>=BYTEBUFSIZE)?1:0; //flush the write buffer at the next byte alignment
			for (int z=0;ullbytes[buffer[x]]->strval[z]!='\0';++z) {
				writebuffer[bufferpos] |= (ullbytes[buffer[x]]->strval[z]-'0')<<bitshmt;
				if (--bitshmt<0) {
					bitshmt=7;
					++bufferpos;
					if (flushbuffer) {
						flushbuffer=0;
						fwrite(writebuffer,sizeof(byte),bufferpos,outfile);
						fflush(outfile);
						bufferpos=0;
					}
					writebuffer[bufferpos]=0;
				}
			}
		}
	}
	// write the final bits
	if (bitshmt<7 || bufferpos) {
		if (bitshmt<7) fwrite(writebuffer,sizeof(byte),bufferpos+1,outfile);
		else if (bufferpos) fwrite(writebuffer,sizeof(byte),bufferpos,outfile);
		fflush(outfile);
	}
	// the third byte of the compressed file indicates how many bits to read from the last byte
	// pre-calculating this now
//	writebuffer[0]=bitshmt;
//	fseek(outfile,2,SEEK_SET);
//	fwrite(writebuffer,sizeof(byte),1,outfile);
//	fflush(outfile);

//for curses visualization
#ifndef _DONTFREETREES
	freehuffmantree(hfcroot);
#endif
}

