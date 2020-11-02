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

unsigned char maxstringlength=0; // could make the generateencoding return this

static inline char *getpathstring(const unsigned int path,const unsigned int steps) {
	if (steps>maxstringlength) maxstringlength=steps;
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
		((ullbyte*)root->left)->depth = steps+1;
		if (root->vals==2) { // this node has a right ullbyte
			((ullbyte*)root->right)->strval = getpathstring((path<<1)+1,steps+1);
			((ullbyte*)root->right)->depth = steps+1;
		}
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
/*
Algorithm descriptions I have seen say to add nodes back to the queue as they area created
The code below accomplishes this logic, though the resulting file in my test cases was larger doing this
	int mini;
	unsigned long long minval;
	if (*rootslen>1) {
		while (1) {
			mini=0;
			minval=roots[0]->sum;
			for (int i=1;i<*rootslen;++i) {
				if (roots[i]->sum<minval) {
					mini=i;
					minval=roots[i]->sum;
				}
			}
			//a subtree's value is less than val1, merge the subtrees first
			if (minval<val1->count) {
				int min2i=(mini==0)?1:0;
				unsigned long long min2val = roots[min2i]->sum;
				for (int i=0;i<*rootslen;++i) {
					if (i==mini || i==min2i) continue;
					if (roots[i]->sum<min2val) {
						min2i=i;
						min2val=roots[i]->sum;
					}
				}
				node *newroot = (node*)malloc(sizeof(node));
				const int lefti = (mini<min2i)?mini:min2i;
				const int righti = (lefti==mini)?min2i:mini;
				newroot->left = (void*)roots[mini];
				newroot->right = (void*)roots[min2i];
				newroot->sum = roots[mini]->sum+roots[min2i]->sum;
				newroot->vals = 0; // this node only points to other nodes, it does not point to any uul bytes
				roots[lefti] = newroot;
				*rootslen = *rootslen-1;
				//if righti < rootslen there exists an unmerged node at [rootslen] that needs to swap with [righti]
				if (righti<*rootslen) roots[righti] = roots[*rootslen];
				if (*rootslen==1) {
					mini=-1;
					minval=val1->count+val2->count;
					break;
				}
			}
			else {
				if (minval+val1->count>val1->count+val2->count) {
					mini=-1;
					minval=val1->count+val2->count;
				}
				else minval+=val1->count;
				break;
			}
		}
	}
	else if (*rootslen==1) {
		if (val2->count<roots[0]->sum) {
			mini=-1;
			minval=val1->count+val2->count;
		}
		else {
			mini=0;
			minval=val1->count+roots[0]->sum;
		}
	}
	else {
		mini=-1;
		minval=val1->count+val2->count;
	}
*/
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
		newroot->sum = roots[mini]->sum+roots[min2i]->sum;
		newroot->vals = 0; // this node only points to other nodes, it does not point to any uul bytes
		roots[lefti] = newroot;
		*rootslen = *rootslen-1;
		//if righti < rootslen there exists an unmerged node at [rootslen] that needs to swap with [righti]
		if (righti<*rootslen) roots[righti] = roots[*rootslen];
	}
}
// destructor for the Huffman tree built using this file.
// Each node has the member .vals
// if vals==2, both left and right are ullbytes
// if vals==1, the left is a ullbyte and the right is a node
// if vals==0, left and right are nodes
static void freehuffmantree(node *root) {
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
		if (strlen(((ullbyte*)root->left)->strval)<10)
			printf("0x%.2x: %.9s\t\t(count=%llu)",((ullbyte*)root->left)->val,((ullbyte*)root->left)->strval,((ullbyte*)root->left)->count);
		else
			printf("0x%.2x: %s\t(count=%llu)",((ullbyte*)root->left)->val,((ullbyte*)root->left)->strval,((ullbyte*)root->left)->count);
		printf("\tbit count change = %lld\n",-1*(8-strlen(((ullbyte*)root->left)->strval))*((long long int)((ullbyte*)root->left)->count));
		if (root->vals==2) {
			if (strlen(((ullbyte*)root->right)->strval)<10)
				printf("0x%.2x: %.9s\t\t(count=%llu)",((ullbyte*)root->right)->val,((ullbyte*)root->right)->strval,((ullbyte*)root->right)->count);
			else
				printf("0x%.2x: %s\t(count=%lld)",((ullbyte*)root->right)->val,((ullbyte*)root->right)->strval,((ullbyte*)root->right)->count);
			printf("\tbit count change = %lld\n",-1*(8-strlen(((ullbyte*)root->right)->strval))*((long long int)((ullbyte*)root->right)->count));
		}
		else printhuffmantree((node*)root->right);
	}
}
static void processinput(FILE *infile,FILE *outfile) {
	// buffer to read the file
	const int buffersize=4096;
	byte buffer[buffersize];

	// count the frequency of each byte
	unsigned long long frequencies[256]; //2^8, spot for each possible byte value
	memset(frequencies,0,sizeof(unsigned long long)*256);
	// read the file to get the frequency of bytes
	int i, x, uniquebytes=0;
	while ((i=fread(buffer,sizeof(byte),buffersize,infile))>0) {
		for (x=0;x<i;++x) {
			if (!frequencies[buffer[x]]) ++uniquebytes;
			frequencies[buffer[x]]+=1;
		}
	}
	fseek(infile,0L,SEEK_SET); //rewind file

	// build the minheap
	ullbyte **minheap = (ullbyte**)malloc(sizeof(ullbyte*)*uniquebytes);
	// an array to keep a static reference of every ullbyte so we don't need to search
	ullbyte *ullbytes[256];
	// for each element of frequency, add it to the minheap
	x=0;
	i=0;
	while (i<256) {
		if (frequencies[i]) {
			minheap[x] = (ullbyte*)malloc(sizeof(ullbyte));
			minheap[x]->count=frequencies[i];
			minheap[x]->val=i;
			ullbytes[i] = minheap[x]; //ullbytes[] can be referenced by actual value, used for direct indexing in compression
			if (x) {
				int z=x;
				while (z && minheap[z>>1]->count>minheap[z]->count) {
					const int parent = z>>1;
					ullbyte *swap = minheap[parent];
					minheap[parent] = minheap[z];
					minheap[z] = swap;
					//make sure the sibling follows the same conditions, this further pushes large values down
					if (z==x && z<<1==parent) z=-1; // z is the left child, though we have no right child yet
					if (z<<1 == parent) ++z; // z is the left child, make sure the right child is ok
					else --z; // z is the right child, check the left child
					if (z>0) {
						if (minheap[parent]->count>minheap[z]->count) {
							swap = minheap[parent];
							minheap[parent] = minheap[z];
							minheap[z] = swap;
						}
					}
					z=parent;
				}
			}
			if (++x==uniquebytes) break; // no more values, no reason to check remaining frequencies
		}
		++i;
	}

	// print the minheap
	i=0;
	x=2; //print level for newlines, so each line is a new level in the min heap
	printf("Row #) Min heap frequencies\n0x#) ");
	while (i<uniquebytes) {
		printf("%llu",minheap[i]->count);
		if (i==x-2) {
			printf("\n%dx#) ",x);
			x<<=1;
		}
		else if (i<uniquebytes-1) printf(", ");
		else printf("\n");
		++i;
	}

	// build a Huffman tree
	// we can have a maximum (if we only ever have 1 subtree) of ((uniquebytes+1)/2)+1 nodes
	// allocate a list large enough for the maximum number of nodes, necessary nodes are dynamically allocated
	node **roots = (node**)malloc(sizeof(node*)*(((uniquebytes+1)>>1)+1));
	int numroots=0;
	i=0;
	while (i<uniquebytes) {
		if (i+1<uniquebytes) // at least two values remaining
			i+=addtorootlist(roots,&numroots,minheap[i],minheap[i+1]);
		else { // last value
			finalizehuffmantree(roots,&numroots,minheap[i]);
			break;
		}
	}
	// if i==uniquebytes then the last elements were added as a pair, need to finalize the tree still
	if (i==uniquebytes) finalizehuffmantree(roots,&numroots,NULL);
	// the nodes are now all in this tree
	node *huffmanroot = roots[0];
	// dont need the root list anymore
	free(roots);

	// calculate encodings by shifting the current path to the left and adding zero or one
	// actual encoding will be of length of the depth of the node, with leading zeroes filling this length
	// each ullbyte's string path is calculated using these two values
	// left is zero, right is one
	generateencoding(huffmanroot,0,0);

	// printing every ullnode's strval prints the encoding
	printf("Generated encoding:\n");
	printhuffmantree(huffmanroot);

	// write decoding information to front of output file
	printf("Writing metadata . . .");
	byte writebuffer[buffersize];
//(could count from 0==1 to use 7 instead of 8 bits ...)
	writebuffer[0] = uniquebytes; //first byte is count of unique bytes, <=256
//(could also count from 0, 5bits=32, 6bits=64...) doing this and reducing the previous line will never save more than a few bits
	writebuffer[1] = maxstringlength; //second byte is the max length of an encoded string, this is (1/2) of the step value
	// the depth is the string length ... need 2x the string length to ensure leading zeroes are handled correctly
	// an immediate path of "1010" is different than the path "001010"
	// after using the length of the string, the string could be "000001010" ....
	// we need the depth to recover the actual string (doing it this way at least) ....
/*****
The first maxstringlength bits can be the depth, the second set of bits can be the string without unecessary leading zeroes *********************
*****/
	int bufferpos=2; //track position for write buffer
	int bitpos=0; //bit position within bufferpos
	//for each byte + encoding . . .
	for (i=0;i<uniquebytes;++i) {
		// store the original byte value
		for (x=7;x>=0;--x) {
			writebuffer[bufferpos] <<= 1;
			writebuffer[bufferpos] |= ((minheap[i]->val)>>x)&1;
			if (++bitpos==8) {
				bitpos=0;
				writebuffer[++bufferpos]=0;
			}
		}
		// store extra leading zeros to match max length
		const int startx = maxstringlength-strlen(minheap[i]->strval);
		for (x=0;x<startx;++x) {
			writebuffer[bufferpos] <<= 1;
			if (++bitpos==8) {
				bitpos=0;
				writebuffer[++bufferpos]=0;
			}
		}
		// store the encoding string
		for (x=0;x<maxstringlength-startx;++x) {
			writebuffer[bufferpos] <<= 1;
			writebuffer[bufferpos] |= minheap[i]->strval[x]-'0';
			if (++bitpos==8) {
				bitpos=0;
				writebuffer[++bufferpos]=0;
			}
		}
		// need the value's depth to avoid leading zero issues, + (another maxlength(encoded string)) bits
		for (x=maxstringlength-1;x>=0;--x) {
			writebuffer[bufferpos] <<= 1;
			writebuffer[bufferpos] |= (minheap[i]->depth>>x)&1;
			if (++bitpos==8) {
				bitpos=0;
				writebuffer[++bufferpos]=0;
			}
		}
	}
	printf(" %d bits\n",(bufferpos<<3)+bitpos);
	// don't need the heap anymore
	free(minheap);


	printf("Compressing . . .\n");
	while ((i=fread(buffer,sizeof(byte),buffersize,infile))>0) {
		for (x=0;x<i;++x) {
//verbose printing
//			printf("0x%.2x -> %.10s",buffer[x],ullbytes[buffer[x]]->strval);
//			if (x>0 && x%10==0) printf("\n");
//			else printf(", ");
			unsigned char flushbuffer = (bufferpos+16>=buffersize)?1:0; //flush the write buffer soon
			for (int z=0;ullbytes[buffer[x]]->strval[z]!='\0';++z) {
				writebuffer[bufferpos] <<= 1;
				writebuffer[bufferpos] |= ullbytes[buffer[x]]->strval[z]-'0';
				if (++bitpos == 8) {
					bitpos=0;
					if (flushbuffer) {
						flushbuffer=0;
						fwrite(writebuffer,sizeof(byte),bufferpos,outfile);
						bufferpos=0;
						writebuffer[0]=0;
					}
					else writebuffer[++bufferpos]=0;
				}
			}
		}
//		printf("\n");
	}
	//final bits
	if (bufferpos || bitpos) {
		if (bitpos) {
			while (bitpos<8) { ++bitpos; writebuffer[bufferpos]<<=1; } //shift final byte's bits to left
			++bufferpos;
		}
		fwrite(writebuffer,sizeof(byte),bufferpos,outfile);
	}
	//final clean up, remove all huffman nodes and ullbytes+their encodings
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
	FILE *outfile = fopen(argv[2],"wb");
	if (!outfile) {
		fprintf(stderr,"Unable to open output file \"%s\" !\n",argv[2]);
		printusage(argv[0]);
		fclose(infile);
		return 0;
	}
	processinput(infile,outfile);
	fclose(infile);
	fclose(outfile);
	return 0;
}
#endif //_USECURSES


