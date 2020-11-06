#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

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
	docompress(infile,outfile,1);
	clearhfcvars();
	fclose(infile);
	fclose(outfile);

	printf("Opening compressed file . . .\n");
	infile = fopen(argv[2],"rb");
	if (!infile) { printf("Something bad happened :(\n"); return 0; }
	char newfilename[256];
	strcpy(newfilename,argv[1]);
	strcat(newfilename,".restored");
	outfile = fopen(newfilename,"wb");
	if (!outfile) { printf("unable to open %s\n",newfilename); fclose(infile); return 0; }
	dorestore(infile,outfile,1);
	clearhfcvars();
	fclose(infile);
	fclose(outfile);
	printf("Exiting program . . .\n");
	return 0;
}

