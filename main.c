#ifndef lint
static char RCSid[] = "$Header: main.c,v 1.4 86/10/07 14:50:17 arnold Exp $";
#endif

/*
 * $Log:	main.c,v $
 * Revision 1.4  86/10/07  14:50:17  arnold
 * Changed setterm to set_term, to avoid Unix/PC shared library conflict.
 * 
 * Revision 1.3  86/07/17  17:20:58  arnold
 * Terminal initialization code cleaned up considerably.
 * 
 * Revision 1.2  86/07/11  15:12:26  osadr
 * Removed code that was Georgia Tech specific
 * 
 * Revision 1.1  86/05/06  13:37:38  osadr
 * Initial revision
 * 
 * 
 */

/*
** main.c
**
** main program and lots of other routines
** for the se screen editor.
*/

#include "se.h"

/* declare global variables */

/* Concerning line numbers: */
int Line1;		/* first line number on command */
int Line2;		/* second line number on command */
int Nlines;		/* number of line numbers specified */
int Curln;		/* current line; value of dot */
int Lastln;		/* last line; value of dollar */


/* Concerning patterns: */
char Pat[MAXPAT] = "";	/* saved pattern */


/* Concerning the text of lines: */
char Txt[MAXLINE];	/* text of current line */


/* Concerning file names: */
char Savfil[MAXLINE] = "";	/* remembered file name */


/* Concerning line descriptors: */
LINEDESC Buf[MAXBUF];
LINEDESC *Line0;	/* head of list of line descriptors */
#ifdef OLD_SCRATCH
LINEDESC *Lastbf;	/* last pointer used in Buf */
LINEDESC *Free;		/* head of free list */
#endif


/* Concerning the 'undo' command: */
LINEDESC *Limbo;	/* head of limbo list for undo */
int Limcnt;		/* number of lines in limbo list */


/* Concerning the scratch file: */
filedes Scr;		/* scratch file descriptor */
unsigned Scrend;	/* end of info on scratch file */
char Scrname[MAXLINE];	/* name of scratch file */
int Lost_lines;		/* number of garbage lines in scratch file */


/* Concerning miscellaneous variables */
int Buffer_changed = NO;/* YES if buffer changed since last write */
int Errcode = ENOERR;	/* cause of most recent error */
int Saverrcode = ENOERR;/* cause of previous error */
int Probation = NO;	/* YES if unsaved buffer can be destroyed */
int Argno;		/* command line argument pointer */
char Last_char_scanned = 0;	/* last char scanned w/ctl-[sl], init illegal  */
char Peekc = EOS;	/* push a SKIP_RIGHT if adding delimiters */
#ifdef HARD_TERMS
int Tspeed;		/* terminal speed in characters/second */
#endif
#ifdef BSD4_2
int Reading = NO;	/* are we doing terminal input? */
#endif


/* Concerning options: */
int Tabstops[MAXLINE];	/* array of tab stops */
char Unprintable = ' ';	/* char to print for unprintable chars */
int Absnos = NO;	/* use absolute numbers in margin */
int Nchoise = EOS;	/* choice of line number for cont. display */
int Overlay_col = 0;	/* initial cursor column for 'v' command */
int Warncol;		/* where to turn on column warning, set in dosopt() */
int Firstcol = 0;	/* leftmost column to display */
int Indent = 1;		/* indent col; 0=same as previous line */
int Notify = YES;	/* notify user if he has mail in mail file */
int Globals = NO;	/* substitutes in a global don't fail */
int No_hardware;	/* never use hardware insert/delete */


#ifdef HARD_TERMS
/* Concerning the terminal type */
int Term_type;		/* terminal type */
#endif


