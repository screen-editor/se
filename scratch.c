/*
** scratch.c
**
** scratch file handling for se screen editor.
**
** If OLD_SCRATCH is defined, then this file will contain the
** original scratch file handling, based on linked lists,
** from the ratfor version of Software Tools.  This method is
** real good at moving lines around, but is poor for finding lines.
**
** If OLD_SCRATCH is not defined, which is the default, this file will use
** the line handling methodology presented in Software Tools In Pascal,
** *without* changing the way any of the routines are called.
**
** Bascially, the lines are always kept in order in the Buf array.
** Thus, lines 1 through 5 are in Buf[1] through Buf[5]. blkmove() and
** reverse() do the work of moving lines around in the buffer. The alloc()
** routine, therefore, always allocates the first empty slot, which will be
** at Lastln + 1, if there is room.
**
** Deleted lines are kept at the end of the buffer. Limbo points to the first
** line in the group of lines which were last deleted, or else Limbo == NOMORE.
**
** It is a very good idea to read the chapters on editing in BOTH editions of
** Software Tools, before trying to muck with this. It also helps to be a
** little bit off the wall....
**
** In fact, I would go as far as saying, "If you touch this, it will break.
** It is held together with chewing gum, scotch tape, and bobby pins."
** (Of course, you could always use OLD_SCRATCH, which definitely works.
** It just increases the size of the editor.)  So much for the old
** "Just replace the implementation of the underlying primitives..."
*/

#include "se.h"
#include "extern.h"

/* alloc --- allocate space for a new pointer block */


static LINEDESC *alloc (ptr)
register LINEDESC **ptr;
{
#ifdef OLD_SCRATCH	/* old way */
	if (Free == NOMORE)   /* no free list, expand into unallocated space */
	{
		if (Lastbf - Buf + BUFENT <= MAXBUF)   /* see if there's room */
		{
			*ptr = Lastbf;
			Lastbf += BUFENT;
		}
		else
			*ptr = NOMORE;		/* out of pointer space */
	}
	else	/* remove a block from free list */
	{
		*ptr = Free;
		Free = Free->Prevline;
	}
#else	/* new way */
	int limbo_index = Limbo - Buf;	/* use indices instead of pointers */
		/* N.B.: this statement is meaningless if Limbo == NOMORE */
		/* but if so, we don't use limbo_index anyway */

	if (Limbo == NOMORE)
		if (Lastln < (MAXBUF - 1) - 1)	/* dumb zero based indexing! */
			*ptr = &Buf[Lastln + 1];
		else
			*ptr = NOMORE;
	else if (limbo_index - Lastln > 1)
		*ptr = &Buf[Lastln + 1];
	else
		*ptr = NOMORE;
#endif

	return (*ptr);
}


/* bump --- advance line number and corresponding index simultaneously */

bump (line, ix, way)
int *line, way;
LINEDESC **ix;
{
	if (way == FORWARD)	/* increment line number */
	{
#ifdef OLD_SCRATCH
		*ix = (*ix)->Nextline;
		if (*ix == Line0)
			*line = 0;
		else
			(*line)++;
#else
		(*ix)++;
		if (*ix == &Buf[Lastln+1])
		{
			*line = 0;
			*ix = Line0;
		}
		else
			(*line)++;
#endif
	}
	else	/* decrement line number */
	{
		if (*ix == Line0)
			*line = Lastln;
		else
			(*line)--;
#ifdef OLD_SCRATCH
		*ix = (*ix)->Prevline;
#else
		if (*ix == Line0)
			*ix = &Buf[Lastln];
		else
			(*ix)--;
#endif
	}
}



/* closef --- close a file */

static closef (fd)
filedes fd;
{
	close (fd);
}




/* clrbuf --- purge scratch file */

clrbuf ()
{

	if (Lastln > 0)
		svdel (1, Lastln);

	closef (Scr);
	unlink (Scrname);
}



/* garbage_collect --- compress scratch file */

