/*
** edit.c
**
** editor main routine, plus other routines used a lot.
**
** This file is in the public domain.
*/

#include "config.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>	/* stuff to find out who we are */
#endif /* HAVE_SYS_UTSNAME_H */

#ifdef HAVE_WINSOCK2_H
#include <Winsock2.h>
#endif /* HAVE_WINSOCK2_H */

#ifdef LOG_USAGE
#include <syslog.h>
#endif /* LOG_USAGE */

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

	status = SE_OK;

	while (status == SE_OK && Argno < argc)
	{
		memset (lin, SE_EOS, MAXLINE);
		strncpy (lin, argv[Argno], MAXLINE-1);
		loadstr (lin, Argno, POOPCOL, Ncols);
		if (lin[0] == '-')
		{
			len = strlen (lin) + 1;
			lin[len - 1] = '\n';
			lin[len] = SE_EOS;
			len = 0;
			status = doopt (lin, &len);
		}
		else
		{
			dfltsopt (lin);
			status = doread (Lastln, lin, SE_NO);
		}
		Argno++;
	}

	if (status == SE_ERR)
	{
		if (Errcode == EHANGUP)
			hangup ();
		print_verbose_err_msg ();
	}
	else
		Curln = min (1, Lastln);

	Buffer_changed = SE_NO;
	First_affected = 1;     /* maintained by updscreen & commands */
	updscreen ();

	if (status != SE_ERR)	/* leave offending file name or option */
	{
		lin[0] = SE_EOS;
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
		if (getlst (lin, &len, &status) == SE_OK)
		{
			if (ckglob (lin, &len, &status) == SE_OK)
				doglob (lin, &len, &cursav, &status);
			else if (status != SE_ERR)
				docmd (lin, len, SE_NO, &status);
		}
		if (status == SE_ERR)
		{
			if (Errcode == EHANGUP)
				hangup ();
			print_verbose_err_msg ();
			Curln = min (cursav, Lastln);
		}
		else if (term != FUNNY)
		{
			cursor = 0;
			lin[0] = SE_EOS;
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
	for (Nlines = 0; getone (lin, i, &num, status) == SE_OK; )
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
		*status = SE_ERR;
		Errcode = EBACKWARD;
	}

	if (*status != SE_ERR)
		*status = SE_OK;

	return (*status);
}


/* getnum --- convert one term to line number */

