#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include "common.h"
#include <ncurses.h>

extern struct node *hfcroot;
extern struct node **hfcheap;
extern int uniquebytes;

#define setcolor(W,C); wattr_on(W,COLOR_PAIR(C),NULL);
#define unsetcolor(W,C); wattr_off(W,COLOR_PAIR(C),NULL);
// draw the border box on window W with color pair C and refresh the window
#define refreshwithborder(W,C); setcolor(W,C); box(W,0,0); wrefresh(W); unsetcolor(W,C);
// Another way to do borders, specifying every element -> wborder(stdscr,'|','|','_','_',' ',' ','|','|');
// print on window W at row X col Y using color pair C with format F and args (like printf)
#define printcolor(W,X,Y,C,F,...); setcolor(W,C); mvwprintw(W,X,Y,F,__VA_ARGS__); unsetcolor(W,C);
// print with attribute A on window W at row X col Y with format F and args
#define printwithattr(W,X,Y,A,F,...); wattron(W,A); mvwprintw(W,X,Y,F,__VA_ARGS__); wattroff(W,A);
// print with attribute and color
#define printwithattrandcolor(W,X,Y,A,C,F,...); setcolor(W,C); wattron(W,A); mvwprintw(W,X,Y,F,__VA_ARGS__); wattroff(W,A); unsetcolor(W,C);
// general menu char buffer size
#define MAXSTRLEN 100
// max items of any menu, plus one for empty str which signifies menu end
#define MAXMENUITEMS 7
// row number for the title
#define TITLELINENUM 3
// first line of menus
#define MENUFIRSTLINE 5
// first left column position, if CENTERMARGIN doesn't work
#define LEFTMARGIN 4
// margin specifies left column number
// attempts to center text on the screen based on length of char* titlebar
// if LEFTMARGIN doesn't exist to the right, CENTERMARGIN falls back to LEFTMARGIN
#define CENTERMARGIN ((COLS>>1)-(strlen(titlebar)>>1)+strlen(titlebar)<COLS-LEFTMARGIN)?(COLS>>1)-(strlen(titlebar)>>1):LEFTMARGIN
// max FILENAMELEN, where we stop the input
#define FILENAMELEN 40

// node defined in "common.h"
typedef struct node node;

// sets global menuoptions[][] strings, final string begins with '\0'.
static int setmenuoptions(unsigned char menulevel);

// process input, ch is user's input from main menus
// *menulevel is from menulevels enum, *highlight is the active line
// lastmenuitem is used for wrapping arrow selection navigation
static unsigned char processinput(const int ch,int *menulevel,int *highlight,const int lastmenuitem);

// active display for compression/decompression, operation determined by codec parameter
void codecscreen(const unsigned char codec);

// display for the huffman tree
static void treescreen();

// the ncurses window ... using "stdscr" now
//WINDOW *w;

// enum to indicate current menu level
enum menulevels { MAINMENU, COMPRESSMENU, DECOMPRESSMENU };
// text on background color (foreground / background)
enum fancycolors { HFCBLKGRN=1,HFCBLKCYN=2,HFCBLKRED=3,HFCWHTBLK=4,HFCWHTBLU=5,HFCCYNBLU=6,HFCCYNBLK=7,HFCGRNBLK=8,HFCREDBLK=9,HFCREDCYN=10,HFCBLUWHT=11 };

// constant prompts
const char * const titlebar = "Data Compression using Huffman Encoding ~ implemented by Chase Phelps";
const char * const enterpathprompt = "(enter a valid file / path)";
const char * const entershortprompt = "(enter a valid non-zero number from 1-9)";

