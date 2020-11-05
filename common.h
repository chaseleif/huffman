#ifndef _HUFFCOMMON_H_
#define _HUFFCOMMON_H_
#include <stdint.h>
#include <stdio.h>

struct ullbyte {
	unsigned long long count;
	uint8_t val;
	char *strval;
};
struct node {
	void *left;
	void *right;
	unsigned long long sum;
	uint8_t vals;
};

//huffman.c: libhfc.so
void dorestore(FILE *infile,FILE *outfile);
void docompress(FILE *infile,FILE *outfile);
void clearhfcvars();
/*
static inline void pushdownminheap(struct ullbyte **minheap,const int lasti);
static inline int bitlength(int num);
static inline char *getpathstring(const int path,const int steps);
static byte generateencoding(struct node *root,const int path,const int steps);
static void finalizehuffmantree(struct node **roots,int *rootslen,struct ullbyte *lastval);
static void freehuffmantree(struct node *root);
static void printhuffmantree(struct node *root);
static void recreatehuffmantree(struct node *root,struct ullbyte *val);
static void freedecodingtree(struct node *T);
static byte decodestring(struct node *root,unsigned char *path);
*/

#endif
