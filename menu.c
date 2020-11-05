
#include "common.h"

#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//general menu char buffer size
#define MAXSTRLEN 100
//max items plus one for empty str
#define MAXMENUITEMS 7
//row number for the title
#define TITLELINENUM 3
//first line of menus
#define MENUFIRSTLINE 5
//first left column position, if CENTERMARGIN doesn't work
#define LEFTMARGIN 4
//margin specifies left column number
//attempts to center text on the screen based on length of char* titlebar
//if LEFTMARGIN doesn't exist to the right, CENTERMARGIN falls back to LEFTMARGIN
#define CENTERMARGIN ((COLS>>1)-(strlen(titlebar)>>1)+strlen(titlebar)<COLS-LEFTMARGIN)?(COLS>>1)-(strlen(titlebar)>>1):LEFTMARGIN
//max FILENAMELEN, where we stop the input
#define FILENAMELEN 40
//number of chunks for the total filesize to be broken up into, this is the limit of threads
#define NUMFILECHUNKS 4

//node defined in "common.h"
typedef struct node node;

//sets global menuoptions[][] strings, final string begins with '\0'.
static int setmenuoptions(unsigned char menulevel);
//process input, ch is user's input from main menus
//*menulevel is from menulevels enum, *highlight is the active line
//lastmenuitem is used for wrapping arrow selection navigation
static unsigned char processinput(const int ch,int *menulevel,int *highlight,const int lastmenuitem);
//active display for compression
static void drawcompressionscreen();
//active display for decompression
static void drawdecompressionscreen();

// huffmantree.c:
//compress function
extern void compress();
//decompress function
extern void decompress();

//the ncurses window
WINDOW *w;

//enum to indicate current menu level
enum menulevels { MAINMENU, COMPRESSMENU, DECOMPRESSMENU };
//constant prompts
const char const *titlebar = "Data Compression with Huffman Encoding ~ implemented by Chase Phelps";
const char const *enterpathprompt = "(enter a valid file / path, or press ESC followed by another character to cancel)";
const char const *entershortprompt = "(enter a valid non-zero number less than 5 digits)";
//buffers for input / output filenames, user changes through menus
char compressinfile[FILENAMELEN] = "(not set)";
char compressoutfile[FILENAMELEN] = "out.hfc";
char decompressinfile[FILENAMELEN] = "(not set)";
char decompressoutfile[FILENAMELEN] = "(auto)";
//the number of worker threads spawned by the main thread for compression / decompression
unsigned int NUMTHREADS = 4;
//the number of compression passes to make
unsigned int NUMPASSES = 1;

//the current menu, text is set in setmenuoptions(unsigned char menulevel)
char menuoptions[MAXMENUITEMS][MAXSTRLEN];

//inline short helper functions
//lhs = lhs + "\"" + rhs + "\""
static inline void concatwithparenthesis(char *lhs,char *rhs) {
	strcat(lhs,"\""); strcat(lhs,rhs); strcat(lhs,"\"");
}
//move cursor to specified row/col, delete line after, redraw box, refresh w
static inline void dobackspace(const int row,const int col) {
	wmove(w,row,col);
	wclrtoeol(w);
	box(w,0,0);
	wrefresh(w);
}
//sets menu string array for choices to display and returns initial highlight position
//the last element of char **menuoptions is a string whose first element is '\0'
static int setmenuoptions(const unsigned char menulevel) {
	int i=-1;
	while (1) {
		++i;
		if (menulevel==MAINMENU) {
			if (i==0) strcpy(menuoptions[i],"Compress\n");
			else if (i==1) strcpy(menuoptions[i],"Decompress\n");
			else if (i==2) strcpy(menuoptions[i],"Exit\n");
			else { menuoptions[i][0]='\0'; return 0; }
		}
		else if (menulevel==COMPRESSMENU) {
			if (i==0)
			{ strcpy(menuoptions[i],"File to compress: "); concatwithparenthesis(menuoptions[i],compressinfile); }
			else if (i==1)
			{ strcpy(menuoptions[i],"Output file: "); concatwithparenthesis(menuoptions[i],compressoutfile); }
			else if (i==2) sprintf(menuoptions[i],"Number of worker threads: %u",NUMTHREADS);
			else if (i==3) sprintf(menuoptions[i],"Pass count: %u",NUMPASSES);
			else if (i==4) strcpy(menuoptions[i],"Begin compression");
			else if (i==5) strcpy(menuoptions[i],"Return to main menu");
			else { menuoptions[i][0]='\0'; return 0; }
		}
		else {//if (menulevel==DECOMPRESSMENU) {
			if (i==0)
			{ strcpy(menuoptions[i],"File to decompress: "); concatwithparenthesis(menuoptions[i],decompressinfile); }
			else if (i==1)
			{ strcpy(menuoptions[i],"Output file: "); concatwithparenthesis(menuoptions[i],decompressoutfile); }
			else if (i==2) strcpy(menuoptions[i],"Begin decompression");
			else if (i==3) strcpy(menuoptions[i],"Return to main menu");
			else { menuoptions[i][0]='\0'; return 0; }
		}
	}
}