int getnum (char lin[], int *i, int *pnum, int *status)
{
	int j, ret;
	int k;

	ret = SE_OK;
	SKIPBL (lin, *i);
	if (lin[*i] >= Rel_a && lin[*i] <= Rel_z && Absnos == SE_NO)
		*pnum = Topln - Toprow + lin[*i] - Rel_a;
	else if (lin[*i] == CURLINE)
		*pnum = Curln;
	else if (lin[*i] == PREVLN || lin[*i] == PREVLN2)
		*pnum = Curln - 1;
	else if (lin[*i] == LASTLINE)
		*pnum = Lastln;
	else if (lin[*i] == SCAN || lin[*i] == BACKSCAN)
	{
		int missing_delim = SE_YES;

		/* see if trailing delim supplied, since command can follow pattern */
		for (k = *i + 1; lin[k] != SE_EOS; k++)
			if (lin[k] == ESCAPE)
				k++;	/* skip esc, loop will skip escaped char */
			else if (lin[k] == lin[*i])
			{
				missing_delim = SE_NO;
				break;
			}
			/* else
				continue */

		if (missing_delim == SE_YES)
		{
			for (; lin[k] != SE_EOS; k++)
				;
			k--;		/* k now at newline */

			/* supply trailing delim */
			lin[k] = lin[*i];
			lin[++k] = '\n';
			lin[++k] = SE_EOS;
			Peekc = SKIP_RIGHT;
		}

		if (optpat (lin, i) == SE_ERR)
			ret = SE_ERR;
		else if (lin[*i] == SCAN)
			ret = ptscan (FORWARD, pnum);
		else 
			ret = ptscan (BACKWARD, pnum);
	}
	else if (lin[*i] == SEARCH || lin[*i] == BACKSEARCH)
	{
		j = *i;
		(*i)++;
		if (getkn (lin, i, &Savknm, Savknm) == SE_ERR)
			ret = SE_ERR;
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

	if (ret == SE_OK)
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

	if (getnum (lin, i, num, status) == SE_OK)		/* first term */
	{
		ret = SE_OK;	/* to indicate we've seen something */
		do {			/* + or - terms */
			porm = SE_EOS;
			SKIPBL (lin, *i);
			if (lin[*i] == '-' || lin[*i] == '+')
			{
				porm = lin[*i];
				(*i)++;
			}
			if (getnum (lin, i, &pnum, status) == SE_OK)
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
			if (*status == EOF && porm != SE_EOS)	/* trailing + or - */
			{
				*status = SE_ERR;
			}
		} while (*status == SE_OK);
	}

	if (*num < 0 || *num > Lastln)	/* make sure number is in range */
	{
		*status = SE_ERR;
		Errcode = EORANGE;
	}

	if (*status == SE_ERR)
		ret = SE_ERR;
	else
		*status = ret;

	return (ret);
}


static int special_casing = SE_NO;

/* ckglob --- if global prefix, mark lines to be affected */

int ckglob (char lin[], int *i, int *status)
{
	int line, tmp;
	int usepat, usemark;
	LINEDESC *k;

	*status = SE_OK;
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
		special_casing = SE_YES;
		remark ("GLOB");
		return (SE_OK);
	}

	if (lin[*i] == GMARK || lin[*i] == XMARK)	/* global markname prefix? */
	{
		if (lin[*i] == GMARK)	/* tag lines with the specified markname */
			usemark = SE_YES;
		else			/* tag lines without the specified markname */
			usemark = SE_NO;
		(*i)++;
		*status = getkn (lin, i, &Savknm, Savknm);
	}

	if (*status == SE_OK)	/* check for a pattern prefix too */
	{
		if (lin[*i] == GLOBAL || lin[*i] == UCGLOBAL)
			usepat = SE_YES;

		if (lin[*i] == EXCLUDE || lin[*i] == UCEXCLUDE)
			usepat = SE_NO;

		if (usepat != EOF)
		{
			(*i)++;
			if (optpat (lin, i) == SE_ERR)
				*status = SE_ERR;
			else
				(*i)++;
		}
	}

	if (*status == SE_OK && usepat == EOF && usemark == EOF)
		*status = EOF;
	else if (*status == SE_OK)
		defalt (1, Lastln);

	if (*status == SE_OK)	/* no errors so far, safe to proceed */
	{
		remark ("GLOB");

		k = Line0;      /* unmark all lines preceeding range */
		for (line = 0; line < Line1; line++)
		{
			k -> Globmark = SE_NO;
			k = NEXTLINE(k);
		}

		for (; line <= Line2; line++)	/* mark lines in range */
		{
			if (intrpt ())
			{
				*status = SE_ERR;
				return (*status);
			}
			tmp = SE_NO;
			if (((usemark == EOF
			    || usemark == SE_YES) && k -> Markname == Savknm)
			    || (usemark == SE_NO && k -> Markname != Savknm))
			{
				if (usepat == EOF)	/* no global pattern to look for */
					tmp = SE_YES;
				else	/* there is also a pattern to look for */
				{
					gtxt (k);
					if (match (Txt, Pat) == usepat)
						tmp = SE_YES;
				}
			}

			k -> Globmark = tmp;

			k = NEXTLINE(k);
		}

		/* mark remaining lines */
		for (; line <= Lastln; line++)
		{
			k -> Globmark = SE_NO;
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
		special_casing = SE_NO;
		Buffer_changed = SE_YES;
		First_affected = min (1, First_affected);
		remark ("");
		adjust_window (Curln, Curln);
		updscreen ();
		return (SE_OK);
	}

	*status = SE_OK;
	istart = *i;
	k = Line0;
	line = 0;

	do {
		line++;
		k = NEXTLINE(k);
		if (k -> Globmark == SE_YES)	/* line is marked */
		{
			k -> Globmark = SE_NO;	/* unmark the line */
			Curln = line;
			*cursav = Curln;	/* remember where we are */
			*i = istart;
			if (getlst (lin, i, status) == SE_OK)
				docmd (lin, *i, SE_YES, status);
			line = 0;		/* lines may have been moved */
			k = Line0;
		}
		if (intrpt ())
			*status = SE_ERR;
	} while (line <= Lastln && *status == SE_OK);

	return (*status);
}


/* ckchar --- look for ch or altch on lin at i, set flag if found */

int ckchar (char ch, char altch, char lin[], int *i, int *flag, int *status)
{

	if (lin[*i] == ch || lin[*i] == altch)
	{
		(*i)++;
		*flag = SE_YES;
	}
	else
		*flag = SE_NO;

	*status = SE_OK;
	return (SE_OK);
}


/* ckp --- check for "p" after command */

int ckp (char lin[], int i, int *pflag, int *status)
{

	if (lin[i] == PRINT || lin[i] == UCPRINT)
	{
		i++;
		*pflag = SE_YES;
	}
	else
		*pflag = SE_NO;

	if (lin[i] == '\n')
		*status = SE_OK;
	else
		*status = SE_ERR;

	return (*status);
}


/* ckupd --- make sure it is ok to destroy the buffer */

int ckupd (char lin[], int *i, char cmd, int *status)
{
	int flag;

	*status = ckchar (ANYWAY, ANYWAY, lin, i, &flag, status);
	if (flag == SE_NO && Buffer_changed == SE_YES && Probation != cmd)
	{
		*status = SE_ERR;
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

	ret = SE_ERR;
	if (lin[i + 1] == ' ')
	{
		j = i + 2;      /* get new file name */
		SKIPBL (lin, j);
		for (k = 0; lin[j] != NEWLINE; k++, j++)
		{
			filename[k] = lin[j];
		}
		filename[k] = SE_EOS;
		if (k > 0)
		{
			ret = SE_OK;
		}
	}
	else if (lin[i + 1] == '\n' && Savfil[0] != SE_EOS)
	{
		memset (filename, SE_EOS, filenamesize-1);
		strncpy (filename, Savfil, filenamesize);	/* or old name */
		ret = SE_OK;
	}
	else
	{
		if (lin[i + 1] == '\n')
		{
			Errcode = ESE_NOFN;
		}
		else
		{
			Errcode = EFILEN;
		}
	}

	if (ret == SE_OK && Savfil[1] == SE_EOS)
	{
		memset (Savfil, SE_EOS, MAXLINE);
		strncpy (Savfil, filename, MAXLINE-1);		/* save if no old one */
		mesg (Savfil, FILE_MSG);
	}

	return (ret);
}


/* getkn --- get mark name from lin[i], increment i */

int getkn (char lin[], int *i, char *kname, char dfltnm)
{

	if (lin[*i] == '\n' || lin[*i] == SE_EOS)
	{
		*kname = dfltnm;
		return (EOF);
	}

	*kname = lin[*i];
	(*i)++;
	return (SE_OK);
}


/* getrange --- get 'from' range for tlit command */

int getrange (char array[], int *k, char set[], int size, int *allbut)
{
	int i, j;

	Errcode = EBADLIST;	/* preset error code */

	i = *k + 1;
	if (array[i] == SE_NOTINCCL)	/* check for negated character class */
	{
		*allbut = SE_YES;
		i++;
	}
	else
		*allbut = SE_NO;

	j = 0;
	filset (array[*k], array, &i, set, &j, size);
	if (array[i] != array[*k])
	{
		set[0] = SE_EOS;
		return (SE_ERR);
	}
	if (set[0] == SE_EOS)
	{
		Errcode = ESE_NOLIST;
		return (SE_ERR);
	}
	if (j > 0 && addset (SE_EOS, set, &j, size) == SE_NO)
	{
		set[0] = SE_EOS;
		return (SE_ERR);
	}

	*k = i;
	Errcode = EEGARB;

	return (SE_OK);
}


/* getrhs --- get substitution string for 's' command */

int getrhs (char lin[], int *i, char sub[], size_t subsize, int *gflag)
{
	static char Subs[MAXPAT] = "";	/* saved replacement pattern */
	int j;
	/* saved replacement pattern char */


	Errcode = EBADSUB;

	if (lin[*i] == SE_EOS)	/* missing the middle delimeter */
		return (SE_ERR);

	if (lin[*i + 1] == '%' && (lin[*i + 2] == lin[*i]
					|| lin[*i + 2] == '\n'))
	{
	/*
	 * s//%/ --- should mean do the same thing as I did last time, even
	 * s//&/ --- if I deleted something. So we comment out these lines.
	 *
		if (Subs[0] == SE_EOS)
		{
			Errcode = ESE_NOSUB;
			return (SE_ERR);
		}
	 */
		memset (sub, SE_EOS, subsize);
		strncpy (sub, Subs, subsize-1);
		*i += 2;
		if (lin[*i] == '\n')
		{
			/* fix it up for pattern matching routines */
			lin[*i] = lin[*i - 2];
			lin[*i + 1] = '\n';
			lin[*i + 2] = SE_EOS;
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
			lin[*i + 3] = SE_EOS;
			Peekc = SKIP_RIGHT;
			/* return (SE_ERR);     this is the original action */
		}
		else
		{
			/* stuff in pattern, check end of line */
			for (j = *i; lin[j] != SE_EOS; j++)
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
						lin[++j] = SE_EOS;
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
					lin[++j] = SE_EOS;
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
					lin[++j] = SE_EOS;
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
				lin[++j] = SE_EOS;
				Peekc = SKIP_RIGHT;
			}
			/* else
				unescaped delim is there,
				leave well enough alone */
		}

		if ((*i = maksub (lin, *i + 1, lin[*i], sub)) == SE_ERR)
		{
			return (SE_ERR);
		}

		memset (Subs, SE_EOS, MAXPAT);
		strncpy (Subs, sub, MAXPAT-1);	/* save pattern for later */
	}

	if (lin[*i + 1] == GLOBAL || lin[*i + 1] == UCGLOBAL)
	{
		(*i)++;
		*gflag = SE_YES;
	}
	else
	{
		*gflag = SE_NO;
	}

	Errcode = EEGARB;	/* the default */

	return (SE_OK);

}


/* getstr --- get string from lin at i, copy to dst, bump i */

/*
** SE_NOTE: this routine only called for doing the join command.
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
		lin[++j] = SE_EOS;
		j = *i;
		delim = lin[j];
		Peekc = SKIP_RIGHT;
		/* now fall thru */

		/* return (SE_ERR);	 old way */
	}
	else if ((delim == 'p' || delim == 'P') && lin[j + 1] == '\n')	/* jp */
	{
		lin[j] = '/';
		lin[++j] = ' ';		/* join with a single blank */
		lin[++j] = '/';
		lin[++j] = delim;	/* 'p' or 'P' */
		lin[++j] = '\n';
		lin[++j] = SE_EOS;
		j = *i;
		delim = lin[j];
		Peekc = SKIP_RIGHT;
		/* now fall thru */
	}

	if (lin[j + 1] == '\n')		/* command was 'j/' */
	{
		dst[0] = SE_EOS;
		Errcode = ESE_NOSE_ERR;
		return (SE_OK);
		/* return (SE_ERR);	old way */
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
			lin[++k] = SE_EOS;
			Peekc = SKIP_RIGHT;
		}
	}
	else if (lin[k] != delim || (lin[k-1] == ESCAPE && lin[k-2] != ESCAPE))
	{
		/* no delim and no p, or last char is escaped delim */
		k++;
		lin[k] = delim;
		lin[++k] = '\n';
		lin[++k] = SE_EOS;
		Peekc = SKIP_RIGHT;
	}
	/* else
		delim is there
		leave well enough alone */

	/* code to actually do the join */

	for (k = j + 1; lin[k] != delim; k++)	/* find end */
	{
		if (lin[k] == '\n' || lin[k] == SE_EOS)
		{
			if (delim == ' ')
			{
				break;
			}
			else
			{
				return (SE_ERR);
			}
		}
		esc (lin, &k);
	}
	if (k - j > maxdst)
	{
		return (SE_ERR);
	}

	for (d = 0, j++; j < k; d++, j++)
	{
		dst[d] = esc (lin, &j);
	}
	dst[d] = SE_EOS;

	*i = j;
	Errcode = EEGARB;	/* the default */

	return (SE_OK);
}