// buffers for input / output filenames
char compressinfile[FILENAMELEN] = "(not set)";
char compressoutfile[FILENAMELEN] = "out.hfc";
char decompressinfile[FILENAMELEN] = "(not set)";
char decompressoutfile[FILENAMELEN] = "(auto)";

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
	wmove(stdscr,row,col);
	wclrtoeol(stdscr);
	refreshwithborder(stdscr,HFCBLKGRN);
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
			else if (i==2) strcpy(menuoptions[i],"View huffman tree\n");
			else if (i==3) {
				if (!hfcroot)
					strcpy(menuoptions[i],"Free compression data structures\n");
				else
					strcpy(menuoptions[i],"*Free compression data structures*\n");
			}
			else if (i==4) strcpy(menuoptions[i],"Exit\n");
			else { menuoptions[i][0]='\0'; return 0; }
		}
		else if (menulevel==COMPRESSMENU) {
			if (i==0)
			{ strcpy(menuoptions[i],"File to compress: "); concatwithparenthesis(menuoptions[i],compressinfile); }
			else if (i==1)
			{ strcpy(menuoptions[i],"Output file: "); concatwithparenthesis(menuoptions[i],compressoutfile); }
			else if (i==2) sprintf(menuoptions[i],"Pass count: %u",NUMPASSES);
			else if (i==3) strcpy(menuoptions[i],"Begin compression");
			else if (i==4) strcpy(menuoptions[i],"View huffman tree\n");
			else if (i==5) strcpy(menuoptions[i],"Return to main menu");
			else { menuoptions[i][0]='\0'; return 0; }
		}
		else {//if (menulevel==DECOMPRESSMENU) {
			if (i==0)
			{ strcpy(menuoptions[i],"File to decompress: "); concatwithparenthesis(menuoptions[i],decompressinfile); }
			else if (i==1)
			{ strcpy(menuoptions[i],"Output file: "); concatwithparenthesis(menuoptions[i],decompressoutfile); }
			else if (i==2) strcpy(menuoptions[i],"Begin decompression");
			else if (i==3) strcpy(menuoptions[i],"View huffman tree\n");
			else if (i==4) strcpy(menuoptions[i],"Return to main menu");
			else { menuoptions[i][0]='\0'; return 0; }
		}
	}
}

