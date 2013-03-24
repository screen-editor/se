/*
** extern.h
**
** external data definitions
** for the screen editor
*/

/* Concerning line numbers: */
extern int Line1;		/* first line number on command */
extern int Line2;		/* second line number on command */
extern int Nlines;		/* number of line numbers specified */
extern int Curln;		/* current line; value of dot */
extern int Lastln;		/* last line; value of dollar */


/* Concerning patterns: */
extern char Pat[MAXPAT];	/* saved pattern */


/* Concerning the text of lines: */
extern char Txt[MAXLINE];	/* text of current line */


/* Concerning file names: */
extern char Savfil[MAXLINE];	/* remembered file name */


/* Concerning line descriptors: */
extern LINEDESC Buf[MAXBUF];
#ifdef OLD_SCRATCH
extern LINEDESC *Lastbf;	/* last pointer used in Buf */
extern LINEDESC *Free;		/* head of free list */
#endif
extern LINEDESC *Line0;		/* head of list of line descriptors */


/* Concerning the 'undo' command: */
extern LINEDESC *Limbo;		/* head of limbo list for undo */
extern int Limcnt;		/* number of lines in limbo list */


/* Concerning the scratch file: */
extern filedes Scr;		/* scratch file descriptor */
extern unsigned Scrend;		/* end of info on scratch file */
extern char Scrname[MAXLINE];	/* name of scratch file */
extern int Lost_lines;		/* number of garbage lines in scratch file */


/* Concerning miscellaneous variables */
extern int Buffer_changed;	/* YES if buffer changed since last write */
extern int Errcode;		/* cause of most recent error */
extern int Saverrcode;		/* cause of previous error */
extern int Probation;		/* YES if unsaved buffer can be destroyed */
extern int Argno;		/* command line argument pointer */
extern char Last_char_scanned;	/* last char scanned with ctl-s or -l */
#ifdef HARD_TERMS
extern int Tspeed;		/* terminal speed in characters/second */
#endif
extern char Peekc;		/* push a SKIP_RIGHT if adding delimiters */
#ifdef BSD4_2
extern int Reading;		/* are we doing terminal input? */
#endif


/* Concerning options: */
extern int Tabstops[MAXLINE];	/* array of tab stops */
extern char Unprintable;	/* char to print for unprintable chars */
extern int Absnos;		/* use absolute numbers in margin */
extern int Nchoise;		/* choice of line number for cont. display */
extern int Overlay_col;		/* initial cursor column for 'v' command */
extern int Warncol;		/* where to turn on column warning */
extern int Firstcol;		/* leftmost column to display */
extern int Indent;		/* indent col; 0=same as previous line */
extern int Notify;		/* notify user if he has mail in mail file */
extern int Globals;		/* substitutes in a global don't fail */
extern int No_hardware;		/* never use hardware insert/delete */


#ifdef HARD_TERMS
/* Concerning the terminal type */
extern int Term_type;          /* terminal type */
#endif


/* Concerning the screen format: */
extern char Screen_image[MAXROWS][MAXCOLS];
extern char Msgalloc[MAXCOLS];	/* column allocation of status line */
extern int Nrows;		/* number of rows on screen */
extern int Ncols;		/* number of columns on screen */
extern int Currow;		/* vertical cursor coordinate */
extern int Curcol;		/* horizontal cursor coordinate */
extern int Toprow;		/* top row of window field on screen */
extern int Botrow;		/* bottom row of window field on screen */
extern int Cmdrow;		/* row number of command line */
extern int Topln;		/* line number of first line on screen */
extern int Insert_mode;		/* flag to specify character insertion */
extern int Invert_case;		/* flag to specify case mapping on input */
extern int First_affected;	/* number of first line affected by cmd */
extern int Rel_a;		/* char to use for first alpha line number */
extern int Rel_z;		/* char to use for last alpha line number */
extern int Scline[MAXROWS];	/* lines currently on screen (rel to Sctop) */
extern int Sctop;		/* first line currently on screen */
extern int Sclen;		/* number of lines currently on screen */
extern char Blanks[MAXCOLS];	/* all blanks for filling in lines on screen */
extern char Tobuf[MAXTOBUF];	/* buffer for collecting terminal output */
extern char *Tobp;		/* pointer to last used part of Tobuf */


/* Concerning interrupts: */
extern int Int_caught;		/* caught a SIGINT from user */
extern int Hup_caught;		/* caught a SIGHUP when phone line dropped */
#ifdef SIGTSTP
extern int Catching_stops;	/* catching or ignoring SIGTSTP's? */
#endif

/* Concerning Unix and SWT compatiblity: */
extern int Unix_mode;		/* behaving like Unix editors? */
extern char BACKSCAN;		/* back scan character */
extern char NOTINCCL;		/* class negation character */
extern char XMARK;		/* global exclude on mark name */
extern char ESCAPE;		/* escape character */

/* Concerning Georgia Tech I.C.S. specific code: */
extern int At_gtics;		/* are we at Georgia Tech ICS? */

/* Concerning file encryption: */
extern int Crypting;		/* doing file encryption? */
extern char Key[KEYSIZE];	/* encryption key */
