/*
** term.c
**
** provide terminal functions for se
**
** On System V systems, we have two possibilities.  Release 1 did not have
** the terminfo package, so we assume that if it is Release 1, someone will
** have ported the BSD termlib library.  If it is Release 2, then the new
** terminfo package is there, and we wil use it.
**
** This file is in the public domain.
*/

#include "config.h"

#include <stdlib.h>
#include <signal.h>
#include <sys/ioctl.h>

#include <ncurses.h>

#include "se.h"
#include "extern.h"
#include "main.h"
#include "misc.h"
#include "screen.h"
#include "term.h"

/* outc -- write a character to the terminal */

int outc (int i)
{
	char c;
	c = (char) i;
	twrite (1, &c, 1);
	return i;
}

/*
 * code for using BSD termlib -- getting capabilities, and writing them out.
 */

/* capabilities from termcap */

static int AM;		/* automatic margins, i.e. wraps at column 80 */

static char *VS;	/* visual start -- e.g. clear status line */
static char *VE;	/* visual end -- e.g. restore status line */
static char *TI;	/* terminal init -- whatever needed for screen ops */
static char *TE;	/* terminal ops end */
static char *CM;	/* cursor motion, used by tgoto() */
static char *CE;	/* clear to end of line */
static char *DL;	/* hardware delete line */
static char *AL;	/* hardware add (insert) line */
static char *CL;	/* clear screen */
static char *STANDOUT;	/* standout on  (SO conflicts w/ASCII character name) */
static char *SE;	/* standout end */

extern char PC;		/* Pad character, usually '\0' */

static char *pcstr;

static char caps[128];		/* space for decoded capability strings */
static char *addr_caps;		/* address of caps for relocation */

#define TERMBUFSIZ	1024+1
static char termbuf[TERMBUFSIZ];

/* getdescrip --- get descriptions out of termcap entry */

void getdescrip (void)
{
	int i;
	static struct _table {
		char *name;
		char **ptr_to_cap;
	} table[] = {
		{"vs",	& VS},
		{"ve",	& VE},
		{"ti",	& TI},
		{"te",	& TE},
		{"cm",	& CM},
		{"ce",	& CE},
		{"dl",	& DL},
		{"al",	& AL},
		{"cl",	& CL},
		{"so",	& STANDOUT},
		{"se",	& SE},
		{"pc",	& pcstr},
		{NULL,	NULL}
	};

	AM = tgetflag ("am");		/* only boolean se needs */

	/* get string values */

	for (i = 0; table[i].name != NULL; i++)
	{
		*(table[i].ptr_to_cap) = tgetstr (table[i].name, & addr_caps);
	}
}

/* setcaps -- get the capabilities from termcap file into termbuf */

int setcaps (char *term)
{
	switch (tgetent (termbuf, term)) {
	case -1:
		error (SE_NO, "se: couldn't open termcap file.");

	case 0:
		error (SE_NO, "se: no termcap entry for terminal.");

	case 1:
		addr_caps = caps;
		getdescrip ();		/* get terminal description */
		Nrows = tgetnum ("li");
		Ncols = tgetnum ("co");
		PC = pcstr ? pcstr[0] : SE_EOS;
		break;

	default:
		error (SE_YES, "in setcaps: can't happen.\n");
	}

	return (SE_OK);
}


/* t_init -- put out terminal initialization string */

void t_init (void)
{
	/* terminal initializations */
	initscr ();
}

/* ttynormal -- set the terminal to correct modes for normal use */

void ttynormal (void)
{
	raw ();
	cbreak ();
}

/* ttyedit -- set the terminal to correct modes for editing */

void ttyedit (void)
{
	nocbreak ();
}

/* t_exit -- put out strings to turn off whatever modes we had turned on */

void t_exit (void)
{
	/* terminal exiting strings */
	endwin ();
}

/* winsize --- get the size of the window from the windowing system */
/*		also arrange to catch the windowing signal */

/* 4.3 BSD and/or Sun 3.x */
#define WINSIG		SIGWINCH
#define WINIOCTL	TIOCGWINSZ
#define WINSTRUCT	winsize
#define COLS w.ws_col
#define ROWS w.ws_row

static struct WINSTRUCT w;

void winsize (int sig)
{
	static int first = 1;
	static char savestatus[MAXCOLS];
	int row, oldstatus = Nrows - 1;
	int cols, rows;

	signal (WINSIG, winsize);

	if (ioctl (0, WINIOCTL, (char *) & w) != -1)
	{
		cols = COLS;
		rows = ROWS;

		if (first)
		{
			first = 0;
			if (cols && rows)
			{
				Ncols = cols;
				Nrows = rows;
			}
			return;		/* don't redraw screen */
		}
		else if (Ncols == cols && Nrows == rows)
		{
			/* only position changed */
			return;
		}
		else
		{
			if (cols && rows)
			{
				Ncols = cols;
				Nrows = rows;
			}
		}
	}
	else
		return;

	move_ (Screen_image[oldstatus], savestatus, MAXCOLS);
	clrscreen ();
	Toprow = 0;
	Botrow = Nrows - 3;
	Cmdrow = Botrow + 1;
	Sclen = -1;

	for (row = 0; row < Nrows; row++)
		move_ (Blanks, Screen_image[row], MAXCOLS);
		/* clear screen */

	First_affected = Topln;
	adjust_window (Curln, Curln);
	updscreen ();	/* reload from buffer */
	loadstr (savestatus, Nrows - 1, 0, Ncols);
	remark ("window size change");
	tflush ();
}


