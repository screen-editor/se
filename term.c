/*
** term.c
**
** provide terminal functions for se
**
** If HARD_TERMS is *not* defined, which is the default, se will
** use the termlib library, which provides terminal independant operations.
** This makes se both smaller, and more flexible.
**
** If HARD_TERMS is defined, then se will use the original code, which
** had terminal types hard-wired into the code.  This would be useful for
** a system which does not have the termlib library.
**
** On System V systems, we have two possibilities.  Release 1 did not have
** the terminfo package, so we assume that if it is Release 1, someone will
** have ported the BSD termlib library.  If it is Release 2, then the new
** terminfo package is there, and we wil use it.
*/

#include "se.h"
#include "extern.h"

#ifndef HARD_TERMS

int outc ();	/* defined later */

#if defined (BSD) || !defined (S5R2)
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

extern char PC;		/* Pad character, usually '\0' */

static char *pcstr;
extern char *tgoto (), *tgetstr ();	/* termlib routines */

static char caps[128];		/* space for decoded capability strings */
static char *addr_caps;		/* address of caps for relocation */

#define TERMBUFSIZ	1024+1
static char termbuf[TERMBUFSIZ];



/* setterm -- initialize terminal parameters and actual capabilities */

static setterm (type)
char *type; 
{
	if (type[0] == EOS)
	{
		ttynormal ();
		fprintf (stderr, "in setterm: can't happen.\n");
		exit (1);
	}

	Ncols = Nrows = 0;

	/*
	 * we used to set certain mininum and maximum screen sizes,
	 * but since we could end up on things like ATT 5620s, with big
	 * screens, we just do it dynamically, but add some error
	 * checking, and exit if can't do it.
	 */
	Nrows = tgetnum ("li");
	Ncols = tgetnum ("co");

	if (Nrows == -1)
	{
		ttynormal ();
		fprintf (stderr, "se: could not determine number of rows\n");
		exit (1);
	}

	if (Ncols == -1)
	{
		ttynormal ();
		fprintf (stderr, "se: could not determine number of columns\n");
		exit (1);
	}

	
	addr_caps = caps;

	getdescrip ();			/* get terminal description */

	if (*tgoto (CM, 0, 0) == 'O')	/* OOPS returned.. */
		CM = 0;

	PC = pcstr ? pcstr[0] : EOS;

	if (CM == 0)
	{
		ttynormal ();
		fprintf (stderr, "se: terminal does not have cursor motion.\n");
		exit (2);
	}

	return OK;
}

/* getdescrip --- get descriptions out of termcap entry */

static getdescrip ()
{
	int i;
	static struct _table {
		char *name;
		char **ptr_to_cap;
		} table[] = {
			"vs",	& VS,
			"ve",	& VE,
			"ti",	& TI,
			"te",	& TE,
			"cm",	& CM,
			"ce",	& CE,
			"dl",	& DL,
			"al",	& AL,
			"cl",	& CL,
			"pc",	& pcstr,
			NULL,	NULL
			};

	AM = tgetflag ("am");		/* only boolean se needs */

	/* get string values */

	for (i = 0; table[i].name != NULL; i++)
		*(table[i].ptr_to_cap) = tgetstr (table[i].name, & addr_caps);
}


/* setcaps -- get the capabilities from termcap file into termbuf */

setcaps (term)
char *term;
{
	switch (tgetent (termbuf, term)) {
	case -1:
		ttynormal ();
		fprintf (stderr, "se: couldn't open termcap file.\n");
		return (ERR);

	case 0:
		ttynormal ();
		fprintf (stderr, "se: no termcap entry for %s terminals.\n", term);
		return (ERR);

	case 1:
		break;

	default:
		error ("in setcaps: can't happen.\n");
	}

	return (OK);
}

#else

/* use the new terminfo package */
/*
 * Do NOT include <curses.h>, since it redefines
 * USG, ERR, and OK, to values inconsisten with what
 * we use.
 */

/* fix a problem in /usr/include/term.h */
#include <termio.h>
typedef struct termio SGTTY;

#include <term.h>	/* should be all we really need */

#define AM	auto_right_margin
#define TI	enter_ca_mode
#define TE	exit_ca_mode
#define VS	cursor_visible
#define VE	cursor_normal
#define CL	clear_screen
#define CE	clr_eol
#define DL	delete_line
#define AL	insert_line

/* setcaps --- called from main() to get capabilities */

setcaps (term)
char *term;
{
	int ret = 0;

	setupterm (term, 1, & ret);
	if (ret != 1)
		return (ERR);
	Nrows = lines;
	Ncols = columns;
	return (OK);
}

#endif

/* outc -- write a character to the terminal */

int outc (c)
char c;
{
	twrite (1, &c, 1);
}

/* t_init -- put out terminal initialization string */

t_init ()
{
	if (VS)
		tputs (VS, 1, outc);
	if (TI)
		tputs (TI, 1, outc);	/* terminal initializations */
}

/* t_exit -- put out strings to turn off whatever modes we had turned on */

t_exit ()
{
	/* terminal exiting strings */
	if (TE)
		tputs (TE, 1, outc);
	if (VE)
		tputs (VE, 1, outc);
	tflush ();	/* force it out */
}
#endif

