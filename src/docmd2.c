/*
** docmd2.c
**
** routines to actually execute commands
**
** This file is in the public domain.
*/

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "se.h"
#include "changetty.h"
#include "docmd1.h"
#include "docmd2.h"
#include "edit.h"
#include "extern.h"
#include "main.h"
#include "misc.h"
#include "pat.h"
#include "scratch.h"
#include "screen.h"
#include "term.h"

/* append --- append lines after "line" */

int append (int line, char str[])
{
	char lin[MAXLINE];
	char term;
	int ret;
	int len, i, dpos, dotseen;

	Curln = line;

	if (str[0] == ':')	/* text to be added is in the command line */
		ret = inject (&str[1]);
	else
	{
		Cmdrow = Toprow + (Curln - Topln) + 1;  /* 1 below Curln */
		lin[0] = EOS;
		if (Indent > 0 || line <= 0)
			len = max (0, Indent - 1);
		else /* do auto indent */
		{
			LINEDESC *k;
			k = se_gettxt (line);
			for (len = 0; Txt[len] == ' '; len++)
				;
		}
		dpos = len;     /* position for terminating '.' */

		for (ret = NOSTATUS; ret == NOSTATUS; )
		{
			if (! hwinsdel())   /* do it the old, slow way */
			{
				if (Cmdrow > Botrow)
				{
					Cmdrow = Toprow + 1;
					cprow (Botrow, Toprow);
					adjust_window (Curln, Curln);
					if (First_affected > Topln)
						First_affected = Topln;
				}
				clrrow (Cmdrow);
				if (Cmdrow < Botrow)
					clrrow (Cmdrow + 1);
			}
			else	/* try to be smart about it */
			{
				if (Cmdrow > Botrow)
				{
					Cmdrow--;
					dellines (Toprow, 1);
					inslines (Cmdrow, 1);
					Topln++;
				}
				else
				{
					dellines (Botrow, 1);
					inslines (Cmdrow, 1);
				}
			}
			prompt ("apd>");
			do
				getcmd (lin, Firstcol, &len, &term);
			while (term == CURSOR_UP || term == CURSOR_DOWN
			    || term == CURSOR_SAME);

			dotseen = 0;
			if (lin[0] == '.' && lin[1] == '\n' && lin[2] == EOS)
				dotseen = 1;
			for (i = 0; i < dpos && lin[i] == ' '; i++)
				;
			if (i == dpos && lin[dpos] == '.' && lin[dpos + 1] == '\n'
			    && lin[dpos+2] == EOS)
				dotseen = 1;

			if (dotseen)
			{
				if (hwinsdel())
				{
					dellines (Cmdrow, 1);
					inslines (Botrow, 1);
				}
				ret = OK;
			}
			else if (inject (lin) == ERR)
				ret = ERR;
			else			/* inject occured */
				prompt ("");	/* erase prompt */
			Cmdrow++;
			if (term != FUNNY)
			{
				if (Indent > 0)
					len = Indent - 1;
				else /* do auto indent */
					for (len = 0; lin[len] == ' '; len++)
						;
				dpos = len;
				lin[0] = EOS;
			}
		}
		Cmdrow = Botrow + 1;
		if (hwinsdel())			/* since we take control */
		{				/* of the screen, we're sure */
			Sctop = Topln;		/* it's still OK */

			for (i = 0; i < Sclen; i++)
				Scline[i] = Sctop + i <= Lastln ? i : -1;
		}
	}
	if (Curln == 0 && Lastln > 0)   /* for 0a or 1i followed by "." */
		Curln = 1;
	if (First_affected > line)
		First_affected = line;

	tflush ();
	return (ret);
}

/* copy --- copy line1 through line2 after line3 */

int copy (int line3)
{
	int i;
	int ret;
	LINEDESC *ptr3, *after3, *k;

	ret = ERR;

	if (Line1 <= 0)
		Errcode = EORANGE;
	else
	{
		ret = OK;
		Curln = line3;
		k = getind (Line1);
		for (i = Line1; i <= Line2; i++)
		{
			gtxt (k);
			if (inject (Txt) == ERR || intrpt ())
			{
				ret = ERR;
				break;
			}
			if (Line1 < line3)
				k++;
			else
				k += 2;
			/*
			 * inject calls blkmove, which will shift the
			 * lines down one in the array, so we add two
			 * instead of one to get to the next line.
			 */
		}
		First_affected = min (First_affected, line3 + 1);
	}
	return (ret);
}