//handles input from menus, may update highlight pos or menulevel, returns nonzero to exit program
static unsigned char processinput(const int ch,int *menulevel,int *highlight,const int lastmenuitem) {
	if (ch==KEY_UP) {
		if (*highlight==0) *highlight=lastmenuitem;
		else *highlight-=1;
	}
	else if (ch==KEY_DOWN) {
		if (*highlight==lastmenuitem) *highlight=0;
		else *highlight+=1;
	}
	else if (ch==KEY_ENTER || ch=='\n') { //not completely certain if both of these are needed
		char buffer[MAXSTRLEN+1];
		char *inputpos;
		int usrinput,i;
		const int leftstop=CENTERMARGIN;
		unsigned int runnum;
		switch (*menulevel) {
			case MAINMENU:
				if (*highlight==0) {
					*menulevel=COMPRESSMENU;
					*highlight=setmenuoptions(COMPRESSMENU);
				}
				else if (*highlight==1) {
					*menulevel=DECOMPRESSMENU;
					*highlight=setmenuoptions(DECOMPRESSMENU);
				}
				else return 1; //quit program
				break;
			case COMPRESSMENU: case DECOMPRESSMENU: //same menu layout for filenames
				if (*highlight<2) {
					curs_set(2);
					mvwprintw(w,MENUFIRSTLINE+lastmenuitem+2,leftstop,"%s",enterpathprompt);
					//change input or output file, filename enclosed in quotes
					inputpos = strchr(menuoptions[*highlight],'\"');
					*inputpos='\0';
					wmove(w,*highlight+MENUFIRSTLINE,leftstop+strlen(menuoptions[*highlight]));
					wclrtoeol(w);
					box(w,0,0);
					wrefresh(w);
					i=0;
					while (1) {
						usrinput = wgetch(w);
						if (usrinput==KEY_RESET || usrinput==KEY_BREAK || usrinput==KEY_CANCEL || usrinput==KEY_EXIT || usrinput==27) {
							curs_set(0);
							return 0;
						}
						if (usrinput==KEY_BACKSPACE || usrinput==127 || usrinput=='\b') {
							if (i) dobackspace(*highlight+MENUFIRSTLINE,(--i)+leftstop+strlen(menuoptions[*highlight]));
						}
						else if (isalpha(usrinput) || usrinput=='/' || usrinput=='.' || usrinput=='~' || usrinput=='_' || isdigit(usrinput)) {
							waddch(w,usrinput);
							buffer[i++]=usrinput;
							wrefresh(w);
						}
						if (usrinput==KEY_ENTER || usrinput=='\n' || i==FILENAMELEN) {
							if (i) {
								buffer[i]='\0';
								if (*menulevel==COMPRESSMENU && *highlight==0) strcpy(compressinfile,buffer);
								else if (*menulevel==COMPRESSMENU) strcpy(compressoutfile,buffer);
								else if (*menulevel==DECOMPRESSMENU && *highlight==0) strcpy(decompressinfile,buffer);
								else strcpy(decompressoutfile,buffer);
							}
							else {
								if (*menulevel==COMPRESSMENU && *highlight==0) strcpy(buffer,compressinfile);
								else if (*menulevel==COMPRESSMENU) strcpy(buffer,compressoutfile);
								else if (*menulevel==DECOMPRESSMENU && *highlight==0) strcpy(buffer,decompressinfile);
								else strcpy(buffer,decompressoutfile);
								mvwprintw(w,*highlight+MENUFIRSTLINE,leftstop+strlen(menuoptions[*highlight]),"%s",buffer);
								box(w,0,0);
								wrefresh(w);
							}
							break;
						}
					}
					curs_set(0);
					concatwithparenthesis(menuoptions[*highlight],buffer);
				}
				else if (*highlight<4) { //compress: set worker thread size or num passes, decompress: decompress()
					if (*menulevel==DECOMPRESSMENU) {
						if (*highlight==2) drawdecompressionscreen();
						else {
							*menulevel=MAINMENU;
							*highlight=setmenuoptions(MAINMENU);
						}
						break;
					}
					curs_set(2);
					i=0; while (i<5) { buffer[i++]='\0'; }
					runnum=0;
					mvwprintw(w,MENUFIRSTLINE+lastmenuitem+2,leftstop,"%s",entershortprompt);
					//change digit, format("menu line: %d")
					inputpos = strchr(menuoptions[*highlight],':')+2;
					*inputpos='\0';
					wmove(w,*highlight+MENUFIRSTLINE,leftstop+strlen(menuoptions[*highlight]));
					wclrtoeol(w);
					box(w,0,0);
					wrefresh(w);
					i=0;
					while (1) {
						usrinput = wgetch(w);
						if (usrinput==KEY_RESET || usrinput==KEY_BREAK || usrinput==KEY_CANCEL || usrinput==KEY_EXIT || usrinput==27) {
							wmove(w,*highlight+MENUFIRSTLINE,leftstop+strlen(menuoptions[*highlight]));
							wclrtoeol(w);
							const unsigned int thisval = (*highlight==2)?NUMTHREADS:NUMPASSES;
							sprintf(buffer,"%u",thisval);
							strcat(inputpos,buffer);
							break;
						}
						if (usrinput==KEY_BACKSPACE || usrinput==127 || usrinput=='\b') {
							if (i) {
								runnum/=10;
								buffer[i]='\0';
								dobackspace(*highlight+MENUFIRSTLINE,(--i)+leftstop+strlen(menuoptions[*highlight]));
							}
						}
						else if (isdigit(usrinput)) {
							if (!i && usrinput=='0') continue; // no leading zeroes
							if (i>=5)
								flash();
							else {
								waddch(w,usrinput);
								buffer[i++]=usrinput;
								runnum=runnum*10+(usrinput-'0');
								wrefresh(w);
							}
						}
						else if (usrinput==KEY_ENTER || usrinput=='\n') {
							if (i) {
								if (*highlight==2) {
									if (runnum>NUMFILECHUNKS) {
										runnum=NUMFILECHUNKS;
										sprintf(buffer,"%u",runnum);
									}
									NUMTHREADS=runnum;
								}
								else NUMPASSES=runnum;
								buffer[i]='\0';
								strcat(menuoptions[*highlight],buffer);
							}
							else {
								const unsigned int thisval = (*highlight==2)?NUMTHREADS:NUMPASSES;
								sprintf(buffer,"%u",thisval);
								strcat(menuoptions[*highlight],buffer);
							}
							break;
						}
					}
					box(w,0,0);
					wrefresh(w);
					curs_set(0);
				}
				else if (*highlight==4) drawcompressionscreen();
				else {
					*menulevel=MAINMENU;
					*highlight=setmenuoptions(MAINMENU);
				}
				break;
		}
	}
	return 0;
}
static inline void drawopenerrorandgetch(const int row,const int col,char *filename) {
	char errormsg[] = "Press the any key to continue . . .";
	mvwprintw(w,row,col,"Unable to open \"%s\" !",filename);
	wattron(w,A_STANDOUT);
	mvwprintw(w,row+2,col,errormsg);
	wattroff(w,A_STANDOUT);
	wmove(w,row+2,col+strlen(errormsg));
	curs_set(2);
	wgetch(w);
	curs_set(0);
}
// ncurses extended char chart -> http://melvilletheatre.com/articles/ncurses-extended-characters/index.html
// compress screen
void drawcompressionscreen() {
	wclear(w);
	wrefresh(w);
	mvwprintw(w,TITLELINENUM,CENTERMARGIN,"%s",titlebar);
	FILE *infile = fopen(compressinfile,"rb");
	if (!infile) {
		drawopenerrorandgetch(TITLELINENUM+2,CENTERMARGIN,compressinfile);
		return;
	}
	fseek(infile,0L,SEEK_END);
	const unsigned long long insize = ftell(infile);
	fseek(infile,0L,SEEK_SET);
	fclose(infile);
	mvwprintw(w,TITLELINENUM+2,LEFTMARGIN,"Running compression, input = \"%s\" (%llu bytes), output = \"%s\", byte grouping size = %u, number passes = %u",compressinfile,insize,compressoutfile,NUMTHREADS,NUMPASSES);
	const int topline = TITLELINENUM+4;
	box(w,0,0);
	wrefresh(w);
	wgetch(w);
}
// decompress screen
void drawdecompressionscreen() {
	wclear(w);
	wrefresh(w);
	mvwprintw(w,TITLELINENUM,CENTERMARGIN,"%s",titlebar);
	mvwprintw(w,TITLELINENUM+2,LEFTMARGIN,"Running decompression, input = \"%s\", output = \"%s\"",decompressinfile,decompressoutfile);
	const int topline = TITLELINENUM+4;
	box(w,0,0);
	wrefresh(w);
	wgetch(w);
}

