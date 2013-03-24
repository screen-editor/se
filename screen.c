/*
** screen.c
**
** screen handling functions for the screen editor.
*/


#include "se.h"
#include "extern.h"
#include <time.h>

/* clrrow --- clear out all of a row except the bar */

clrrow (row)
register int row;
{
	loadstr ("", row, 0, BARCOL - 1);
	loadstr ("", row, BARCOL + 1, Ncols - 1);
}



/* display_message --- copy contents of message file to screen */

display_message (fp)
FILE *fp;
{
	char lin[MAXCOLS];
	char *fgets ();
	int row, col, eof, k;
	static char more[] = " M O R E   T O   C O M E ";

	if (Toprow > 0)
	{
		Topln = max (0, Topln - (Toprow - 1));
		Toprow = 0;
	}

	eof = NO;
	for (row = Toprow; row <= Botrow; row++)
	{
		if (fgets (lin, Ncols, fp) == NULL)
		{
			eof = YES;
			break;
		}
		Toprow++;
		Topln++;
		lin[strlen (lin)] = EOS;		/* remove '\n' */
		loadstr (lin, row, 0, Ncols);
	}

	if (eof == NO)
	{
		k = (Ncols - strlen (more)) / 2;
		for (col = 0; col < k; col++)
			load ('*', row, col);

		for (k = 0; more[k] != EOS; k++, col++)
			load (more[k], row, col);

		for (; col < Ncols; col++)
			load ('*', row, col);

		Toprow++;
		Topln++;
		row++;
	}

	for (col = 0; col < Ncols; col++)
		load ('-', row, col);
	Toprow++;
	Topln++;

	if (Topln > Lastln)
		adjust_window (0, Lastln);

	if (Curln < Topln)
		Curln = Topln;

	First_affected = Topln;		/* must rewrite the whole screen */

	mesg ("Enter o- to restore display", HELP_MSG);

	if (eof == YES)
		return (EOF);

	return (OK);
}

static char smargin[] = "MARGIN";

/* getcmd --- read a line from the terminal (for se) */