/* delete --- delete lines from through to */

int se_delete (int from, int to, int *status)
{
	LINEDESC *k1, *k2, *j1, *j2, *l1;

	if (from <= 0)          /* can't delete line 0 */
	{
		*status = ERR;
		Errcode = EORANGE;
	}
	else
	{
		if (First_affected > from)
			First_affected = from;
		blkmove (from, to, MAXBUF - 1);	/* stick at end of buffer */

		Lastln -= to - from + 1;        /* adjust number of last line */
		Curln = prevln (from);

		Lost_lines += Limcnt;
		Limcnt = to - from + 1;		/* number of lines "deleted" */

		/* point at first deleted */
		Limbo = &Buf[MAXBUF - (to - from + 1)];

		*status = OK;
		svdel (from, to - from + 1);
		Buffer_changed = YES;
	}

	return (*status);
}


/* join --- join a group of lines into a single line */

int join (char sub[])
{
	char new[MAXLINE];
	int l, line, sublen;
	int ret;
	LINEDESC *k;

	ret = OK;
	if (Line1 <= 0)
	{
		Errcode = EORANGE;
		return (ERR);
	}

	sublen = strlen (sub) + 1;      /* length of separator & EOS */
	line = Line1;
	k = getind (line);
	gtxt (k);
	move_ (Txt, new, (int) k -> Lineleng);	/* move in first chunk */
	l = k -> Lineleng;

	for (line++; line <= Line2; line++)
	{
		if (intrpt ())
			return (ERR);
		if (new[l - 2] == '\n') /* zap the NEWLINE */
			l--;
		k = NEXTLINE(k);	/* get the next line */
		gtxt (k);
		if (l + sublen - 1 + k -> Lineleng - 1 > MAXLINE)	/* won't fit */
		{
			Errcode = E2LONG;
			return (ERR);
		}
		move_ (sub, &new[l - 1], sublen);	/* insert separator string */
		l += sublen - 1;
		move_ (Txt, &new[l - 1], (int) k -> Lineleng);	/* move next line */
		l += k -> Lineleng - 1;
	}
	Curln = Line2;          /* all this will replace line1 through line2 */
	ret = inject (new);	/* inject the new line */
	if (ret == OK)
		ret = se_delete (Line1, Line2, &ret);	/* delete old lines */
	Curln++;

	if (First_affected > Curln)
		First_affected = Curln;

	return (ret);
}


/* move --- move line1 through line2 after line3 */

int move (int line3)
{
	LINEDESC *k0, *k1, *k2, *k3, *k4, *k5;

	if (Line1 <= 0)
	{
		Errcode = EORANGE;
		return (ERR);
	}

	if (Line1 <= line3 && line3 <= Line2)
	{
		Errcode = EINSIDEOUT;
		return (ERR);
	}

	blkmove (Line1, Line2, line3);

	if (line3 > Line1)
	{
		Curln = line3;
	}
	else
		Curln = line3 + (Line2 - Line1 + 1);

	Buffer_changed = YES;
	First_affected = min (First_affected, min (Line1, line3));

	return (OK);
}

/* overlay --- let user edit lines directly */