/* Concerning the screen format: */
char Screen_image[MAXROWS][MAXCOLS];
char Msgalloc[MAXCOLS];	/* column allocation of status line */
int Nrows;		/* number of rows on screen */
int Ncols;		/* number of columns on screen */
int Currow;		/* vertical cursor coordinate */
int Curcol;		/* horizontal cursor coordinate */
int Toprow;		/* top row of window field on screen */
int Botrow;		/* bottom row of window field on screen */
int Cmdrow;		/* row number of command line */
int Topln;		/* line number of first line on screen */
int Insert_mode;	/* flag to specify character insertion */
int Invert_case;	/* flag to specify case mapping on input */
int First_affected;	/* number of first line affected by cmd */
int Rel_a;		/* char to use for first alpha line number */
int Rel_z;		/* char to use for last alpha line number */
int Scline[MAXROWS];	/* lines currently on screen (rel to Sctop) */
int Sctop;		/* first line currently on screen */
int Sclen;		/* number of lines currently on screen */
char Blanks[MAXCOLS];	/* all blanks for filling in lines on screen */
char Tobuf[MAXTOBUF];	/* buffer for collecting terminal output */
char *Tobp = Tobuf - 1;	/* pointer to last used part of Tobuf */


/* Concerning interrupts: */
int Int_caught = 0;	/* caught a SIGINT from user */
int Hup_caught = 0;	/* caught a SIGHUP when phone line dropped */
#ifdef BSD
int Catching_stops;	/* catching or ignoring SIGTSTP's? */
#endif

/* Concerning file encryption: */
int Crypting = NO;	/* doing file encryption? */
char Key[KEYSIZE] = "";	/* saved encryption key */

extern char *getenv ();

/* main --- main program for screen editor */

main (argc, argv)
int argc;
char *argv[];
{
	char *basename ();
	int int_hdlr (), hup_hdlr ();
	int (*old_int)(), (*old_quit)();
#ifdef BSD
	int stop_hdlr (), (*old_stop)();
#endif
	/* catch quit and hangup signals */
	/*
	 * In the terminal driver munging routines, we set Control-P
	 * to generate an interrupt, and turn off generating Quits from
	 * the terminal.  Now we just ignore them if sent from elsewhere.
	 */

	signal (SIGHUP, hup_hdlr);

	old_int = signal (SIGINT, int_hdlr);
	old_quit = signal (SIGQUIT, SIG_IGN);

#ifdef notdef
/*
 * This is commented out so that se can be run from the news
 * software.  Commenting it out will also allow you to put it
 * in the background, which could give you trouble. So beware.
 */

	if (old_int == SIG_IGN || old_quit == SIG_IGN)
	{
		/* fired off into the background, refuse to run */
		if (isatty (fileno (stdin)))
		{
			fprintf (stderr, "%s: I refuse to run in the background.\n",
				basename (argv[0]));
			exit (2);
		}
		/* else
			assume input is a script */
	}
#endif

#ifdef BSD
	old_stop = signal (SIGTSTP, stop_hdlr);

	if (old_stop == SIG_IGN)	/* running bourne shell */
	{
		signal (SIGTSTP, SIG_IGN);	/* restore it */
		Catching_stops = NO;
	}
	else
		Catching_stops = YES;
		/* running C-shell or BRL sh, catch Control-Z's */
#endif

	/* set terminal to no echo, no output processing, break enabled */
	ttyedit ();

#ifdef HARD_TERMS
	Tspeed = getspeed (1);		/* speed of stdout */
#endif

	initialize (argc, argv);

	edit (argc, argv);

#ifndef HARD_TERMS
	t_exit ();
#endif

	/* reset the terminal mode */
	ttynormal ();
}

/* error --- print error message and die semi-gracefully */

error (coredump, msg)
int coredump;
char *msg;
{
	/*
	 * You might think we want to try and save the buffer,
	 * BUT, fatal errors can be caused by buffer problems,
	 * which would end up putting us into a non-ending recursion.
	 */

	ttynormal ();
	fprintf (stderr, "%s\n", msg);
	signal (SIGQUIT, SIG_DFL);	/* restore normal quit handling */
	if (coredump)
		kill (getpid (), SIGQUIT);	/* dump memory */
	else
		exit (1);
}

