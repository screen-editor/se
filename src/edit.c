/*
** edit.c
**
** editor main routine, plus other routines used a lot.
**
** This file is in the public domain.
*/

#include "config.h"

#include <string.h>
#include <sys/utsname.h>	/* stuff to find out who we are */
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifdef LOG_USAGE
#include <syslog.h>
#endif

#include "se.h"
#include "docmd1.h"
#include "docmd2.h"
#include "extern.h"
#include "edit.h"
#include "main.h"
#include "misc.h"
#include "pat.h"
#include "scratch.h"
#include "screen.h"
#include "term.h"

static char Savknm = DEFAULTNAME;	/* saved mark name for < and > */

/* edit --- main routine for screen editor */

void edit (int argc, char *argv[])
{
	int cursav, status, len, cursor;
	char lin[MAXLINE], term;

	watch ();       /* display time of day */

#ifdef LOG_USAGE
	log_usage ();		/* log who used the program */
#endif

	serc ();	/* execute commands in ./.serc or $HOME/.serc */

	status = OK;

	while (status == OK && Argno < argc)
	{
		memset (lin, EOS, MAXLINE);
		strncpy (lin, argv[Argno], MAXLINE-1);
		loadstr (lin, Argno, POOPCOL, Ncols);
		if (lin[0] == '-')
		{
			len = strlen (lin) + 1;
			lin[len - 1] = '\n';
			lin[len] = EOS;
			len = 0;
			status = doopt (lin, &len);
		}
		else
		{
			dfltsopt (lin);
			status = doread (Lastln, lin, NO);
		}
		Argno++;
	}

	if (status == ERR)
	{
		if (Errcode == EHANGUP)
			hangup ();
		print_verbose_err_msg ();
	}
	else
		Curln = min (1, Lastln);

	Buffer_changed = NO;
	First_affected = 1;     /* maintained by updscreen & commands */
	updscreen ();

	if (status != ERR)	/* leave offending file name or option */
	{
		lin[0] = EOS;
	}
	cursor = 0;

	/* main command loop */
	do {
		intrpt ();	/* discard pending breaks (interrupts) */
		if (Lost_lines > GARB_THRESHOLD
		    && (Lastln + Limcnt) / Lost_lines <= GARB_FACTOR)
			garbage_collect ();

		mswait ();	/* check for pending messages */
		Cmdrow = Botrow + 1;    /* reset the command line location */
		prompt ("cmd>");
		getcmd (lin, 0, &cursor, &term);
		remark ("");	/* clear out any error messages */

		while (term == CURSOR_UP || term == CURSOR_DOWN
		    || term == CURSOR_SAME)
		{
			switch (term) {
			case CURSOR_UP:
				if (Curln > 1)
					Curln--;
				else
					Curln = Lastln;
				break;

			case CURSOR_DOWN:
				if (Curln < Lastln)
					Curln++;
				else
					Curln = min (1, Lastln);
				break;
			}
			adjust_window (Curln, Curln);
			updscreen ();
			getcmd (lin, 0, &cursor, &term);
		}

		prompt ("");	/* remove prompt */

		cursav = Curln;		/* remember it in case of an error */
		Errcode = EEGARB;	/* default error code for garbage at end */

		len = 0;
		if (getlst (lin, &len, &status) == OK)
		{
			if (ckglob (lin, &len, &status) == OK)
				doglob (lin, &len, &cursav, &status);
			else if (status != ERR)
				docmd (lin, len, NO, &status);
		}
		if (status == ERR)
		{
			if (Errcode == EHANGUP)
				hangup ();
			print_verbose_err_msg ();
			Curln = min (cursav, Lastln);
		}
		else if (term != FUNNY)
		{
			cursor = 0;
			lin[0] = EOS;
		}

		adjust_window (Curln, Curln);
		updscreen ();

	} while (status != EOF);

	clrscreen ();
	clrbuf ();
	tflush ();

	return;
}


/* getlst --- collect line numbers (if any) at lin[*i], increment i */