/* send --- send a printable character, predict cursor position */

send (chr)
char chr;
{
	if (Currow == Nrows - 1 && Curcol == Ncols - 1)
		return;         /* anything in corner causes scroll... */

#ifndef HARD_TERMS
	outc (chr);
#else
	twrite (1, &chr, 1);
#endif

	if (Curcol == Ncols - 1)
	{
#ifndef HARD_TERMS
		if (AM)		/* terminal wraps when hits last column */
#else
		if (Term_type != TVT && Term_type != NETRON
		    && Term_type != ESPRIT && Term_type != VI300)
#endif
		{
			Curcol = 0;
			Currow++;
		}
	}
	else		/* cursor not at extreme right */
		Curcol++;
}

/* terminal handling functions used throughout the editor */

/* clrscreen --- clear entire screen */

clrscreen ()
{
	Curcol = Currow = 0;
	/* clearing screen homes cursor to upper left corner */
	/* on all terminals */

#ifndef HARD_TERMS
	tputs (CL, 1, outc);
#else
	switch (Term_type) {
	case ADDS980:
	case ADDS100:
	case GT40:
	case CG:
	case ISC8001:
	case ANP:
	case NETRON:
		twrite (1, "\014", 1);
		break;
	case FOX:
		twrite (1, "\033K", 2);		/* clear display and all tabs */
		break;
	case TVT:
		twrite (1, "\014\017", 2);	/* home, erase to end of screen */
		break;
	case BEE150:
	case BEE200:
	case SBEE:
	case SOL:
	case H19:
		twrite (1, "\033E", 2);
		break;
	case HAZ1510:
	case ESPRIT:
		twrite (1, "\033\034", 2);
		break;
	case ADM3A:
	case VC4404:
	case TVI950:
		twrite (1, "\032", 1);
		break;
	case TS1:
		twrite (1, "\033*", 2);
		break;
	case ADM31:
		twrite (1, "\033+", 2);
		break;
	case IBM:
		twrite (1, "\033L", 2);
		break;
	case HP21:
		twrite (1, "\033H\033J", 4);	/* home cursor, erase to end of screen */
		break;
	case TRS80:
		twrite (1, "\034\037", 2);
		break;
	case VI200:
		twrite (1, "\033v", 2);
		break;
	case VI300:
		twrite (1, "\033[H\033[J", 6);
		/* home cursor, clear screen */
		break;
	case VI50:
		twrite (1, "\033v", 2);
		senddelay (30);
		break;
	}

	senddelay (20);
#endif
}


/* position_cursor --- position terminal's cursor to (row, col) */

position_cursor (row, col)
int row, col;
{
	if (row < Nrows && row >= 0		/* within vertical range? */
	    && col < Ncols && col >= 0		/* within horizontal range? */
	    && (row != Currow || col != Curcol))/* not already there? */
#ifndef HARD_TERMS
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
#else
		switch (Term_type) {
		case ADDS980:
			addspos (row, col);
			break;
		case ADDS100:
			regentpos (row, col);
			break;
		case HP21:
			hp21pos (row, col);
			break;
		case FOX:
			pepos (row, col);
			break;
		case TVT:
			tvtpos (row, col);
			break;
		case GT40:
			gt40pos (row, col);
			break;
		case BEE150:
		case BEE200:
		case SBEE:
		case SOL:
			beepos (row, col);
			break;
		case VC4404:
			vcpos (row, col);
			break;
		case HAZ1510:
			hazpos (row, col);
			break;
		case ESPRIT:
			espritpos (row, col);
			break;
		case CG:
			cgpos (row, col);
			break;
		case ISC8001:
			iscpos (row, col);
			break;
		case ADM3A:
		case ADM31:
		case TS1:
		case TVI950:
			admpos (row, col);
			break;
		case IBM:
			ibmpos (row, col);
			break;
		case ANP:
			anppos (row, col);
			break;
		case NETRON:
			netpos (row, col);
			break;
		case H19:
			h19pos (row, col);
			break;
		case TRS80:
			trspos (row, col);
			break;
		case VI200:  
		case VI50:
			vipos (row, col);
			break;
		case VI300:
			ansipos (row, col);
			break;
		}
#endif
}


/* setscreen --- initialize screen and associated descriptive variables */

setscreen ()
{
	register int row, col;

#ifndef HARD_TERMS
	char *getenv ();

#if defined (BSD) || !defined (S5R2)
	setterm (getenv ("TERM"));
#endif

	t_init ();	/* put out the 'ti' and 'vs' capabilities */
#else
	switch (Term_type) {
	case ADDS980: 
	case FOX: 
	case HAZ1510: 
	case ADDS100:
	case BEE150: 
	case ADM3A: 
	case IBM: 
	case HP21: 
	case H19:
	case ADM31: 
	case VI200: 
	case VC4404: 
	case ESPRIT: 
	case TS1:
	case TVI950: 
	case VI50: 
	case VI300:
		Nrows = 24;
		Ncols = 80;
		break;
	case ANP:
		Nrows = 24;
		Ncols = 96;
		break;
	case SOL: 
	case NETRON: 
	case TRS80:
		Nrows = 16;
		Ncols = 64;
		break;
	case TVT:
		Nrows = 16;
		Ncols = 63;
		break;
	case GT40:
		Nrows = 32;
		Ncols = 73;
		break;
	case CG:
		Nrows = 51;
		Ncols = 85;
		break;
	case ISC8001:
		Nrows = 48;
		Ncols = 80;
		break;
	case BEE200: 
	case SBEE:
		Nrows = 25;
		Ncols = 80;
		break;
	}
#endif
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
		Msgalloc[col] = NOMSG;

	Insert_mode = NO;
}