// handles input from menus, may update highlight pos or menulevel, returns nonzero to exit program
static unsigned char processinput(const int ch,int *menulevel,int *highlight,const int lastmenuitem) {
	if (ch==KEY_UP) {
		if (*highlight==0) *highlight=lastmenuitem;
		else *highlight-=1;
	}
	else if (ch==KEY_DOWN) {
		if (*highlight==lastmenuitem) *highlight=0;
		else *highlight+=1;
	}
	else if (ch==KEY_ENTER || ch=='\n') {
		char buffer[MAXSTRLEN+1];
		char *inputpos;
		int usrinput,i;
		const int leftstop=CENTERMARGIN;
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
				else if (*highlight==2) // tree screen
					treescreen();
				else if (*highlight==3) {// clear datastructures
					if (hfcroot) {
						clearhfcvars();
						strcpy(menuoptions[*highlight],"Free compression data structures\n");
						wmove(stdscr,*highlight+MENUFIRSTLINE,leftstop);
						wclrtoeol(stdscr);
						mvwprintw(stdscr,*highlight+MENUFIRSTLINE,leftstop,"%s",menuoptions[*highlight]);
						refreshwithborder(stdscr,HFCBLKGRN);
					}
				}
				else return 1; //quit program
				break;
			case COMPRESSMENU: case DECOMPRESSMENU:
				//same menu layout for filenames, first two options are input/output filenames
				if (*highlight<2) {
					curs_set(2);
					mvwprintw(stdscr,MENUFIRSTLINE+lastmenuitem+2,leftstop,"%s",enterpathprompt);
					//change input or output file, filename enclosed in quotes
					inputpos = strchr(menuoptions[*highlight],'\"');
					*inputpos='\0';
					wmove(stdscr,*highlight+MENUFIRSTLINE,leftstop+strlen(menuoptions[*highlight]));
					wclrtoeol(stdscr);
					refreshwithborder(stdscr,HFCBLKGRN);
					i=0;
					while (1) {
						usrinput = wgetch(stdscr);
						if (usrinput==KEY_RESET || usrinput==KEY_BREAK || usrinput==KEY_CANCEL || usrinput==KEY_EXIT || usrinput==27) {
							flushinp();
							*inputpos='\"';
							wmove(stdscr,*highlight+MENUFIRSTLINE,leftstop);
							wclrtoeol(stdscr);
							mvwprintw(stdscr,*highlight+MENUFIRSTLINE,leftstop,"%s",menuoptions[*highlight]);
							refreshwithborder(stdscr,HFCBLKGRN);
							break;
						}
						if (usrinput==KEY_BACKSPACE || usrinput==127 || usrinput=='\b') {
							if (i) dobackspace(*highlight+MENUFIRSTLINE,(--i)+leftstop+strlen(menuoptions[*highlight]));
							else flash();
						}
						else if (isalpha(usrinput) || usrinput=='/' || usrinput=='.' || usrinput=='~' || usrinput=='_' || isdigit(usrinput)) {
							if (i<FILENAMELEN) {
								waddch(stdscr,usrinput);
								buffer[i++]=usrinput;
								wrefresh(stdscr);
							}
							else flash();
						}
						if (usrinput==KEY_ENTER || usrinput=='\n') {
							if (i) {
								buffer[i]='\0';
								concatwithparenthesis(menuoptions[*highlight],buffer);
								if (*menulevel==COMPRESSMENU && *highlight==0) strcpy(compressinfile,buffer);
								else if (*menulevel==COMPRESSMENU) strcpy(compressoutfile,buffer);
								else if (*menulevel==DECOMPRESSMENU && *highlight==0) strcpy(decompressinfile,buffer);
								else strcpy(decompressoutfile,buffer);
							}
							else {
								*inputpos='\"';
								wmove(stdscr,*highlight+MENUFIRSTLINE,leftstop);
								wclrtoeol(stdscr);
								mvwprintw(stdscr,*highlight+MENUFIRSTLINE,leftstop,"%s",menuoptions[*highlight]);
								refreshwithborder(stdscr,HFCBLKGRN);
							}
							break;
						}
					}
					curs_set(0);
				}
				else if (*highlight==2) { //compress: set num passes, decompress: decompress()
					if (*menulevel==DECOMPRESSMENU) {
						codecscreen(*menulevel);
						break;
					}
					curs_set(2);
					mvwprintw(stdscr,MENUFIRSTLINE+lastmenuitem+2,leftstop,"%s",entershortprompt);
					//change digit, format("menu line: %d")
					inputpos = strchr(menuoptions[*highlight],':')+2;
					*inputpos='\0';
					wmove(stdscr,*highlight+MENUFIRSTLINE,leftstop+strlen(menuoptions[*highlight]));
					wclrtoeol(stdscr);
					refreshwithborder(stdscr,HFCBLKGRN);
					i=0;
					while (1) {
						usrinput = wgetch(stdscr);
						if (usrinput==KEY_RESET || usrinput==KEY_BREAK || usrinput==KEY_CANCEL || usrinput==KEY_EXIT || usrinput==27) {
							flushinp();
							wmove(stdscr,*highlight+MENUFIRSTLINE,leftstop+strlen(menuoptions[*highlight]));
							wclrtoeol(stdscr);
							sprintf(buffer,"%u",NUMPASSES);
							mvwprintw(stdscr,*highlight+MENUFIRSTLINE,leftstop+strlen(menuoptions[*highlight]),"%s",buffer);
							strcat(menuoptions[*highlight],buffer);
							break;
						}
						if (usrinput==KEY_BACKSPACE || usrinput==127 || usrinput=='\b') {
							if (i) dobackspace(*highlight+MENUFIRSTLINE,(--i)+leftstop+strlen(menuoptions[*highlight]));
							else flash();
						}
						else if (isdigit(usrinput)) {
							if (!i && usrinput!='0') {
								waddch(stdscr,usrinput);
								buffer[i++]=usrinput;
								wrefresh(stdscr);
							}
							else flash();
						}
						else if (usrinput==KEY_ENTER || usrinput=='\n') {
							if (i) {
								NUMPASSES=buffer[0]-'0';
								buffer[1]='\0';
								strcat(menuoptions[*highlight],buffer);
							}
							else {
								sprintf(buffer,"%u",NUMPASSES);
								mvwprintw(stdscr,*highlight+MENUFIRSTLINE,leftstop+strlen(menuoptions[*highlight]),"%s",buffer);
								strcat(menuoptions[*highlight],buffer);
							}
							break;
						}
					}
					curs_set(0);
//					wborder(stdscr,'|','|','_','_',' ',' ','|','|');
					refreshwithborder(stdscr,HFCBLKGRN);
				}
				else if (*highlight==3) { // compress: compress(), decompress: tree menu
					if (*menulevel==DECOMPRESSMENU) treescreen();
					else codecscreen(*menulevel);
				}
				else if (*highlight==4) { // compress: tree menu, decompress: return to main menu
					if (*menulevel==DECOMPRESSMENU) {
						*menulevel=MAINMENU;
						*highlight=setmenuoptions(MAINMENU);
					}
					else treescreen();
				}
				else { // compress: return to main menu
					*menulevel=MAINMENU;
					*highlight=setmenuoptions(MAINMENU);
				}
				break;
		}
	}
	return 0;
}

