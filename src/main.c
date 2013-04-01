/*
** main.c
**
** main program and lots of other routines
** for the se screen editor.
**
** This file is in the public domain.
*/

#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "se.h"
#include "changetty.h"
#include "docmd1.h"
#include "edit.h"
#include "main.h"
#include "misc.h"
#include "scratch.h"
#include "screen.h"
#include "term.h"

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


/* Concerning the 'undo' command: */
LINEDESC *Limbo;	/* head of limbo list for undo */
int Limcnt;		/* number of lines in limbo list */


/* Concerning the scratch file: */
int Scr;		/* scratch file descriptor */
unsigned Scrend;	/* end of info on scratch file */
char Scrname[MAXLINE];	/* name of scratch file */
int Lost_lines;		/* number of garbage lines in scratch file */


/* Concerning miscellaneous variables */
int Buffer_changed = SE_NO;/* SE_YES if buffer changed since last write */
int Errcode = ESE_NOSE_ERR;	/* cause of most recent error */
int Saverrcode = ESE_NOSE_ERR;/* cause of previous error */
int Probation = SE_NO;	/* SE_YES if unsaved buffer can be destroyed */
int Argno;		/* command line argument pointer */
char Last_char_scanned = 0;	/* last char scanned w/ctl-[sl], init illegal  */
char Peekc = SE_EOS;	/* push a SKIP_RIGHT if adding delimiters */
int Reading = SE_NO;	/* are we doing terminal input? */


/* Concerning options: */
int Tabstops[MAXLINE];	/* array of tab stops */
char Unprintable = ' ';	/* char to print for unprintable chars */
int Absnos = SE_NO;	/* use absolute numbers in margin */
int Nchoise = SE_EOS;	/* choice of line number for cont. display */
int Overlay_col = 0;	/* initial cursor column for 'v' command */
int Warncol;		/* where to turn on column warning, set in dosopt() */
int Firstcol = 0;	/* leftmost column to display */
int Indent = 1;		/* indent col; 0=same as previous line */
int Notify = SE_YES;	/* notify user if he has mail in mail file */
int Globals = SE_NO;	/* substitutes in a global don't fail */
int No_hardware;	/* never use hardware insert/delete */


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

#ifdef HAVE_CRYPT_PROG
/* Concerning file encryption: */
int Crypting = SE_NO;	/* doing file encryption? */
char Key[KEYSIZE] = "";	/* saved encryption key */
#endif /* HAVE_CRYPT_PROG */

extern char *getenv ();

/* main --- main program for screen editor */

int main (int argc, char *argv[])
{
#ifdef SIGINT
	void (*old_int) (int);
#endif /* SIGINT */

#ifdef SIGQUIT
	void (*old_quit) (int);
#endif /* SIGQUIT */

	/* catch quit and hangup signals */
	/*
	 * In the terminal driver munging routines, we set Control-P
	 * to generate an interrupt, and turn off generating Quits from
	 * the terminal.  Now we just ignore them if sent from elsewhere.
	 */

#ifdef SIGHUP
	signal (SIGHUP, hup_hdlr);
#endif /* SIGHUP */

#ifdef SIGINT
	old_int = signal (SIGINT, int_hdlr);
#endif /* SIGINT */

#ifdef SIGQUIT
	old_quit = signal (SIGQUIT, SIG_IGN);
#endif /* SIGQUIT */

	if (
#ifdef SIGINT
		old_int == SIG_IGN ||
#endif /* SIGINT */
#ifdef SIGQUIT
		old_quit == SIG_IGN ||
#endif /* SIGQUIT */
		0
	)
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

	/* set terminal to no echo, no output processing, break enabled */
	ttyedit ();

	initialize (argc, argv);

	edit (argc, argv);

	/* reset the terminal mode */
	ttynormal ();

	t_exit ();

	return 0;
}

/* error --- print error message and die semi-gracefully */

void error (int coredump, char *msg)
{
	/*
	 * You might think we want to try and save the buffer,
	 * BUT, fatal errors can be caused by buffer problems,
	 * which would end up putting us into a non-ending recursion.
	 */

	ttynormal ();
	fprintf (stderr, "%s\n", msg);

#ifdef SIGQUIT

	signal (SIGQUIT, SIG_DFL);	/* restore normal quit handling */

	if (coredump)
		kill (getpid (), SIGQUIT);	/* dump memory */
	else
		exit (1);

#else /* !SIGQUIT */

	exit (1);

#endif /* !SIGQUIT */
}

/* initialize --- set up global data areas, get terminal type */

