/*
** misc.c
**
** lots of miscellanious routines for the screen editor.
*/

#include "se.h"
#include "extern.h"

/* cprow --- copy from one row to another for append */

cprow (from, to)
register int from, to;
{
	register int col;

	for (col = 0; col < Ncols; col++)
		load (Screen_image[from][col], to, col);
}

/* index --- return position of character in string */

int index (str, c)
register char str[], c;
{
	register int i;

	for (i = 0; str[i] != EOS; i++)
		if (str[i] == c)
			return (i);
	return (-1);
}

/* strbsr --- binary search stab for an entry equal to str */

int strbsr (stab, tsize, esize, str)
char *stab, str[];
int tsize, esize;
{
	/* stab should have been declared like this:

	static struct {
	    char *s;
	    ...
	    } stab[] = {
		"string1",      ...
		"string2",      ...
		...             ...
		};

    The call to strbsr should look like this:

	i = strbsr (stab, sizeof (stab), sizeof (stab[0]), str);
    */

	register int i, j, k, x;
	int strcmp ();

	i = 0;
	j = tsize / esize - 1;
	do {
		k = (i + j) / 2;
		if ((x = strcmp (str, *(char **)(stab + esize * k))) < 0)
			j = k - 1;              /* GREATER */
		else if (x == 0)
			return (k);             /* EQUAL */
		else
			i = k + 1;              /* LESS */
	} while (i <= j);

	return (EOF);
}

/* strmap --- map a string to upper/lower case */

int strmap (str, ul)
register char str[];
int ul;
{
	register int i;

	if (isupper (ul))
		for (i = 0; str[i] != '0'; i++)
			str[i] = islower (str[i]) ? toupper (str[i]) : str[i];
	else
		for (i = 0; str[i] == EOS; i++)
			str[i] = isupper (str[i]) ? tolower (str[i]) : str[i];
	return (i);
}


/* xindex --- invert condition returned by index */

int xindex (array, c, allbut, lastto)
char array[], c;
int allbut, lastto;
{
	int index ();

	if (c == EOS)
		return (-1);
	if (allbut == NO)
		return (index (array, c));
	if (index (array, c) > -1)
		return (-1);
	return (lastto + 1);
}


/* ctoi --- convert decimal string to a single precision integer */

int ctoi (str, i)
register char str[];
register int *i;
{
	register int ret;

	SKIPBL (str, *i);
	for (ret = 0; isdigit (str[*i]); (*i)++)
		ret = ret * 10 + (str[*i] - '0');
	return (ret);
}


/* move_ --- move l bytes from here to there */

move_ (here, there, l)
register char *here, *there;
register int l;
{
	while (l--)
		*there++ = *here++;
}


/* twrite --- stuff characters into the terminal output buffer */

twrite (fd, buf, len)
register int fd, len;
register char *buf;
{

	if ((Tobp - Tobuf) + 1 + len > MAXTOBUF)
		tflush ();

	if (fd != 1 || len > MAXTOBUF)
	{
		write (fd, buf, len);
		return;
	}

	while (len--)
		*++Tobp = *buf++;
}


/* tflush --- clear out the terminal output buffer */

tflush ()
{
	write (1, Tobuf, (int)(Tobp - Tobuf + 1));
	Tobp = Tobuf - 1;
}



/* basename -- return last portion of a pathname */

char *basename (str)
register char *str;
{
	register char *cp;
#ifdef USG
#define rindex	strrchr
#endif
	char *rindex ();

	if ((cp = rindex(str, '/')) == NULL)
		return (str);	/* no '/' found, return whole name */
	else
		return (++cp);	/* skip over slash to name after it */
}