int getlst (char lin[], int *i, int *status)
{
	int num;

	Line2 = 0;
	for (Nlines = 0; getone (lin, i, &num, status) == OK; )
	{
		Line1 = Line2;
		Line2 = num;
		Nlines++;
		if (lin[*i] != ',' && lin[*i] != ';')
			break;
		if (lin[*i] == ';')
			Curln = num;
		(*i)++;
	}

	if (Nlines > 2)
		Nlines = 2;

	if (Nlines <= 1)
		Line1 = Line2;

	if (Line1 > Line2)
	{
		*status = ERR;
		Errcode = EBACKWARD;
	}

	if (*status != ERR)
		*status = OK;

	return (*status);
}


/* getnum --- convert one term to line number */

int getnum (char lin[], int *i, int *pnum, int *status)
{
	int j, ret;
	int k;

	ret = OK;
	SKIPBL (lin, *i);
	if (lin[*i] >= Rel_a && lin[*i] <= Rel_z && Absnos == NO)
		*pnum = Topln - Toprow + lin[*i] - Rel_a;
	else if (lin[*i] == CURLINE)
		*pnum = Curln;
	else if (lin[*i] == PREVLN || lin[*i] == PREVLN2)
		*pnum = Curln - 1;
	else if (lin[*i] == LASTLINE)
		*pnum = Lastln;
	else if (lin[*i] == SCAN || lin[*i] == BACKSCAN)
	{
		int missing_delim = YES;

		/* see if trailing delim supplied, since command can follow pattern */
		for (k = *i + 1; lin[k] != EOS; k++)
			if (lin[k] == ESCAPE)
				k++;	/* skip esc, loop will skip escaped char */
			else if (lin[k] == lin[*i])
			{
				missing_delim = NO;
				break;
			}
			/* else
				continue */

		if (missing_delim == YES)
		{
			for (; lin[k] != EOS; k++)
				;
			k--;		/* k now at newline */

			/* supply trailing delim */
			lin[k] = lin[*i];
			lin[++k] = '\n';
			lin[++k] = EOS;
			Peekc = SKIP_RIGHT;
		}

		if (optpat (lin, i) == ERR)
			ret = ERR;
		else if (lin[*i] == SCAN)
			ret = ptscan (FORWARD, pnum);
		else 
			ret = ptscan (BACKWARD, pnum);
	}
	else if (lin[*i] == SEARCH || lin[*i] == BACKSEARCH)
	{
		j = *i;
		(*i)++;
		if (getkn (lin, i, &Savknm, Savknm) == ERR)
			ret = ERR;
		else if (lin[j] == SEARCH)
			ret = knscan (FORWARD, pnum);
		else
			ret = knscan (BACKWARD, pnum);
		(*i)--;
	}
	else if (isdigit (lin[*i]))
	{
		*pnum = ctoi (lin, i);
		(*i)--;
	}
	else if (lin[*i] == TOPLINE)
		*pnum = Topln;
	else
		ret = EOF;

	if (ret == OK)
		(*i)++;
	*status = ret;
	return (ret);
}


/* getone --- evaluate one line number expression */

int getone (char lin[], int *i, int *num, int *status)
{
	int pnum, ret;
	char porm;	/* "plus or minus" (sic) */

	ret = EOF;	/* assume we won't find anything for now */
	*num = 0;

	if (getnum (lin, i, num, status) == OK)		/* first term */
	{
		ret = OK;	/* to indicate we've seen something */
		do {			/* + or - terms */
			porm = EOS;
			SKIPBL (lin, *i);
			if (lin[*i] == '-' || lin[*i] == '+')
			{
				porm = lin[*i];
				(*i)++;
			}
			if (getnum (lin, i, &pnum, status) == OK)
			{
				if (porm == '-')
				{
					*num -= pnum;
				}
				else
				{
					*num += pnum;
				}
			}
			if (*status == EOF && porm != EOS)	/* trailing + or - */
			{
				*status = ERR;
			}
		} while (*status == OK);
	}

	if (*num < 0 || *num > Lastln)	/* make sure number is in range */
	{
		*status = ERR;
		Errcode = EORANGE;
	}

	if (*status == ERR)
		ret = ERR;
	else
		*status = ret;

	return (ret);
}


static int special_casing = NO;

/* ckglob --- if global prefix, mark lines to be affected */