/* getwrd --- get next word from line at i; increment i */

int getwrd (char line[], int *i, char word[], int size)
{
	int j;

	SKIPBL (line, *i);
	j = 0;
	while (line[*i] != ' ' && line[*i] != '\n' && line[*i] != SE_EOS)
	{
		if (j < size - 1)
		{
			word[j] = line[*i];
			j++;
		}
		(*i)++;
	}
	word[j] = SE_EOS;

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
			return (SE_OK);
	} while (*num != Curln && ! intrpt ());

	if (Errcode == EEGARB)
		Errcode = EKSE_NOTFND;
	return (SE_ERR);

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

	if (array[*k] == SE_EOS)
		return (SE_ERR);

	if (array[*k + 1] == '%' && (array[*k + 2] == array[*k]
					   || array[*k + 2] == '\n'))
	{
		memset (set, SE_EOS, size);
		strncpy (set, Tset, size-1);
		*k += 2;
		if (array[*k] == '\n')
		{
			/* fix it up for rest of the routines */
			array[*k] = array[*k - 2];
			array[*k+ 1] = '\n';
			array[*k+ 2] = SE_EOS;
		}
		Peekc = SKIP_RIGHT;
	}
	else
	{

		for (l = *k; array[l] != SE_EOS; l++)
			;
		l -= 2;		/* l now points to char before '\n' */

		if (l == *k)	/* "y/.../\n" */
		{
			array[*k + 1] = array[*k];	/* add delimiter */
			array[*k + 2] = '\n';
			array[*k + 3] = SE_EOS;
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
				array[++l] = SE_EOS;
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
			array[++l] = SE_EOS;
			Peekc = SKIP_RIGHT;
		}
		/* else
			delim is there,
			leave well enough alone */

		j = 0;
		i = *k + 1;
		filset (array[*k], array, &i, set, &j, size);

		if (array[i] != array[*k])
			return (SE_ERR);

		if (addset (SE_EOS, set, &j, size) == SE_NO)
			return (SE_ERR);

		memset (Tset, SE_EOS, MAXPAT);
		strncpy (Tset, set, MAXPAT-1);	/* save for later */
		*k = i;

	}

	Errcode = EEGARB;

	return (SE_OK);
}