/* initialize --- set up global data areas, get terminal type */

initialize (argc, argv)
int argc;
char *argv[];
{
	int i, dosopt ();

	Argno = 1;
#ifdef HARD_TERMS
	/* Determine what type of terminal we're on */
	if (Argno < argc && argv[Argno][0] == '-' && argv[Argno][2] == EOS
	    && (argv[Argno][1] == 't' || argv[Argno][1] == 'T'))
	{
		Argno = 2;
		if (Argno < argc)
		{
			strmap (argv[Argno], 'l');
			if (set_term (argv[Argno]) == ERR)
				usage ();
			else
				Argno++;
		}
		else
			usage ();
	}
	else
		/* fall through to the if, below */
#endif
		if (set_term (getenv ("TERM")) == ERR)
			usage ();

	/* Initialize the scratch file: */
	mkbuf ();

	/* Initialize screen format parameters: */
	setscreen ();

	/* Initialize the array of blanks to blanks */
	for (i = 0; i < Ncols; i++)
		Blanks[i] = ' ';
	Blanks[i] = '\0';

	if (dosopt ("") == ERR)
		error ("in initialize: can't happen");
}

/* intrpt --- see if there has been an interrupt or hangup */

int intrpt ()
{
	if (Int_caught)
	{
		Errcode = EBREAK;
		Int_caught = 0;
		return (1);
	}
	else if (Hup_caught)
	{
		Errcode = EHANGUP;
		Hup_caught = 0;
		return (1);
	}
	return (0);
}

/* int_hdlr --- handle an interrupt signal */

int_hdlr ()
{
#ifndef BSD4_2
	signal (SIGINT, int_hdlr);
#endif
	Int_caught = 1;
}

/* hup_hdlr --- handle a hangup signal */

hup_hdlr ()
{
#ifndef BSD4_2
	signal (SIGHUP, hup_hdlr);
	Hup_caught = 1;
#else
	/* do things different cause of 4.2 (sigh) */
	Hup_caught = 1;
	if (Reading)	/* doing tty i/o, and that is where hup came from */
		hangup ();
#endif
}

#ifdef BSD
/* stop_hdlr --- handle the berkeley stop/suspend signal */

int stop_hdlr ()
{
	clrscreen ();
	tflush ();
	ttynormal ();
#ifdef BSD4_2
	/* this handler remains in effect, use uncatchable signal */
	kill (getpid(), SIGSTOP);
#else
	/* action was reset to default when we caught it */
	kill (getpid(), SIGTSTP);
#endif
	/*
	 * user does a "fg"
	 */
#ifndef BSD4_2
	signal (SIGTSTP, stop_hdlr);	/* reset stop catching */
#endif
	ttyedit ();
	restore_screen ();
}
#endif

/* hangup --- dump contents of edit buffer if SIGHUP occurs */

hangup ()
{
	/* close terminal to avoid hanging on any accidental I/O: */
	close (0);
	close (1);
	close (2);

	signal (SIGHUP, SIG_IGN);
	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
	Hup_caught = 0;
	Crypting = NO;		/* force buffer to be clear text */
	dowrit (1, Lastln, "se.hangup", NO, YES, NO);
	clrbuf ();
	exit (1);
}

/* mswait --- message waiting subroutine */

/* if the user wants to be notified, and the mail file is readable, */
/* and there is something in it, then he is given the message. */
/* the om command toggles Notify, controlling notification. */

#include <sys/types.h>
#include <sys/stat.h>

mswait ()
{
	int access ();
	struct stat buf;
	static char *mbox = NULL;
	static int first = YES;
	static unsigned long mtime = 0L;

	if (! Notify)
		return;

	if (first) 
	{
		first = NO;
		if ((mbox = getenv ("MAIL")) != NULL && access (mbox, 4) == 0)
		{
			if (stat (mbox, &buf) >= 0)
			{
				mtime = buf.st_mtime;
				if (buf.st_size > 0)
					remark ("You have mail");
			}
		}
	}
	else if (mbox && stat (mbox, &buf) >= 0 && buf.st_mtime > mtime)
	{
		mtime = buf.st_mtime;
		remark ("You have new mail");
		twrite (1, "\007", 1);	/* Bell */
	}
}