void initialize (int argc, char *argv[])
{
	int i;

	Argno = 1;

	if (se_set_term (getenv ("TERM")) == SE_ERR)
		usage ();

	/* Initialize the scratch file: */
	mkbuf ();

	/* Initialize screen format parameters: */
	setscreen ();

	/* Initialize the array of blanks to blanks */
	for (i = 0; i < Ncols; i++)
		Blanks[i] = ' ';
	Blanks[i] = '\0';

	if (dosopt ("") == SE_ERR)
		error (SE_NO, "in initialize: can't happen");
}

/* intrpt --- see if there has been an interrupt or hangup */

int intrpt (void)
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

void int_hdlr (int sig)
{
	if (sig == SIGINT) {

		signal (SIGINT, int_hdlr);
		Int_caught = 1;
	}
}

/* hup_hdlr --- handle a hangup signal */

void hup_hdlr (int sig)
{
#ifdef SIGHUP
	if (sig == SIGHUP)
	{

		signal (SIGHUP, hup_hdlr);
		Hup_caught = 1;

		/* do things different cause of 4.2 (sigh) */
		Hup_caught = 1;
		if (Reading)	/* doing tty i/o, and that is where hup came from */
		{
			hangup ();
		}
	}
#endif /* SIGHUP */
}

/* hangup --- dump contents of edit buffer if SIGHUP occurs */

void hangup (void)
{
	/* close terminal to avoid hanging on any accidental I/O: */
	close (0);
	close (1);
	close (2);

#ifdef SIGHUP
	signal (SIGHUP, SIG_IGN);
#endif /* SIGHUP */

#ifdef SIGINT
	signal (SIGINT, SIG_IGN);
#endif /* SIGINT */

#ifdef SIGQUIT
	signal (SIGQUIT, SIG_IGN);
#endif /* SIGQUIT */

	Hup_caught = 0;

#ifdef HAVE_CRYPT_PROG
	Crypting = SE_NO;		/* force buffer to be clear text */
#endif /* HAVE_CRYPT_PROG */

	dowrit (1, Lastln, "se.hangup", SE_NO, SE_YES, SE_NO);
	clrbuf ();
	exit (1);
}

/* mswait --- message waiting subroutine */

/* if the user wants to be notified, and the mail file is readable, */
/* and there is something in it, then he is given the message. */
/* the om command toggles Notify, controlling notification. */

void mswait (void)
{
	struct stat buf;
	static char *mbox = NULL;
	static int first = SE_YES;
	static unsigned long mtime = 0L;

	if (! Notify)
		return;

	if (first) 
	{
		first = SE_NO;
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

/* print_verbose_err_msg --- print verbose error message */

void print_verbose_err_msg (void)
{
	switch (Errcode) {
	case EBACKWARD:
		remark ("Line numbers in backward order");
		break;
	case ESE_NOPAT:
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
	case EKSE_NOTFND:
		remark ("No line has that mark name");
		break;
	case ELINE1:
		remark ("");
		break;
	case E2LONG:
		remark ("Resultant line too long to handle");
		break;
	case ESE_NOSE_ERR:
		remark ("No error to report");
		break;
	case ESE_NOLIMBO:
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
	case EPSE_NOTFND:
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
	case ESE_NOMATCH:
		remark ("No match for pattern");
		break;
	case ESE_NOFN:
		remark ("No saved filename");
		break;
	case EBADLIST:
		remark ("Bad syntax in character list");
		break;
	case ESE_NOLIST:
		remark ("No saved character list -- sorry");
		break;
	case ESE_NONSENSE:
		remark ("Unreasonable value");
		break;
	case ESE_NOHELP:
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
	case ESE_NOLANG:
		remark ("Unknown source language");
		break;
	case ETRUNC:
		remark ("Lines were truncated");
		break;
	case ESE_NOSHELL:
		remark ("Type control-q to rebuild screen");
		break;
	case ECANTFORK:
		remark ("Can't fork --- get help!");
		break;
	case ESE_NOSUB:
		remark ("No saved replacement --- sorry");
		break;
	case ESE_NOCMD:
		remark ("No saved shell command --- sorry");
		break;
	default:
		remark ("?");
		break;
	}
	Errcode = ESE_NOSE_ERR;
}

/* usage --- print usage diagnostic and die */

void usage (void)
{
	ttynormal ();
	fprintf (stderr, "Usage: se%s%s\n",
	" ",
	"[ --acdfghiklmstuvwxyz ] [ file ... ]");
	exit (1);
}