void overlay (int *status)
{
	char savtxt[MAXLINE], term, kname;
	static char empty[] = "\n";
	int lng, vcol, lcurln, scurln;
	LINEDESC *indx;

	*status = OK;
	if (Line1 == 0)
	{
		Curln = 0;
		*status = inject (empty);
		if (*status == ERR)
			return;
		First_affected = 1;
		Line1 = 1;
		Line2++;
	}

	for (lcurln = Line1; lcurln <= Line2; lcurln++)
	{
		Curln = lcurln;
		vcol = Overlay_col - 1;
		do {
			adjust_window (Curln, Curln);
			updscreen ();
			Cmdrow = Curln - Topln + Toprow;
			indx = se_gettxt (Curln);
			lng = indx -> Lineleng;
			if (Txt[lng - 2] == '\n')       /* clobber newline */
				lng--;
			if (vcol < 0)
				vcol = lng - 1;
			while (lng - 1 < vcol)
			{
				Txt[lng - 1] = ' ';
				lng++;
			}
			Txt[lng - 1] = '\n';
			Txt[lng] = EOS;
			move_ (Txt, savtxt, lng + 1);	/* make a copy of the line */
			getcmd (Txt, Firstcol, &vcol, &term);
			if (term == FUNNY)
			{
				if (First_affected > Curln)
					First_affected = Curln;
				Cmdrow = Botrow + 1;
				return;
			}
			if (strcmp (Txt, savtxt) != 0)  /* was line changed? */
			{
				kname = indx -> Markname;
				se_delete (Curln, Curln, status);
				scurln = Curln;
				if (*status == OK)
					*status = inject (Txt);
				if (*status == ERR)
				{
					Cmdrow = Botrow + 1;
					return;
				}
				indx = getind (nextln (scurln));
				indx -> Markname = kname;
			}
			else
			{           /* in case end-of-line is moved */
				if (First_affected > Curln)
					First_affected = Curln;
			}
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
			case CURSOR_SAME:
				vcol = 0;
				break;
			}
		} while (term == CURSOR_UP || 
		    term == CURSOR_DOWN ||
		    term == CURSOR_SAME);
	}
	Cmdrow = Botrow + 1;
	return;
}


/* subst --- substitute "sub" for occurrences of pattern */

int subst (char sub[], int gflag, int glob)
{
	char new[MAXLINE], kname;
	int line, m, k, lastm;
	int j, junk, status, subbed, ret;
	int tagbeg[10], tagend[10];
	LINEDESC *inx;

	if (Globals && glob)
		ret = OK;
	else
		ret = ERR;

	if (Line1 <= 0)
	{
		Errcode = EORANGE;
		return (ERR);
	}

	/* the following code has been removed for your protection
	   se_index() occasionally grabs newlines out of the character class
	   counter in a pattern.  for example [0-9] doesn't work due to this

		if (se_index (Pat, '\n') != -1)    # never delete NEWLINE
		{
			Errcode = EBADPAT;
			return (ERR);
		}
	*/

	for (line = Line1; line <= Line2; line++)
	{
		if (intrpt ())
			break;
		j = 0;
		subbed = NO;
		inx = se_gettxt (line);
		lastm = -1;
		for (k = 0; Txt[k] != EOS; )
		{
			for (m = 1; m <= 9; m++)
			{
				tagbeg[m] = -1;
				tagend[m] = -1;
			}
			if (gflag == YES || subbed == NO)
				m = amatch (Txt, k, Pat, &tagbeg[1], &tagend[1]);
			else
				m = -1;
			if (m > -1 && lastm != m)       /* replace matched text */
			{
				subbed = YES;
				tagbeg[0] = k;
				tagend[0] = m;
				catsub (Txt, tagbeg, tagend, sub, new, &j, MAXLINE);
				lastm = m;
			}
			if (m == -1 || m == k)  /* no match or null match */
			{
				junk = addset (Txt[k], new, &j, MAXLINE);
				k++;
			}
			else
				k = m;	/* skip matched text */
		}
		if (subbed == YES)
		{
			if (addset (EOS, new, &j, MAXLINE) == NO)
			{
				ret = ERR;
				Errcode = E2LONG;
				break;
			}
			kname = inx -> Markname;
			se_delete (line, line, &status);	/* remembers dot */
			ret = inject (new);
			if (First_affected > Curln)
				First_affected = Curln;
			if (ret == ERR)
				break;
			inx = getind (Curln);
			inx -> Markname = kname;
			ret = OK;
			Buffer_changed = YES;
		}
		else	/* subbed == NO */
			Errcode = ENOMATCH;
	}

	return (ret);
}


/* uniquely_name --- mark-name line; make sure no other line has same name */

void uniquely_name (char kname, int *status)
{
	int line;
	LINEDESC *k;

	defalt (Curln, Curln);

	if (Line1 <= 0)
	{
		*status = ERR;
		Errcode = EORANGE;
		return;
	}

	*status = OK;
	line = 0;
	k = Line0;

	do {
		line++;
		k = NEXTLINE(k);
		if (line == Line2)
			k -> Markname = kname;
		else if (k -> Markname == kname)
			k -> Markname = DEFAULTNAME;
	} while (line < Lastln);

	return;
}