int ckglob (char lin[], int *i, int *status)
{
	int line, tmp;
	int usepat, usemark;
	LINEDESC *k;

	*status = OK;
	usepat = EOF;
	usemark = EOF;

	if (	/* g/^/m0  or g/$/m0 -- special case the pathological */
		/* cases in order to save time */
		(lin[*i] == GLOBAL || lin[*i] == UCGLOBAL)
		&& (lin[*i + 1] == lin[*i + 3])
		&& (lin[*i + 2] == '^' || lin[*i + 2] == '$')
		&& (lin[*i + 4] == MOVECOM || lin[*i + 4] == UCMOVECOM)
		&& (lin[*i + 5] == '0' && lin[*i + 6] == '\n')   )
	{
		special_casing = YES;
		remark ("GLOB");
		return (OK);
	}

	if (lin[*i] == GMARK || lin[*i] == XMARK)	/* global markname prefix? */
	{
		if (lin[*i] == GMARK)	/* tag lines with the specified markname */
			usemark = YES;
		else			/* tag lines without the specified markname */
			usemark = NO;
		(*i)++;
		*status = getkn (lin, i, &Savknm, Savknm);
	}

	if (*status == OK)	/* check for a pattern prefix too */
	{
		if (lin[*i] == GLOBAL || lin[*i] == UCGLOBAL)
			usepat = YES;

		if (lin[*i] == EXCLUDE || lin[*i] == UCEXCLUDE)
			usepat = NO;

		if (usepat != EOF)
		{
			(*i)++;
			if (optpat (lin, i) == ERR)
				*status = ERR;
			else
				(*i)++;
		}
	}

	if (*status == OK && usepat == EOF && usemark == EOF)
		*status = EOF;
	else if (*status == OK)
		defalt (1, Lastln);

	if (*status == OK)	/* no errors so far, safe to proceed */
	{
		remark ("GLOB");

		k = Line0;      /* unmark all lines preceeding range */
		for (line = 0; line < Line1; line++)
		{
			k -> Globmark = NO;
			k = NEXTLINE(k);
		}

		for (; line <= Line2; line++)	/* mark lines in range */
		{
			if (intrpt ())
			{
				*status = ERR;
				return (*status);
			}
			tmp = NO;
			if (((usemark == EOF
			    || usemark == YES) && k -> Markname == Savknm)
			    || (usemark == NO && k -> Markname != Savknm))
			{
				if (usepat == EOF)	/* no global pattern to look for */
					tmp = YES;
				else	/* there is also a pattern to look for */
				{
					gtxt (k);
					if (match (Txt, Pat) == usepat)
						tmp = YES;
				}
			}

			k -> Globmark = tmp;

			k = NEXTLINE(k);
		}

		/* mark remaining lines */
		for (; line <= Lastln; line++)
		{
			k -> Globmark = NO;
			k = NEXTLINE (k);
		}

		remark ("");
	}

	return (*status);
}


/* doglob --- do command at lin[i] on all marked lines */

int doglob (char lin[], int *i, int *cursav, int *status)
{
	int istart, line;
	LINEDESC *k;

	if (special_casing)
	{
		/*
		remark ("Warp 7, Captain!");
		*/
		/* not on the screen too long anyway */
		reverse (1, Lastln);
		Curln = Lastln;
		special_casing = NO;
		Buffer_changed = YES;
		First_affected = min (1, First_affected);
		remark ("");
		adjust_window (Curln, Curln);
		updscreen ();
		return (OK);
	}

	*status = OK;
	istart = *i;
	k = Line0;
	line = 0;

	do {
		line++;
		k = NEXTLINE(k);
		if (k -> Globmark == YES)	/* line is marked */
		{
			k -> Globmark = NO;	/* unmark the line */
			Curln = line;
			*cursav = Curln;	/* remember where we are */
			*i = istart;
			if (getlst (lin, i, status) == OK)
				docmd (lin, *i, YES, status);
			line = 0;		/* lines may have been moved */
			k = Line0;
		}
		if (intrpt ())
			*status = ERR;
	} while (line <= Lastln && *status == OK);

	return (*status);
}


/* ckchar --- look for ch or altch on lin at i, set flag if found */

int ckchar (char ch, char altch, char lin[], int *i, int *flag, int *status)
{

	if (lin[*i] == ch || lin[*i] == altch)
	{
		(*i)++;
		*flag = YES;
	}
	else
		*flag = NO;

	*status = OK;
	return (OK);
}