getcmd (lin, col1, curpos, termchar)
char *lin, *termchar;
int col1, *curpos;
{
	int cursor, nlpos, prev_cursor, prev_status, status,
	scan_pos, tab_pos, first, strlen (),
	scan_tab (), scan_char (), cread ();
	char c;

	nlpos = strlen (lin) - 1;
	if (nlpos == -1 || lin[nlpos] != '\n')
		nlpos++;

	if (*curpos < 0)
		cursor = 0;
	else if (*curpos >= MAXLINE - 1)
		cursor = nlpos;
	else
		set_cursor (*curpos, &status, &cursor, &nlpos, lin);
	prev_cursor = cursor;

	watch ();	/* display the time of day */

	switch (Nchoise) {	/* update the line number display */
	case CURLINE:
		litnnum ("line ", Curln, LINE_MSG);
		break;
	case LASTLINE:
		litnnum ("$ = ", Lastln, LINE_MSG);
		break;
	case TOPLINE:
		litnnum ("# = ", Topln, LINE_MSG);
		break;
	default:
		mesg ("", LINE_MSG);
		break;
	}

	if (cursor + 1 < Warncol)	/* erase the column display */
		mesg ("", COL_MSG);

	*termchar = EOS;	/* not yet terminated */
	status = OK;
	prev_status = ERR;
	first = col1;

	while (*termchar == EOS)
	{
		lin[nlpos] = EOS;	/* make sure the line has an EOS */
		if (status == ERR)	/* last iteration generated an error */
			twrite (1, "\007", 1);	/* Bell */
		else if (prev_status == ERR)	/* last one OK but one before had error */
			mesg ("", CHAR_MSG);

		prev_status = status;
		status = OK;

		if (first > cursor)     /* do horiz. scroll if needed */
			first = cursor;
		else if (first < cursor - Ncols + POOPCOL + 1)
			first = cursor - Ncols + POOPCOL + 1;

		if (first == col1)      /* indicate horizontally shifted line */
			load ('|', Cmdrow, BARCOL);
		else if (first > col1)
			load ('<', Cmdrow, BARCOL);
		else if (first < col1)
			load ('>', Cmdrow, BARCOL);
		loadstr (&lin[first], Cmdrow, POOPCOL, Ncols - 1);

		if (cursor == Warncol - 1 && prev_cursor < Warncol - 1)
			twrite (1, "\007", 1);   /* Bell */
		if (cursor >= Warncol - 1)
			litnnum ("col ", cursor + 1, COL_MSG);
		else if (prev_cursor >= Warncol - 1)
			mesg ("", COL_MSG);

		position_cursor (Cmdrow, cursor + POOPCOL - first);
		prev_cursor = cursor;

		/* get a character  */
		switch (c = cread()) {		/* branch on character value */

	/* Literal characters: */
		case ' ': case '!': case '"': case '#': case '$': case '%': 
		case '&': case '\'': case '(': case ')': case '*': case '+':
		case ',': case '-': case '.': case '/': case '0': case '1': 
		case '2': case '3': case '4': case '5': case '6': case '7':
		case '8': case '9': case ':': case ';': case '<': case '=': 
		case '>': case '?': case '@': case '[': case '\\': case ']':
		case '^': case '_': case '`': case '{': case '|': case '}': 
		case '~': case 'A': case 'B': case 'C': case 'D': case 'E':
		case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': 
		case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q':
		case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': 
		case 'X': case 'Y': case 'Z': case 'a': case 'b': case 'c':
		case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': 
		case 'j': case 'k': case 'l': case 'm': case 'n': case 'o':
		case 'p': case 'q': case 'r': case 's': case 't': case 'u': 
		case 'v': case 'w': case 'x': case 'y': case 'z': case ESC:
			if (c == ESC) /* take next char literally */
			{
				if ((c = cread()) == '\r')
					c = '\n';
			}
			else if (Invert_case == YES && isalpha (c))
				c ^= 040;       /* toggle case (ASCII only) */
			if (Insert_mode == YES)
				insert (1, &nlpos, &status, cursor, lin);
			else if (cursor >= MAXLINE - 2)
			{
				status = ERR;
				mesg (smargin, CHAR_MSG);
			}
			if (status != ERR)
			{
				lin[cursor++] = c;
				if (nlpos < cursor)
					nlpos = cursor;
				if (c == '\n')
					*termchar = CURSOR_SAME;
			}
			break;

	/* Leftward cursor functions: */
		case CURSOR_LEFT:
			set_cursor (cursor - 1, &status, &cursor, &nlpos, lin);
			break;

		case TAB_LEFT:
			tab_pos = scan_tab (TAB_LEFT, cursor, &status);
			if (status != ERR)
				cursor = tab_pos;
			break;

		case SKIP_LEFT:
			cursor = 0;
			break;

		case SCAN_LEFT:
			scan_pos = scan_char (c, YES, cursor, nlpos,
			    lin, &status);
			if (status != ERR)
				cursor = scan_pos;
			break;

		case G_LEFT:
			set_cursor (cursor - 1, &status, &cursor, &nlpos, lin);
			if (status != ERR)
				gobble (1, cursor, &status, &nlpos, lin);
			break;

		case G_TAB_LEFT:
			tab_pos = scan_tab (TAB_LEFT, cursor, &status);
			if (status != ERR)
			{
				cursor = tab_pos;
				gobble (prev_cursor - tab_pos, cursor,
				    &status, &nlpos, lin);
			}
			break;

		case KILL_LEFT:
			cursor = 0;
			gobble (prev_cursor /* - 1 */, cursor, &status, &nlpos, lin);
			break;

		case G_SCAN_LEFT:
			scan_pos = scan_char (c, NO, cursor, nlpos,
			    lin, &status);
			if (status != ERR)
			{
				cursor = scan_pos;
				gobble (prev_cursor - scan_pos, cursor,
				    &status, &nlpos, lin);
			}
			break;

	/* Rightward cursor functions: */
		case CURSOR_RIGHT:
			set_cursor (cursor + 1, &status, &cursor, &nlpos, lin);
			break;

		case TAB_RIGHT:
			tab_pos = scan_tab (TAB_RIGHT, cursor, &status);
			if (status != ERR)
				set_cursor (tab_pos, &status, &cursor,
				    &nlpos, lin);
			break;

		case SKIP_RIGHT:
			cursor = nlpos;
			first = col1;
			break;

		case SCAN_RIGHT:
			scan_pos = scan_char (c, YES, cursor, nlpos,
			    lin, &status);
			if (status != ERR)
				cursor = scan_pos;
			break;

		case G_RIGHT:
			gobble (1, cursor, &status, &nlpos, lin);
			break;

		case G_TAB_RIGHT:
			tab_pos = scan_tab (TAB_RIGHT, cursor,
			    &status);
			if (status != ERR)
				gobble (tab_pos - cursor, cursor, &status,
				    &nlpos, lin);
			break;

		case KILL_RIGHT:
			gobble (nlpos - cursor, cursor, &status, &nlpos, lin);
			break;

		case G_SCAN_RIGHT:
			scan_pos = scan_char (c, NO, cursor, nlpos,
			    lin, &status);
			if (status != ERR)
				gobble (scan_pos - cursor, cursor, &status,
				    &nlpos, lin);
			break;

	/* Line termination functions: */
		case T_SKIP_RIGHT:
			cursor = nlpos;
			*termchar = c;
			break;

		case T_KILL_RIGHT:
			nlpos = cursor;
			*termchar = c;
			break;

		case FUNNY:
		case CURSOR_UP:
		case CURSOR_DOWN:
			*termchar = c;
			break;

	/* Insertion functions: */
		case INSERT_BLANK:
			insert (1, &nlpos, &status, cursor, lin);
			if (status != ERR)
				lin[cursor] = ' ';
			break;

		case INSERT_NEWLINE:
			insert (1, &nlpos, &status, cursor, lin);
			if (status != ERR)
			{
				lin[cursor] = '\n';
				*termchar = CURSOR_UP;
			}
			break;

		case INSERT_TAB:
			while (lin[cursor] == ' ' || lin[cursor] == '\t')
				cursor++;
			tab_pos = scan_tab (TAB_RIGHT, cursor, &status);
			if (status != ERR)
				insert (tab_pos - cursor, &nlpos, &status,
				    cursor, lin);
			if (status != ERR)
				for (; cursor < tab_pos; cursor++)
					lin[cursor] = ' ';
			cursor = prev_cursor;
			break;

	/* Miscellanious control functions: */
		case TOGGLE_INSERT_MODE:
			Insert_mode = YES + NO - Insert_mode;
			if (Insert_mode == NO)
				mesg ("", INS_MSG);
			else
				mesg ("INSERT", INS_MSG);
			break;

		case SHIFT_CASE:
			Invert_case = YES + NO - Invert_case;
			if (Invert_case == NO)
				mesg ("", CASE_MSG);
			else
				mesg ("CASE", CASE_MSG);
			break;

		case KILL_ALL:
			nlpos = cursor = 0;
			break;

		case FIX_SCREEN:
			restore_screen ();
			break;

		default:
			status = ERR;
			mesg ("WHA?", CHAR_MSG);
			break;
		} /* end switch */
	} /* while (termchar == EOS) */

	lin[nlpos] = '\n';
	lin[nlpos + 1] = EOS;

	load ('|', Cmdrow, BARCOL);
	if (nlpos <= col1)
		loadstr ("", Cmdrow, POOPCOL, Ncols - 1);
	else
		loadstr (&lin[col1], Cmdrow, POOPCOL, Ncols - 1);

	if (cursor >= Warncol - 1)
		litnnum ("col ", cursor + 1, COL_MSG);
	else if (prev_cursor >= Warncol - 1)
		mesg ("", COL_MSG);

	*curpos = cursor;
}



