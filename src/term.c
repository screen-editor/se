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

#ifdef CURSES_LOC
#include CURSES_LOC
#else
#include <curses.h>
#endif

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
	echochar (c);
	return i;
}

static int t_initialized = 0;

/* t_init -- put out terminal initialization string */

void t_init (void)
{
	if (!t_initialized)
	{
		/* terminal initializations */
		initscr ();
		t_initialized = 1;
	}
}

/* t_exit -- put out strings to turn off whatever modes we had turned on */

void t_exit (void)
{
	if (t_initialized)
	{
		/* terminal exiting strings */
		endwin ();
		t_initialized = 0;
	}
}

/* ttynormal -- set the terminal to correct modes for normal use */

void ttynormal (void)
{
	nocbreak ();
}

/* ttyedit -- set the terminal to correct modes for editing */

void ttyedit (void)
{
	cbreak ();
}

/* winsize --- get the size of the window from the windowing system */
/*		also arrange to catch the windowing signal */

void winsize (int sig)
{
	static int first = 1;
	static char savestatus[MAXCOLS];
	int row, oldstatus = Nrows - 1;
	int cols, rows;

	signal (SIGWINCH, winsize);

	if (!first)
	{
		struct winsize size;
		ioctl(0, TIOCGWINSZ, &size);
		resizeterm(size.ws_row, size.ws_col);
	}

	cols = COLS;
	rows = LINES;

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

	move_ (Screen_image[oldstatus], savestatus, MAXCOLS);
	clrscreen ();
	Toprow = 0;
	Botrow = Nrows - 3;
	Cmdrow = Botrow + 1;
	Sclen = -1;

	for (row = 0; row < Nrows; row++)
	{
		move_ (Blanks, Screen_image[row], MAXCOLS);
		/* clear screen */
	}

	First_affected = Topln;
	adjust_window (Curln, Curln);
	updscreen ();	/* reload from buffer */
	loadstr (savestatus, Nrows - 1, 0, Ncols);
	remark ("window size change");
	refresh ();
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
		/* terminal wraps when hits last column */
		Curcol = 0;
		Currow++;
	}
	else		/* cursor not at extreme right */
	{
		Curcol++;
	}
}

/* clrscreen --- clear entire screen */

void clrscreen (void)
{
	Curcol = Currow = 0;
	/* clearing screen homes cursor to upper left corner */
	/* on all terminals */

	erase ();
	refresh ();
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
			{
				for (; Curcol != col; Curcol++)
				{
					outc(Screen_image[Currow][Curcol]);
				}
			}
			else
			{
				for (; Curcol != col; Curcol--)
				{
					outc('\b');
				}
			}
		}
		else
		{
			move (row, col);
			refresh ();
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
		insertln ();
		refresh ();
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
		deleteln ();
		refresh ();
	}

	for (i = Currow; i + n < Nrows; i++)
		move_ (Screen_image[i + n], Screen_image[i], Ncols);

	for (; i < Nrows; i++)
		move_ (Blanks, Screen_image[i], Ncols);
}


/* hwinsdel --- return 1 if the terminal has hardware insert/delete */

int hwinsdel (void)
{
	return SE_YES;
}


/* clear_to_eol --- clear screen to end-of-line */

void clear_to_eol (int row, int col)
{
	int c, flag;
	int hardware = SE_NO;

	hardware = SE_YES;

	flag = SE_NO;

	for (c = col; c < Ncols; c++)
	{
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
	}

	if (flag == SE_YES)
	{
		position_cursor (row, col);
		clrtoeol ();
		refresh ();
	} /* end if (flag == SE_YES) */
}

/* se_set_term -- initialize terminal parameters and actual capabilities */

int se_set_term (char *type)
{
	if (type == NULL)
	{
		error (SE_NO, "se: terminal type not available");
	}

	t_init ();

	Ncols = Nrows = -1;

	move (0, 0);
	refresh ();

	/*
	 * first, get it from the library. then check the
	 * windowing system, if there is one.
	 */
	winsize (SIGWINCH);

	if (Nrows == -1)
		error (SE_NO, "se: could not determine number of rows");

	if (Ncols == -1)
		error (SE_NO, "se: could not determine number of columns");

	return SE_OK;
}

/* brighton --- turn on reverse video/standout mode */

void brighton (void)
{
	attron (A_REVERSE);
}

/* brightoff --- turn off reverse video/standout mode */

void brightoff (void)
{
	attroff (A_REVERSE);
}