/* draw_box --- draw or erase a box at coordinates in command line */

int draw_box (char lin[], int *i)
{
	int left, right, col, len;
	int junk;
	LINEDESC *k;
	char text[MAXLINE];
	char kname, ch;

	left = ctoi (lin, i);
	if (left <= 0 || left > MAXLINE)
	{
		Errcode = EBADCOL;
		return (ERR);
	}

	if (lin[*i] == ',')
	{
		(*i)++;
		SKIPBL (lin, *i);
		right = ctoi (lin, i);
		if (right <= 0 || right >= MAXLINE || left > right)
		{
			Errcode = EBADCOL;
			return (ERR);
		}
	}
	else
		right = left;

	SKIPBL (lin, *i);
	if (lin[*i] == '\n')
		ch = ' ';
	else
		ch = lin[(*i)++];

	if (lin[*i] != '\n')
	{
		Errcode = EEGARB;
		return (ERR);
	}

	if (Line1 <= 0)
	{
		Errcode = EORANGE;
		return (ERR);
	}

	for (Curln = Line1; Curln <= Line2; Curln++)
	{
		k = se_gettxt (Curln);
		len = k -> Lineleng;
		move_ (Txt, text, len);

		if (text[len - 2] == '\n')
			col = len - 1;
		else
			col = len;
		while (col <= right)
		{
			text[col - 1] = ' ';
			col++;
		}
		text[col - 1] = '\n';
		text[col] = EOS;

		if (Curln == Line1 || Curln == Line2)
			for (col = left; col <= right; col++)
				text[col - 1] = ch;
		else
		{
			text[left - 1] = ch;
			text[right - 1] = ch;
		}

		if (strcmp (text, Txt) != 0)
		{
			kname = k -> Markname;
			if (se_delete (Curln, Curln, &junk) == ERR
			    || inject (text) == ERR)
				return (ERR);
			k = getind (Curln);
			k -> Markname = kname;
			Buffer_changed = YES;
		}
	}

	Curln = Line1;		/* move to top of box */
	if (First_affected > Curln)
		First_affected = Curln;
	adjust_window (Curln, Curln);
	updscreen ();

	return (OK);
}


/* dfltsopt --- set the 's' option to the extension on the file name */

void dfltsopt (char name[])
{
	int i;

	for (i = strlen (name) - 1; i >= 0; i--)
		if (name[i] == '.')
		{
			dosopt (&name[i + 1]);
			break;
		}
	if (i < 0)
		dosopt ("");
}



/* doshell --- escape to the Shell to run one or more Unix commands */

/*
** emulate vi: if running just a shell, redraw the screen as
** soon as the shell exits. if running a program, let the user
** redraw the screen when he/she is ready.
**
** also emulate USG Unix 5.0 ed: a ! as the first character is
** replaced by the previous shell command; an unescaped % is replaced
** by the saved file name. The expanded command is echoed.
*/

#define DEFAULT_PATH	SHELL
#define DEF_SHELL	(rindex(DEFAULT_PATH, '/') ? rindex(DEFAULT_PATH, '/') + 1 : NULL)

