/*
** pat.c
**
** pattern matching subroutines for the se screen editor.
**
** routines declared static are not necessary for the rest
** of the editor, therefore make them static in the name
** of modularity.
*/

#include "config.h"

#include <stdio.h>
#include <ctype.h>

#include "constdef.h"
#include "pat.h"
#include "main.h"
#include "misc.h"

/* Definitions used only for pattern matching */

#define AND             '&'
#define CCL             '['
#define CCLEND          ']'
#define CHAR            'a'
#define CLOSIZE         1
#define CLOSURE         '*'
#define DASH            '-'
#define DITTO           (char)0200
#define EOL             '$'
#define NCCL            'n'
#define NEWLINE         '\n'
#define TAB             '\t'
#define ANY		'.'
#define BOL		'^'
#define SE_NOTINCCL	'^'
#define START_TAG	'('
#define STOP_TAG	')'
#define ESCAPE		'\\'

/* Array dimensions and other limit values */
#define MAXLINE         128
#define MAXPAT          128

/* Pattern matching subroutines: */

/* match () --- find match anywhere on line */

int match (char lin[], char pat[])
{
	int junk[9];
	char *pc;

	for (pc = lin; *pc != SE_EOS; pc++)
	{
		if (amatch (lin, pc - lin, pat, junk, junk) >= 0)
		{
			return (SE_YES);
		}
	}

	return (SE_NO);
}


/* amatch() --- (recursive) look for match starting at lin[from] */

int amatch(char lin[], int from, char pat[], int tagbeg[], int tagend[])
{
	char *ch, *lastc;
	char *ppat;
	int k = 0;

	lastc = lin + from;     /* next unexamined input character */
	for (ppat = pat; *ppat != SE_EOS; ppat += patsiz (ppat))
	{
		if (*ppat == CLOSURE)   /* a closure entry */
		{
			ppat++;
			for (ch = lastc; *ch != SE_EOS; )
			{
				/* match as many as possible */
				if (omatch (lin, &ch, ppat) == SE_NO)
				{
					break;
				}
			}

			/*
			 * ch now points to character that made us fail
			 * try to match rest of pattern against rest of input
			 * shrink the closure by 1 after each failure
			 */
			for (ppat += patsiz (ppat); ch >= lastc; ch--)
			{
				/* successful match of rest of pattern */
				if ((k = amatch (lin, ch - lin, ppat, tagbeg,
				    tagend)) >= 0)
				{
					break;
				}
			}
			lastc = lin + k;        /* if k < 0, failure */
						/* if k >= 0, success */
			break;
		}
		else if (*ppat == START_TAG)
			tagbeg[(int) *(ppat + 1)] = lastc - lin;
		else if (*ppat == STOP_TAG)
			tagend[(int) *(ppat + 1)] = lastc - lin;
			/* non-closure */
		else if (omatch (lin, &lastc, ppat) == SE_NO)
			return (-1);
		/* else
			omatch succeeded */
	}

	return (lastc - lin);
}

/* omatch () --- try to match a single pattern at ppat */

int omatch (char lin[], char **adplin, char *ppat)
{
	char *plin;
	int bump, retval;

	plin = *adplin;
	retval = SE_NO;
	if (*plin == SE_EOS)
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
		if (locate (*plin, ppat + 1) == SE_YES)
			bump = 1;
		break;

	case NCCL:
		if (*plin != NEWLINE && locate (*plin, ppat + 1) == SE_NO)
			bump = 1;
		break;

	default:
		error (SE_NO, "in omatch: can't happen.");
	}

	if (bump >= 0)
	{
		*adplin += bump;
		retval = SE_YES;
	}
	return (retval);
}


/* locate () --- look for c in char class at ppat */

int locate (char c, char *ppat)
{
	char *pclas;

	/* size of class is at ppat, characters follow */
	for (pclas = ppat + *ppat; pclas > ppat; pclas--)
	{
		if (c == *pclas)
		{
			return (SE_YES);
		}
	}

	return (SE_NO);
}


/* patsiz () --- returns size of pattern entry at ppat */