// main. draw / handle ncurses window, menus, driver for huffman.c
int main(int argc, char **argv) {

	initscr(); //init ncurses

	cbreak();
	noecho();
	w = newwin(0,0,0,0); //new window, (height,width,starty,startx)

	keypad(w,TRUE);
	notimeout(w,TRUE);
	intrflush(w,TRUE);
	curs_set(0); //disable cursor
//	else NUMPASSES=4;

	int menulevel = MAINMENU; //COMPRESSMENU, DECOMPRESSMENU
	int highlight = setmenuoptions(menulevel);

	while (1) {
		int i=0;
		wattroff(w,A_STANDOUT);
		const int leftstop = CENTERMARGIN;
		mvwprintw(w,TITLELINENUM,leftstop,"%s",titlebar);
		while (menuoptions[i][0]) {
			//wmove(w,i,0);
			//clrtoeol();
			if (i==highlight)
				wattron(w,A_STANDOUT); //active highlighted option
			else wattroff(w,A_STANDOUT);
			mvwprintw(w,i+MENUFIRSTLINE,leftstop,"%s",menuoptions[i]);
			++i;
		}
		box(w,0,0);
		wrefresh(w);
		if (processinput(wgetch(w),&menulevel,&highlight,i-1)) break;
		wclear(w);
		wrefresh(w);
	}
	nocbreak();
	delwin(w);
	endwin();
	return 0;
}