int doshell (char lin[], int *pi)
{
	int rc;
	int forkstatus, childstatus;
	void (*save_quit)(int);
	void (*save_int)(int);
	int i, auto_redraw;
	char *path, *name, *p;
	char new_command[MAXLINE];
	int j, k;
	static char sav_com[MAXLINE] = "";
	int expanded = NO;

	if (Nlines == 0)        /* use normal 'ed' behavior */
	{
		tflush ();	/* flush out the terminal output */
		position_cursor (Nrows - 1, 0);	/* bottom left corner */

		if ((p = getenv ("SHELL")) == NULL || strcmp (p, DEFAULT_PATH) == 0)
		{
			path = DEFAULT_PATH;
			name = DEF_SHELL;	/* default */
		}
		/* on Berkeley systems, check the other shell */
		else if (strcmp (p, "/bin/sh") == 0)
		{
			path = "/bin/sh";
			name = "sh";
		}
		else
		{
			if (p[0] == '/')	/* full pathname there */
			{
				/* work backwards to find just name */
				path = p;
				i = strlen (p);
				while (p[i] != '/')
					i--;
				i++;		/* skip '/' */
				name = &p[i];
			}
			else
			{
				char buf[MAXLINE];
				memset(buf, '\0', MAXLINE);
				snprintf (buf, MAXLINE-1,
					"unknown shell, using %s",
					DEF_SHELL);
				remark (buf);
				path = DEFAULT_PATH;
				name = DEF_SHELL;
			}
		}

		auto_redraw = (lin[*pi] == '\n') ? YES : NO;

		/* build command, checking for leading !, and % anywhere */
		if (lin[*pi] == '!')
		{
			if (sav_com[0] != EOS)
			{
				for (j = 0; sav_com[j] != EOS; j++)
					new_command[j] = sav_com[j];
				if (new_command[j-1] == '\n')
					j--;
				(*pi)++;
				expanded = YES;
			}
			else
			{
				Errcode = ENOCMD;
				return (ERR);
			}
		}
		else
			j = 0;

		for (i = *pi; lin[i] != EOS; i++)
		{
			if (lin[i] == ESCAPE)
			{
				if (lin[i+1] != '%')
				{
					new_command[j++] = ESCAPE;
					new_command[j++] = lin[++i];
				}
				else
					new_command[j++] = lin[++i];
			}
			else if (lin[i] == '%')
			{
				for (k = 0; Savfil[k] != EOS; k++)
					new_command[j++] = Savfil[k];
				expanded = YES;
			}
			else
				new_command[j++] = lin[i];
		}

		if (new_command[j-1] == '\n')
			j--;
		new_command[j] = EOS;

		memset (sav_com, EOS, MAXLINE);
		strncpy (sav_com, new_command, MAXLINE-1);	/* save it */

		ttynormal ();
		t_exit ();
		rc = write (1, "\n\n", 2);            /* clear out a line */
		if (rc == -1)
		{
			Errcode = ESTUPID;
			return ERR;
		}

		forkstatus = fork();
		if (forkstatus == -1)   /* the fork failed */
		{
			ttyedit ();
			t_init ();
			Errcode = ECANTFORK;
			return ERR;
		}

		if (forkstatus == 0)    /* we're in the child process */
		{
			signal (SIGINT, SIG_DFL);
			signal (SIGQUIT, SIG_DFL);

			if (strcmp (name, "sh") != 0)	/* not /bin/sh */
			{
				signal (SIGTSTP, SIG_DFL);
			}
			else
			{
				signal (SIGTSTP, SIG_IGN);
			}

			if (auto_redraw)	/* no params; run a shell */
			{
				execl (path, name, NULL);
				_exit (RETERR);   /* exec failed, notify parent */
			}
			else
			{
				if (expanded)		/* echo it */
				{
					printf ("%s\n", new_command);
				}
				execl (path, name, "-c", new_command, NULL);
				_exit (RETERR);
			}
		}

		/* we're in the parent process here */
		save_int = signal (SIGINT, SIG_IGN);        /* ignore interrupts */
		save_quit = signal (SIGQUIT, SIG_IGN);
		while (wait (&childstatus) != forkstatus)
			;
		save_int = signal (SIGINT, save_int);       /* catch interupts */
		save_quit = signal (SIGQUIT, save_quit);

		rc = write (1, "\n\n", 2);    /* clear out some message space */
		if (rc == -1)
		{
			Errcode = ESTUPID;
			return ERR;
		}

		Currow = Nrows - 1;
		Curcol = 0;
		if ((childstatus >> 8) != 0)
		{
			ttyedit ();
			t_init ();
			Errcode = ENOSHELL;
			return ERR;
		}

		/* a la vi: */
		if (! auto_redraw)
		{
			int c;

			printf ("type return to continue: ");
			while ((c = getchar()) != '\n' && c != EOF)
				;
		}

		ttyedit ();
		t_init ();
		restore_screen ();

		return OK;
	}
	else
	{
		remark ("Not implemented yet");
	}

	return OK;
}