/* cread --- read a character, handle interrupts and hangups */
/*              ALL keyboard input should pass through this routine */

int cread ()
{
	char c;
	int read ();

	tflush ();
	if (Peekc)
	{
		c = Peekc;
		Peekc = EOS;
	}
	else
	{
#ifdef BSD4_2
		Reading = YES;
#endif
		if (read (0, &c, 1) == -1)
			if (Hup_caught)
				hangup ();
			else	/* must be a SIGINT at present */
			{
				Int_caught = 0;
				Errcode = ENOERR;
				c = '\177';
			}
#ifdef BSD4_2
		Reading = NO;
#endif
	}

	return c;
}



/* scan_char --- scan current line for a character */

static int scan_char (chr, wrap, cursor, nlpos, lin, status)
char chr, *lin;
int wrap, cursor, nlpos, *status;
{
	register char c;
	register int inc, scan_pos;
	int cread ();

	c = cread ();
	if (Invert_case == YES && isalpha (c))
		c ^= 040;       /* toggle case */
	if (c == chr)
		c = Last_char_scanned;
	Last_char_scanned = c;

	if (chr == SCAN_LEFT || chr == G_SCAN_LEFT)
		inc = -1;
	else
		inc = 1;

	/* NOTE:  modify this code AT YOUR OWN RISK! */
	scan_pos = cursor;
	do
	{
		if (scan_pos < 0)
			if (wrap == NO)
				break;
			else
				scan_pos = nlpos;
		else if (scan_pos > nlpos)
			if (wrap == NO)
				break;
			else
				scan_pos = 0;
		else
			scan_pos += inc;
		if (-1 < scan_pos && scan_pos < nlpos && lin[scan_pos] == c)
			break;
	} while (scan_pos != cursor);

	if (scan_pos < 0 || scan_pos >= nlpos || lin[scan_pos] != c)
	{
		*status = ERR;
		mesg ("NOCHAR", CHAR_MSG);
	}

	return (scan_pos);
}