/* ckp --- check for "p" after command */

int ckp (char lin[], int i, int *pflag, int *status)
{

	if (lin[i] == PRINT || lin[i] == UCPRINT)
	{
		i++;
		*pflag = YES;
	}
	else
		*pflag = NO;

	if (lin[i] == '\n')
		*status = OK;
	else
		*status = ERR;

	return (*status);
}


/* ckupd --- make sure it is ok to destroy the buffer */

int ckupd (char lin[], int *i, char cmd, int *status)
{
	int flag;

	*status = ckchar (ANYWAY, ANYWAY, lin, i, &flag, status);
	if (flag == NO && Buffer_changed == YES && Probation != cmd)
	{
		*status = ERR;
		Errcode = ESTUPID;
		Probation = cmd;        /* if same command is repeated, */
	}                       /* we'll keep quiet */

	return (*status);
}


/* defalt --- set defaulted line numbers */

void defalt (int def1, int def2)
{

	if (Nlines == 0)        /* no line numbers supplied, use defaults */
	{
		Line1 = def1;
		Line2 = def2;
	}

	return;
}


/* getfn --- get file name from lin[i]... */

int getfn (char lin[], int i, char filename[], size_t filenamesize)
{
	int j, k, ret;

	ret = ERR;
	if (lin[i + 1] == ' ')
	{
		j = i + 2;      /* get new file name */
		SKIPBL (lin, j);
		for (k = 0; lin[j] != NEWLINE; k++, j++)
		{
			filename[k] = lin[j];
		}
		filename[k] = EOS;
		if (k > 0)
		{
			ret = OK;
		}
	}
	else if (lin[i + 1] == '\n' && Savfil[0] != EOS)
	{
		memset (filename, EOS, filenamesize-1);
		strncpy (filename, Savfil, filenamesize);	/* or old name */
		ret = OK;
	}
	else
	{
		if (lin[i + 1] == '\n')
		{
			Errcode = ENOFN;
		}
		else
		{
			Errcode = EFILEN;
		}
	}

	if (ret == OK && Savfil[1] == EOS)
	{
		memset (Savfil, EOS, MAXLINE);
		strncpy (Savfil, filename, MAXLINE-1);		/* save if no old one */
		mesg (Savfil, FILE_MSG);
	}

	return (ret);
}


/* getkn --- get mark name from lin[i], increment i */

int getkn (char lin[], int *i, char *kname, char dfltnm)
{

	if (lin[*i] == '\n' || lin[*i] == EOS)
	{
		*kname = dfltnm;
		return (EOF);
	}

	*kname = lin[*i];
	(*i)++;
	return (OK);
}


/* getrange --- get 'from' range for tlit command */

int getrange (char array[], int *k, char set[], int size, int *allbut)
{
	int i, j;

	Errcode = EBADLIST;	/* preset error code */

	i = *k + 1;
	if (array[i] == NOTINCCL)	/* check for negated character class */
	{
		*allbut = YES;
		i++;
	}
	else
		*allbut = NO;

	j = 0;
	filset (array[*k], array, &i, set, &j, size);
	if (array[i] != array[*k])
	{
		set[0] = EOS;
		return (ERR);
	}
	if (set[0] == EOS)
	{
		Errcode = ENOLIST;
		return (ERR);
	}
	if (j > 0 && addset (EOS, set, &j, size) == NO)
	{
		set[0] = EOS;
		return (ERR);
	}

	*k = i;
	Errcode = EEGARB;

	return (OK);
}


/* getrhs --- get substitution string for 's' command */