/* inslines --- insert 'n' lines on the screen at 'row' */

inslines (row, n)
int row, n;
{
	register int i;
	int delay;

	position_cursor (row, 0);
#ifdef HARD_TERMS
	if (Term_type == VI300)
	{
		char pseq[10];
		register int pp = 0;
		pseq[pp++] = '\033';
		pseq[pp++] = '[';
		if (n >= 10)
			pseq[pp++] = '0' + n / 10;
		pseq[pp++] = '0' + n % 10;
		pseq[pp++] = 'L';
		twrite (1, pseq, pp);
		delay = 0;
	}
	else
#endif
		for (i = 0; i < n; i++)
		{
#ifndef HARD_TERMS
			tputs (AL, n, outc);
			tflush ();
#else
			switch (Term_type) {
			case VI200:
				twrite (1, "\033L", 2);
				delay = 0;
				break;
			case VI50:
			case H19:
				twrite (1, "\033L", 2);
				delay = 32;
				break;
			case ESPRIT:
				twrite (1, "\033\032", 2);
				delay = 32;
				break;
			case TS1:
			case TVI950:
				twrite (1, "\033E", 2);
				delay = 0;
				break;
			case ADDS100:
				twrite (1, "\033M", 2);
				delay = 96;
				break;
			default:
				error ("in inslines: shouldn't happen");
			}

			if (delay != 0)
				senddelay (delay);
#endif
		}

	for (i = Nrows - 1; i - n >= Currow; i--)
		move_ (Screen_image[i - n], Screen_image[i], Ncols);

	for (; i >= Currow; i--)
		move_ (Blanks, Screen_image[i], Ncols);
}


/* dellines --- delete 'n' lines beginning at 'row' */

dellines (row, n)
int row, n;
{
	register int i;
	int delay;

	position_cursor (row, 0);
#ifdef HARD_TERMS
	if (Term_type == VI300)
	{
		char pseq[10];
		register int pp = 0;
		pseq[pp++] = '\033';
		pseq[pp++] = '[';
		if (n >= 10)
			pseq[pp++] = '0' + n / 10;
		pseq[pp++] = '0' + n % 10;
		pseq[pp++] = 'M';
		twrite (1, pseq, pp);
		delay = 0;
	}
	else
#endif
		for (i = 0; i < n; i++)
		{
#ifndef HARD_TERMS
			tputs (DL, n, outc);
			tflush ();
#else
			switch (Term_type) {
			case VI200:
				twrite (1, "\033M", 2);
				delay = 0;
				break;
			case VI50:
				twrite (1, "\033M", 2);
				delay = 32;
				break;
			case H19:
				twrite (1, "\033M", 2);
				delay = 32;
				break;
			case TS1:
			case TVI950:
				twrite (1, "\033R", 2);
				delay = 0;
				break;
			case ESPRIT:
				twrite (1, "\033\023", 2);
				delay = 32;
				break;
			case ADDS100:
				twrite (1, "\033l", 2);
				delay = 96;
				break;
			default:
				error ("in dellines: shouldn't happen");
			}

			if (delay != 0)
				senddelay (delay);
#endif
		}

	for (i = Currow; i + n < Nrows; i++)
		move_ (Screen_image[i + n], Screen_image[i], Ncols);

	for (; i < Nrows; i++)
		move_ (Blanks, Screen_image[i], Ncols);
}


/* hwinsdel --- return 1 if the terminal has hardware insert/delete */

int hwinsdel ()
{
	if (No_hardware == YES)
		return (NO);

#ifndef HARD_TERMS
	return (AL != NULL && DL != NULL);
#else
	switch (Term_type) {
	case VI300:
	case VI200:
	case VI50:
	case ESPRIT:
	case H19:
	case TS1:
	case TVI950:
	case ADDS100:
		return 1;
	}
	return 0;
#endif
}


/* clear_to_eol --- clear screen to end-of-line */