static int scan_tab (chr, cursor, status)
char chr;
int cursor, *status;
{
	register int inc, tab_pos;

	if (chr == TAB_LEFT)
	{
		inc = -1;
		tab_pos = cursor - 1;
	}
	else
	{
		inc = 1;
		tab_pos = cursor + 1;
	}

	for (; -1 < tab_pos && tab_pos < MAXLINE; tab_pos += inc)
		if (Tabstops[tab_pos] == YES)
			break;

	if (tab_pos < 0 || tab_pos >= MAXLINE - 1)
	{
		*status = ERR;
		mesg (smargin, CHAR_MSG);
	}

	return (tab_pos);
}



/* gobble --- delete characters starting at the current cursor position */

static gobble (len, cursor, status, nlpos, lin)
int len, cursor, *status, *nlpos;
char *lin;
{
	if (cursor + len > *nlpos)
	{
		*status = ERR;
		mesg (smargin, CHAR_MSG);
	}
	else if (len > 0)
	{
		strcpy (&lin[cursor], &lin[cursor + len]);
		*nlpos -= len;
	}
}



/* insert --- shift characters right starting at the current cursor position */

static insert (len, nlpos, status, cursor, lin)
register int len, *nlpos, cursor;
int *status;
register char *lin;
{
	register int fr, to;

	if (*nlpos + len >= MAXLINE - 1)
	{
		*status = ERR;
		mesg (smargin, CHAR_MSG);
	}
	else
	{
		for (fr = *nlpos, to = *nlpos + len; fr >= cursor; fr--, to--)
			lin[to] = lin[fr];
		*nlpos += len;
	}
}



/* set_cursor --- move the cursor, extend line if necessary */

static set_cursor (pos, status, cursor, nlpos, lin)
register int pos, *status, *cursor, *nlpos;
register char *lin;
{
	if (pos < 0 || pos >= MAXLINE - 1)
	{
		*status = ERR;
		mesg (smargin, CHAR_MSG);
	}
	else
	{
		*cursor = pos;
		for (; *nlpos < *cursor; (*nlpos)++)
			lin[*nlpos] = ' ';
	}
}


/* litnnum --- display literal and number in message area */

litnnum (lit, num, type)
register char *lit;
register int num, type;
{
	char msg[MAXLINE];

	sprintf (msg, "%s%d", lit, num);
	mesg (msg, type);
}



/* load --- load a character onto the screen at given coordinates */

load (chr, row, col)
register char chr;
register int row, col;
{
	register char ch;

	ch = (chr < ' ' || chr >= DEL) ? Unprintable : chr;

	if (row >= 0 && row < Nrows && col >= 0 && col < Ncols
	    && Screen_image[row][col] != ch)
	{
		position_cursor (row, col);
		Screen_image[row][col] = ch;
		send (ch);
	}
}


/* loadstr --- load a string into a field of the screen */

