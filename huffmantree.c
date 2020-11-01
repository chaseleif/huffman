#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "common.h"

typedef struct node node;
typedef struct rootnoot rootnode;
typedef struct bottomnode bottomnode;
typedef uint8_t byte;
typedef struct ullbyte ullbyte;

static inline char *getpathstring(const unsigned int path,const unsigned int steps) {
	char *ret = (char*)malloc(sizeof(char)*(steps+1));
	ret[steps]='\0';
	for (int i=0;i<steps;++i) {
		ret[i] = ((path>>((steps-i)-1))&1)+'0';
	}
	return ret;
}

// go left is zero, go right is one, the string is derived from path and depth
static void generateencoding(node *root,const unsigned int path,const unsigned int steps) {
	if (root->vals>0) { // this node has at least one ullbyte
		// the left pointer will be a ullbyte
		((ullbyte*)root->left)->strval = getpathstring((path<<1),steps+1);
		if (root->vals==2) // this node has another ullbyte
			((ullbyte*)root->right)->strval = getpathstring((path<<1)+1,steps+1);
		else // the right side is a node
			generateencoding((node*)root->right,(path<<1)+1,steps+1);
	}
	else { // this is just a node pointing to nodes
		generateencoding((node*)root->left,(path<<1),steps+1);
		generateencoding((node*)root->right,(path<<1)+1,steps+1);
	}
}