clear_to_eol (row, col)
int row, col;
{
	register int c, flag; 
#ifdef HARD_TERMS
	register int hardware;

	switch (Term_type) {
	case BEE200:
	case BEE150:
	case FOX:
	case SBEE:
	case ADDS100:
	case HP21:
	case IBM:
	case ANP:
	case NETRON:
	case H19:
	case TS1:
	case TRS80:
	case ADM31:
	case VI200:
	case VI300:
	case VI50:
	case VC4404:
	case ESPRIT:
	case TVI950:
		hardware = YES;
		break;
	default:
		hardware = NO;
		if (Term_type == ADDS980 && row < Nrows - 1)
			hardware = YES;
		if (Term_type == TVT && row > 0)
			hardware = YES;
	}
#endif

	flag = NO;

	for (c = col; c < Ncols; c++)
		if (Screen_image[row][c] != ' ')
		{
			Screen_image[row][c] = ' ';
#ifndef HARD_TERMS
			if (CE != NULL)		/* there is hardware */
#else
			if (hardware == YES)
#endif
				flag = YES;
			else
			{
				position_cursor (row, c);
				send (' ');
			}
		}

	if (flag == YES)
	{
		position_cursor (row, col);
#ifndef HARD_TERMS
		tputs (CE, 1, outc);
#else
		switch (Term_type) {
		case BEE200: 
		case BEE150:
		case SBEE:
		case ADDS100:
		case HP21:
		case H19:
		case VC4404:
		case TS1:
		case TVI950:
		case VI50:
			twrite (1, "\033K", 2);
			break;
		case FOX:
		case IBM:
			twrite (1, "\033I", 2);
			break;
		case ADDS980:
			twrite (1, "\n", 1);
			Currow++;
			Curcol = 0;
			break;
		case ANP:
			twrite (1, "\033L", 2);
			break;
		case NETRON:
			twrite (1, "\005", 1);
			break;
		case TRS80:
			twrite (1, "\036", 1);
			break;
		case ADM31:
			twrite (1, "\033T", 2);
			break;
		case VI200:
			twrite (1, "\033x", 2);
			break;
		case VI300:
			twrite (1, "\033[K", 3);
			break;
		case ESPRIT:
			twrite (1, "\033\017", 2);
			break;
		case TVT:
			twrite (1, "\013\012", 2);
			break;
		} /* end switch */
#endif
	} /* end if (flag == YES) */
} /* end clear_to_eol */


#ifdef HARD_TERMS

/* begin terminal dependant routines */

/* addspos --- position cursor to (row, col) on ADDS Consul 980 */

static addspos (row, col)
int row, col;
{
	char coord;
	int ntabs, where;

	if (Currow != row || col < Curcol - 7)
	{
		twrite (1, "\013", 1);	/* VT */
		coord = '@' + row;
		twrite (1, &coord, 1);
		Currow = row;
		Curcol = 0;
	}

	if (col > Curcol + 2)
	{
		ntabs = (col + 2) / 5;	/* from beginning */
		where = ntabs * 5;
		ntabs -= Curcol / 5;	/* from Curcol */
		if (ntabs + abs (where - col) <= 4)
		{
			for (; ntabs > 0; ntabs--)
				twrite (1, "\t", 1);
			Curcol = where;
		}
	}

	if (col > Curcol + 4)
	{
		where = col - Curcol;
		twrite (1, "\033\005", 2);	/* ESC ENQ */
		coord = '0' + (where / 10);
		twrite (1, &coord, 1);
		coord = '0' + (where % 10);
		twrite (1, &coord, 1);
		Curcol = col;
	}

	while (Curcol < col)
	{
		twrite (1, &Screen_image[Currow][Curcol], 1);
		Curcol++;
	}

	while (Curcol > col)
	{
		twrite (1, "\b", 1);
		Curcol--;
	}
}

/* admpos --- position cursor to (row, col) on ADM-3A and ADM-31 terminals */

static admpos (row, col)
int row, col;
{
	int dist;
	char coord;

	dist = col - Curcol;
	if (dist < 0)
		dist = -dist;
	if (row == Currow && dist < 4)  /* 4 chars for abs. position */
	{
		while (Curcol < col)
		{
			twrite (1, &Screen_image[Currow][Curcol], 1);
			Curcol++;
		}
		while (Curcol > col)
		{
			twrite (1, "\b", 1);
			Curcol--;
		}
	}
	else
	{
		twrite (1, "\033=", 2);
		coord = row + ' ';
		twrite (1, &coord, 1);
		coord = col + ' ';
		twrite (1, &coord, 1);
		Currow = row;
		Curcol = col;
	}
}



/* ansipos --- position cursor on ANSI X something-or-other terminals */

