/*
** scriptse.c
**
** turn input into a form se
** can use as a script.
**
** This is very quick-and-dirty, not checking
** for any of se's control characters.
**
** This file is in the public domain.
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char *argv[])
{
	int c;
	int dflag = 0;

	if (argc > 1)
	{
		if (strcmp (argv[1], "-d") == 0)
		{
			dflag = 1;
		}
		else
		{
			fprintf (stderr, "usage: %s [-d] < file > newfile\n",
				argv[0]);
			exit (1);
		}
	}

	while ((c = getchar()) != EOF)
	{
		if (c != '\n')		/* most frequent case */
			putchar (c);
		else
		{
			putchar ('\r');
			if (! dflag)
				putchar ('\177');
		}
	}

	return 0;
}
