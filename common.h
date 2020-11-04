#ifndef _HUFFCOMMON_H_
#define _HUFFCOMMON_H_
#include <stdint.h>

//toggle the curses utility
//#define _USECURSES

struct ullbyte {
	unsigned long long count;
	uint8_t val;
	char *strval;
};
struct node {
	void *left;
	void *right;
	unsigned long long sum;
	unsigned char vals;
};

#endif
