#ifndef _HUFFCOMMON_H_
#define _HUFFCOMMON_H_
#include <stdint.h>
#include <stdio.h>

struct node {
	struct node *left;
	struct node *right;
	unsigned long long count;
	uint8_t val;
	char *strval;
};

// shared globals from huffmantree.c
struct node *hfcroot=NULL;
struct node **hfcheap=NULL;
int uniquebytes;

// huffmantree.c: libhfc.so
// Shared functions ***********
// free the data structures kept for display purposes
void clearhfcvars();
// decompress infile and write to outfile, doprints enables data structure printf statements
void dorestore(FILE *infile,FILE *outfile,const uint8_t doprints);
// compress infile and write to outfile, doprints enables data structure printf statements
void docompress(FILE *infile,FILE *outfile,const uint8_t doprints);
/*
// Static functions ***********
//print the huffman tree
static void printhuffmantree(node *root) {
// destructor for the Huffman tree built using this file.
static void freehuffmantree(node *root) {
// called to get the number of bits needed to represent a number. (max return is 8)
static inline byte bitlength(int num) {
// returns a string from a path value and number of steps, handles leading zeroes
static inline char *getpathstring(const int path,const int steps) {
// generate encoding, called after a huffman tree is created, sets the strval of value nodes
// go left is zero, go right is one, the string is derived from path taken
// returns the maximum depth (longest encoding)
static byte generateencoding(node *root,const int path,const int steps) {
// called after a root is swapped with the end, need to push the root down
static inline void pushdownminheap(const int lasti) {
// takes two nodes (in pairs), if there were only one new value then finalizehuffmantree would be called
// finds the minimum of (val1->count+val2->count) or val1->count + (some subtree->count)
// either extends a subtree or creates a new subtree, rootslen is updated as needed
// returns the number of nodes consumed (2 or 1)
static byte addtorootlist(node **roots,int *rootslen,node *val1,node *val2) {
// finalizehuffmantree merges any subtrees into one tree, lastval may be NULL
// (optionally add lastval to smallest subtree)
// while there is more than one root -> merge the two smallest roots, all roots condense to roots[0]
static void finalizehuffmantree(node **roots,int *rootslen,node *lastval) {
// this takes an insertnode as input that has a strval
// the insertnode is inserted from the root using the path in its strval
static void recreatehuffmantree(node *insertnode) {
*/

#endif