int getrhs (char lin[], int *i, char sub[], size_t subsize, int *gflag)
{
	static char Subs[MAXPAT] = "";	/* saved replacement pattern */
	int j;
	/* saved replacement pattern char */


	Errcode = EBADSUB;

	if (lin[*i] == EOS)	/* missing the middle delimeter */
		return (ERR);

	if (lin[*i + 1] == '%' && (lin[*i + 2] == lin[*i]
					|| lin[*i + 2] == '\n'))
	{
	/*
	 * s//%/ --- should mean do the same thing as I did last time, even
	 * s//&/ --- if I deleted something. So we comment out these lines.
	 *
		if (Subs[0] == EOS)
		{
			Errcode = ENOSUB;
			return (ERR);
		}
	 */
		memset (sub, EOS, subsize);
		strncpy (sub, Subs, subsize-1);
		*i += 2;
		if (lin[*i] == '\n')
		{
			/* fix it up for pattern matching routines */
			lin[*i] = lin[*i - 2];
			lin[*i + 1] = '\n';
			lin[*i + 2] = EOS;
			Peekc = SKIP_RIGHT;
		}
	}
	else		/* not using saved substitution pattern */
	{
		if (lin[*i + 1] == '\n')
		{
			/* missing the trailing delimiter */
			/* pattern was empty */
			lin[*i + 1] = lin[*i];	/* supply missing delimiter */
			lin[*i + 2] = '\n';
			lin[*i + 3] = EOS;
			Peekc = SKIP_RIGHT;
			/* return (ERR);     this is the original action */
		}
		else
		{
			/* stuff in pattern, check end of line */
			for (j = *i; lin[j] != EOS; j++)
				;
			j -= 2;		/* j now points to char before '\n' */

			if (lin[j] == 'p' || lin[j] == 'P')
			{
				--j;
				if (lin[j] == GLOBAL || lin[j] == UCGLOBAL)
				{
					if (j >= *i + 1 && lin[j-1] == lin[*i]
						&& (lin[j-2] != ESCAPE
						    || lin[j-3] == ESCAPE))
						; 	/* leave alone */
					else
					{
						/* \<delim>gp\n is pattern */
						/* supply trailing delim */
						j +=  2;	/* j at \n */
						lin[j] = lin[*i];
						lin[++j] = '\n';
						lin[++j] = EOS;
						Peekc = SKIP_RIGHT;
					}
				}
				else if (j >= *i + 1 && lin[j] == lin[*i] &&
						(lin[j-1] != ESCAPE
						 || lin[j-2] == ESCAPE))
					;	/* leave alone */
				else
				{
					/* \<delim>p\n is pattern */
					/* supply trailing delim */
					j += 2;
					lin[j] = lin[*i];
					lin[++j] = '\n';
					lin[++j] = EOS;
					Peekc = SKIP_RIGHT;
				}
			}
			else if (lin[j] == GLOBAL || lin[j] == UCGLOBAL)
			{
				--j;
				if (j >= *i + 1 && lin[j] == lin[*i] &&
					(lin[j-1] != ESCAPE
					 || lin[j-2] == ESCAPE))
					; 	/* leave alone */
				else
				{
					/* \<delim>g\n is pattern */
					/* supply trailing delim */
					j +=  2;	/* j at \n */
					lin[j] = lin[*i];
					lin[++j] = '\n';
					lin[++j] = EOS;
					Peekc = SKIP_RIGHT;
				}
			}
			else if ((lin[j] != lin[*i]) ||
				(lin[j] == lin[*i] &&
				lin[j-1] == ESCAPE && lin[j-2] != ESCAPE))
			{
				/* simply missing trailing delimeter */
				/* supply it */
				j++;		/* j at \n */
				lin[j] = lin[*i];
				lin[++j] = '\n';
				lin[++j] = EOS;
				Peekc = SKIP_RIGHT;
			}
			/* else
				unescaped delim is there,
				leave well enough alone */
		}

		if ((*i = maksub (lin, *i + 1, lin[*i], sub)) == ERR)
		{
			return (ERR);
		}

		memset (Subs, EOS, MAXPAT);
		strncpy (Subs, sub, MAXPAT-1);	/* save pattern for later */
	}

	if (lin[*i + 1] == GLOBAL || lin[*i + 1] == UCGLOBAL)
	{
		(*i)++;
		*gflag = YES;
	}
	else
	{
		*gflag = NO;
	}

	Errcode = EEGARB;	/* the default */

	return (OK);

}


/* getstr --- get string from lin at i, copy to dst, bump i */

/*
** NOTE: this routine only called for doing the join command.
** therefore, don't do anything else with it.
*/