static ansipos (row, col)
register int row, col;
{
	register int dist;

	char absseq[20], relseq[50];
	int absp = 0;
	register int relp = 0;
	register int trow, tcol;

	/*** Build relative positioning string--handle row first ***/
	trow = Currow; 
	tcol = Curcol;
	if (row >= trow && row <= trow + 3)
		for (; trow < row; trow++)
			relseq[relp++] = '\012';
	else if (row < trow && row >= trow - 1)
		for (; trow > row; trow--)
		{ 
			relseq[relp++] = '\033'; 
			relseq[relp++] = 'M'; 
		}
	else if (row >= trow)
	{
		relseq[relp++] = '\033';
		relseq[relp++] = '[';
		dist = row - trow;
		if (dist >= 10)
			relseq[relp++] = '0' + dist / 10;
		relseq[relp++] = '0' + dist % 10;
		relseq[relp++] = 'B';
		trow = row;
	}
	else /* row < trow */
	{
		relseq[relp++] = '\033';
		relseq[relp++] = '[';
		dist = trow - row;
		if (dist >= 10)
			relseq[relp++] = '0' + dist / 10;
		relseq[relp++] = '0' + dist % 10;
		relseq[relp++] = 'A';
		trow = row;
	}

	/*** Now do the column part of relative positioning ***/
	if (col >= tcol - 2 && col <= tcol + 2)
		;       /* skip coarse positioning -- just do the fine stuff */
	else
	{
		if (col <= 4)
		{
			relseq[relp++] = '\015';
			tcol = 0;
		}
		dist = col - tcol;
		if (col < 72 && dist > 2
		    && dist < 8 && (col + 1) % 8 <= 2)
		{
			relseq[relp++] = '\t';
			tcol = ((tcol + 8) / 8) * 8;
		}
	}
	dist = col - tcol;
	if (dist < 0)
		dist = -dist;
	if (dist == 0)
		;
	else if (dist < 4)  /* 4 chars for abs. position */
	{
		while (tcol < col)
		{
			relseq[relp++] = Screen_image[trow][tcol];
			tcol++;
		}
		while (tcol > col)
		{
			relseq[relp++] = '\b';
			tcol--;
		}
	}
	else if (col >= tcol)
	{
		relseq[relp++] = '\033';
		relseq[relp++] = '[';
		if (dist >= 10)
			relseq[relp++] = '0' + dist / 10;
		relseq[relp++] = '0' + dist % 10;
		relseq[relp++] = 'C';
		tcol = col;
	}
	else /* if (col < tcol) */
	{
		relseq[relp++] = '\033';
		relseq[relp++] = '[';
		if (dist >= 10)
			relseq[relp++] = '0' + dist / 10;
		relseq[relp++] = '0' + dist % 10;
		relseq[relp++] = 'D';
		tcol = col;
	}

	/*** If relative positioning will do it, forget absolute ***/
	if (relp <= 5)
		twrite (1, relseq, relp);
	else
	{
		absseq[absp++] = '\033';
		absseq[absp++] = '[';
		if (row >= 9)
			absseq[absp++] = '0' + (row + 1) / 10;
		absseq[absp++] = '0' + (row + 1) % 10;
		absseq[absp++] = ';';
		if (col >= 9)
			absseq[absp++] = '0' + (col + 1) / 10;
		absseq[absp++] = '0' + (col + 1) % 10;
		absseq[absp++] = 'H';
		if (absp >= relp)
			twrite (1, relseq, relp);
		else
			twrite (1, absseq, absp);
	}
	Curcol = col;
	Currow = row;
}



/* anppos --- position cursor on Allen & Paul model 1 */

static anppos (row, col)
int row, col;
{
	register char coord;

	if (row == Currow)      /* if close, just sneak right or left */
	{
		if (col == Curcol + 1)
			twrite (1, "\t", 1);
		else if (col == Curcol + 2)
			twrite (1, "\t\t", 2);
		else if (col == Curcol - 1)
			twrite (1, "\b", 1);
		else if (col == Curcol - 2)
			twrite (1, "\b\b", 2);
		else
		{
			twrite (1, "\033C", 2);
			coord = col + ' ';
			twrite (1, &coord, 1);
		}
	}

	else if (col == Curcol) /* if close, sneak up or down */
	{
		if (row == Currow + 1)
			twrite (1, "\012", 1);
		else if (row == Currow + 2)
			twrite (1, "\012\012", 2);
		else if (row == Currow - 1)
			twrite (1, "\013", 1);
		else if (row == Currow - 2)
			twrite (1, "\013\013", 2);
		else
		{
		/* because of bug in anp software, abs row pos is not working.
		 * the following code was replaced to compensate:
		 *
		 *          twrite (1, "\033R", 2);
		 *          coord = row + ' ';
		 *          twrite (1, &coord, 1);
		 */
			twrite (1, "\033P", 2);
			coord = row + ' ';
			twrite (1, &coord, 1);
			coord = col + ' ';
			twrite (1, &coord, 1);
		}
	}
	else	/* resort to absolute positioning */
	{
		twrite (1, "\033P", 2);
		coord = row + ' ';
		twrite (1, &coord, 1);
		coord = col + ' ';
		twrite (1, &coord, 1);
	}

	Currow = row;
	Curcol = col;
}

/* b200coord --- transmit a coordinate for Beehive 200 cursor addressing */

static b200coord (coord)
int coord;
{
	char acc;
	int tens, units;

	tens = coord / 10;
	units = coord - 10 * tens;
	acc = units + 16 * tens;

	twrite (1, & acc, 1);
}

/* beepos --- position cursor on Beehive terminal */

static beepos (row, col)
int row, col;
{
	if (row == Currow + 1 && col == 0 && Term_type != SBEE)
	{
		twrite (1, "\r\n", 2);		/*  CR LF */
		Curcol = 0;
		Currow++;
	}
	else if (row == 0 && col == 0)		/* home cursor */
	{
		twrite (1, "\033H", 2);
		Currow = Curcol = 0;
	}
	else if (row == Currow && col > Curcol && col <= Curcol + 4)
		while (Curcol != col)
		{
			twrite (1, &Screen_image[Currow][Curcol], 1);
			Curcol++;
		}
	else if (row == Currow && col < Curcol && col >= Curcol - 4)
		while (Curcol != col)
		{
			twrite (1, "\b", 1);
			Curcol--;
		}
	else		/* resort to absolute addressing */
	{
		twrite (1, "\033F", 2);
		if (Term_type == BEE200 || Term_type == SOL)
		{
			b200coord (row);
			b200coord (col);
		}
		else if (Term_type == BEE150)
		{
			char r, c;

			r = row + ' ';
			c = col + ' ';
			twrite (1, &r, 1);
			twrite (1, &c, 1);
		}
		else		/* is superbee */
		{
			sbeecoord (col);
			sbeecoord (row);
		}

		Currow = row;
		Curcol = col;
	}
}