loadstr (str, row, stcol, endcol)
register char *str;
int row, stcol, endcol;
{
	register char ch;
	register int p, c, limit;

	if (row >= 0 && row < Nrows && stcol >= 0)
	{
		c = stcol;
		for (p = 0; str[p] != EOS; p++)
		{
			if (c >= Ncols)
				break;
			ch = str[p];
			if (ch < ' ' || ch >= DEL)
				ch = Unprintable;
			if (Screen_image[row][c] != ch)
			{
				Screen_image[row][c] = ch;
				position_cursor (row, c);
				send (ch);
			}
			c++;
		}
		if (endcol >= Ncols - 1 && c < Ncols - 1)
			clear_to_eol (row, c);
		else
		{
			limit = (endcol < Ncols - 1) ? endcol : Ncols - 1;
			for (; c <= limit; c++)
				if (Screen_image[row][c] != ' ')
				{
					Screen_image[row][c] = ' ';
					position_cursor (row, c);
					send (' ');
				}
		}
	}
}


/* mesg --- display a message in the status row */

mesg (s, t)
char *s;
register int t;
{
	register int col, need, c, first, last;
	int strlen ();

	for (first = 0; first < Ncols; first++)
		if (Msgalloc[first] == t)
			break;
	for (last = first; last < Ncols; last++)
	{
		if (Msgalloc[last] != t)
			break;
		Msgalloc[last] = NOMSG;
	}
	for (; first > 0 && Msgalloc[first - 1] == NOMSG; first--)
		;

	need = strlen (s) + 2;  /* for two blanks */

	if (need > 2)           /* non-empty message */
	{
		if (need <= last - first)       /* fits in previous slot */
			col = first;		/* keep it there */
		else		/* needs a new slot */
			for (col = 0; col < Ncols - 1; col = c)
			{
				while (col < Ncols - 1
				    && Msgalloc[col] != NOMSG)
					col++;
				for (c = col; Msgalloc[c] == NOMSG; c++)
					if (c >= Ncols - 1)
						break;
				if (c - col >= need)
					break;
			}

		if (col + need >= Ncols)        /* have to collect garbage */
		{
			col = 0;
			for (c = 0; c < Ncols; c++)
				if (Msgalloc[c] != NOMSG)
				{
					load (Screen_image[Nrows - 1][c],
					    Nrows - 1, col);
					Msgalloc[col] = Msgalloc[c];
					col++;
				}
			for (c = col; c < Ncols; c++)
				Msgalloc[c] = NOMSG;
		}

		load (' ', Nrows - 1, col);
		loadstr (s, Nrows - 1, col + 1, 0);
		load (' ', Nrows - 1, col + need - 1);
		last = col + need - 1;
		if (last > Ncols - 1)
			last = Ncols - 1;
		for (c = col; c <= last; c++)
			Msgalloc[c] = t;
	}

	for (col = 0; col < Ncols; col++)
		if (Msgalloc[col] == NOMSG)
			load ('.', Nrows - 1, col);
	tflush ();
}

/* prompt --- load str into margin of command line */

prompt (str)
char *str;
{
	loadstr (str, Cmdrow, 0, BARCOL - 1);
	tflush ();
}



/* restore_screen --- screen has been garbaged; fix it */

restore_screen ()
{
	register int row, col;
	int intrpt ();

	clrscreen ();
	for (row = 0; row < Nrows && ! intrpt (); row++)
		for (col = 0; col < Ncols; col++)
			if (Screen_image[row][col] != ' ')
			{
				position_cursor (row, col);
				send (Screen_image[row][col]);
			}

	remark ("");	/* get rid of 'type control-q....' */
}

/* saynum --- display a number in the message area */

saynum (n)
int n;
{
	char s[MAXLINE];

	sprintf (s, "%d", n);
	remark (s);
}

/* updscreen --- update screen from edit buffer */