garbage_collect ()
{
	char new_name [MAXLINE];
	register int i, new_scrend;
	int new_fd;
	register LINEDESC *p;

	makscr (&new_fd, new_name);
	remark ("collecting garbage");
	new_scrend = 0;
#ifdef OLD_SCRATCH
	for (p = Limbo, i = 1; i <= Limcnt; p = p->Nextline, i++)
#else
	for (p = Limbo, i = 1; i <= Limcnt; p++, i++)
#endif
	{
		gtxt (p);
		seekf ((long) new_scrend * 8, new_fd);
		writef (Txt, (int) p->Lineleng, new_fd);
		p->Seekaddr = new_scrend;
		new_scrend += (p->Lineleng + 7) / 8;
	}
#ifdef OLD_SCRATCH
	for (p = Line0, i = 0; i <= Lastln; p = p->Nextline, i++)
#else
	for (p = Line0, i = 0; i <= Lastln; p++, i++)
#endif
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
	sprintf (Scrname, "%s", new_name);
	Scrend = new_scrend;
	Lost_lines = 0;

	remark ("");
}



/* gettxt --- locate text for line, copy to txt */

LINEDESC *gettxt (line)
int line;
{
	register LINEDESC *k;
	LINEDESC *getind ();

	k = getind (line);
	gtxt (k);

	return (k);
}



/* gtxt --- retrieve a line from the scratch file */

gtxt (ptr)
register LINEDESC *ptr;
{
	int readf ();

	seekf ((long) ptr->Seekaddr * 8, Scr); /* position to start of file */
	/*
	 * rounded Seekaddr to 8 byte sections, giving larger
	 * buffer space for text (*8)
	 */

	return (readf (Txt, (int) ptr->Lineleng, Scr) - 1);
}



/* inject --- insert a new line after curln */

inject (lin)
register char lin [];
{
	register int i;
	int maklin ();
	register LINEDESC *k1, *k2;
	LINEDESC *k3;
	LINEDESC *getind ();

	for (i = 0; lin [i] != EOS; )
	{
		i = maklin (lin, i, &k3);       /* create a single line */
		if (i == ERR)
		{
			Errcode = ECANTINJECT;
			return (ERR);
		}
#ifdef OLD_SCRATCH
		k1 = getind (Curln);            /* get pointer to curln */
		k2 = k1-> Nextline;             /* get pointer to nextln */
		relink (k1, k3, k3, k2);        /* set pointers of new line */
		relink (k3, k2, k1, k3);        /* set pointers of prev, next */
		svins (Curln, 1);
		Lastln++;		/* update Lastln */
#else
		Lastln++;		/* update Lastln */
		blkmove (Lastln, Lastln, Curln);
		svins (Curln, 1);
#endif
		Curln++;		/* update Curln */
	}
	return (OK);
}



/* maklin --- construct a new line, add to scratch file */

maklin (lin, i, newind)
register char lin [];
register int i;
LINEDESC **newind;
{

	char text [MAXLINE];
	register int l, n;
	LINEDESC *ptr;
	LINEDESC *alloc ();

	if (alloc (&ptr) == NOMORE)     /* get space for pointer block */
		return (ERR);

	for (n = i; lin [n] != EOS; n++)	/* find end of line */
		if (lin [n] == '\n')
		{
			n++;
			break;
		}

	if (n - i >= MAXLINE )  /* can't handle more than MAXLINE chars/line */
		n = i + MAXLINE - 1;
	l = n - i + 1;          /* length of new line (including EOS) */

	move_ (&lin [i], text, l);      /* move new line into text */
	text [l - 1] = EOS;             /* add EOS */

	ptr->Seekaddr = Scrend; /* will be added to end of scratch file */
	ptr->Lineleng = l;      /* line length including EOS */
	ptr->Globmark = NO;     /* not marked for Global command */
	ptr->Markname = DEFAULTNAME;    /* give it default mark name */

	seekf ((long) Scrend * 8, Scr); /* go to end of scratch file */
	writef (text, l, Scr);          /* write line on scratch file */
	Scrend += (l + 7) / 8;          /* update end-of-file pointer */

	Buffer_changed = YES;

	*newind = ptr;                  /* return index of new line */
	return (n);                     /* return next char of interest in lin */
}



/* makscr --- create a new scratch file */

makscr (fd, str)
register filedes *fd;
register char str[];
{
	register int i;

	for (i = 0; i <= 9; i++)
	{
		sprintf (str, "/usr/tmp/se%d.%d", getpid(), i);
		/* create str name in /usr/tmp */
		if ((*fd = open (str, 0)) < 0)
		{
			/* if the file is not there, close it and create it */
			close (*fd);
			if ((*fd = creat (str, 0700)) > 0)
			{
				close (*fd);
				if ((*fd = open (str, 2)) > 0)
					return;
			}
		}
		else
			close (*fd);

	}
	error ("can't create scratch file");
}