/* cgpos --- position cursor on Chromatics CG */

static cgpos (row, col)
int row, col;
{
	char i, j;

	if (row == Currow + 1 && col == 0)
	{
		twrite (1, "\r\n", 2);		/* CR LF */
		Curcol = 0;
		Currow++;
	}
	else if (row == 0 && col == 0)		/* home cursor */
	{
		twrite (1, "\034", 1);	/* FS */
		Currow = Curcol = 0;
	}
	else if (row == Currow && col > Curcol && col <= Curcol + 7)
		while (Curcol != col)
		{
			twrite (1, "\035", 1);	/* GS */
			Curcol++;
		}
	else if (row == Currow && col < Curcol && col >= Curcol - 7)
		while (Curcol != col)
		{
			twrite (1, "\b", 1);
			Curcol--;
		}
	else
	{
		/* resort to absolute addressing */
		twrite (1, "\001U", 2);		/* SOH U */
		i = 511 - (10 * row);
		j = 6 * col;
		cgcoord (j);
		cgcoord (i);
		Currow = row;
		Curcol = col;
	}

}


/* cgcoord --- output a decimal coordinate for Chromatics CG */

static cgcoord (i)
int i;
{
	int units, tens, hundreds;
	char coords[4];

	units = i % 10;
	i /= 10;
	tens = i % 10;
	i /= 10;
	hundreds = i % 10;

	coords[0] = hundreds + 16 + ' ';
	coords[1] = tens + 16 + ' ';
	coords[2] = units + 16 + ' ';
	coords[3] = EOS;
	twrite (1, coords, 3);
}



/* gt40pos --- position cursor to (row, col) on DEC GT40 with Waugh software */

static gt40pos (row, col)
int row, col;
{
	char coord;

	if (row != Currow && col != Curcol)	/* absolute positioning */
	{
		twrite (1, "\033", 1);
		coord = row + ' ';
		twrite (1, &coord, 1);
		coord = col + ' ';
		twrite (1, &coord, 1);
		Currow = row;
		Curcol = col;
	}
	else if (row != Currow)		/* col must = Curcol */
	{				/* vertical positioning */
		twrite (1, "\006", 1);	/* ACK */
		coord = row + ' ';
		twrite (1, &coord, 1);
		Currow = row;
	}
	else if (abs (col - Curcol) < 2)
		uhcm (col);
	else
	{
		twrite (1, "\025", 1);	/* NACK */
		coord = col + ' ';
		twrite (1, &coord, 1);
		Curcol = col;
	}
}



/* h19pos --- position cursor on Heath H19 (DEC VT52 compatible, supposedly) */

static h19pos (row, col)
int row, col;
{
	int dist;
	char coord;

	dist = col - Curcol;
	if (dist < 0)
		dist = -dist;
	if (row == Currow && dist < 4)  /* 4 chars for abs. position */
	{
		while (Curcol < col)
		{
			twrite (1, &Screen_image[Currow][Curcol], 1);
			Curcol++;
		}
		while (Curcol > col)
		{
			twrite (1, "\b", 1);
			Curcol--;
		}
	}
	else
	{
		twrite (1, "\033Y", 2);
		coord = row + ' ';
		twrite (1, &coord, 1);
		coord = col + ' ';
		twrite (1, &coord, 1);
		Currow = row;
		Curcol = col;
	}
}

/* hp21pos --- position cursor on HP 2621 terminal */

static hp21pos (row, col)
int row, col;
{
	int units, tens;

	if (row == Currow && col == 0)
	{
		twrite (1, "\r\n", 2);		/* CR LF */
		Curcol = 0;
		Currow++;
	}
	else if (row == 0 && col == 0)		/* home cursor */
	{
		twrite (1, "\033H", 2);
		Currow = Curcol = 0;
	}
	else if (row == Currow && col > Curcol && col <= Curcol + 4)
		while (Curcol != col)
		{
			twrite (1, &Screen_image[Currow][Curcol], 1);
			Curcol++;
		}
	else if (row == Currow && col < Curcol && col >= Curcol - 4)
		while (Curcol != col)
		{
			twrite (1, "\b", 1);
			Curcol--;
		}
	else if (2 * abs (Currow - row) + abs (Curcol - col) <= 7)
	{
		while (Currow < row)
		{
			twrite (1, "\033B", 2);
			Currow++;
		}
		while (Currow > row)
		{
			twrite (1, "\033A", 2);
			Currow--;
		}
		while (Curcol > col)
		{
			twrite (1, "\b", 1);
			Curcol--;
		}
		while (Curcol < col)
		{
			twrite (1, & Screen_image[Currow][Curcol], 1);
			Curcol++;
		}
	}
	else
	{
		/* resort to absolute addressing */
		char c;

		twrite (1, "\033&a", 3);
		units = row % 10;
		tens = row / 10;
		if (tens != 0)
		{
			c = tens + '0';
			twrite (1, &c, 1);
		}
		c = units + '0';
		twrite (1, &c, 1);
		twrite (1, "y", 1);
		units = col % 10;
		tens = col / 10;
		if (tens != 0)
		{
			c = tens + '0';
			twrite (1, &c, 1);
		}
		c = units + '0';
		twrite (1, &c, 1);
		twrite (1, "C", 1);
		Currow = row;
		Curcol = col;
	}
}


