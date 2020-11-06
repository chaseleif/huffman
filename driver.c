#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

static inline void printusage(char *prog) {
	fprintf(stderr,"This utility compresses files using Huffman encoding\n\tUsage: %s [-c/-d] infile [-o] outfile\n",prog);
	fprintf(stderr,"\t-c\tcompress the file specified in the next word\n");
	fprintf(stderr,"\t-d\tdecompress the file specified in the next word\n");
	fprintf(stderr,"\t-o\toutput file location\n");
	fprintf(stderr,"\t(must include -c or -d to specify operation, -o is optional but must immediately precede the output file name if used)\n");
}
int main(int argc, char **argv) {
	if (argc<4) {
		printusage(argv[0]);
		return 0;
	}
	char *infilename=NULL;
	char *outfilename=NULL;
	unsigned char action=0;
	for (int i=1;i<argc;++i) {
		if (argv[i][0]=='-') {
			if ((argv[i][1]|0x20)=='c')
				action=1; // compress
			else if ((argv[i][1]|0x20)=='d')
				action=2; // decompress
			else if ((argv[i][1]|0x20)=='o') {
				if (i==argc-1) {
					printusage(argv[0]);
					return 0;
				}
				outfilename=argv[++i];
			}
			else {
				printusage(argv[0]);
				return 0;
			}
		}
		else if (!infilename)
			infilename=argv[i];
		else if (!outfilename)
			outfilename=argv[i];
		else {
			printusage(argv[0]);
			return 0;
		}
	}
	if (!action || !infilename || !outfilename) {
		printusage(argv[0]);
		return 0;
	}
	FILE *infile = fopen(infilename,"rb");
	if (!infile) {
		fprintf(stderr,"Unable to open input file \"%s\" !\n",infilename);
		printusage(argv[0]);
		return 0;
	}
	FILE *outfile = fopen(outfilename,"wb");
	if (!outfile) {
		fprintf(stderr,"Unable to open output file \"%s\" !\n",outfilename);
		printusage(argv[0]);
		fclose(infile);
		return 0;
	}
	if (action==1) { // compress
		printf("Compressing \"%s\", output file: \"%s\"\n",infilename,outfilename);
		docompress(infile,outfile,1); // when last parameter is non-zero printing is enabled
		clearhfcvars();
	}
	else { // if action==2
		printf("Decompressing \"%s\", output file: \"%s\"\n",infilename,outfilename);
		dorestore(infile,outfile,1);
		clearhfcvars();
	}
	// seek to end of the files (the compression leaves the file pointer at the third byte)
	fseek(outfile,0L,SEEK_END);
	fseek(infile,0L,SEEK_END);
	const unsigned long long insize = ftell(infile);
	const unsigned long long outsize = ftell(outfile);
	fclose(infile);
	fclose(outfile);
	printf("Input file \"%s\" is %llu bytes, outputfile \"%s\" is %llu bytes\n",infilename,insize,outfilename,outsize);
	const double pctcompare = ((double)outsize)/((double)insize);
	const double pctchange = (outsize>=insize)?((double)(outsize-insize))/((double)insize):((double)(insize-outsize))/((double)insize);
	printf("The output file is %.2f%% the size of the input, this is a ",pctcompare*100);
	if (outsize<insize) printf("-");
	printf("%.2f%% ",pctchange*100);
	if (outsize<insize) printf("decrease\n");
	else printf("increase\n");
	printf("Exiting program\n");
	return 0;
}