static inline void drawerrorandgetch(const int row,const int col,char *format1,char *str1,char *format2,char *str2) {
	if (format1) {
		printwithattrandcolor(stdscr,row,col,A_STANDOUT,HFCWHTBLU,format1,str1);
	}
	else return;
	if (format2) {
		printwithattrandcolor(stdscr,row+2,col,A_STANDOUT,HFCBLKGRN,format2,str2);
	}
	curs_set(2);
	wgetch(stdscr);
	curs_set(0);
}
// display for the huffman tree
static void treescreen() {
}
// ncurses extended char chart -> http://melvilletheatre.com/articles/ncurses-extended-characters/index.html
// compress screen
void codecscreen(const unsigned char codec) {
	wclear(stdscr);
	wrefresh(stdscr);
	int leftstop=CENTERMARGIN;
	printcolor(stdscr,TITLELINENUM,leftstop,HFCGRNBLK,"%s",titlebar);
	char *input,*output;
	if (codec==1) {
		input=compressinfile;
		output=compressoutfile;
	}
	else {
		input=decompressinfile;
		output=decompressoutfile;
	}
	FILE *infile = fopen(input,"rb");
	if (!infile) {
		if (codec==1)
		drawerrorandgetch(TITLELINENUM+2,leftstop,"Unable to open input \"%s\" !",input,"Press the any key to continue . . . ",NULL);
		return;
	}
	FILE *outfile = fopen(output,"wb");
	if (!outfile) {
		drawerrorandgetch(TITLELINENUM+2,leftstop,"Unable to open output \"%s\" !",output,"Press the any key to continue . . . ",NULL);
		return;
	}
	if (codec==1)
		docompress(infile,outfile,0); // last parameter disables printing
	else dorestore(infile,outfile,0);
	fseek(infile,0L,SEEK_END);
	fseek(outfile,0L,SEEK_END);
	const unsigned long long insize = ftell(infile);
	const unsigned long long outsize = ftell(outfile);
	fclose(infile);
	fclose(outfile);
	setcolor(stdscr,HFCGRNBLK);
	wmove(stdscr,TITLELINENUM+2,leftstop);
	if (codec==1) wprintw(stdscr,"Co");
	else wprintw(stdscr,"Deco");
	wprintw(stdscr,"mpressed the input file \"%s\" (%lluB) and created \"%s\" (%lluB)",input,insize,output,outsize);
	double pctcompare = ((double)outsize)/((double)insize);
	double pctchange = (outsize>=insize)?((double)(outsize-insize))/((double)insize):((double)(insize-outsize))/((double)insize);
	mvwprintw(stdscr,TITLELINENUM+4,leftstop,"The output file is %.2f%% the size of the input, this is a ",pctcompare*100);
	if (outsize<insize)
		waddch(stdscr,'-');
	wprintw(stdscr,"%.2f%% ",pctchange*100);
	if (outsize<insize) waddstr(stdscr,"decrease\n");
	else waddstr(stdscr,"increase\n");
	unsetcolor(stdscr,HFCGRNBLK);
	refreshwithborder(stdscr,HFCBLKGRN);
	wgetch(stdscr);
}