/* hazpos --- position cursor on Hazeltine 1510 */

static hazpos (row, col)
int row, col;
{
	int dist;
	char c;

	dist = col - Curcol;
	if (dist < 0)
		dist = -dist;
	if (row == Currow && dist < 4)  /* 4 chars for abs. position */
	{
		while (Curcol < col)
		{
			twrite (1, &Screen_image[Currow][Curcol], 1);
			Curcol++;
		}
		while (Curcol > col)
		{
			twrite (1, "\b", 1);
			Curcol--;
		}
	}
	else
	{
		twrite (1, "\033\021", 2);
		c = col;
		twrite (1, &c, 1);
		c = row;
		twrite (1, &c, 1);
		Currow = row;
		Curcol = col;
	}
}


/* ibmpos --- position cursor on IBM 3101 terminal */

static ibmpos (row, col)
int row, col;
{
	int dist;
	static char abspos[] = "\033\Y\0\0";

	dist = col - Curcol;
	if (dist < 0)
		dist = -dist;
	
	if (row == Currow && dist < 4)		/* 4 chars for abs pos */
	{
		while (Curcol < col)
		{
			twrite (1, & Screen_image[Currow][Curcol], 1);
			Curcol++;
		}
		while (Curcol > col)
		{
			twrite (1, "\b", 1);
			Curcol--;
		}
	}
	else
	{
		abspos[2] = row + ' ';
		abspos[3] = col + ' ';
		twrite (1, abspos, 4);
		Currow = row;
		Curcol = col;
	}
}



/* iscpos --- position cursor on ISC 8001 color terminal */

static iscpos (row, col)
int row, col;
{
	char r, c;

	if (row == 0 && col == 0)
		twrite (1, "\b", 1);
	else
	{
		twrite (1, "\003", 1);		/* ETX */
		r = row;
		c = col;
		twrite (1, & r, 1);
		twrite (1, & c, 1);
	}

	Currow = row;
	Curcol = col;
}

/* netpos --- position cursor on Netron terminal */

static netpos (row, col)
int row, col;
{
	static char abspos[] = "\033=\0\0";

	abspos[2] = (char) row;
	abspos[3] = (char) col;
	twrite (1, abspos, 4);
	Currow = row;
	Curcol = col;
}

/* pepos --- position cursor on Perkin-Elmer 550 & 1100 terminals */

static pepos (row, col)
int row, col;
{
	char coord;

	/* get on correct row first */
	if (Currow == row)
		;		/* already on correct row; nothing to do */
	else if (row == Currow - 1)
	{
		twrite (1, "\033A", 2);		/* cursor up */
		Currow--;
	}
	else if (row == Currow + 1)
	{
		twrite (1, "\033B", 2);		/* cursor down */
		Currow++;
	}
	else
	{
		/* vertical absolute positioning */
		twrite (1, "\033X", 2);
		coord = row + ' ';
		twrite (1, & coord, 1);
	}

	/* now perform horizontal motion */
	if (abs (col - Curcol) > 3)	/* do absolute horizontal position */
	{
		twrite (1, "\033Y", 2);
		coord = col + ' ';
		twrite (1, &coord, 1);
		Curcol = col;
	}
	else
		uhcm (col);
}

/* sbeecoord --- transmit a coordinate for Superbee terminal */

static sbeecoord (coord)
int coord;
{
	char r, c;

	r = (coord / 10) + ' ';
	c = (coord % 10) + ' ';
	twrite (1, & r, 1);
	twrite (1, & c, 1);
}

/* trspos --- position cursor on TRS80 Model 1 */

static trspos (row, col)
int row, col;
{
	while (Currow != row)
	{
		if (Currow > row)
		{
			twrite (1, "\033", 1);
			Currow--;
		}
		else
		{
			twrite (1, "\032", 1);		/* SUB */
			Currow++;
		}
	}

	if (Curcol != col)
	{
		if (col > Curcol)
			while (col > Curcol)
			{
				twrite (1, "\031", 1);	/* EM */
				Curcol++;
			}
		else if (col < Curcol / 2)
		{
			twrite (1, "\035", 1);	/* GS */
			Curcol = 0;
			while (Curcol < col)
			{
				twrite (1, "\031", 1);	/* EM */
				Curcol++;
			}
		}
		else
			while (col < Curcol)
			{
				twrite (1, "\030", 1);	/* CAN */
				Curcol--;
			}
	}
}



/* tvtpos --- position cursor on Allen's TV Typetwriter II */