int getstr (char lin[], int *i, char dst[], int maxdst)
{
	char delim;
	int j, k, d;

	j = *i;
	Errcode = EBADSTR;

	delim = lin[j];

	if (delim == '\n')
	{
		lin[j] = '/';
		lin[++j] = ' ';		/* join with a single blank */
		lin[++j] = '/';
		lin[++j] = '\n';
		lin[++j] = EOS;
		j = *i;
		delim = lin[j];
		Peekc = SKIP_RIGHT;
		/* now fall thru */

		/* return (ERR);	 old way */
	}
	else if ((delim == 'p' || delim == 'P') && lin[j + 1] == '\n')	/* jp */
	{
		lin[j] = '/';
		lin[++j] = ' ';		/* join with a single blank */
		lin[++j] = '/';
		lin[++j] = delim;	/* 'p' or 'P' */
		lin[++j] = '\n';
		lin[++j] = EOS;
		j = *i;
		delim = lin[j];
		Peekc = SKIP_RIGHT;
		/* now fall thru */
	}

	if (lin[j + 1] == '\n')		/* command was 'j/' */
	{
		dst[0] = EOS;
		Errcode = ENOERR;
		return (OK);
		/* return (ERR);	old way */
	}

	/*
	 * otherwise, stuff there in the string, try to allow for
	 * a missing final delimiter.
	 */

	for (k = j + 1; lin[k] != '\n'; k++)
		;	/* find end */

	k--;	/* now points to char before newline */

	if (lin[k] == 'p' || lin[k] == 'P')
	{
		k--;
		if (lin[k] == delim &&
			(lin[k-1] != ESCAPE || lin[k-2] == ESCAPE))
			;	/* it's fine, leave it alone */
		else
		{
			/* ESCAPE delim p NEWLINE is the join string */
			/* supply trailing delimiter. */
			k += 2;
			lin[k] = delim;
			lin[++k] = '\n';
			lin[++k] = EOS;
			Peekc = SKIP_RIGHT;
		}
	}
	else if (lin[k] != delim || (lin[k-1] == ESCAPE && lin[k-2] != ESCAPE))
	{
		/* no delim and no p, or last char is escaped delim */
		k++;
		lin[k] = delim;
		lin[++k] = '\n';
		lin[++k] = EOS;
		Peekc = SKIP_RIGHT;
	}
	/* else
		delim is there
		leave well enough alone */

	/* code to actually do the join */

	for (k = j + 1; lin[k] != delim; k++)	/* find end */
	{
		if (lin[k] == '\n' || lin[k] == EOS)
		{
			if (delim == ' ')
			{
				break;
			}
			else
			{
				return (ERR);
			}
		}
		esc (lin, &k);
	}
	if (k - j > maxdst)
	{
		return (ERR);
	}

	for (d = 0, j++; j < k; d++, j++)
	{
		dst[d] = esc (lin, &j);
	}
	dst[d] = EOS;

	*i = j;
	Errcode = EEGARB;	/* the default */

	return (OK);
}


/* getwrd --- get next word from line at i; increment i */

int getwrd (char line[], int *i, char word[], int size)
{
	int j;

	SKIPBL (line, *i);
	j = 0;
	while (line[*i] != ' ' && line[*i] != '\n' && line[*i] != EOS)
	{
		if (j < size - 1)
		{
			word[j] = line[*i];
			j++;
		}
		(*i)++;
	}
	word[j] = EOS;

	return (j);
}


/* knscan --- scan for a line with a given mark name */

int knscan (int way, int *num)
{
	LINEDESC *k;

	*num = Curln;
	k = getind (*num);
	do {
		bump (num, &k, way);
		if (k -> Markname == Savknm)
			return (OK);
	} while (*num != Curln && ! intrpt ());

	if (Errcode == EEGARB)
		Errcode = EKNOTFND;
	return (ERR);

}


/* makset --- make set from array[k] in set */