updscreen ()
{
	char abs_lineno[10], rel_lineno[10];
	register int line, row;
	register int i;
	int prevln ();
	register LINEDESC *k;
	LINEDESC *getind ();

	fixscreen ();

	line = Topln;
	k = getind (line);

	for (row = Toprow; row <= Botrow; row++)
	{
		if (line > Lastln || line < 1)
		{
			loadstr ("", row, 0, BARCOL - 1);
			load ('|', row, BARCOL);
			loadstr ("", row, BARCOL + 1, Ncols - 1);
		}
		else
		{
			if (line == Curln)
				loadstr (".  ->", row, 0, NAMECOL - 1);
			else if (line == 1)
				loadstr ("1", row, 0, NAMECOL - 1);
			else if (line == Lastln)
				loadstr ("$", row, 0, NAMECOL - 1);
			else if (Absnos == NO && row <= 25)
			{
				rel_lineno[0] = Rel_a + row;
				rel_lineno[1] = EOS;
				loadstr (rel_lineno, row, 0, NAMECOL - 1);
			}
			else
			{
				sprintf (abs_lineno, "%d", line);
				loadstr (abs_lineno, row, 0, NAMECOL - 1);
			}

			load ((char) k->Markname, row, NAMECOL);
			load ('|', row, BARCOL);

			if (line >= First_affected)
			{
				gtxt (k);
				if (Firstcol >= k->Lineleng)
					loadstr ("", row, POOPCOL, Ncols - 1);
				else
					loadstr (&Txt[Firstcol], row, POOPCOL,
					    Ncols - 1);
			}
		}

		line++;
		k = NEXTLINE(k);
	}

	First_affected = Lastln;

	Sctop = Topln;
	Sclen = Botrow - Toprow + 1;
	for (i = 0; i < Sclen; i++)
		Scline[i] = Sctop + i <= Lastln ? i : -1;
}



/* warn_deleted --- indicate which rows on screen are no longer valid */

warn_deleted (from, to)
register int from, to;
{
	register int row;

	for (row = Toprow; row <= Botrow; row++)
		if (Topln + row - Toprow >= from
		    && Topln + row - Toprow <= to)
			loadstr ("gone", row, 0, BARCOL - 1);
}


/* watch --- keep time displayed on screen for users without watches */

watch ()
{
	long clock;
	struct tm *now, *localtime ();
	char face[10];

	time (&clock);
	now = localtime (&clock);
	sprintf (face, "%02d:%02d", now->tm_hour, now->tm_min);
	mesg (face, TIME_MSG);
}

/* adjust_window --- position window to include lines 'from' through 'to' */

adjust_window (from, to)
register int from, to;
{
	register int i, l1, l2, hw;
	int hwinsdel();

	/* see if the whole range of lines is on the screen */
	if (from < Topln || to > Topln + (Botrow - Toprow))
	{
		/* find the first and last lines that are on the screen */
		for (i = 0; i < Sclen && Scline[i] == -1; i++)
			;
		l1 = i < Sclen ? Scline[i] + Sctop : Lastln + 1;
		for (i = Sclen - 1; i >= 0 && Scline[i] == -1; i--)
			;
		l2 = i >= 0 ? Scline[i] + Sctop : 0;

		/* see if we have hardware line insert/delete */
		hw = hwinsdel();

		/* now find the best place to move the screen */
		if (to - from > Botrow - Toprow)
			Topln = to - (Botrow - Toprow);     /* put last part on screen */
		else if (hw && from >= l1 && to <= l2)
			Topln = (l1 + l2 + Toprow - Botrow) / 2;/* center l1 through l2 */
		else if (hw && from < l1 && l1 - from < (Botrow - Toprow + 1) / 2)
			Topln = from;                       /* slide the screen down */
		else if (hw && to > l2 && to - l2 < (Botrow - Toprow + 1) / 2)
			Topln = to - (Botrow - Toprow);     /* slide the screen up */
		else
			Topln = (from + to + Toprow - Botrow) / 2; /* center the range */
		if (Topln + (Botrow - Toprow) > Lastln)
			Topln = Lastln - (Botrow - Toprow);
		if (Topln < 1)
			Topln = 1;
		if (First_affected > Topln)
			First_affected = Topln;
	}
}


/* svdel --- record the deletion of buffer lines for screen update */

svdel (ln, n)
register int ln, n;
{
	register int i, j, lb, ub;

	if (ln + n <= Sctop)
		Sctop -= n;
	else if (ln < Sctop)
	{
		ub = ln + n - Sctop;
		for (i = 0; i < Sclen; i++)
			if (Scline[i] == -1)
				;
			else if (Scline[i] < ub)
				Scline[i] = -1;
			else
				Scline[i] -= ub;
		Sctop = ln;
	}
	else
	{
		lb = ln - Sctop;
		ub = ln + n - Sctop;
		for (i = 0; i < Sclen; i++)
			if (Scline[i] == -1 || Scline[i] < lb)
				;
			else if (Scline[i] < ub)
				Scline[i] = -1;
			else
				Scline[i] -= n;
	}
}


