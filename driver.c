#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

// expecting at least: ./driver -c in.file out.file
static inline void printusage(char *prog) {
	fprintf(stderr,"###\n#   This utility compresses files using Huffman encoding\n#\tUsage: %s [-c/-d] infile [-o] outfile\n",prog);
	fprintf(stderr,"#\t-c or -d is required to specify operation\n");
	fprintf(stderr,"#\t-o is optional, but if used must immediately precede the output file name\n");
	fprintf(stderr,"#\t-c\tdo compression\n");
	fprintf(stderr,"#\t-d\tdo decompression\n");
	fprintf(stderr,"#\t-o\toutput file location\n");
	fprintf(stderr,"#\t-v\tverbose (prints data structure infos)\n");
	fprintf(stderr,"#\t-b\tprint space separated values (during compression): frequency byte encoding\n");
	fprintf(stderr,"#\tinput filename must precede the output filename (unless -o is used)\n###\n");
}
int main(int argc, char **argv) {
	if (argc<4) {
		printusage(argv[0]);
		return 0;
	}
	char *infilename=NULL;
	char *outfilename=NULL;
	unsigned char action=0;
	unsigned char doprint=0;
	// set some options
	for (int i=1;i<argc;++i) {
		if (argv[i][0]=='-') {
			if ((argv[i][1]|0x20)=='c') // compress
				action=1;
			else if ((argv[i][1]|0x20)=='d') // decompress
				action=2;
			else if ((argv[i][1]|0x20)=='v') // verbose output
				doprint=1;
			// batch output (compression prints a space separated value section for dot file creation)
			// this enables compression to create the tree structure, the output file will be the space separated values
			else if ((argv[i][1]|0x20)=='b')
				doprint=2;
			else if ((argv[i][1]|0x20)=='o') { // next arg is the outputfilename
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
		else if (!infilename)  // input filename precedes output filename
			infilename=argv[i];
		else if (!outfilename)
			outfilename=argv[i];
		else {
			printusage(argv[0]);
			return 0;
		}
	}
	if (doprint==2) action=1;
	// something wasn't set
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
	// action
	if (action==1) { // compress
		printf("Compressing \"%s\", output file: \"%s\"\n",infilename,outfilename);
		docompress(infile,outfile,doprint); // when last parameter is non-zero printing is enabled
		if (doprint==2) { // we just wanted a dot file
			printf("Output for dot creation was written to %s\n",outfilename);
			fclose(infile);
			fclose(outfile);
			return 0;
		}
	}
	else { // (if action==2) decompress
		printf("Decompressing \"%s\", output file: \"%s\"\n",infilename,outfilename);
		dorestore(infile,outfile,doprint);
	}
	// seek to end of the files to get the size, outfile will not point to the end after compression
	fseek(infile,0L,SEEK_END);
	const unsigned long long insize = ftell(infile);
	fclose(infile);
	// data structures are not cleared automatically so they can be used with the curses utility
	clearhfcvars();
	// outfile
	fseek(outfile,0L,SEEK_END);
	const unsigned long long outsize = ftell(outfile);
	fclose(outfile);

	// file size comparison output
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