int makset (char array[], int *k, char set[], size_t size)
{
	static char Tset[MAXPAT] = "";	/* saved translit dest range */
	int i, j;
	int l;

	Errcode = EBADLIST;

	/*
	 * try to allow missing delimiter for translit command.
	 */

	if (array[*k] == EOS)
		return (ERR);

	if (array[*k + 1] == '%' && (array[*k + 2] == array[*k]
					   || array[*k + 2] == '\n'))
	{
		memset (set, EOS, size);
		strncpy (set, Tset, size-1);
		*k += 2;
		if (array[*k] == '\n')
		{
			/* fix it up for rest of the routines */
			array[*k] = array[*k - 2];
			array[*k+ 1] = '\n';
			array[*k+ 2] = EOS;
		}
		Peekc = SKIP_RIGHT;
	}
	else
	{

		for (l = *k; array[l] != EOS; l++)
			;
		l -= 2;		/* l now points to char before '\n' */

		if (l == *k)	/* "y/.../\n" */
		{
			array[*k + 1] = array[*k];	/* add delimiter */
			array[*k + 2] = '\n';
			array[*k + 3] = EOS;
			Peekc = SKIP_RIGHT;
		}
		else if (array[l] == 'p' || array[l] == 'P')
		{
			--l;
			if (l >= *k + 1 && array[l] == array[*k] &&
				(array[l-1] != ESCAPE || array[l-2] == ESCAPE))
				;	/* leave alone */
			else
			{
				/* \<delim>p\n is set */
				/* supply trailing delim */
				l += 2;
				array[l] = array[*k];
				array[++l] = '\n';
				array[++l] = EOS;
				Peekc = SKIP_RIGHT;
			}
		}
		else if (array[l] != array[*k]	/* no delim, and no p */
		    || (array[l-1] == ESCAPE	/* or last char is escaped delim */
			&& array[l-2] != ESCAPE))
		{
			/* simply missing trailing delimeter */
			/* supply it */
			l++;		/* l now at \n */
			array[l] = array[*k];
			array[++l] = '\n';
			array[++l] = EOS;
			Peekc = SKIP_RIGHT;
		}
		/* else
			delim is there,
			leave well enough alone */

		j = 0;
		i = *k + 1;
		filset (array[*k], array, &i, set, &j, size);

		if (array[i] != array[*k])
			return (ERR);

		if (addset (EOS, set, &j, size) == NO)
			return (ERR);

		memset (Tset, EOS, MAXPAT);
		strncpy (Tset, set, MAXPAT-1);	/* save for later */
		*k = i;

	}

	Errcode = EEGARB;

	return (OK);
}


/* optpat --- make pattern specified at lin[i] */

int optpat (char lin[], int *i)
{
	if (lin[*i] == EOS)
		*i = ERR;
	else if (lin[*i + 1] == EOS)
		*i = ERR;
	else if (lin[*i + 1] == lin[*i])	/* repeated delimiter */
		(*i)++;		/* leave existing pattern alone */
	else
		*i = makpat (lin, *i + 1, lin[*i], Pat);

	if (Pat [0] == EOS)
	{
		Errcode = ENOPAT;
		return (ERR);
	}
	if (*i == ERR)
	{
		Pat[0] = EOS;
		Errcode = EBADPAT;
		return (ERR);
	}
	return (OK);
}


/* ptscan --- scan for next occurrence of pattern */

int ptscan (int way, int *num)
{
	LINEDESC *k;

	*num = Curln;
	k = getind (*num);
	do {
		bump (num, &k, way);
		gtxt (k);
		if (match (Txt, Pat) == YES)
			return (OK);
	} while (*num != Curln && ! intrpt ());

	if (Errcode == EEGARB)
		Errcode = EPNOTFND;

	return (ERR);
}


/* settab --- set tab stops */

int settab (char str[])
{
	int i, j, n, maxstop, last, inc, ret;
	int ctoi ();

	for (i = 0; i < MAXLINE; i++)   /* clear all tab stops */
		Tabstops[i] = NO;

	ret = OK;
	maxstop = 0;
	last = 1;

	i = 0;
	SKIPBL (str, i);
	while (str[i] != EOS && str[i] != '\n')
	{
		if (str[i] == '+')      /* increment */
		{
			i++;
			inc = YES;
		}
		else
			inc = NO;

		n = ctoi (str, &i);

		if (n <= 0 || n >= MAXLINE)
		{
			ret = ERR;
			Errcode = ENONSENSE;
			break;
		}

		if (str[i] != ' ' && str[i] != '+' &&
		    str[i] != '\n' && str[i] != EOS)
		{
			ret = ERR;
			Errcode = EBADTABS;
			break;
		}

		if (inc == YES)
		{
			for (j = last + n; j < MAXLINE; j += n)
			{
				Tabstops[j - 1] = YES;
				maxstop = max (j, maxstop);
			}
		}
		else
		{
			Tabstops[n - 1] = YES;
			last = n;
			maxstop = max (n, maxstop);
		}
		SKIPBL (str, i);
	}       /* while ... */

	if (ret == ERR)
		maxstop = 0;

	if (maxstop == 0)       /* no tab stops specified, use defaults */
	{
		for (i = 4; i < MAXLINE - 1; i += 4)
			Tabstops[i] = YES;
		maxstop = i - 4 + 1;
	}

	Tabstops[0] = YES;      /* always set to YES */

	for (i = maxstop; i < MAXLINE; i++)
		Tabstops[i] = YES;

	return (ret);
}


