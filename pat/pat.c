#ifndef lint
static char RCSid[] = "$Header: pat.c,v 1.2 86/07/11 15:24:44 osadr Exp $";
#endif

/*
 * $Log:	pat.c,v $
 * Revision 1.2  86/07/11  15:24:44  osadr
 * Removed Georgia Tech-ism of changeable pattern characters.
 * 
 * Revision 1.1  86/05/06  13:32:49  osadr
 * Initial revision
 * 
 * 
 */

/*
** pat.c
**
** pattern matching subroutines for the se screen editor.
**
** routines declared static are not necessary for the rest
** of the editor, therefore make them static in the name
** of modularity.
*/

#include <stdio.h>
#include <ctype.h>
#include "../constdefs.h"

/* Definitions used only for pattern matching */

#define AND             '&'
#define CCL             '['
#define CCLEND          ']'
#define CHAR            'a'
#define CLOSIZE         1
#define CLOSURE         '*'
#define DASH            '-'
#define DITTO           0200
#define EOL             '$'
#define NCCL            'n'
#define NEWLINE         '\n'
#define TAB             '\t'
#define ANY		'.'
#define BOL		'^'
#define NOTINCCL	'^'
#define START_TAG	'('
#define STOP_TAG	')'
#define ESCAPE		'\\'

/* Array dimensions and other limit values */
#define MAXLINE         128
#define MAXPAT          128

/* Pattern matching subroutines: */

/* match () --- find match anywhere on line */

match (lin, pat)
register char lin[];
register char pat[];
{
	int junk[9];
	register char *pc;

	for (pc = lin; *pc != EOS; pc++)
		if (amatch (lin, pc - lin, pat, junk, junk) >= 0)
			return (YES);
	return (NO);
}


/* amatch() --- (recursive) look for match starting at lin[from] */

amatch(lin, from, pat, tagbeg, tagend)
int from, tagbeg[], tagend[];
char lin[], pat[];
{
	char *ch, *lastc;
	register char *ppat;
	int k;

	lastc = lin + from;     /* next unexamined input character */
	for (ppat = pat; *ppat != EOS; ppat += patsiz (ppat))
		if (*ppat == CLOSURE)   /* a closure entry */
		{
			ppat++;
			for (ch = lastc; *ch != EOS; )
				/* match as many as possible */
				if (omatch (lin, &ch, ppat) == NO)
					break;
			/*
			 * ch now points to character that made us fail
			 * try to match rest of pattern against rest of input
			 * shrink the closure by 1 after each failure
			 */
			for (ppat += patsiz (ppat); ch >= lastc; ch--)
				/* successful match of rest of pattern */
				if ((k = amatch (lin, ch - lin, ppat, tagbeg,
				    tagend)) >= 0)
					break;
			lastc = lin + k;        /* if k < 0, failure */
						/* if k >= 0, success */
			break;
		}
		else if (*ppat == START_TAG)
			tagbeg[*(ppat + 1)] = lastc - lin;
		else if (*ppat == STOP_TAG)
			tagend[*(ppat + 1)] = lastc - lin;
			/* non-closure */
		else if (omatch (lin, &lastc, ppat) == NO)
			return (-1);
		/* else
			omatch succeeded */
	return (lastc - lin);
}

/* omatch () --- try to match a single pattern at ppat */

static omatch (lin, adplin, ppat)
char lin[], **adplin, *ppat;
{
	register char *plin;
	register int bump, retval;

	plin = *adplin;
	retval = NO;
	if (*plin == EOS)
		return (retval);
	bump = -1;
	switch (*ppat) {
	case CHAR:
		if (*plin == *(ppat + 1))
			bump = 1;
		break;

	case BOL:
		if (plin == lin)
			bump = 0;
		break;

	case ANY:
		if (*plin != NEWLINE)
			bump = 1;
		break;

	case EOL:
		if (*plin == NEWLINE)
			bump = 0;
		break;

	case CCL:
		if (locate (*plin, ppat + 1) == YES)
			bump = 1;
		break;

	case NCCL:
		if (*plin != NEWLINE && locate (*plin, ppat + 1) == NO)
			bump = 1;
		break;

	default:
		error ("in omatch: can't happen.");
	}

	if (bump >= 0)
	{
		*adplin += bump;
		retval = YES;
	}
	return (retval);
}


/* locate () --- look for c in char class at ppat */

static locate (c, ppat)
register char c, *ppat;
{
	register char *pclas;

	/* size of class is at ppat, characters follow */
	for (pclas = ppat + *ppat; pclas > ppat; pclas--)
		if (c == *pclas)
			return (YES);
	return (NO);
}


/* patsiz () --- returns size of pattern entry at ppat */