// main. draw / handle ncurses window, menus, driver for huffman.c
int main(int argc, char **argv) {

// init ncurses
	setlocale(LC_ALL,"");
	initscr();
	start_color();
// HFCBLKGRN=1,HFCBLKCYN=2,HFCWHTBLK=4,HFCWHTBLU=5,HFCCYNBLU=6,HFCCYNBLK=7,HFCGRNBLK=8,HFCREDBLK=9,HFCREDCYN=10,HFCBLUWHT=11 
	init_pair(HFCBLKGRN, COLOR_BLACK, COLOR_GREEN);
	init_pair(HFCBLKCYN, COLOR_BLACK, COLOR_CYAN);
	init_pair(HFCBLKRED, COLOR_BLACK, COLOR_RED);
	init_pair(HFCWHTBLK, COLOR_WHITE, COLOR_BLACK);
	init_pair(HFCWHTBLU, COLOR_WHITE, COLOR_BLUE);
	init_pair(HFCCYNBLU, COLOR_CYAN, COLOR_BLUE);
	init_pair(HFCCYNBLK, COLOR_CYAN, COLOR_BLACK);
	init_pair(HFCGRNBLK, COLOR_GREEN, COLOR_BLACK);
	init_pair(HFCREDBLK, COLOR_RED, COLOR_BLACK);
	init_pair(HFCREDCYN, COLOR_RED, COLOR_CYAN);
	init_pair(HFCBLUWHT, COLOR_BLUE, COLOR_WHITE);
	cbreak();
	noecho();

//	w = newwin(0,0,0,0); //new window, (height,width,starty,startx)

	keypad(stdscr,TRUE);
	notimeout(stdscr,TRUE);
	intrflush(stdscr,TRUE);
	curs_set(0); //disable cursor

	int menulevel = MAINMENU; //COMPRESSMENU, DECOMPRESSMENU
	int highlight = setmenuoptions(menulevel);
	wclear(stdscr);
	wrefresh(stdscr);
	while (1) {
		int i=0;
		const int leftstop = CENTERMARGIN;
		printcolor(stdscr,TITLELINENUM,leftstop,HFCGRNBLK,"%s",titlebar);
		while (menuoptions[i][0]) {
			if ((menuoptions[i][0]=='F' && menuoptions[i][1]=='r' && menuoptions[i][2]=='e') ||
				(menuoptions[i][0]=='*' && menuoptions[i][1]=='F' && menuoptions[i][2]=='r')) {
				if (hfcroot) {
					if (i==highlight) {
						printwithattrandcolor(stdscr,i+MENUFIRSTLINE,leftstop,A_STANDOUT,HFCWHTBLU,"%s",menuoptions[i]);
					}
					else {
						printwithattrandcolor(stdscr,i+MENUFIRSTLINE,leftstop,A_DIM,HFCGRNBLK,"%s",menuoptions[i]); // A_DIM (dim)
					}
				}
				else {
					if (i==highlight) {
						printwithattrandcolor(stdscr,i+MENUFIRSTLINE,leftstop,A_DIM,HFCGRNBLK,"%s",menuoptions[i]);
					}
					else {
						printwithattr(stdscr,i+MENUFIRSTLINE,leftstop,A_INVIS,"%s",menuoptions[i]);
					}
				}
			}
			else if (i==highlight) { // active highlighted option
				printwithattr(stdscr,i+MENUFIRSTLINE,leftstop,A_STANDOUT,"%s",menuoptions[i]);
			}
			else mvwprintw(stdscr,i+MENUFIRSTLINE,leftstop,"%s",menuoptions[i]);
			++i;
		}
		refreshwithborder(stdscr,HFCBLKGRN);
		if (processinput(wgetch(stdscr),&menulevel,&highlight,i-1)) break;
		wclear(stdscr);
		wrefresh(stdscr);
	}
	nocbreak();
//	delwin(w);
	endwin();
	if (hfcroot) clearhfcvars(); // shame shame
	return 0;
}