/* serc_safe --- check if the file permissions and ownership are safe */

/*
 * err on the side of caution and only exec ~/.serc and ./serc files
 * that we own and cannot be written by others.
 */

int serc_safe (char *path)
{
	int rc;
	uid_t our_euid;
	struct stat sbuf;

	rc = stat (path, &sbuf);
	if (rc != 0)
	{
		return NO;
	}

	our_euid = geteuid ();

	/* don't exec .serc files that aren't ours */
	if (sbuf.st_uid != our_euid)
	{
		return NO;
	}

	/* don't .serc files that others can write to */
	if ((sbuf.st_mode & S_IWGRP) || (sbuf.st_mode & S_IWOTH))
	{
		return NO;
	}

	return YES;
}


/* serc --- read in ./.serc or $HOME/.serc and execute the commands in it. */

/*
 * note that se's special control characters are NOT processed,
 * and therefore should NOT be used in one's .serc file.
 */

void serc (void)
{
	char file[MAXLINE];
	char lin[MAXLINE];
	char *homeserc;
	FILE *fp;
	int status = ENOERR;
	int len, cursav, i;
	char *serc_files[3];

	serc_files[0] = "./.serc";
	serc_files[1] = file;
	serc_files[2] = NULL;

	homeserc = expand_env ("$HOME/.serc");

	memset (file, EOS, MAXLINE);
	strncpy (file, homeserc, MAXLINE-1);

	fp = NULL;

	for (i = 0; serc_files[i]; i++)
	{
		if ((serc_safe (serc_files[i]) == YES) &&
			 ((fp = fopen (serc_files[i], "r")) != NULL))
		{
			break;
		}
	}

	if (fp == NULL)
	{
		return;
	}

	while (fgets (lin, MAXLINE, fp) != NULL && status != EOF /*??*/)
	{
		if (lin[0] == '#' || lin[0] == '\n')
		{
			continue;	/* comment in .serc file */
		}

		/* most of this code stolen from edit() */
		len = 0;
		cursav = Curln;
		if (getlst (lin, &len, &status) == OK)
		{
			if (ckglob (lin, &len, &status) == OK)
			{
				doglob (lin, &len, &cursav, &status);
			}
			else if (status != ERR)
			{
				docmd (lin, len, NO, &status);
			}
		}

		if (status == ERR)
		{
			if (Errcode == EHANGUP)
			{
				hangup ();
			}
			Curln = min (cursav, Lastln);
		}
	}
	fclose (fp);
}

#ifdef LOG_USAGE

/* log -- log se usage */


void log_usage (void)
{
	char logname[MAXLINE], tod[26];		/* tod => time of day */
	long clock;

	/* get the login name */
	memset (logname, EOS, MAXLINE);
	strncpy (logname, getlogin (), MAXLINE-1);

	time (&clock);

	memset (tod, EOS, 26);
	strncpy (tod, ctime (&clock), 26-1);	/* see the manual on ctime(3C)  */
	tod[24] = EOS;				/* delete the '\n' at the end */

	openlog (PACKAGE, 0, LOG_USER);
	syslog (LOG_INFO, "%s was invoked by %s on %s.", PACKAGE, logname, tod);
	closelog ();

}
#endif

/* sysname --- return a string telling us who we are */

char *sysname (void)
{
	static struct utsname whoarewe;
	uname (& whoarewe);
	return (whoarewe.nodename);
}