int patsiz (char *ppat)
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
		error (SE_NO, "in patsiz: can't happen.");
		return SE_ERR; /* error() doesn't return -- will never get here */
	}
}


/* makpat () --- make pattern from arg[from], terminate at delim */

int makpat (char arg[], int from, char delim, char pat[])
{
	char ch;
	int argsub, lastsub, ls, patsub, tag_nest, tag_num, tag_stack[9];

	lastsub = patsub = 0;
	tag_num = -1;
	tag_nest = -1;
	for (argsub = from; arg[argsub] != delim && arg[argsub] != SE_EOS;
	    argsub++)
	{
		ls = patsub;
		if (arg[argsub] == ANY)
			addset (ANY, pat, &patsub, MAXPAT);
		else if (arg[argsub] == BOL && argsub == from)
			addset (BOL, pat, &patsub, MAXPAT);
		else if (arg[argsub] == EOL && arg[argsub + 1] == delim)
			addset (EOL, pat, &patsub, MAXPAT);
		else if (arg[argsub] == CCL)
		{
			if (getccl (arg, &argsub, pat, &patsub) == SE_ERR)
				return (SE_ERR);
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
			addset (START_TAG, pat, &patsub, MAXPAT);
			addset (tag_num, pat, &patsub, MAXPAT);
		}
		else if (stop_tag(arg, &argsub) && tag_nest > -1)
		{
			addset (STOP_TAG, pat, &patsub, MAXPAT);
			addset (tag_stack[tag_nest], pat, &patsub, MAXPAT);
			tag_nest--;
		}
		else
		{
			addset (CHAR, pat, &patsub, MAXPAT);

			/* don't allow match of newline other than via $ */
			if ((ch = esc(arg, &argsub)) == NEWLINE)
				return (SE_ERR);
			addset (ch, pat, &patsub, MAXPAT);
		}
		lastsub = ls;
	}

	if (arg[argsub] != delim)               /* terminated early */
		return (SE_ERR);
	else if (addset (SE_EOS, pat, &patsub, MAXPAT) == SE_NO)      /* no room */
		return (SE_ERR);
	else if (tag_nest != -1)
		return (SE_ERR);
	else
		return (argsub);
}


/* getccl () --- expand char class at arg[*pasub] into pat[*pindex] */

int getccl (char arg[], int *pasub, char pat[], int *pindex)
{
	int start;

	(*pasub)++;             /* skip over [ */
	if (arg[*pasub] == SE_NOTINCCL)
	{
		addset (NCCL, pat, pindex, MAXPAT);
		(*pasub)++;
	}
	else
	{
		addset (CCL, pat, pindex, MAXPAT);
	}

	start = *pindex;
	addset (0, pat, pindex, MAXPAT); /* leave room for count */
	filset (CCLEND, arg, pasub, pat, pindex, MAXPAT);
	pat[start] = *pindex - start - 1;

	if (arg[*pasub] == CCLEND)
		return (SE_OK);
	else
		return (SE_ERR);
}


/* stclos () --- insert closure entry at pat[*ppatsub] */

void stclos (char pat[], int *ppatsub, int *plastsub)
{
	int i, j;

	for (i = *ppatsub - 1; i >= *plastsub; i--)     /* make a hole */
	{
		j = i + CLOSIZE;
		addset (pat[i], pat, &j, MAXPAT);
	}
	*ppatsub += CLOSIZE;
	/* put closure in it */
	addset (CLOSURE, pat, plastsub, MAXPAT);
}


/* maksub () --- make substitution string in sub */

int maksub (char arg[], int from, char delim, char sub[])
{
	int argsub, index;

	index = 0;
	for (argsub = from; arg[argsub] != delim && arg[argsub] != SE_EOS;
	    argsub++)
	{
		if (arg[argsub] == AND)
		{
			addset (DITTO, sub, &index, MAXPAT);
			addset (0, sub, &index, MAXPAT);
		}
		else if (arg[argsub] == ESCAPE && isdigit (arg[argsub + 1]))
		{
			argsub++;
			addset (DITTO, sub, &index, MAXPAT);
			addset (arg[argsub] - '0', sub, &index, MAXPAT);
		}
		else
		{
			addset (esc (arg, &argsub), sub, &index, MAXPAT);
		}
	}

	if (arg[argsub] != delim)               /* missing delimeter */
	{
		return (SE_ERR);
	}
	else if (addset (SE_EOS, sub, &index, MAXPAT) == SE_NO)       /* no room */
	{
		return (SE_ERR);
	}
	else
	{
		return (argsub);
	}
}


