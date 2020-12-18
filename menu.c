#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <ncurses.h>
#include "common.h"

// node defined in "common.h", extern var in huffmantree.c (libhfc)
extern struct node *hfcroot;
typedef struct node node;

// macros need braces after a conditional because they encompass more than one 'line'
// shorthand to enable / disable colors on a window
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


// sets global menuoptions[][] strings, final string begins with '\0'.
static int setmenuoptions(unsigned char menulevel);

// process input, ch is user's input from main menus
// *menulevel is from menulevels enum, *highlight is the active line
// lastmenuitem is used for wrapping arrow selection navigation
static unsigned char processinput(const int ch,int *menulevel,int *highlight,const int lastmenuitem);

// active display for compression/decompression, operation determined by codec parameter
void codecscreen(const unsigned char codec);

// display for the huffman tree
static void treescreen(const int callingmenu);

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
char decompressinfile[FILENAMELEN] = "out.hfc";
char decompressoutfile[FILENAMELEN] = "(not set)";
char treefile[FILENAMELEN] = "out.hfc";

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
			else if (i==2) strcpy(menuoptions[i],"Huffman tree menu\n");
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
			else if (i==4) strcpy(menuoptions[i],"Huffman tree menu\n");
			else if (i==5) strcpy(menuoptions[i],"Return to main menu");
			else { menuoptions[i][0]='\0'; return 0; }
		}
		else {//if (menulevel==DECOMPRESSMENU) {
			if (i==0)
			{ strcpy(menuoptions[i],"File to decompress: "); concatwithparenthesis(menuoptions[i],decompressinfile); }
			else if (i==1)
			{ strcpy(menuoptions[i],"Output file: "); concatwithparenthesis(menuoptions[i],decompressoutfile); }
			else if (i==2) sprintf(menuoptions[i],"Pass count: %u",NUMPASSES);
			else if (i==3) strcpy(menuoptions[i],"Begin decompression");
			else if (i==4) strcpy(menuoptions[i],"Huffman tree menu\n");
			else if (i==5) strcpy(menuoptions[i],"Return to main menu");
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
				else if (*highlight==2) {// tree screen
					if (hfcroot)
						treescreen(MAINMENU);
				}
				else if (*highlight==3) {// clear datastructures
					if (hfcroot) {
						freehuffmantree(hfcroot);
						hfcroot=NULL;
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
						else if (isalpha(usrinput)||usrinput=='/'||usrinput=='.'||usrinput=='~'||usrinput=='-'||usrinput=='_'||isdigit(usrinput)) {
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
				else if (*highlight==2) { //set num passes
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
				else if (*highlight==3) // compress: compress(), decompress: restore()
					codecscreen(*menulevel);
				else if (*highlight==4) // tree menu
					treescreen(*menulevel);
				else { // return to main menu
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
// activex and y represent the highlighted positions
// activex: x=0 menu(free), x=1 menu(quit), x=2 is row zero of the element printing
// the highlighted node is set from activex and y as a countdown, when highlighttimer reaches zero we are at the highlight node
static void popanode(node *root,int *printrow,int *printcol,int *modifier,int *highlighttimer,
					const int leftstop,const int numcols,const int colwidth,const int ylimit) {
	// can't print any more rows
	if (*printrow>ylimit) return;
	// value node, print and return
	if (root->strval) {
		if (*modifier>0) { // skip printing first rows to do window frame 'movement'
			if (root->strval) {
				*printcol = *printcol+1;
				if (*printcol==numcols) {
					*printcol=0;
					*modifier = *modifier-1;
				}
				return;
			}
		}
		// this is the highlight node
		if (*highlighttimer==0) {
			if (root->count) {
				printwithattrandcolor(stdscr,*printrow,leftstop+(*printcol)*colwidth,A_STANDOUT,HFCWHTBLU,"(%llu) [0x%.2x: %s]",root->count,root->val,root->strval);
			}
			else {
				printwithattrandcolor(stdscr,*printrow,leftstop+(*printcol)*colwidth,A_STANDOUT,HFCWHTBLU,"[0x%.2x: %s]",root->val,root->strval);
			}
			*highlighttimer=-1;
		}
		// non-highlighted value node
		else {
			if (root->count) {
				printcolor(stdscr,*printrow,leftstop+(*printcol)*colwidth,HFCCYNBLK,"(%llu) [0x%.2x: %s]",root->count,root->val,root->strval);	
			}
			else {
				printcolor(stdscr,*printrow,leftstop+(*printcol)*colwidth,HFCCYNBLK,"[0x%.2x: %s]",root->val,root->strval);
			}
			if (*highlighttimer>0)
				*highlighttimer = *highlighttimer-1;
		}
		*printcol = *printcol+1;
		if (*printcol==numcols) {
			*printrow = *printrow + 1;
			*printcol = 0;
		}
		return;
	}
	popanode(root->right,printrow,printcol,modifier,highlighttimer,leftstop,numcols,colwidth,ylimit);
	popanode(root->left,printrow,printcol,modifier,highlighttimer,leftstop,numcols,colwidth,ylimit);
}
static inline int numuniquebytes(node *root) {
	if (!root) return 0;
	if (root->strval) return 1;
	return numuniquebytes(root->left)+numuniquebytes(root->right);
}
// display for the huffman tree, calls drawtreeframe for each screen, handles input and print positions
// if no tree exists it directs the menu to loadatree() which can return here or the main menu
static void treescreen(const int callingmenu) {
	if (!hfcroot) return; // no tree.
	unsigned char lockrow=0,activey=0,activex=2;
	const int uniquebytes = numuniquebytes(hfcroot);
	while (1) {
		wclear(stdscr);
		// title display area
		int leftstop=CENTERMARGIN;
		printcolor(stdscr,TITLELINENUM,leftstop,HFCGRNBLK,"%s",titlebar);
		const char *freeoption = "Free the huffman tree";
		const char *exitoption = "Exit tree view";
		int xlimit,ylimit;
		getmaxyx(stdscr,ylimit,xlimit);
		// the two menu entries
		if (activex==0) {
			printwithattr(stdscr,2+TITLELINENUM,leftstop,A_STANDOUT,"%s (num byte nodes = %d)",freeoption,uniquebytes);
		}
		else {
			printcolor(stdscr,2+TITLELINENUM,leftstop,HFCGRNBLK,"%s",freeoption);
		}
		if (activex==1) {
			printwithattr(stdscr,3+TITLELINENUM,leftstop,A_STANDOUT,"%s",exitoption);
		}
		else {
			printcolor(stdscr,3+TITLELINENUM,leftstop,HFCGRNBLK,"%s",exitoption);
		}
		// we want to use the bottom rectangle to draw the tree values.
		// ("[0x%.2x]",node->val) <- this is 6 chars
		// ("[0x%.2x: %s]",node->val,node->strval) <- this is 8 chars + strlen(path), ~( 10-20 )
		// if it were to also include frequency (tree from compression)
		// ("(134123)[0x00: "00"]",node->count,node->val,node->strval)
		// put frequency at 6 digits, then we have the above length + 8.
		// let's try value gap size of { (with frequencies) : 25, (without frequencies) : 22 }
		// the screen should be divided in to regions.
		const int numcols = (hfcroot->count)?(xlimit-2)/25 : (xlimit-2)/22;
		const int colwidth = xlimit/numcols;
		int firstcol = (xlimit-(numcols*colwidth))>>1;
		if (firstcol<2) firstcol=2;
		const int numrows = (uniquebytes+numcols-1)/numcols;
		int currentcol = 0;
		int currentrow = 5+TITLELINENUM;
		// dynamically center the row by skipping modifier rows until the active row is visible
		int modifier = 0;
		const int firstsplit=ylimit>>1;
		if (activex>=firstsplit) {// active row is beyond the top half of the screen
			modifier = activex-firstsplit;
		}
		// we are skipping lines. setting the lockrow locks the modifier, resetting the lockrow allows window resizing.
		if (modifier) {
			// if the remaining rows (minus modifier)
			// is less than the ylimit - current row
			// then we have empty space, lock modifier at numrows-(
			if ((numrows-modifier)+currentrow<ylimit-2) {
				if (!lockrow) lockrow = modifier;
				else modifier=lockrow;
			}
			else lockrow=0;
		}
		else lockrow=0;
		// highlighted node is a combination of the activex and activey, compute the highlighted node's number now
		int highlighttimer = (activex-modifier-2)*numcols + activey;
		// print node values
		popanode(hfcroot,&currentrow,&currentcol,&modifier,&highlighttimer,firstcol,numcols,colwidth,ylimit);
		refreshwithborder(stdscr,HFCBLKGRN);
		int ch = wgetch(stdscr);
		if (ch==KEY_HOME) activex=1;
		else if (ch==KEY_END) {
			unsigned char oldactivex=activex;
			if (uniquebytes%numcols) {
				activex=numrows+1;
				activey=uniquebytes%numcols-1;
			}
			else { activex=numrows+1; activey=numcols-1; }
			// copied the row modifier logic from above to do a fast forward
			if (!lockrow) {
				while (oldactivex++<activex) {
					if (oldactivex>=firstsplit) {
						modifier = oldactivex-firstsplit;
						if (modifier) {
							if ((numrows-modifier)+5+TITLELINENUM<ylimit-2) { // current row is at the bottom of the screen now
								lockrow=modifier;
								break;
							}
						}
					}
				}
			}
		}
		else if (ch==KEY_UP) {
			if (activex>0) {
				if (--activex==1) activey=0;
			}
		}
		else if (ch==KEY_RIGHT) {
			const int lastcols = uniquebytes%numcols;
			if (lastcols && activex==numrows+1) {
				if (activey < lastcols-1) ++activey;
			}
			else if (activey<numcols-1) ++activey;
		}
		else if (ch==KEY_DOWN) {
			if (activex<numrows) {
				if (++activex==2) activey=0;
			}
			else if (activex==numrows) {
				const int lastcols = uniquebytes%numcols;
				if (!lastcols || (lastcols && activey<lastcols))
					++activex;
			}
		}
		else if (ch==KEY_LEFT) { if (activey>0) --activey; }
		else if (ch==KEY_ENTER || ch=='\n') {
			if (activex<2) {
				if (activex==0) {
					freehuffmantree(hfcroot);
					hfcroot=NULL;
				}
				break;
			}
		}
	}
	wclear(stdscr);
	wrefresh(stdscr);
}
// ncurses extended char chart -> http://melvilletheatre.com/articles/ncurses-extended-characters/index.html
// compress/decompress screen, do the action and show the in/out file name+size + amount of change
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
	if (hfcroot) {
		freehuffmantree(hfcroot);
		hfcroot=NULL;
	}
	int passesremaining=NUMPASSES;
	if (passesremaining>1) {
		char *tmp1name = "._multipass.hfc";
		char *tmp2name = "._multipass2.hfc";
		FILE *tmpfile = fopen(tmp1name,"wb");
		if (!tmpfile) { fprintf(stderr,"Error opening %s for multiple passes\n",tmp1name); }
		else {
			FILE *tmp2file=NULL;
			if (codec==1) docompress(infile,tmpfile);
			else dorestore(infile,tmpfile);
			fclose(tmpfile);
			tmpfile=fopen(tmp1name,"rb");
			unsigned char inpos=0; // 0 to read tmp, 1 to read tmp2
			if (--passesremaining) {
				while (--passesremaining) {
					if (!inpos) tmp2file=fopen(tmp2name,"wb");
					else tmpfile=fopen(tmp1name,"wb");
					if (!tmpfile || !tmp2file) {
						fprintf(stderr,"Error opening tmp files for multiple passes\n");
						break;
					}
					freehuffmantree(hfcroot); hfcroot=NULL;
					if (codec==1 && !inpos) docompress(tmpfile,tmp2file);
					else if (codec==1) docompress(tmp2file,tmpfile);
					else if (!inpos) dorestore(tmpfile,tmp2file);
					else dorestore(tmp2file,tmpfile);
					fclose(tmpfile); fclose(tmp2file);
					if (!inpos) {
						tmp2file=fopen(tmp2name,"rb");
						inpos=1;
					}
					else {
						tmpfile=fopen(tmp1name,"rb");
						inpos=0;
					}
				}
			}
			freehuffmantree(hfcroot); hfcroot=NULL;
			if (codec==1 && !inpos) docompress(tmpfile,outfile);
			else if (codec==1) docompress(tmp2file,outfile);
			else if (!inpos) dorestore(tmpfile,outfile);
			else dorestore(tmp2file,outfile);
			if (!inpos) fclose(tmpfile);
			else fclose(tmp2file);
			remove(tmp1name); remove(tmp2name);
		}
	}
	if (passesremaining) {
		if (codec==1)
			docompress(infile,outfile);
		else dorestore(infile,outfile);
	}
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
	wprintw(stdscr,"%.2f%% ",pctchange*100);
	if (outsize<insize) waddstr(stdscr,"decrease\n");
	else waddstr(stdscr,"increase\n");
	unsetcolor(stdscr,HFCGRNBLK);
	printwithattrandcolor(stdscr,TITLELINENUM+8,leftstop,A_STANDOUT,HFCBLKGRN,"%s","Press the any key to continue . . . ");
	curs_set(2);
	refreshwithborder(stdscr,HFCBLKGRN);
	wgetch(stdscr);
	curs_set(0);
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
		while (menuoptions[i][0]!='\0') {
			if (!hfcroot && i==4 && (menulevel==COMPRESSMENU || menulevel==DECOMPRESSMENU)) { // tree menu option, no tree
				if (i==highlight) {
					printwithattrandcolor(stdscr,i+MENUFIRSTLINE,leftstop,A_DIM,HFCGRNBLK,"%s",menuoptions[i]);
				}
				else {
					printwithattr(stdscr,i+MENUFIRSTLINE,leftstop,A_INVIS,"%s",menuoptions[i]);
				}
			}
			else if (menulevel==MAINMENU && (i==2 || i==3)) { // tree options
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
	if (hfcroot) freehuffmantree(hfcroot); // shame shame
	return 0;
}