/* nextln --- get line after "line" */

nextln (line)
int line;
{
	register int ret;

	ret = line + 1;
	if (ret > Lastln)
		ret = 0;

	return (ret);
}



/* prevln --- get line before "line" */

prevln (line)
int line;
{
	register int ret;

	ret = line - 1;
	if (ret < 0)
		ret = Lastln;

	return (ret);
}



/* readf --- read count words from fd into buf */

readf (buf, count, fd)
char buf [];
int  count, fd;
{
	register int ret;

	ret = read (fd, buf, count);
	if (ret != count)
		error ("Fatal scratch file read error");

	return (ret);
}


#ifdef OLD_SCRATCH
/* relink --- rewrite two half links */

relink (a, x, y, b)
LINEDESC *a, *b, *x, *y;
{
	x->Prevline = a;
	y->Nextline = b;
}
#endif



/* seekf --- position file open on fd to pos */

static seekf (pos, fd)
long pos;
filedes fd;
{
	register long ret;
	long lseek ();

	ret = lseek (fd, pos, 0);               /* abs seek */
	if (ret != pos)
		error ("Fatal scratch file seek error");
	return (OK);
}



/* mkbuf --- create scratch file, initialize line 0 */

mkbuf ()
{
	LINEDESC *p;

	makscr (&Scr, Scrname);	/* create a scratch file */
	Scrend = 0;		/* initially empty */

	Curln = 0;
#ifdef OLD_SCRATCH
	Lastln = 0;
#else
	Lastln = -1;		/* alloc depends on this... */
#endif

#ifdef OLD_SCRATCH
	Lastbf = &Buf[0];       /* next word available for allocation ?? */
	Free = NOMORE;          /* free list initially empty */
#endif
	Limbo = NOMORE;         /* no lines in limbo */
	Limcnt = 0;
	Lost_lines = 0;         /* no garbage in scratch file yet */

	maklin ("", 0, &p);     /* create an empty line */
#ifdef OLD_SCRATCH
	relink (p, p, p, p);    /* establish initial linked list */
#endif
	p->Markname = EOS;	/* give it an illegal mark name */
	Line0 = p;              /* henceforth and forevermore */

#ifndef OLD_SCRATCH
	Lastln = 0;
#endif
}



/* sp_inject --- special inject for reading files */

LINEDESC *sp_inject (lin, len, line)
char lin[];
int  len;
LINEDESC *line;
{
	register LINEDESC *k, *ret;
	LINEDESC *ptr;
	LINEDESC *alloc ();

	ret = alloc (&ptr);
	if (ptr == NOMORE)
	{
		Errcode = ECANTINJECT;
		return (ret);
	}

	ptr->Seekaddr = Scrend;
	ptr->Lineleng = len + 1;
	ptr->Globmark = NO;
	ptr->Markname = DEFAULTNAME;

	seekf ((long) Scrend * 8, Scr);
	writef (lin, len + 1, Scr);
	Scrend += ((len + 1) + 7) / 8;          /* fudge for larger buffer */
	Lastln++;

	Buffer_changed = YES;

#ifdef OLD_SCRATCH
	k = line->Nextline;
	relink (line, ptr, ptr, k);
	relink (ptr, k, line, ptr);
#else
	/*
	 * this part dependant on the fact that we set
	 * Curln = line in the routine do_read.
	 */
	blkmove (ptr - Buf, ptr - Buf, Curln);	/* need line no's */
	Curln++;
#endif

	return (ret);
}



/* writef --- write count words from buf onto fd */

writef (buf, count, fd)
char buf[];
int  count;
filedes fd;
{
	register int ret;

	ret = write (fd, buf, count);
	if (ret != count)
		error ("Fatal scratch file write error");
	return (ret);
}



/* getind --- locate line index in buffer */

LINEDESC *getind (line)
register int line;
{
#ifdef OLD_SCRATCH
	register LINEDESC *k;

	k = Line0;
	line++;
	while (--line)
		k = k->Nextline;

	return (k);
#else
	return (&Buf[line]);
#endif
}

#ifndef OLD_SCRATCH

/* blkmove -- use SWT in Pascal line handling */

blkmove (n1, n2, n3)	/* move block of lines n1..n2 to after n3 */
int n1, n2, n3;
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

reverse (n1, n2)
register int n1, n2;
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
#endif
