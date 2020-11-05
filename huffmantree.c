#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

typedef struct node node;
typedef struct rootnoot rootnode;
typedef struct bottomnode bottomnode;
typedef uint8_t byte;
typedef struct ullbyte ullbyte;

//free data structures kept for display purposes
void clearhfcvars() {
}

// called when a root is swapped with the end to push the root down
static inline void pushdownminheap(ullbyte **minheap,const int lasti) {
	int z=0; // push the root down the heap
	while ((z<<1)+1<=lasti) { //while this node has a child
		const int childpos=(z<<1)+1; // first child
		if (childpos+1<=lasti) { // have two children
			if (minheap[z]->count<=minheap[childpos]->count &&
				minheap[z]->count<=minheap[childpos+1]->count) break; // this node is in the right spot
			if (minheap[childpos]->count<=minheap[childpos+1]->count) { // the right child has a larger count.
				ullbyte *swap = minheap[childpos];
				minheap[childpos]=minheap[z];
				minheap[z]=swap;
				z=childpos;
			}
			else { // the right child is smaller than the left
				const int rchild = childpos+1;
				ullbyte *swap = minheap[rchild];
				minheap[rchild]=minheap[z];
				minheap[z]=swap;
				z=rchild;
			}
		}
		else { // only have one child
			if (minheap[z]->count<=minheap[childpos]->count) break; // the right spot
			ullbyte *swap=minheap[childpos];
			minheap[childpos]=minheap[z];
			minheap[z]=swap;
			z=childpos;
		}
	}
}
static inline int bitlength(int num) {
	byte ret = 0;
	while (num) {
		num>>=1;
		++ret;
	}
	return ret;
}
static inline char *getpathstring(const int path,const int steps) {
	char *ret = (char*)malloc(sizeof(char)*(steps+1));
	ret[steps]='\0';
	for (int i=0;i<steps;++i) {
		ret[i] = ((path>>((steps-i)-1))&1)+'0';
	}
	return ret;
}

// go left is zero, go right is one, the string is derived from path and depth
// returns the maximum depth (longest encoding)
static byte generateencoding(node *root,const int path,const int steps) {
	if (root->vals>0) { // this node has at least one ullbyte
		// the left pointer will be a ullbyte
		((ullbyte*)root->left)->strval = getpathstring((path<<1),steps+1);
		if (root->vals==2) { // this node has a right ullbyte
			((ullbyte*)root->right)->strval = getpathstring((path<<1)+1,steps+1);
			return steps+1;
		}
		else // the right side is a node
			return generateencoding((node*)root->right,(path<<1)+1,steps+1);
	}
	// this is just a node pointing to nodes
	//else { 
	byte leftdepth = generateencoding((node*)root->left,(path<<1),steps+1);
	byte rightdepth = generateencoding((node*)root->right,(path<<1)+1,steps+1);
	if (leftdepth>rightdepth) return leftdepth;
	return rightdepth;
}

