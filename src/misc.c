/*
** misc.c
**
** lots of miscellanious routines for the screen editor.
*/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "se.h"
#include "extern.h"
#include "misc.h"
#include "screen.h"

/* cprow --- copy from one row to another for append */

void cprow (int from, int to)
{
	int col;

	for (col = 0; col < Ncols; col++)
	{
		load (Screen_image[from][col], to, col);
	}
}

/* se_index --- return position of character in string */

int se_index (char str[], char c)
{
	int i;

	for (i = 0; str[i] != SE_EOS; i++)
	{
		if (str[i] == c)
		{
			return (i);
		}
	}

	return (-1);
}

/* strbsr --- binary search stab for an entry equal to str */

int strbsr (char *stab, int tsize, int esize, char str[])
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

	int i, j, k, x;

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

int strmap (char str[], int ul)
{
	int i;

	if (isupper (ul))
	{
		for (i = 0; str[i] != '0'; i++)
		{
			str[i] = islower (str[i]) ? toupper (str[i]) : str[i];
		}
	}
	else
	{
		for (i = 0; str[i] == SE_EOS; i++)
		{
			str[i] = isupper (str[i]) ? tolower (str[i]) : str[i];
		}
	}

	return (i);
}


/* xindex --- invert condition returned by se_index */

int xindex (char array[], char c, int allbut, int lastto)
{
	if (c == SE_EOS)
		return (-1);
	if (allbut == SE_NO)
		return (se_index (array, c));
	if (se_index (array, c) > -1)
		return (-1);
	return (lastto + 1);
}


/* ctoi --- convert decimal string to a single precision integer */

int ctoi (char str[], int *i)
{
	int ret;

	SKIPBL (str, *i);

	for (ret = 0; isdigit (str[*i]); (*i)++)
	{
		ret = ret * 10 + (str[*i] - '0');
	}

	return (ret);
}


/* move_ --- move l bytes from here to there */

void move_ (char *here, char *there, int l)
{
	while (l--)
		*there++ = *here++;
}


/* twrite --- stuff characters into the terminal output buffer */

int twrite (int fd, char *buf, int len)
{
	int rc;

	if ((Tobp - Tobuf) + 1 + len > MAXTOBUF)
	{
		tflush ();
	}

	if (fd != 1 || len > MAXTOBUF)
	{
		rc = write (fd, buf, len);
		return rc;
	}

	while (len--)
	{
		*++Tobp = *buf++;
	}

	return 0;
}


/* tflush --- clear out the terminal output buffer */

int tflush (void)
{
	int rc;

	rc = write (1, Tobuf, (int)(Tobp - Tobuf + 1));
/* Reset Tobp before the start of the buffer; make sure to pre-increment. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
	Tobp = Tobuf - 1;
#pragma GCC diagnostic pop

	return rc;
}



/* basename -- return last portion of a pathname */

char *basename (char *str)
{
	char *cp;

	if ((cp = strrchr(str, '/')) == NULL)
	{
		return (str);	/* no '/' found, return whole name */
	}
	else
	{
		return (++cp);	/* skip over slash to name after it */
	}
}