// takes two ullbytes (in pairs), if there were only one new value then finalizehuffmantree would be called
// finds the minimum of (val1->count+val2->count) or val1->count + (some subtree->sum)
// either extends a subtree or creates a new subtree, rootslen is updated as needed
// returns the number of values consumed (2 or 1)
static unsigned char addtorootlist(node **roots,int *rootslen,ullbyte *val1,ullbyte *val2) {
	int mini=-1; // mini==-1 when we make a new tree
	unsigned long long minval=val1->count+val2->count; // minval will be the nodes sum
	for (int i=0;i<*rootslen;++i) {
		if (roots[i]->sum+val1->count<minval) {
			mini=i;
			minval=roots[i]->sum+val1->count;
		}
	}
	// adding the next two values is less than adding to a tree
	if (mini<0) {
		// add a new subtree
		roots[*rootslen] = (node*)malloc(sizeof(node));
		roots[*rootslen]->left = (void*)val1;
		roots[*rootslen]->right = (void*)val2;
		roots[*rootslen]->sum = minval;
		roots[*rootslen]->vals = 2; // this node points to 2 values
		*rootslen=*rootslen+1; //increase number of roots
		return 2; // added both vals as a new subtree
	}
	// extend the lowest root with val1, (val2 will not be used)
	node *newroot = (node*)malloc(sizeof(node));
	newroot->right=(void*)roots[mini];
	newroot->left = (void*)val1;
	newroot->sum = minval;
	newroot->vals = 1; // this node points to one value and another node
	roots[mini] = newroot;
	return 1; // added 1 val to an existing subtree
}
// finalizehuffmantree merges any subtrees into one tree, lastval may be NULL
// (optional: add lastval to smallest subtree) then, while there is more than one root -> merge the two smallest roots
static void finalizehuffmantree(node **roots,int *rootslen,ullbyte *lastval) {
	// if there is no final value then lastval==NULL
	if (lastval) {
		// insert lastval into the smallest subtree
		int mini=0;
		unsigned long long minval=roots[0]->sum;
		for (int i=1;i<*rootslen;++i) {
			if (roots[i]->sum<minval) {
				minval=roots[i]->sum;
				mini=i;
			}
		}
		node *newroot = (node*)malloc(sizeof(node));
		newroot->right = (void*)roots[mini];
		newroot->left = (void*)lastval;
		newroot->sum = lastval->count + minval;
		newroot->vals = 1; // this node points to one value and another node
		roots[mini] = newroot;
	}
	// merge all subtrees
	while (*rootslen>1) {
		int mini, min2i;
		unsigned long long minval, min2val;
		//set first two roots as the initial minimums
		if (roots[0]->sum > roots[1]->sum) {
			mini=1; min2i=0;
			minval=roots[1]->sum; min2val=roots[0]->sum;
		}
		else {
			mini=0; min2i=1;
			minval=roots[0]->sum; min2val=roots[1]->sum;
		}
		//get the actual minimum positions if there are at least 3 roots
		for (int i=2;i<*rootslen;++i) {
			if (roots[i]->sum<minval) {
				min2i=mini;
				min2val=minval;
				mini=i;
				minval=roots[i]->sum;
			}
			else if (roots[i]->sum<min2val) {
				min2i=i;
				min2val=roots[i]->sum;
			}
		}
		//the merged tree takes the leftmost position
		const int lefti = (mini<min2i)?mini:min2i;
		const int righti = (lefti==mini)?min2i:mini;
		node *newroot = (node*)malloc(sizeof(node));
		newroot->left = (void*)roots[mini];
		newroot->right = (void*)roots[min2i];
		newroot->vals = 0; // this node only points to other nodes, it does not point to any uul bytes
		roots[lefti] = newroot;
		*rootslen = *rootslen-1;
		//if righti < rootslen there exists an unmerged node at [rootslen] that needs to swap with [righti]
		if (righti<*rootslen) roots[righti] = roots[*rootslen];
	}
}
// destructor for the Huffman tree built using this file.
// Each node as the member .vals
// if vals==2, both left and right are ullbytes
// if vals==1, the left is a ullbyte and the right is a node
// if vals==0, left and right are nodes
static void freehuffmantree(node *root) {
	if (!root) return;
	if (root->vals==0) { // left and right are both nodes
		freehuffmantree((node*)root->left);
		freehuffmantree((node*)root->right);
		free(root);
	}
	else {
		// left is a ullbyte
		free(((ullbyte*)root->left)->strval);
		free(root->left);
		// right is a ullbyte
		if (root->vals==2) {
			free(((ullbyte*)root->right)->strval);
			free((node*)root->right);
		}
		// right is a node
		else freehuffmantree((node*)root->right);
		free(root);
	}
}
//print the huffman tree
static void printhuffmantree(node *root) {
	if (root->vals==0) {
		printhuffmantree((node*)root->left);
		printhuffmantree((node*)root->right);
	}
	else {
		printf("0x%.2x: %s\t(count=%llu)\n",((ullbyte*)root->left)->val,((ullbyte*)root->left)->strval,((ullbyte*)root->left)->count);
		if (root->vals==2)
			printf("0x%.2x: %s\t(count=%llu)\n",((ullbyte*)root->right)->val,((ullbyte*)root->right)->strval,((ullbyte*)root->right)->count);
		else printhuffmantree((node*)root->right);
	}
}
static void processinput(FILE *infile) {
	// buffer to read the file
	const int buffersize=4096;
	byte buffer[buffersize];
	// count the frequency of each byte
	unsigned long long frequencies[256]; //2^8, spot for each possible byte value
	memset(frequencies,0,sizeof(unsigned long long)*256);
	// read the file to get the frequency of bytes
	int i, uniquebytes=0;
	while ((i=fread(buffer,sizeof(byte),buffersize,infile))>0) {
		for (int x=0;x<i;++x) {
			if (!frequencies[buffer[x]]) ++uniquebytes;
			frequencies[buffer[x]]+=1;
		}
	}
	// build the minheap
	ullbyte **minheap = (ullbyte**)malloc(sizeof(ullbyte*)*uniquebytes);
	// for each element of frequency, add it to the minheap
	int x=0;
	i=0;
	while (i<256) {
		if (frequencies[i]) {
			minheap[x] = (ullbyte*)malloc(sizeof(ullbyte));
			minheap[x]->count=frequencies[i];
			minheap[x]->val=i;
			if (x) {
				int z=x;
				while (z && minheap[z>>1]->count>minheap[z]->count) {
					const int parent = z>>1;
					ullbyte swap;
					swap.count = minheap[parent]->count;
					swap.val = minheap[parent]->val;
					minheap[parent]->count = minheap[z]->count;
					minheap[parent]->val = minheap[z]->val;
					minheap[z]->count=swap.count;
					minheap[z]->val=swap.val;
					z=parent;
				}
			}
			if (++x==uniquebytes) break; // no more values, no reason to check remaining frequencies
		}
		++i;
	}
	// build a Huffman tree
	// we can have a maximum (if we only ever have 1 subtree) of ((uniquebytes+1)/2)+1 nodes
	node **roots = (node**)malloc(sizeof(node*)*(((uniquebytes+1)>>1)+1));
	int numroots=0;
	i=0;
	while (i<uniquebytes) {
		if (i+1<uniquebytes) //at least two values remaining
			i+=addtorootlist(roots,&numroots,minheap[i],minheap[i+1]);
		else { //last value
			finalizehuffmantree(roots,&numroots,minheap[i]);
			break;
		}
	}
	// if i==uniquebytes then the last elements were added as a pair, need to finalize the tree still
	if (i==uniquebytes) finalizehuffmantree(roots,&numroots,NULL);
	//don't need the heap anymore
	free(minheap);
	//the nodes are all in this tree
	node *huffmanroot = roots[0];
	//dont need the root list anymore
	free(roots);
	// calculate encodings by shifting the current path to the left and adding zero or one
	// actual encoding will be of length of the depth of the node, with leading zeroes filling this length
	// each ullbyte's string path is calculated using these two values
	// left is zero, right is one
	generateencoding(huffmanroot,0,0);
	printf("Generated the encoding\n");
	printhuffmantree(huffmanroot);
	freehuffmantree(huffmanroot);
}

//define in common.h for curses or plain utility
#ifndef _USECURSES
static inline void printusage(char *prog) {
	fprintf(stderr,"This utility compresses files using Huffman encoding\n\tUsage: %s infile outfile\n",prog);
}
int main(int argc, char **argv) {
	if (argc<3) {
		printusage(argv[0]);
		return 0;
	}
	FILE *infile = fopen(argv[1],"rb");
	if (!infile) {
		fprintf(stderr,"Unable to open input file \"%s\" !\n",argv[1]);
		printusage(argv[0]);
		return 0;
	}
	fseek(infile,0L,SEEK_END);
	const unsigned long long insize = ftell(infile);
	fseek(infile,0L,SEEK_SET);
	printf("Input file \"%s\" is %llu bytes\n",argv[1],insize);
	processinput(infile);
	fclose(infile);
	return 0;
}
#endif //_USECURSES