/* optpat --- make pattern specified at lin[i] */

int optpat (char lin[], int *i)
{
	if (lin[*i] == SE_EOS)
		*i = SE_ERR;
	else if (lin[*i + 1] == SE_EOS)
		*i = SE_ERR;
	else if (lin[*i + 1] == lin[*i])	/* repeated delimiter */
		(*i)++;		/* leave existing pattern alone */
	else
		*i = makpat (lin, *i + 1, lin[*i], Pat);

	if (Pat [0] == SE_EOS)
	{
		Errcode = ESE_NOPAT;
		return (SE_ERR);
	}
	if (*i == SE_ERR)
	{
		Pat[0] = SE_EOS;
		Errcode = EBADPAT;
		return (SE_ERR);
	}
	return (SE_OK);
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
		if (match (Txt, Pat) == SE_YES)
			return (SE_OK);
	} while (*num != Curln && ! intrpt ());

	if (Errcode == EEGARB)
		Errcode = EPSE_NOTFND;

	return (SE_ERR);
}


/* settab --- set tab stops */

int settab (char str[])
{
	int i, j, n, maxstop, last, inc, ret;
	int ctoi ();

	for (i = 0; i < MAXLINE; i++)   /* clear all tab stops */
		Tabstops[i] = SE_NO;

	ret = SE_OK;
	maxstop = 0;
	last = 1;

	i = 0;
	SKIPBL (str, i);
	while (str[i] != SE_EOS && str[i] != '\n')
	{
		if (str[i] == '+')      /* increment */
		{
			i++;
			inc = SE_YES;
		}
		else
			inc = SE_NO;

		n = ctoi (str, &i);

		if (n <= 0 || n >= MAXLINE)
		{
			ret = SE_ERR;
			Errcode = ESE_NONSENSE;
			break;
		}

		if (str[i] != ' ' && str[i] != '+' &&
		    str[i] != '\n' && str[i] != SE_EOS)
		{
			ret = SE_ERR;
			Errcode = EBADTABS;
			break;
		}

		if (inc == SE_YES)
		{
			for (j = last + n; j < MAXLINE; j += n)
			{
				Tabstops[j - 1] = SE_YES;
				maxstop = max (j, maxstop);
			}
		}
		else
		{
			Tabstops[n - 1] = SE_YES;
			last = n;
			maxstop = max (n, maxstop);
		}
		SKIPBL (str, i);
	}       /* while ... */

	if (ret == SE_ERR)
		maxstop = 0;

	if (maxstop == 0)       /* no tab stops specified, use defaults */
	{
		for (i = 4; i < MAXLINE - 1; i += 4)
			Tabstops[i] = SE_YES;
		maxstop = i - 4 + 1;
	}

	Tabstops[0] = SE_YES;      /* always set to SE_YES */

	for (i = maxstop; i < MAXLINE; i++)
		Tabstops[i] = SE_YES;

	return (ret);
}


