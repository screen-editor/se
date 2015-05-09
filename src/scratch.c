/*
** scratch.c
**
** scratch file handling for se screen editor.
**
** This file will use the line handling methodology presented in Software
** Tools In Pascal, *without* changing the way any of the routines are
** called.
**
** Bascially, the lines are always kept in order in the Buf array.
** Thus, lines 1 through 5 are in Buf[1] through Buf[5]. blkmove() and
** reverse() do the work of moving lines around in the buffer. The alloc()
** routine, therefore, always allocates the first empty slot, which will be
** at Lastln + 1, if there is room.
**
** Deleted lines are kept at the end of the buffer. Limbo points to the first
** line in the group of lines which were last deleted, or else Limbo == SE_NOMORE.
**
** It is a very good idea to read the chapters on editing in BOTH editions of
** Software Tools, before trying to muck with this. It also helps to be a
** little bit off the wall....
**
** In fact, I would go as far as saying, "If you touch this, it will break.
** It is held together with chewing gum, scotch tape, and bobby pins."
**
** This file is in the public domain.
*/

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "se.h"
#include "docmd1.h"
#include "extern.h"
#include "main.h"
#include "misc.h"
#include "scratch.h"
#include "screen.h"

/* alloc --- allocate space for a new pointer block */


LINEDESC *alloc (LINEDESC **ptr)
{
	int limbo_index = Limbo - Buf;	/* use indices instead of pointers */
		/* N.B.: this statement is meaningless if Limbo == SE_NOMORE */
		/* but if so, we don't use limbo_index anyway */

	if (Limbo == SE_NOMORE)
		if (Lastln < (MAXBUF - 1) - 1)	/* dumb zero based indexing! */
			*ptr = &Buf[Lastln + 1];
		else
			*ptr = SE_NOMORE;
	else if (limbo_index - Lastln > 1)
		*ptr = &Buf[Lastln + 1];
	else
		*ptr = SE_NOMORE;

	return (*ptr);
}


/* bump --- advance line number and corresponding index simultaneously */

void bump (int *line, LINEDESC **ix, int way)
{
	if (way == FORWARD)	/* increment line number */
	{
		(*ix)++;
		if (*ix == &Buf[Lastln+1])
		{
			*line = 0;
			*ix = Line0;
		}
		else
			(*line)++;
	}
	else	/* decrement line number */
	{
		if (*ix == Line0)
			*line = Lastln;
		else
			(*line)--;
		if (*ix == Line0)
			*ix = &Buf[Lastln];
		else
			(*ix)--;
	}
}



/* closef --- close a file */

void closef (int fd)
{
	close (fd);
}




/* clrbuf --- purge scratch file */

void clrbuf (void)
{

	if (Lastln > 0)
		svdel (1, Lastln);

	closef (Scr);
	unlink (Scrname);
}



/* garbage_collect --- compress scratch file */

void garbage_collect (void)
{
	char new_name [MAXLINE];
	int i, new_scrend;
	int new_fd;
	LINEDESC *p;

	makscr (&new_fd, new_name, MAXLINE);
	remark ("collecting garbage");
	new_scrend = 0;
	for (p = Limbo, i = 1; i <= Limcnt; p++, i++)
	{
		gtxt (p);
		seekf ((long) new_scrend * 8, new_fd);
		writef (Txt, (int) p->Lineleng, new_fd);
		p->Seekaddr = new_scrend;
		new_scrend += (p->Lineleng + 7) / 8;
	}
	for (p = Line0, i = 0; i <= Lastln; p++, i++)
	{
		gtxt (p);
		seekf ((long) new_scrend * 8, new_fd);
		writef (Txt, (int) p->Lineleng, new_fd);
		p->Seekaddr = new_scrend;
		new_scrend += (p->Lineleng + 7) / 8;
	}

	closef (Scr);
	unlink (Scrname);

	Scr = new_fd;
	memset (Scrname, '\0', MAXLINE);
	snprintf (Scrname, MAXLINE-1, "%s", new_name);
	Scrend = new_scrend;
	Lost_lines = 0;

	remark ("");
}



/* se_gettxt --- locate text for line, copy to txt */

LINEDESC *se_gettxt (int line)
{
	LINEDESC *k;

	k = getind (line);
	gtxt (k);

	return (k);
}



/* gtxt --- retrieve a line from the scratch file */

int gtxt (LINEDESC *ptr)
{
	seekf ((long) ptr->Seekaddr * 8, Scr); /* position to start of file */
	/*
	 * rounded Seekaddr to 8 byte sections, giving larger
	 * buffer space for text (*8)
	 */

	return (readf (Txt, (int) ptr->Lineleng, Scr) - 1);
}



/* inject --- insert a new line after curln */

int inject (char lin[])
{
	int i;
	int maklin ();
	LINEDESC *k3;
	LINEDESC *getind ();

	for (i = 0; lin [i] != SE_EOS; )
	{
		i = maklin (lin, i, &k3);       /* create a single line */
		if (i == SE_ERR)
		{
			Errcode = ECANTINJECT;
			return (SE_ERR);
		}
		Lastln++;		/* update Lastln */
		blkmove (Lastln, Lastln, Curln);
		svins (Curln, 1);
		Curln++;		/* update Curln */
	}
	return (SE_OK);
}



/* maklin --- construct a new line, add to scratch file */