/* catsub () --- add replacement text to end of new */

void catsub (char lin[], int from[], int to[], char sub[], char _new[], int *k, int maxnew)
{
	int ri;
	int i, j;

	for (i = 0; sub[i] != SE_EOS; i++)
	{
		if (((unsigned char)sub[i] & 0xff) == DITTO)
		{
			ri = sub[++i];
			for (j = from[ri]; j < to[ri]; j++)
			{
				addset (lin[j], _new, k, maxnew);
			}
		}
		else
		{
			addset (sub[i], _new, k, maxnew);
		}
	}
}


/* filset () --- expand set at array[*pasub] into set[*pindex], stop at delim */

void filset (char delim, char array[], int *pasub, char set[], int *pindex, int maxset)
{
	static char digits[] = "0123456789";
	static char lowalf[] = "abcdefghijklmnopqrstuvwxyz";
	static char upalf[] = "ABCDEFGHIJKLMSE_NOPQRSTUVWXYZ";

	for ( ; array[*pasub] != delim && array[*pasub] != SE_EOS; (*pasub)++)
		if (array[*pasub] == ESCAPE)
			addset (esc (array, pasub), set, pindex, maxset);
		else if (array[*pasub] != DASH)
			addset (array[*pasub], set, pindex, maxset);
			/* literal DASH */
		else if (*pindex <= 0 || array[*pasub + 1] == SE_EOS ||
		    array[*pasub + 1] == delim)
			addset (DASH, set, pindex, maxset);
		/* else if (se_index (digits, set[*pindex - 1]) >= 0) */
		else if (isdigit(set[*pindex - 1]))
			dodash (digits, array, pasub, set, pindex, maxset);
		/* else if(se_index (lowalf, set[*pindex - 1]) >= 0) */
		else if (islower(set[*pindex - 1]))
			dodash (lowalf, array, pasub, set, pindex, maxset);
		/* else if (se_index (upalf, set[*pindex - 1]) >= 0) */
		else if (isupper(set[*pindex - 1]))
			dodash (upalf, array, pasub, set, pindex, maxset);
		else
			addset (DASH, set, pindex, maxset);
}


/*
** dodash () --- expand array[*pasub - 1]-array[*pasub + 1] into set[*pindex],
**               from valid
*/

void dodash (char valid[], char array[], int *pasub, char set[], int *pindex, int maxset)
{
	int k, limit;

	(*pasub)++;
	(*pindex)--;
	limit = se_index (valid, esc (array, pasub));
	for (k = se_index (valid, set[*pindex]); k <= limit; k++)
		addset (valid[k], set, pindex, maxset);
}


/* addset () --- put c in set[*pindex];  if it fits, increment *pindex */

int addset (char c, char set[], int *pindex, int maxsiz)
{

	if (*pindex >= maxsiz)
		return (SE_NO);
	else
	{
		set[(*pindex)++] = c;
		return (SE_YES);
	}
}


/* esc () --- map array[*pindex] into escaped character if appropriate */

char esc (char array[], int *pindex)
{

	if (array[*pindex] != ESCAPE)
		return (array[*pindex]);
	else if (array[*pindex + 1] == SE_EOS)     /* ESCAPE not special at end */
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

int start_tag(char *arg, int *argsub)
{
	if (arg[*argsub] == ESCAPE && arg[*argsub + 1] == START_TAG)
	{
		(*argsub)++;
		return (SE_YES);
	}
	else
		return (SE_NO);
}

/* stop_tag --- determine if we've seen the end of a tagged pattern */

int stop_tag(char *arg, int *argsub)
{
	if (arg[*argsub] == ESCAPE && arg[*argsub + 1] == STOP_TAG)
	{
		(*argsub)++;
		return (SE_YES);
	}
	else
		return (SE_NO);
}