/* serc_safe_open --- open and check if the file permissions and ownership are safe */

/*
 * err on the side of caution and only exec ~/.serc and ./serc files
 * that we own and cannot be written by others.
 */

FILE *serc_safe_open (char *path)
{
	FILE *fp;

#ifdef HAVE_SYS_STAT_H

	int rc;
	uid_t our_euid;
	struct stat sbuf;

#endif

	if ((fp = fopen (path, "r")) == NULL)
	{
		return NULL;
	}

#ifdef HAVE_SYS_STAT_H

	rc = fstat (fileno(fp), &sbuf);
	if (rc != 0)
	{
		fclose(fp);
		return NULL;
	}

	our_euid = geteuid ();

	/* don't exec .serc files that aren't ours */
	if (sbuf.st_uid != our_euid)
	{
		fclose(fp);
		return NULL;
	}

	/* don't .serc files that others can write to */
	if ((sbuf.st_mode & S_IWGRP) || (sbuf.st_mode & S_IWOTH))
	{
		fclose(fp);
		return NULL;
	}

#endif

	return fp;
}


/* serc --- read in ./.serc or $HOME/.serc and execute the commands in it. */

/*
 * note that se's special control characters are SE_NOT processed,
 * and therefore should SE_NOT be used in one's .serc file.
 */