static tvtpos (row, col)
int row, col;
{
	register int horirel, horiabs, vertrel, vertabs;

	horirel = col - Curcol;
	if (horirel < 0)
		horirel = -horirel;

	horiabs = col;

	if (row <= Currow)
		vertrel = Currow - row;
	else
		vertrel = Nrows - (row - Currow);

	if (row == 0)
		vertabs = 0;
	else
		vertabs = Nrows - row;

	if (1 + horiabs + vertabs <= horirel + vertrel)
	{
		twrite (1, "\014", 1);
		Currow = Curcol = 0;
	}

	while (Currow != row)
	{
		twrite (1, "\013", 1);
		Currow--;
		if (Currow < 0)
			Currow = Nrows - 1;
	}

	if (Curcol > col)
		for (; Curcol != col; Curcol--)
			twrite (1, "\b", 1);
	else
		for (; Curcol != col; Curcol++)
			twrite (1, "\t", 1);
}



/* regentpos --- position cursor on ADDS Regent 100 */

static regentpos (row, col)
int row, col;
{
	int dist;
	char coord;

	dist = col - Curcol;
	if (dist < 0)
		dist = -dist;
	
	if (dist > 4 || Currow != row)
	{
		twrite (1, "\033Y", 2);
		coord = ' ' + row;
		twrite (1, &coord, 1);
		coord = ' ' + col;
		twrite (1, &coord, 1);
		Currow = row;
		Curcol = col;
	}
	else
	{
		while (row < Currow)
		{
			twrite (1, "\032", 1);	/* SUB, cursor up */
			Currow--;
		}
		while (row > Currow)
		{
			twrite (1, "\n", 1);
			Currow++;
			Curcol = 1;
		}
		if (col > Curcol)
			while (col != Curcol)
			{
				twrite (1, &Screen_image[Currow][Curcol], 1);
				Curcol++;
			}
		else if ((Curcol - col) * 2 >= Ncols)
			while (col != Curcol)
			{
				twrite (1, "\006", 1);	/* ACK, cursor right */
				if (Curcol == Ncols)
					Curcol = 1;
				else
					Curcol++;
			}
		else
			while (col != Curcol)
			{
				twrite (1, "\b", 1);
				Curcol--;
			}
	}
}



/* vipos --- position cursor on Visual 200 & 50 */

static vipos (row, col)
register int row, col;
{
	register int dist;
	register char coord;

	if (row == Currow + 1 && col < 3)
	{
		twrite (1, "\015\012", 2);
		Currow++;
		Curcol = 0;
	}
	dist = col - Curcol;
	if (Term_type == VI200 && row == Currow && col < 72 && dist > 2
	    && dist < 8 && (col + 1) % 8 < 2)
	{
		twrite (1, "\t", 1);
		Curcol = ((Curcol + 7) / 8) * 8;
		dist = col - Curcol;
	}
	if (dist < 0)
		dist = -dist;
	if (row == Currow && dist < 4)  /* 4 chars for abs. position */
	{
		while (Curcol < col)
		{
			twrite (1, &Screen_image[Currow][Curcol], 1);
			Curcol++;
		}
		while (Curcol > col)
		{
			twrite (1, "\b", 1);
			Curcol--;
		}
	}
	else
	{
		twrite (1, "\033Y", 2);
		coord = row + ' ';
		twrite (1, &coord, 1);
		coord = col + ' ';
		twrite (1, &coord, 1);
		Currow = row;
		Curcol = col;
	}
}



/* vcpos --- position cursor Volker-Craig 4404 (ADM3A mode) */

static vcpos (row, col)
int row, col;
{
	int dist;
	char coord;

	dist = col - Curcol;
	if (dist < 0)
		dist = -dist;
	if (row == Currow && dist < 4)  /* 4 chars for abs. position */
	{
		while (Curcol < col)
		{
			twrite (1, &Screen_image[Currow][Curcol], 1);
			Curcol++;
		}
		while (Curcol > col)
		{
			twrite (1, "\b", 1);
			Curcol--;
		}
	}
	else
	{
		twrite (1, "\033=", 2);
		coord = row + ' ';
		twrite (1, &coord, 1);
		coord = col + ' ';
		twrite (1, &coord, 1);
		Currow = row;
		Curcol = col;
	}
}


/* espritpos --- position cursor on Hazeltine Esprit */

static espritpos (row, col)
int row, col;
{
	int dist;
	char c;

	dist = col - Curcol;
	if (dist < 0)
		dist = -dist;
	if (row == Currow && dist < 4)  /* 4 chars for abs. position */
	{
		while (Curcol < col)
		{
			twrite (1, &Screen_image[Currow][Curcol], 1);
			Curcol++;
		}
		while (Curcol > col)
		{
			twrite (1, "\b", 1);
			Curcol--;
		}
	}
	else
	{
		twrite (1, "\033\021", 2);
		c = col >= 32 ? col : col + '`';
		twrite (1, &c, 1);
		c = row >= 32 ? row : row + '`';
		twrite (1, &c, 1);
		Currow = row;
		Curcol = col;
	}
}

/* uhcm --- universal horizontal cursor motion */

static uhcm (col)
int col;
{
	while (Curcol < col)
	{
		twrite (1, &Screen_image[Currow][Curcol], 1);
		Curcol++;
	}

	while (Curcol > col)
	{
		twrite (1, "\b", 1);
		Curcol--;
	}
}
#endif