/* terminal handling functions used throughout the editor */

/* send --- send a printable character, predict cursor position */

void send (char chr)
{
	if (Currow == Nrows - 1 && Curcol == Ncols - 1)
		return;         /* anything in corner causes scroll... */

	outc (chr);

	if (Curcol == Ncols - 1)
	{
		if (AM)		/* terminal wraps when hits last column */
		{
			Curcol = 0;
			Currow++;
		}
	}
	else		/* cursor not at extreme right */
		Curcol++;
}

/* clrscreen --- clear entire screen */

void clrscreen (void)
{
	Curcol = Currow = 0;
	/* clearing screen homes cursor to upper left corner */
	/* on all terminals */

	tputs (CL, 1, outc);
}


/* position_cursor --- position terminal's cursor to (row, col) */

void position_cursor (int row, int col)
{
	if (row < Nrows && row >= 0		/* within vertical range? */
	    && col < Ncols && col >= 0		/* within horizontal range? */
	    && (row != Currow || col != Curcol))/* not already there? */
	{
		if (row == Currow && abs (Curcol - col) <= 4)
		{
			/* short motion in current line */
			if (Curcol < col)
				for (; Curcol != col; Curcol++)
					twrite (1, &Screen_image[Currow][Curcol], 1);
			else
				for (; Curcol != col; Curcol--)
					twrite (1, "\b", 1);
		}
		else
		{
#if defined (USG) && defined(S5R2)
			tputs (tparm (cursor_address, row, col), 1, outc);
#else
			tputs (tgoto (CM, col, row), 1, outc);
#endif
			Currow = row;
			Curcol = col;
		}
	}
}


/* setscreen --- initialize screen and associated descriptive variables */

void setscreen ()
{
	int row, col;

	t_init ();	/* put out the 'ti' and 'vs' capabilities */
	clrscreen ();	/* clear physical screen, set cursor position */

	Toprow = 0;
	Botrow = Nrows - 3; /* 1 for 0-origin, 1 for status, 1 for cmd */
	Cmdrow = Botrow + 1;
	Topln = 1;
	Sclen = -1;         /* make sure we assume nothing on the screen */

	for (row = 0; row < Nrows; row++)	/* now clear virtual screen */
		for (col = 0; col < Ncols; col++)
			Screen_image[row][col] = ' ';

	for (col = 0; col < Ncols; col++)	/* and clear out status line */
		Msgalloc[col] = SE_NOMSG;

	Insert_mode = SE_NO;
}


/* inslines --- insert 'n' lines on the screen at 'row' */

void inslines (int row, int n)
{
	int i;

	position_cursor (row, 0);

	for (i = 0; i < n; i++)
	{
		tputs (AL, n, outc);
		tflush ();
	}

	for (i = Nrows - 1; i - n >= Currow; i--)
		move_ (Screen_image[i - n], Screen_image[i], Ncols);

	for (; i >= Currow; i--)
		move_ (Blanks, Screen_image[i], Ncols);
}


/* dellines --- delete 'n' lines beginning at 'row' */

void dellines (int row, int n)
{
	int i;

	position_cursor (row, 0);

	for (i = 0; i < n; i++)
	{
		tputs (DL, n, outc);
		tflush ();
	}

	for (i = Currow; i + n < Nrows; i++)
		move_ (Screen_image[i + n], Screen_image[i], Ncols);

	for (; i < Nrows; i++)
		move_ (Blanks, Screen_image[i], Ncols);
}


/* hwinsdel --- return 1 if the terminal has hardware insert/delete */

int hwinsdel (void)
{
	if (No_hardware == SE_YES)
		return (SE_NO);

	return (AL != NULL && DL != NULL);
}


/* clear_to_eol --- clear screen to end-of-line */

void clear_to_eol (int row, int col)
{
	int c, flag;
	int hardware = SE_NO;

	hardware = (CE != NULL);

	flag = SE_NO;

	for (c = col; c < Ncols; c++)
		if (Screen_image[row][c] != ' ')
		{
			Screen_image[row][c] = ' ';
			if (hardware)
				flag = SE_YES;
			else
			{
				position_cursor (row, c);
				send (' ');
			}
		}

	if (flag == SE_YES)
	{
		position_cursor (row, col);
		tputs (CE, 1, outc);
	} /* end if (flag == SE_YES) */
}

/* se_set_term -- initialize terminal parameters and actual capabilities */

int se_set_term (char *type)
{
	if (type == NULL)
	{
		error (SE_NO, "se: terminal type not available");
	}

	if (type[0] == SE_EOS)
	{
		error (SE_NO, "in set_term: can't happen.");
	}

	Ncols = Nrows = -1;

	if (setcaps (type) == SE_ERR)
	{
		error (SE_NO, "se: could not find terminal in system database");
	}

	if (tgoto (CM, 0, 0) == NULL)	/* OOPS returned.. */
	{
		error (SE_NO, "se: terminal does not have cursor motion.");
	}

	/*
	 * first, get it from the library. then check the
	 * windowing system, if there is one.
	 */
	winsize (WINSIG);

	if (Nrows == -1)
		error (SE_NO, "se: could not determine number of rows");

	if (Ncols == -1)
		error (SE_NO, "se: could not determine number of columns");

	return SE_OK;
}

/* brighton --- turn on reverse video/standout mode */

void brighton (void)
{
	if (STANDOUT)
		tputs (STANDOUT, 1, outc);
}

/* brightoff --- turn off reverse video/standout mode */

void brightoff (void)
{
	if (SE)
		tputs (SE, 1, outc);
}