static patsiz (ppat)
register char *ppat;
{

	switch (*ppat) {
	case CHAR:
	case START_TAG:
	case STOP_TAG:
		return (2);

	case BOL:
	case EOL:
	case ANY:
		return (1);

	case CCL:
	case NCCL:
		return (*(ppat + 1) + 2);

	case CLOSURE:
		return (CLOSIZE);

	default:
		error ("in patsiz: can't happen.");
	}
}


/* makpat () --- make pattern from arg[from], terminate at delim */

makpat (arg, from, delim, pat)
char arg[], delim, pat[];
int from;
{
	char ch, esc ();
	int argsub, junk, lastsub, ls, patsub, tag_nest, tag_num, tag_stack[9];

	lastsub = patsub = 0;
	tag_num = -1;
	tag_nest = -1;
	for (argsub = from; arg[argsub] != delim && arg[argsub] != EOS;
	    argsub++)
	{
		ls = patsub;
		if (arg[argsub] == ANY)
			junk = addset (ANY, pat, &patsub, MAXPAT);
		else if (arg[argsub] == BOL && argsub == from)
			junk = addset (BOL, pat, &patsub, MAXPAT);
		else if (arg[argsub] == EOL && arg[argsub + 1] == delim)
			junk = addset (EOL, pat, &patsub, MAXPAT);
		else if (arg[argsub] == CCL)
		{
			if (getccl (arg, &argsub, pat, &patsub) == ERR)
				return (ERR);
		}
		else if (arg[argsub] == CLOSURE && argsub > from)
		{
			ls = lastsub;
			if (pat[ls] == BOL || pat[ls] == EOL ||
			    pat[ls] == CLOSURE || pat[ls] == START_TAG ||
			    pat[ls] == STOP_TAG)
				break;
			stclos (pat, &patsub, &lastsub);
		}
		else if (start_tag(arg, &argsub))
		{
			/* too many tagged sub-patterns */
			if (tag_num >= 8)
				break;
			tag_num++;
			tag_nest++;
			tag_stack[tag_nest] = tag_num;
			junk = addset (START_TAG, pat, &patsub, MAXPAT);
			junk = addset (tag_num, pat, &patsub, MAXPAT);
		}
		else if (stop_tag(arg, &argsub) && tag_nest > -1)
		{
			junk = addset (STOP_TAG, pat, &patsub, MAXPAT);
			junk = addset (tag_stack[tag_nest], pat, &patsub, MAXPAT);
			tag_nest--;
		}
		else
		{
			junk = addset (CHAR, pat, &patsub, MAXPAT);

			/* don't allow match of newline other than via $ */
			if ((ch = esc(arg, &argsub)) == NEWLINE)
				return (ERR);
			junk = addset (ch, pat, &patsub, MAXPAT);
		}
		lastsub = ls;
	}

	if (arg[argsub] != delim)               /* terminated early */
		return (ERR);
	else if (addset (EOS, pat, &patsub, MAXPAT) == NO)      /* no room */
		return (ERR);
	else if (tag_nest != -1)
		return (ERR);
	else
		return (argsub);
}


/* getccl () --- expand char class at arg[*pasub] into pat[*pindex] */

static getccl (arg, pasub, pat, pindex)
char arg[], pat[];
int *pasub, *pindex;
{
	int junk, start;

	(*pasub)++;             /* skip over [ */
	if (arg[*pasub] == NOTINCCL)
	{
		junk = addset (NCCL, pat, pindex, MAXPAT);
		(*pasub)++;
	}
	else
		junk = addset (CCL, pat, pindex, MAXPAT);

	start = *pindex;
	junk = addset (0, pat, pindex, MAXPAT); /* leave room for count */
	filset (CCLEND, arg, pasub, pat, pindex, MAXPAT);
	pat[start] = *pindex - start - 1;

	if (arg[*pasub] == CCLEND)
		return (OK);
	else
		return (ERR);
}


/* stclos () --- insert closure entry at pat[*ppatsub] */

static stclos (pat, ppatsub, plastsub)
char pat[];
int *ppatsub, *plastsub;
{
	int i, j, junk;

	for (i = *ppatsub - 1; i >= *plastsub; i--)     /* make a hole */
	{
		j = i + CLOSIZE;
		junk = addset (pat[i], pat, &j, MAXPAT);
	}
	*ppatsub += CLOSIZE;
	/* put closure in it */
	junk = addset (CLOSURE, pat, plastsub, MAXPAT);
}


/* maksub () --- make substitution string in sub */