int maklin (char lin[], int i, LINEDESC **newind)
{

	char text [MAXLINE];
	int l, n;
	LINEDESC *ptr;

	if (alloc (&ptr) == SE_NOMORE)     /* get space for pointer block */
		return (SE_ERR);

	for (n = i; lin [n] != SE_EOS; n++)	/* find end of line */
		if (lin [n] == '\n')
		{
			n++;
			break;
		}

	if (n - i >= MAXLINE )  /* can't handle more than MAXLINE chars/line */
		n = i + MAXLINE - 1;
	l = n - i + 1;          /* length of new line (including SE_EOS) */

	move_ (&lin [i], text, l);      /* move new line into text */
	text [l - 1] = SE_EOS;             /* add SE_EOS */

	ptr->Seekaddr = Scrend; /* will be added to end of scratch file */
	ptr->Lineleng = l;      /* line length including SE_EOS */
	ptr->Globmark = SE_NO;     /* not marked for Global command */
	ptr->Markname = DEFAULTNAME;    /* give it default mark name */

	seekf ((long) Scrend * 8, Scr); /* go to end of scratch file */
	writef (text, l, Scr);          /* write line on scratch file */
	Scrend += (l + 7) / 8;          /* update end-of-file pointer */

	Buffer_changed = SE_YES;

	*newind = ptr;                  /* return index of new line */
	return (n);                     /* return next char of interest in lin */
}


/* makscr --- create a new scratch file */

void makscr (int *fd, char str[], size_t strsize)
{
	char *expanded;

	/* old scratch files were in /tmp, some systems use /usr/tmp.
	 * There isn't an easy way to determine /tmp vs. /usr/tmp, so use
	 * the user's home directory which is probably safer than /tmp
	 */
	expanded = expand_env ("$HOME/.scratch-XXXXXX");
	memset (str, SE_EOS, strsize);
	snprintf (str, strsize >= 1 ? strsize - 1 : 0, "%s", expanded);

#ifdef HAVE_MKSTEMP
	*fd = mkstemp (str);
#else /* !HAVE_MKSTEMP */
	*fd = open(str, O_RDWR | O_CREAT | O_EXCL, 0600);
#endif /* !HAVE_MKSTEMP */

	if (*fd == -1)
	{
		error (SE_YES, "can't create scratch file");
	}
}



/* nextln --- get line after "line" */

int nextln (int line)
{
	int ret;

	ret = line + 1;
	if (ret > Lastln)
		ret = 0;

	return (ret);
}



/* prevln --- get line before "line" */

int prevln (int line)
{
	int ret;

	ret = line - 1;
	if (ret < 0)
		ret = Lastln;

	return (ret);
}



/* readf --- read count words from fd into buf */

int readf (char buf[], int count, int fd)
{
	int ret;

	ret = read (fd, buf, count);
	if (ret != count)
		error (SE_YES, "Fatal scratch file read error");

	return (ret);
}


/* seekf --- position file open on fd to pos */

int seekf (long pos, int fd)
{
	long ret;

	ret = lseek (fd, pos, 0);               /* abs seek */
	if (ret != pos)
		error (SE_YES, "Fatal scratch file seek error");
	return (SE_OK);
}



/* mkbuf --- create scratch file, initialize line 0 */

void mkbuf (void)
{
	LINEDESC *p;

	makscr (&Scr, Scrname, MAXLINE);	/* create a scratch file */
	Scrend = 0;				/* initially empty */

	Curln = 0;
	Lastln = -1;				/* alloc depends on this... */
	Limbo = SE_NOMORE;         		/* no lines in limbo */
	Limcnt = 0;
	Lost_lines = 0;         		/* no garbage in scratch file yet */

	maklin ("", 0, &p);     		/* create an empty line */
	p->Markname = SE_EOS;			/* give it an illegal mark name */
	Line0 = p;              		/* henceforth and forevermore */

	Lastln = 0;
}



/* sp_inject --- special inject for reading files */

LINEDESC *sp_inject (char lin[], int len, LINEDESC *line)
{
	LINEDESC *ret;
	LINEDESC *ptr;

	ret = alloc (&ptr);
	if (ptr == SE_NOMORE)
	{
		Errcode = ECANTINJECT;
		return (ret);
	}

	ptr->Seekaddr = Scrend;
	ptr->Lineleng = len + 1;
	ptr->Globmark = SE_NO;
	ptr->Markname = DEFAULTNAME;

	seekf ((long) Scrend * 8, Scr);
	writef (lin, len + 1, Scr);
	Scrend += ((len + 1) + 7) / 8;          /* fudge for larger buffer */
	Lastln++;

	Buffer_changed = SE_YES;

	/*
	 * this part dependant on the fact that we set
	 * Curln = line in the routine do_read.
	 */
	blkmove (ptr - Buf, ptr - Buf, Curln);	/* need line no's */
	Curln++;

	return (ret);
}



/* writef --- write count words from buf onto fd */

int writef (char buf[], int count, int fd)
{
	int ret;

	ret = write (fd, buf, count);
	if (ret != count)
		error (SE_YES, "Fatal scratch file write error");
	return (ret);
}



/* getind --- locate line index in buffer */

LINEDESC *getind (int line)
{
	return (&Buf[line]);
}


/* blkmove -- use SWT in Pascal line handling */

void blkmove (int n1, int n2, int n3)	/* move block of lines n1..n2 to after n3 */
{
	if (n3 < n1 -1)
	{
		reverse (n3 + 1, n1 - 1);
		reverse (n1, n2);
		reverse (n3 + 1, n2);
	}
	else if (n3 > n2)
	{
		reverse (n1, n2);
		reverse (n2 + 1, n3);
		reverse (n1, n3);
	}
}

/* reverse -- reverse buf[n1]..buf[n2] */

void reverse (int n1, int n2)
{
	LINEDESC temp;

	while (n1 < n2)
	{
		temp = Buf[n1];
		Buf[n1] = Buf[n2];
		Buf[n2] = temp;
		n1++;
		n2--;
	}
}