/* printverboseerrormessage --- print verbose error message */

printverboseerrormessage ()
{
	switch (Errcode) {
	case EBACKWARD:
		remark ("Line numbers in backward order"); 
		break;
	case ENOPAT:
		remark ("No saved pattern -- sorry"); 
		break;
	case EBADPAT:
		remark ("Bad syntax in pattern"); 
		break;
	case EBADSTR:
		remark ("Bad syntax in string parameter"); 
		break;
	case EBADSUB:
		remark ("Bad syntax in substitution string"); 
		break;
	case ECANTREAD:
		remark ("File is not readable"); 
		break;
	case EEGARB:
		remark ("Garbage after your command"); 
		break;
	case EFILEN:
		remark ("Bad syntax in file name"); 
		break;
	case EBADTABS:
		remark ("Bad tabstop syntax"); 
		break;
	case EINSIDEOUT:
		remark ("Can't move a group into itself"); 
		break;
	case EKNOTFND:
		remark ("No line has that mark name"); 
		break;
	case ELINE1:
		remark (""); 
		break;
	case E2LONG:
		remark ("Resultant line too long to handle"); 
		break;
	case ENOERR:
		remark ("No error to report"); 
		break;
	case ENOLIMBO:
		remark ("No lines in limbo"); 
		break;
	case EODLSSGTR:
		remark ("Expected '<', '>', or nothing after 'od'"); 
		break;
	case EORANGE:
		remark ("Line number out of range"); 
		break;
	case EOWHAT:
		remark ("Can't recognize option"); 
		break;
	case EPNOTFND:
		remark ("No line contains that pattern"); 
		break;
	case ESTUPID:
		remark ("Buffer hasn't been saved"); 
		break;
	case EWHATZAT:
		remark ("No command recognized"); 
		break;
	case EBREAK:
		remark ("You interrupted me"); 
		break;
	case ELINE2:
		remark ("Last line number beyond end of file"); 
		break;
	case ECANTWRITE:
		remark ("File is not writeable"); 
		break;
	case ECANTINJECT:
		remark ("No room for any more lines!"); 
		break;
	case ENOMATCH:
		remark ("No match for pattern"); 
		break;
	case ENOFN:
		remark ("No saved filename"); 
		break;
	case EBADLIST:
		remark ("Bad syntax in character list"); 
		break;
	case ENOLIST:
		remark ("No saved character list -- sorry"); 
		break;
	case ENONSENSE:
		remark ("Unreasonable value"); 
		break;
	case ENOHELP:
		remark ("No help available"); 
		break;
	case EBADLNR:
		remark ("Line numbers not allowed"); 
		break;
	case EFEXISTS:
		remark ("File already exists"); 
		break;
	case EBADCOL:
		remark ("Improper column number specification"); 
		break;
	case ENOLANG:
		remark ("Unknown source language"); 
		break;
	case ETRUNC:
		remark ("Lines were truncated"); 
		break;
	case ENOSHELL:
		remark ("Type control-q to rebuild screen"); 
		break;
	case ECANTFORK:
		remark ("Can't fork --- get help!"); 
		break;
	case ENOSUB:
		remark ("No saved replacement --- sorry"); 
		break;
	case ENOCMD:
		remark ("No saved shell command --- sorry");
		break;
	default:
		remark ("?"); 
		break;
	}
	Errcode = ENOERR;
}

/* usage --- print usage diagnostic and die */

usage ()
{
	ttynormal ();
	fprintf (stderr, "Usage: se%s%s\n",
#ifdef HARD_TERMS
	" [ -t <terminal> ] ",
#else
	" ",
#endif
	"[ --acdfghiklmstuvwxyz ] [ file ... ]");
	exit (1);
}