maksub (arg, from, delim, sub)
char arg[], delim, sub[];
int from;
{
	char esc ();
	int argsub, index, junk;

	index = 0;
	for (argsub = from; arg[argsub] != delim && arg[argsub] != EOS;
	    argsub++)
		if (arg[argsub] == AND)
		{
			junk = addset (DITTO, sub, &index, MAXPAT);
			junk = addset (0, sub, &index, MAXPAT);
		}
		else if (arg[argsub] == ESCAPE && isdigit (arg[argsub + 1]))
		{
			argsub++;
			junk = addset (DITTO, sub, &index, MAXPAT);
			junk = addset (arg[argsub] - '0', sub, &index, MAXPAT);
		}
		else
			junk = addset (esc (arg, &argsub), sub, &index, MAXPAT);
	if (arg[argsub] != delim)               /* missing delimeter */
		return (ERR);
	else if (addset (EOS, sub, &index, MAXPAT) == NO)       /* no room */
		return (ERR);
	else
		return (argsub);
}


/* catsub () --- add replacement text to end of new */

catsub (lin, from, to, sub, new, k, maxnew)
register char lin[], new[], sub[];
int from[], *k, maxnew, to[];
{
	int junk, ri;
	register int i, j;

	for (i = 0; sub[i] != EOS; i++)
		if ((sub[i] & 0xff) == DITTO)
		{
			ri = sub[++i];
			for (j = from[ri]; j < to[ri]; j++)
				junk = addset (lin[j], new, k, maxnew);
		}
		else
			junk = addset (sub[i], new, k, maxnew);
}


/* filset () --- expand set at array[*pasub] into set[*pindex], stop at delim */

filset (delim, array, pasub, set, pindex, maxset)
char array[], delim, set[];
int maxset, *pasub, *pindex;
{
	char esc ();
	int junk;
	static char digits[] = "0123456789";
	static char lowalf[] = "abcdefghijklmnopqrstuvwxyz";
	static char upalf[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	for ( ; array[*pasub] != delim && array[*pasub] != EOS; (*pasub)++)
		if (array[*pasub] == ESCAPE)
			junk = addset (esc (array, pasub), set, pindex, maxset);
		else if (array[*pasub] != DASH)
			junk = addset (array[*pasub], set, pindex, maxset);
			/* literal DASH */
		else if (*pindex <= 0 || array[*pasub + 1] == EOS ||
		    array[*pasub + 1] == delim)
			junk = addset (DASH, set, pindex, maxset);
		/* else if (index (digits, set[*pindex - 1]) >= 0) */
		else if (isdigit(set[*pindex - 1]))
			dodash (digits, array, pasub, set, pindex, maxset);
		/* else if(index (lowalf, set[*pindex - 1]) >= 0) */
		else if (islower(set[*pindex - 1]))
			dodash (lowalf, array, pasub, set, pindex, maxset);
		/* else if (index (upalf, set[*pindex - 1]) >= 0) */
		else if (isupper(set[*pindex - 1]))
			dodash (upalf, array, pasub, set, pindex, maxset);
		else
			junk = addset (DASH, set, pindex, maxset);
}


/*
** dodash () --- expand array[*pasub - 1]-array[*pasub + 1] into set[*pindex],
**               from valid
*/

static dodash (valid, array, pasub, set, pindex, maxset)
char array[], set[], valid[];
int maxset, *pasub, *pindex;
{
	char esc ();
	int junk, k, limit;

	(*pasub)++;
	(*pindex)--;
	limit = index (valid, esc (array, pasub));
	for (k = index (valid, set[*pindex]); k <= limit; k++)
		junk = addset (valid[k], set, pindex, maxset);
}


/* addset () --- put c in set[*pindex];  if it fits, increment *pindex */

addset (c, set, pindex, maxsiz)
int maxsiz, *pindex;
char c, set[];
{

	if (*pindex >= maxsiz)
		return (NO);
	else
	{
		set[(*pindex)++] = c;
		return (YES);
	}
}


/* esc () --- map array[*pindex] into escaped character if appropriate */

char esc (array, pindex)
char array[];
int *pindex;
{

	if (array[*pindex] != ESCAPE)
		return (array[*pindex]);
	else if (array[*pindex + 1] == EOS)     /* ESCAPE not special at end */
		return (ESCAPE);
	else
	{
		if (array[++(*pindex)] == 'n')
			return (NEWLINE);
		else if (array[*pindex] == 't')
			return (TAB);
		else
			return (array[*pindex]);
	}
}

/* start_tag --- determine if we've seen the start of a tagged pattern */

static int start_tag(arg, argsub)
char *arg;
int *argsub;
{
	if (arg[*argsub] == ESCAPE && arg[*argsub + 1] == START_TAG)
	{
		(*argsub)++;
		return (YES);
	}
	else
		return (NO);
}

/* stop_tag --- determine if we've seen the end of a tagged pattern */

static int stop_tag(arg, argsub)
char *arg;
int *argsub;
{
	if (arg[*argsub] == ESCAPE && arg[*argsub + 1] == STOP_TAG)
	{
		(*argsub)++;
		return (YES);
	}
	else
		return (NO);
}