// takes two ullbytes (in pairs), if there were only one new value then finalizehuffmantree would be called
// finds the minimum of (val1->count+val2->count) or val1->count + (some subtree->sum)
// either extends a subtree or creates a new subtree, rootslen is updated as needed
// returns the number of values consumed (2 or 1)
static byte addtorootlist(node **roots,int *rootslen,ullbyte *val1,ullbyte *val2) {
	int mini=-1; // mini==-1 when we make a new tree
	unsigned long long minval=val1->count+val2->count; // minval will be the nodes sum
	for (int i=0;i<*rootslen;++i) {
		if (roots[i]->sum+val1->count<minval) {
			mini=i;
			minval=roots[i]->sum+val1->count;
		}
	}
/*
// The logic below handles 'reinserting' nodes into the min heap
// Nodes with smaller values become the next candidate to either merge with val1 or merge with another node.
// The file sizes I have seen in testing are slightly larger doing this.
	int mini;
	unsigned long long minval;
	if (*rootslen) {
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
static void recreatehuffmantree(node *root,ullbyte *val) {
	node *trav = root;
	int i=0;
	while (val->strval[i+1]!='\0') {
		if (val->strval[i]=='1') {
			if (!trav->right) {
				node *newnode = (node*)malloc(sizeof(node));
				newnode->right=NULL;
				newnode->left=NULL;
				newnode->sum=0;
				trav->right=(void*)newnode;
			}
			trav=(node*)trav->right;
		}
		else {
			if (!trav->left) {
				node *newnode = (node*)malloc(sizeof(node));
				newnode->right=NULL;
				newnode->left=NULL;
				newnode->sum=0;
				trav->left=(void*)newnode;
			}
			trav=(node*)trav->left;
		}
		++i;
	}
	node *newnode = (node*)malloc(sizeof(node));
	newnode->vals=val->val;
	newnode->sum=4;
	newnode->right=NULL;
	newnode->left=NULL;
	if (val->strval[i]=='1')
		trav->right = (void*)newnode;
	else
		trav->left = (void*)newnode;
}
static void freedecodingtree(node *T) {
	if (T->sum==4) // this node is a value
		free(T);
	else {
		freedecodingtree((node*)T->right);
		freedecodingtree((node*)T->left);
		free(T);
	}
}
static byte decodestring(node *root,unsigned char *path) {
	node *trav = root;
	for (int i=0;path[i]!=42;++i) {
		if (path[i]) trav=(node*)trav->right;
		else trav=(node*)trav->left;
	}
	return trav->vals;
}
void dorestore(FILE *infile,FILE *outfile) {
	const int buffersize=4096;
	byte buffer[buffersize];
	//fill the buffer
	int ret = fread(buffer,sizeof(byte),buffersize,infile);
	// first byte = count of unique bytes, <=256 (count from zero as one for this first count)
	const unsigned int uniquebytes = ((unsigned int)buffer[0])+1;
	// second byte is maximum depth, it is the current depth of values we read
	unsigned int depth = buffer[1];
	// third byte is the final shmt, if this is 7 all bits will be the input data
	// if the final shmt is less than 7, we only have (7-finalshmt) bits in the last byte
	byte finalshmt = buffer[2];
	unsigned char *path = (unsigned char*)malloc(sizeof(unsigned char)*(depth+1)); // going to use this in decompression step
	int bufferpos=3;
	int bitshmt=7;
	node *root = (node*)malloc(sizeof(node));
	root->right=NULL;
	root->left=NULL;
	root->vals=0;
	root->sum=0;
	int bytesremaining=uniquebytes;
	while (bytesremaining>0) {
		int groupsize=0;
		int groupbits = bitlength(bytesremaining);
		if (groupbits>8) groupbits=8;
		for (int x=groupbits-1;x>=0;--x) {
			groupsize|=((buffer[bufferpos]>>bitshmt)&1)<<x;
			if (--bitshmt<0) {
				if (++bufferpos>=ret) {
					ret=fread(buffer,sizeof(byte),buffersize,infile);
					bufferpos=0;
				}
				bitshmt=7;
			}
		}
		// for each byte in this depth group
		for (int x=1;x<=groupsize;++x) {
			ullbyte newval;
			newval.val=0;
			// get the 8 bit value.
			for (int i=7;i>=0;--i) {
				newval.val|=((buffer[bufferpos]>>bitshmt)&1)<<i;
				if (--bitshmt<0) {
					if (++bufferpos>=ret) {
						ret=fread(buffer,sizeof(byte),buffersize,infile);
						bufferpos=0;
					}
					bitshmt=7;
				}
			}
			// get the path
			newval.strval = (char*)malloc(sizeof(char)*(depth+1));
			newval.strval[depth]='\0';
			for (int i=0;i<depth;++i) {
				newval.strval[i]=((buffer[bufferpos]>>bitshmt)&1)+'0';
				if (--bitshmt<0) {
					if (++bufferpos>=ret) {
						ret=fread(buffer,sizeof(byte),buffersize,infile);
						bufferpos=0;
					}
					bitshmt=7;
				}
			}
			printf("Dictionary: val=%.2x, path=%s\n",newval.val,newval.strval);
			recreatehuffmantree(root,&newval); // pass in the root with a val/encoding pair
			free(newval.strval);
		}
		bytesremaining-=groupsize;
		--depth;
	}
	// root is now the same huffman tree the file was compressed with
	// except the leaf (value) nodes are just nodes, not ullvals, don't have(minus frequency)
	// the remainder of the file is data, read the file and traverse the tree to replace the values.
	if (!ret) {
		ret=fread(buffer,sizeof(byte),buffersize,infile);
		bufferpos=0;
		bitshmt=7;
	}
	byte writebuffer[buffersize];
	int writepos=0;
//	printhuffmantree(root);
printf("\nDecoding: ");
	while (ret) {
		node *trav = root;
		while (!trav->sum) {
			if ((buffer[bufferpos]>>bitshmt)&1) trav=(node*)trav->right;
			else trav=(node*)trav->left;
			if (--bitshmt<0) {
				if (++bufferpos>=ret) {
					ret=fread(buffer,sizeof(byte),buffersize,infile);
					if (!ret)
						break;
					bufferpos=0;
				}
				bitshmt=7;
			}
		}
		if (trav->sum) {
			writebuffer[writepos++]=trav->vals;
			if (writepos==buffersize) {
				fwrite(writebuffer,sizeof(byte),writepos,outfile);
				fflush(outfile);
				writepos=0;
			}
		}
		if (bufferpos==ret-1) { // last byte of the buffer, need to ensure this is not the last byte of the file
			if (finalshmt<7 && feof(infile)) { // this is the file's last byte and isn't a 'full' byte
				if (bitshmt==finalshmt) break; // we have read all the bits of the encoding
			}
		}
	}
	if (writepos) {
		fwrite(writebuffer,sizeof(byte),writepos,outfile);
		fflush(outfile);
	}
	printf("Wrote output file\n");
	// free the decoding tree
	freedecodingtree(root);
}
void docompress(FILE *infile,FILE *outfile) {
	// buffer to read the file
	const int buffersize=4096;
	byte buffer[buffersize];

	// count the frequency of each 8-bit byte
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
	// a side array to keep a static reference of every ullbyte so we don't need to search
	ullbyte *ullbytes[256];
	// for each element of frequency, add it to the minheap
	x=0;
	i=0;
	while (i<256) {
		if (frequencies[i]) {
			// adding new elements to the end of the heap and pushing up as needed
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
					//make sure the sibling follows the same conditions
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
			if (++x==uniquebytes) break; // no more unique values, no reason to check remaining frequencies
		}
		++i;
	}

	// print the minheap
	i=0;
	x=2; //print level for newlines, so each line is a new level in the min heap
	printf("Min heap row # count) [byte frequencies] -> (unique bytes=%d)\n1x#) ",uniquebytes);
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
	i=0; // i counts the elements 'popped'
	x=uniquebytes-1; // the last index
	while (i<uniquebytes) {
		if (i+1==uniquebytes) { // the last value
			finalizehuffmantree(roots,&numroots,minheap[0]);
			break;
		}
		if (i+2==uniquebytes) { // two values
			if (minheap[0]->count>minheap[1]->count) { // sanity check
				ullbyte *swap = minheap[0];
				minheap[0]=minheap[1];
				minheap[1]=swap;
			}
			if (addtorootlist(roots,&numroots,minheap[0],minheap[1])==1)
				finalizehuffmantree(roots,&numroots,minheap[1]);
			else finalizehuffmantree(roots,&numroots,NULL);
			break;
		}
		ullbyte *val1=minheap[0]; // the first value is the root
		ullbyte *swap = minheap[0]; // swap the root with the end, reduce array size
		minheap[0]=minheap[x];
		minheap[x]=swap;
		pushdownminheap(minheap,--x); // check the new root, we've taken the old 'root' out of bounds
		ullbyte *val2=minheap[0]; // the new root may not be consumed
		// we consumed the second value as well
		if (addtorootlist(roots,&numroots,val1,val2)==2) {
			swap = minheap[0];
			minheap[0]=minheap[x];
			minheap[x]=swap;
			pushdownminheap(minheap,--x); // made a new subtree
			i+=2;
		}
		else ++i; // only took the root
	}
	// finalizehuffmantree put all of the roots together in this tree
	node *huffmanroot = roots[0];
	// dont need the root list anymore
	free(roots);

	// actual encoding will be of length of the depth of the node, with leading zeroes filling this length
	// left is zero, right is one
	// the maximum depth is returned
	byte maxstringlength = generateencoding(huffmanroot,0,0);

	// printing every ullnode's strval prints the encoding
	printf("Generated encoding:\n");
	printhuffmantree(huffmanroot);

	// write decoding information to front of output file
	printf("Writing metadata . . .");
	byte writebuffer[buffersize];
	// first byte = count of unique bytes, <=256 (count from zero as one for the fwrite)
	writebuffer[0] = uniquebytes-1;
	// second byte = the maxdepth (maximum encoding string length)
	writebuffer[1] = maxstringlength;
	// third byte is reserved for recording the final shmt amount, covering cases where the last byte is not full
	writebuffer[2]=0;
	// for the rest of the metadata
	// the (byte+encoding) will now be grouped according to encoding length
	// length from maxlength, decremented by 1 each time.
	// each group will use the number of bits for remaining size to indicate group count
	// each val will have 1 byte followed by strlen bits
	int bytesremaining=uniquebytes;
	int groupbitsneeded = bitlength(bytesremaining);
	if (groupbitsneeded>8) groupbitsneeded=8;
	// write group information in front of group data, need a marker for this position
	// 7 is first bit shmt, 0 is last
	int groupbitshmt=7; // start shmt for groupsize. shmt is left shift with LOR into destination position. (decrementing to walk right) [7,6,5..,0]
	int groupposstart=3; // pos i for groupsize
	writebuffer[3]=0; // zero out buffers as we increment iterators
	// actual write position, starts (needed bits) behind the group size position
	int bufferpos=groupposstart; // track position for write buffer
	int bitshmt=groupbitshmt-groupbitsneeded; //bit position within bufferpos, where the group will be - the group's bits
	if (bitshmt<0) { bitshmt+=8; writebuffer[++bufferpos]=0; }
	// for all unique bytes
	while (bytesremaining>0) {
		byte groupcount = 0;
		//for each byte + encoding . . .
		for (i=0;i<uniquebytes;++i) {
			if (strlen(minheap[i]->strval)!=maxstringlength) continue; // group lengths/groups by level
			++groupcount;
			// store the original byte value
			for (x=7;x>=0;--x) { // x is val pos shmt
				writebuffer[bufferpos] |= (((minheap[i]->val)>>x)&1)<<bitshmt;
				if (--bitshmt<0) {
					bitshmt=7;
					writebuffer[++bufferpos]=0;
				}
			}
			// store the encoding string
			for (x=0;minheap[i]->strval[x]!='\0';++x) { // x is string i
				writebuffer[bufferpos] |= (minheap[i]->strval[x]-'0')<<bitshmt;
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
		printf("from %d bytes remaining, processed a group of %u (each with string length %u) ",bytesremaining,groupcount,maxstringlength);
		bytesremaining-=groupcount;
		printf(", %d left to process\n",bytesremaining);
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
	printf(" %d bits\n",(bufferpos<<3)+(7-bitshmt));

	// don't need the heap anymore
	free(minheap);

	printf("Compressing . . .\n");
	while ((i=fread(buffer,sizeof(byte),buffersize,infile))>0) {
		for (x=0;x<i;++x) {
//verbose printing
//			printf("0x%.2x -> %.10s",buffer[x],ullbytes[buffer[x]]->strval);
//			if (x>0 && x%10==0) printf("\n");
//			else printf(", ");
			byte flushbuffer = (bufferpos+16>=buffersize)?1:0; //flush the write buffer at the next byte alignment
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
//		printf("\n");
	}
	// final bits
	if (bitshmt<7 || bufferpos) {
		if (bitshmt<7) fwrite(writebuffer,sizeof(byte),bufferpos+1,outfile);
		else if (bufferpos) fwrite(writebuffer,sizeof(byte),bufferpos,outfile);
		fflush(outfile);
	}
	// the third byte of the compressed file indicates how many bits should be in the last byte
	writebuffer[0]=bitshmt;
	fseek(outfile,2,SEEK_SET);
	fwrite(writebuffer,sizeof(byte),1,outfile);
	fflush(outfile);
	//final clean up, remove all huffman nodes and ullbytes+their encodings
	freehuffmantree(huffmanroot);
}