void serc (void)
{
	char file[MAXLINE];
	char lin[MAXLINE];
	char *homeserc;
	FILE *fp;
	int status = ESE_NOSE_ERR;
	int len, cursav, i;
	char *serc_files[3];

	serc_files[0] = "./.serc";
	serc_files[1] = file;
	serc_files[2] = NULL;

	homeserc = expand_env ("$HOME/.serc");

	memset (file, SE_EOS, MAXLINE);
	strncpy (file, homeserc, MAXLINE-1);

	fp = NULL;

	for (i = 0; serc_files[i]; i++)
	{
		if ((fp = serc_safe_open (serc_files[i])) != NULL)
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
		if (getlst (lin, &len, &status) == SE_OK)
		{
			if (ckglob (lin, &len, &status) == SE_OK)
			{
				doglob (lin, &len, &cursav, &status);
			}
			else if (status != SE_ERR)
			{
				docmd (lin, len, SE_NO, &status);
			}
		}

		if (status == SE_ERR)
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
	memset (logname, SE_EOS, MAXLINE);
	strncpy (logname, getlogin (), MAXLINE-1);

	time (&clock);

	memset (tod, SE_EOS, 26);
	strncpy (tod, ctime (&clock), 26-1);	/* see the manual on ctime(3C)  */
	tod[24] = SE_EOS;				/* delete the '\n' at the end */

	openlog (PACKAGE, 0, LOG_USER);
	syslog (LOG_INFO, "%s was invoked by %s on %s.", PACKAGE, logname, tod);
	closelog ();

}
#endif

/* sysname --- return a string telling us who we are */

char *sysname (void)
{

#ifdef HAVE_UNAME

	static struct utsname whoarewe;
	uname (& whoarewe);
	return (whoarewe.nodename);

#else /* !HAVE_UNAME */

#ifdef HAVE_GETHOSTNAME
	WSADATA wsaData;
	static char hostname[MAXLINE];

	memset(hostname, SE_EOS, MAXLINE);

	WSAStartup(MAKEWORD(2, 2), &wsaData);
	gethostname(hostname, MAXLINE-1);
	WSACleanup();

	return hostname;
#else /* !HAVE_GETHOSTNAME */

	return "localhost";

#endif /* !HAVE_GETHOSTNAME */


#endif /* !HAVE_UNAME */
}