/* svins --- record a buffer insertion for screen updating */

svins (ln, n)
register int ln, n;
{
	register int i, lb;

	if (ln < Sctop)
		Sctop += n;
	else
	{
		lb = ln - Sctop;
		for (i = 0; i < Sclen; i++)
			if (Scline[i] != -1 && Scline[i] > lb)
				Scline[i] += n;
	}

}


/* fixscreen --- try to patch up the screen using insert/delete line */

fixscreen ()
{
	register int oi;	/* "old" screen index */
	register int ni;	/* "new" screen index */
	register int dc;	/* number of deletions in current block */
	register int ic;	/* number of insertions in current block */
	register int ul;	/* number of lines that must be rewritten if */
				/* we don't update by insert/delete */
	int wline[MAXROWS];	/* new screen contents; Scline contains old */
	int p;

	/* if the screen width was changed, give up before it's too late */
	if (Botrow - Toprow + 1 != Sclen || ! hwinsdel())
		return;

	/* scale the offsets in Scline to the new Topln; set any that are */
	/* off the screen to -1 so we won't have to fool with them */
	for (oi = 0; oi < Sclen; oi++)
		if (Scline[oi] == -1 || Scline[oi] + Sctop < Topln
		    || Scline[oi] + Sctop > Topln + (Botrow - Toprow))
			Scline[oi] = -1;
		else
			Scline[oi] += Sctop - Topln;

	/* fill in wline with only those numbers that are in Scline */
	for (oi = 0, ni = 0; ni < Sclen; ni++)
	{
		while (oi < Sclen && Scline[oi] == -1)
			oi++;
		if (oi < Sclen && Scline[oi] == ni)
		{
			wline[ni] = ni;
			oi++;
		}
		else
			wline[ni] = -1;
	}

	/* see if it's still advisable to fix the screen: if the number */
	/* of lines that must be rewritten is less than 2 + the number */
	/* of lines that must be inserted, don't bother (this is a dumb */
	/* but fairly effective heuristic) */
	ul = ni = 0;
	for (oi = 0; oi < Sclen; oi++)
	{
		if (Scline[oi] != wline[oi] || Scline[oi] == -1)
			ul++;
		if (wline[oi] == -1)
			ni++;
	}
	if (ul < ni + 2)
		return;

	/* Now scan the screens backwards and put out delete-lines */
	/* for all deletions and changes with more old lines than new */
	oi = ni = Sclen - 1;
	while (oi >= 0 || ni >= 0)
	{
		for (dc = 0; oi >= 0 && Scline[oi] == -1; dc++)
			oi--;
		for (ic = 0; ni >= 0 && wline[ni] == -1; ic++)
			ni--;
		if (dc > ic)
			dellines (oi + 1 + Toprow + ic, dc - ic);
		while (oi >= 0 && Scline[oi] != -1
		    && ni >= 0 && wline[ni] == Scline[oi])
			oi--, ni--;
	}

	/* At last, scan the screens forward and put out insert-lines */
	/* for all insertions and changes with more new lines than old */
	oi = ni = 0;
	while (oi < Sclen || ni < Sclen)
	{
		p = ni;
		for (dc = 0; oi < Sclen && Scline[oi] == -1; dc++)
			oi++;
		for (ic = 0; ni < Sclen && wline[ni] == -1; ic++)
			ni++;
		if (ic > dc)
			inslines (p + Toprow + dc, ic - dc);
		while (oi < Sclen && Scline[oi] != -1
		    && ni < Sclen && wline[ni] == Scline[oi])
			oi++, ni++;
	}

}


#ifdef HARD_TERMS
/* senddelay --- send NULs to delay n milliseconds */

senddelay (n)
int n;
{
	register int q;

	q = (long) n * Tspeed / 1000l;
	while (q > 0)
	{
		twrite (1, "\0\0\0\0\0\0\0\0\0\0", q > 10 ? 10 : q);
		q -= 10;
	}
}
#endif
