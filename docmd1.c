/*
** docmd1.c
**
** main command processor.  routines for individual commands
*/

#include "se.h"
#include "extern.h"

/* static data definitions -- variables only needed in this file */
static char Tlpat[MAXPAT] = "";	/* saved character list for y/t command */
static char Tabstr[MAXLINE] = "";	/* string representation of tab stops */
static char Ddir = FORWARD;		/* delete direction */
static int Compress;			/* compress/expand tabs on read/write */

/* docmd --- handle all commands except globals */

int docmd (lin, i, glob, status)
char lin[];
int i, glob, *status;
{
	char file[MAXLINE], sub[MAXPAT];
	char kname;
	int gflag, line3, pflag, flag, fflag, junk, allbut, tflag;
	int append (), ckchar (), ckp (), ckupd (), copy ();
	int delete (), domark (), doopt (), doprnt (), doread ();
	int doshell ();
	int dotlit (), doundo (), dowrit (), getfn (), getkn ();
	int getone (), getrange (), getrhs (), getstr (), inject ();
	int join (), makset (), move (), nextln (), optpat ();
	int prevln (), substr (), draw_box ();
	char *expand_env ();


	*status = ERR;
	if (intrpt ())  /* catch a pending interrupt */
		return (*status);

	switch (lin[i]) {
	case APPENDCOM:
	case UCAPPENDCOM:
		if (lin[i + 1] == '\n' || lin[i + 1] == ':')
		{
			defalt (Curln, Curln);
			if (lin[i + 1] == '\n')
			{
				/* avoid updating with inline insertion */
				adjust_window (Line1, Line2);
				updscreen ();
			}
			*status = append (Line2, &lin[i + 1]);
		}
		break;

	case PRINTCUR:
		if (lin[i + 1] == '\n')
		{
			defalt (Curln, Curln);
			saynum (Line2);
			*status = OK;
		}
		break;

	case OVERLAYCOM:
	case UCOVERLAYCOM:
		defalt (Curln, Curln);
		if (lin[i + 1] == '\n')
			overlay (status);
		break;

	case CHANGE:
	case UCCHANGE:
		defalt (Curln, Curln);
		if (Line1 <= 0)
			Errcode = EORANGE;
		else if (lin[i + 1] == '\n' || lin[i + 1] == ':')
		{
			if (lin[i + 1] == '\n')
			{
				/* avoid updating with inline insertion */
				adjust_window (Line2, Line2);
				updscreen ();
			}
			First_affected = min (First_affected, Line1);
			if (lin[i + 1] == '\n')
				warn_deleted (Line1, Line2);
			*status = append (Line2, &lin[i + 1]);
			if (*status != ERR)
			{
				line3 = Curln;
				delete (Line1, Line2, status);
				Curln = line3 - (Line2 - Line1 + 1);
				/* adjust for deleted lines */
			}
		}
		break;

	case DELCOM:
	case UCDELCOM:
		if (ckp (lin, i + 1, &pflag, status) == OK)
		{
			defalt (Curln, Curln);
			if (delete (Line1, Line2, status) == OK
			    && Ddir == FORWARD
			    && nextln (Curln) != 0)
				Curln = nextln (Curln);
		}
		break;

	case INSERT:
	case UCINSERT:
		defalt (Curln, Curln);
		if (Line1 <= 0)
			Errcode = EORANGE;
		else if (lin[i + 1] == '\n' || lin[i + 1] == ':')
		{
			if (lin[i + 1] == '\n')
			{
				/* avoid updating with inline insertion */
				adjust_window (Line1, Line2);
				updscreen ();
			}
			*status = append (prevln (Line2), &lin[i + 1]);
		}
		break;

	case MOVECOM:
	case UCMOVECOM:
		i++;
		if (getone (lin, &i, &line3, status) == EOF)
			*status = ERR;
		if (*status == OK && ckp (lin, i, &pflag, status) == OK)
		{
			defalt (Curln, Curln);
			*status = move (line3);
		}
		break;

	case COPYCOM:
	case UCCOPYCOM:
		if (! Unix_mode)
			goto translit;	/* SWT uses 't' for translit */
		/* else
			fall through and act normally */
	docopy:
		i++;
		if (getone (lin, &i, &line3, status) == EOF)
			*status = ERR;
		if (*status == OK && ckp (lin, i, &pflag, status) == OK)
		{
			defalt (Curln, Curln);
			*status = copy (line3);
		}
		break;

	case SUBSTITUTE:
	case UCSUBSTITUTE:
		i++;
		if (lin[i] == '\n')
		{
			/* turn "s\n" into "s//%/\n" */
			lin[i+0] = '/';
			lin[i+1] = '/';
			lin[i+2] = Unix_mode ? '%' : '&';
			lin[i+3] = '/';
			lin[i+4] = '\n';
			lin[i+5] = EOS;
			Peekc = SKIP_RIGHT;
		}
		else
		{
			/* try to handle "s/stuff\n" */
			int j, missing_delim;

			missing_delim = YES;
			for (j = i + 1; lin[j] != '\n'; j++)
				if (lin[j] == ESCAPE && lin[j+1] == lin[i])
					j++;	/* skip esc, loop continues */
				else if (lin[j] == lin[i])
				{
					missing_delim = NO;
					break;	/* for */
				}

			if (missing_delim)
			{
				for (; lin[j] != EOS; j++)
					;
				j--;		/* j now at newline */

				lin[j] = lin[i];	/* delim */
				lin[++j] = '\n';
				lin[++j] = EOS;
				Peekc = SKIP_RIGHT;
				/* rest of routines will continue to fix up */
			}
		}

		if (optpat (lin, &i) == OK
		    && getrhs (lin, &i, sub, &gflag) == OK
		    && ckp (lin, i + 1, &pflag, status) == OK)
		{
			defalt (Curln, Curln);
			*status = subst (sub, gflag, glob);
		}
		break;

	case TLITCOM:
	case UCTLITCOM:
		if (! Unix_mode)
			goto docopy;	/* SWT uses 'y' for copying */
		/* else
			fall through and act normally */
	translit:
		i++;
		if (lin[i] == '\n')
		{
			/* turn "y\n" into "y//%/\n" */
			lin[i+0] = '/';
			lin[i+1] = '/';
			lin[i+2] = Unix_mode ? '%' : '&';
			lin[i+3] = '/';
			lin[i+4] = '\n';
			lin[i+5] = EOS;
			Peekc = SKIP_RIGHT;
		}
		else
		{
			/* try to handle "y/stuff\n" */
			int j, missing_delim;

			missing_delim = YES;
			for (j = i + 1; lin[j] != '\n'; j++)
				if (lin[j] == ESCAPE && lin[j+1] == lin[i])
					j++;	/* skip esc, loop continues */
				else if (lin[j] == lin[i])
				{
					missing_delim = NO;
					break;	/* for */
				}

			if (missing_delim)
			{
				for (; lin[j] != EOS; j++)
					;
				j--;		/* j now at newline */

				lin[j] = lin[i];	/* delim */
				lin[++j] = '\n';
				lin[++j] = EOS;
				Peekc = SKIP_RIGHT;
				/* rest of routines will continue to fix up */
			}
		}

		if (getrange (lin, &i, Tlpat, MAXPAT, &allbut) == OK
		    && makset (lin, &i, sub, MAXPAT) == OK
		    && ckp (lin, i + 1, &pflag, status) == OK)
		{
			defalt (Curln, Curln);
			*status = dotlit (sub, allbut);
		}
		break;

	case JOINCOM:
	case UCJOINCOM:
		i++;
		if (getstr (lin, &i, sub, MAXPAT) == OK
		    && ckp (lin, i + 1, &pflag, status) == OK)
		{
			defalt (prevln (Curln), Curln);
			*status = join (sub);
		}
		break;

	case UNDOCOM:
	case UCUNDOCOM:
		i++;
		defalt (Curln, Curln);
		if (ckchar (UCDELCOM, DELCOM, lin, &i, &flag, status) == OK
		    && ckp (lin, i, &pflag, status) == OK)
			*status = doundo (flag, status);
		break;

	case ENTER:
	case UCENTER:
		i++;
		if (Nlines != 0)
			Errcode = EBADLNR;
		else if (ckupd (lin, &i, ENTER, status) == OK
		    && ckchar ('x', 'X', lin, &i, &tflag, status) == OK)
			if (getfn (lin, i - 1, file) == OK)
			{
				strcpy (Savfil, expand_env (file));
				mesg (Savfil, FILE_MSG);
				clrbuf ();
				mkbuf ();
				dfltsopt (file);
				*status = doread (0, file, tflag);
				First_affected = 0;
				Curln = min (1, Lastln);
				Buffer_changed = NO;
			}
			else
				*status = ERR;
		break;

	case PRINTFIL:
	case UCPRINTFIL:
		if (Nlines != 0)
			Errcode = EBADLNR;
		else if (getfn (lin, i, file) == OK)
		{
			strcpy (Savfil, expand_env (file));
			mesg (Savfil, FILE_MSG);
			*status = OK;
		}
		break;

	case READCOM:
	case UCREADCOM:
		i++;
		if (ckchar ('x', 'X', lin, &i, &tflag, status) == OK)
			if (getfn (lin, i - 1, file) == OK)
			{
				defalt (Curln, Curln);
				*status = doread (Line2, file, tflag);
			}
		break;

	case WRITECOM:
	case UCWRITECOM:
		i++;
		flag = NO;
		fflag = NO;
		junk = ckchar ('>', '+', lin, &i, &flag, &junk);
		if (flag == NO)
			junk = ckchar ('!', '!', lin, &i, &fflag, &junk);
		junk = ckchar ('x', 'X', lin, &i, &tflag, &junk);
		if (getfn (lin, i - 1, file) == OK)
		{
			defalt (1, Lastln);
			*status = dowrit (Line1, Line2, file, flag, fflag, tflag);
		}
		break;

	case PRINT:
	case UCPRINT:
		if (lin[i + 1] == '\n')
		{
			defalt (1, Topln);
			*status = doprnt (Line1, Line2);
		}
		break;

	case PAGECOM:
		defalt (1, min (Lastln, Botrow - Toprow + Topln));
		if (Line1 <= 0)
			Errcode = EORANGE;
		else if (lin[i + 1] == '\n')
		{
			Topln = Line2;
			Curln = Line2;
			First_affected = Line2;
			*status = OK;
		}
		break;

	case NAMECOM:
	case UCNAMECOM:
		i++;
		if (getkn (lin, &i, &kname, DEFAULTNAME) != ERR
		    && lin[i] == '\n')
			uniquely_name (kname, status);
		break;

	case MARKCOM:
	case UCMARKCOM:
		i++;
		if (getkn (lin, &i, &kname, DEFAULTNAME) != ERR
		    && lin[i] == '\n')
		{
			defalt (Curln, Curln);
			*status = domark (kname);
		}
		break;

	case '\n':
		line3 = nextln (Curln);
		defalt (line3, line3);
		*status = doprnt (Line2, Line2);
		break;

	case LOCATECMD:
	case UCLOCATECMD:
		if (lin[i+1] == '\n')
		{
			char *sysname ();

			remark (sysname ());
			*status = OK;
		}
		break;

	case OPTCOM:
	case UCOPTCOM:
		if (Nlines == 0)
			*status = doopt (lin, &i);
		else
			Errcode = EBADLNR;
		break;

	case QUIT:
	case UCQUIT:
		i++;
		if (Nlines != 0)
			Errcode = EBADLNR;
		else if (ckupd (lin, &i, QUIT, status) == OK)
			if (lin[i] == '\n')
				*status = EOF;
			else
				*status = ERR;
		break;

	case HELP:
	case UCHELP:
		i++;
		if (Nlines == 0)
			dohelp (lin, &i, status);
		else
			Errcode = EBADLNR;
		break;

	case MISCCOM:		/* miscellanious features */
	case UCMISCCOM:
		i++;
		switch (lin[i]) {
		case 'b':	/* draw box */
		case 'B':
			defalt (Curln, Curln);
			i++;
			*status = draw_box (lin, &i);
			break;

		default:
			Errcode = EWHATZAT;
			break;
		}
		break;

	case SHELLCOM:
		if (! Unix_mode)
			error ("in docmd: can't happen.");
	shellcom:
		i++;
		defalt (Curln, Curln);
		*status = doshell (lin, &i);
		break;

	case '~':
		if (! Unix_mode)
			goto shellcom;
		/* else
			fall through to default
			which will generate an error */

	default:
		Errcode = EWHATZAT;	/* command not recognized */
		break;
	}

	if (*status == OK)
		Probation = NO;

	return (*status);
}


/* dohelp --- display documentation about editor */

dohelp (lin, i, status)
char lin[];
int *i, *status;
{
	char filename[MAXLINE];
	char swt_filename[MAXLINE];
	static char helpdir[] = "/usr/local/lib/se_h";	/* help scripts */
	int j;
	FILE *fp, *fopen ();

	SKIPBL (lin, *i);
	if (lin[*i] == NEWLINE)
		sprintf (filename, "%s/elp", helpdir);
	else
	{
		/* build filename from text after "h" */
		sprintf (filename, "%s/%s", helpdir, &lin[*i]);
		j = strlen (filename);
		filename[j-1] = EOS;	/* lop off newline */
	}

	/* map to lower case */
	for (j = 0; filename[j] != EOS; j++)
		if (isupper (filename[j]))
			filename[j] = tolower (filename[j]);

	sprintf (swt_filename, "%s_swt", filename);

	if (! Unix_mode && access (swt_filename, 4) == 0)
		strcpy (filename, swt_filename);
		/* SWT version of help file exists, use it */
	/* else
		use Unix or normal version of the help file */

	*status = OK;
	if ((fp = fopen (filename, "r")) == NULL)
	{
		*status = ERR;
		Errcode = ENOHELP;
	}
	else
	{
#ifdef u3b2
		/* 3B2 seems to have problems with stdio and malloc... */
		char buf[BUFSIZ];
		setbuf (fp, buf);
#endif

		/* status is OK */
		display_message (fp);	/* display the help script */
		fclose (fp);
	}
}


/* doopt --- interpret option command */

int doopt (lin, i)
char lin[];
int *i;
{
	int temp, line, stat;
	char tempstr[4];
	int ret;
	int dosopt ();
	int ctoi ();

	(*i)++;
	ret = ERR;

	switch (lin[*i]) {

	case 'g':		/* substitutes in a global can(not) fail */
	case 'G':
		if (lin[*i + 1] == '\n')
		{
			ret = OK;
			Globals = ! Globals;	/* toggle */
			if (Globals == YES)
				remark ("failed global substitutes continue");
			else
				remark ("failed global substitutes stop");
		}
		break;

	case 'h':
	case 'H':		/* do/don't use hardware insert/delete */
		if (lin[*i + 1] == '\n')
		{
			ret = OK;
			No_hardware = ! No_hardware;
			if (No_hardware == YES)
				remark ("no line insert/delete");
			else
				remark ("line insert/delete");
		}
		break;

	case 'k':		/* tell user if buffer saved or not */
	case 'K':
		if (lin[*i + 1] == '\n')
		{
			ret = OK;
			if (Buffer_changed == YES)
				remark ("not saved");
			else
				remark ("saved");
		}
		break;


	case 'z':	/* suspend the editor process */
	case 'Z':
		if (lin[*i + 1] == '\n')
		{
			ret = OK;
#ifdef BSD
			if (Catching_stops)
			{
				char *getenv ();

				/*
				 * order the test this way so that it fails
				 * immediately in the usual case
				 */
				if (getenv ("RSE") != NULL && At_gtics)
				{
					remark ("You may not suspend me");
					break;	/* switch */
				}
				else if (Buffer_changed == YES)
					fprintf (stderr, "WARNING: buffer not saved\n");
				kill (getpid(), SIGTSTP);
				/* stop_hdlr() will do all the work for us */
			}
#else
			remark ("process suspension not available");
#endif
		}
		break;

	case 't':	/* set or display tab stops for expanding tabs */
	case 'T':
		++(*i);
		if (lin[*i] == '\n')
		{
			remark (Tabstr);
			ret = OK;
		}
		else
		{
			ret = settab (&lin[*i]);
			if (ret == OK)
				strcpy (Tabstr, &lin[*i]);
			else	/* defaults were set */
				strcpy (Tabstr, "+4");
		}
		break;

	case 'w':	/* set or display warning column */
	case 'W':
		++(*i);
		if (lin[*i] == '\n')
			ret = OK;
		else
		{
			temp = ctoi (lin, i);
			if (lin[*i] == '\n')
				if (temp > 0 && temp < MAXLINE - 3)
				{
					ret = OK;
					Warncol = temp;
				}
				else
					Errcode = ENONSENSE;
		}
		if (ret == OK)
			saynum (Warncol);
		break;

	case '-':	/* fix window in place on screen, or erase it */
		++(*i);
		if (getnum (lin, i, &line, &stat) == EOF)
		{
			mesg ("", HELP_MSG);
			if (Toprow > 0)
			{
				Topln = max (1, Topln - Toprow);
				Toprow = 0;
				First_affected = Topln;
			}
			ret = OK;
		}
		else if (stat != ERR && lin[*i] == '\n')
			if (Toprow + (line - Topln + 1) < Cmdrow)
			{
				Toprow += line - Topln + 1;
				Topln = line + 1;
				for (temp = 0; temp < Ncols; temp++)
					load ('-', Toprow - 1, temp);
				if (Topln > Lastln)
					adjust_window (1, Lastln);
				if (Curln < Topln)
					Curln = min (Topln, Lastln);
				ret = OK;
			}
			else
				Errcode = EORANGE;
		break;

	case 'a':	/* toggle absolute line numbering */
	case 'A':
		if (lin[*i + 1] == '\n')
		{
			Absnos = ! Absnos;
			ret = OK;
		}
		break;

	case 'c':	/* toggle case option */
	case 'C':
		if (lin[*i + 1] == '\n')
		{
			ret = OK;
			Invert_case = ! Invert_case;
			if (Rel_a == 'A')
			{
				Rel_a = 'a';
				Rel_z = 'z';
			}
			else
			{
				Rel_a = 'A';
				Rel_z = 'Z';
			}
		}

		mesg (Invert_case ? "CASE" : "", CASE_MSG);
		break;

	case 'd':	/* set or display placement of "." after a delete */
	case 'D':
		if (lin[*i + 1] == '\n')
		{
			if (Ddir == FORWARD)
				remark (">");
			else
				remark ("<");
			ret = OK;
		}
		else if (lin[*i + 2] != '\n')
			Errcode = EODLSSGTR;
		else if (lin[*i + 1] == '>')
		{
			ret = OK;
			Ddir = FORWARD;
		}
		else if (lin[*i + 1] == '<')
		{
			ret = OK;
			Ddir = BACKWARD;
		}
		else
			Errcode = EODLSSGTR;
		break;

	case 'v':	/* set or display overlay column */
	case 'V':
		++(*i);
		if (lin[*i] == '\n')
		{
			if (Overlay_col == 0)
				remark ("$");
			else
				saynum (Overlay_col);
			ret = OK;
		}
		else
		{
			if (lin[*i] == '$' && lin[*i + 1] == '\n')
			{
				Overlay_col = 0;
				ret = OK;
			}
			else
			{
				temp = ctoi (lin, i);
				if (lin[*i] == '\n')
				{
					Overlay_col = temp;
					ret = OK;
				}
				else
					Errcode = ENONSENSE;
			}
		}
		break;

	case 'u':	/* set or display character for unprintable chars */
	case 'U':
		if (lin[*i + 1] == '\n')
		{
			ret = OK;
			tempstr[0] = tempstr[2] = '"';
			tempstr[1] = Unprintable;
			tempstr[3] = EOS;
			remark (tempstr);
		}
		else if (lin[*i + 2] == '\n')
		{
			if (lin[*i + 1] < ' ' || lin[*i + 1] >= DEL)
				Errcode = ENONSENSE;
			else 
			{
				ret = OK;
				if (Unprintable != lin[*i + 1])
				{
					Unprintable = lin[*i + 1];
					First_affected = Topln;
				}
			}
		}
		break;

	case 'l':	/* set or display line number display option */
	case 'L':
		if (lin[*i+1] == '\n')
		{
			Nchoise = EOS;
			ret = OK;
		}
		else if (lin[*i + 2] == '\n' && 
		    (lin[*i + 1] == CURLINE || lin[*i + 1] == LASTLINE
		    || lin[*i + 1] == TOPLINE))
		{
			Nchoise = lin[*i + 1];
			ret = OK;
		}
		else if (lin[*i + 1] == 'm' || lin[*i + 1] == 'M')
		{
			/* set or display the left margin */
			(*i)++;
			if (lin[*i + 1] == '\n')
			{
				saynum (Firstcol + 1);
				ret = OK;
			}
			else 
			{
				(*i)++;
				temp = ctoi (lin, i);
				if (lin[*i] == '\n')
					if (temp > 0 && temp < MAXLINE)
					{
						First_affected = Topln;
						Firstcol = temp - 1;
						ret = OK;
					}
					else
						Errcode = ENONSENSE;
			}
		}
		break;

	case 'f':	/* fortran (ugh, yick, gross) options */
	case 'F':
		if (lin[*i + 1] == '\n')
			ret = dosopt ("f");
		break;

	case 's':	/* set source options */
	case 'S':
		ret = dosopt (&lin[*i + 1]);
		break;

	case 'i':	/* set or display indent option */
	case 'I':
		++(*i);
		if (lin[*i] == '\n')
			ret = OK;
		else if ((lin[*i] == 'a' || lin[*i] == 'A') && lin[*i + 1] == '\n')
		{
			Indent = 0;
			ret = OK;
		}
		else
		{
			temp = ctoi (lin, i);
			if (lin[*i] == '\n')
				if (temp > 0 && temp < MAXLINE - 3)
				{
					ret = OK;
					Indent = temp;
				}
				else
					Errcode = ENONSENSE;
		}
		if (ret == OK)
			if (Indent > 0)
				saynum (Indent);
			else
				remark ("auto");
		break;

	case 'm':	/* toggle mail notification */
	case 'M':
		if (lin[*i + 1] == '\n')
		{
			Notify = ! Notify;	/* toggle notification */
			remark (Notify ? "notify on" : "notify off");
			ret = OK;
		}
		break;

	case 'p':		/* toggle pattern and command style */
	case 'P':		/* if additional letter there, it forces mode */
		ret = OK;
		switch (lin[*i + 1]) {
		case EOS:
		case '\n':	/* toggle */
			if (Unix_mode)
				goto no_unix;
				/* currently in Unix mode */
				/* switch to SWT style patterns */
			else
				goto yes_unix;
				/* currently in SWT mode */
				/* switch to Unix style patterns */

			break;

		case 's':	/* force SWT mode */
		case 'S':
	no_unix:	Unix_mode = NO;
			BACKSCAN = '\\';
			NOTINCCL = '~';
			XMARK = '!';
			ESCAPE = '@';
			break;

		case 'u':	/* force UNIX mode */
		case 'U':
	yes_unix:	Unix_mode = YES;
			BACKSCAN = '?';
			NOTINCCL = '^';
			XMARK = '~';
			ESCAPE = '\\';
			break;
		
		default:
			Errcode = EOWHAT;
			ret = ERR;
			goto out_of_here;
		}
		set_patterns (Unix_mode);
		mesg (Unix_mode ? "UNIX" : "SWT", MODE_MSG);
	out_of_here:
		break;
	
	case 'x':
	case 'X':	/* toggle tab compression */
		if (lin[*i + 1] == '\n')
		{
			ret = OK;
			Compress = ! Compress;
			mesg (Compress ? "XTABS" : "", COMPRESS_MSG);
		}
		break;

	case 'y':	/* encrypt files */
	case 'Y':
		if (lin[*i + 1] == '\n')
		{
		crypt_toggle:
			ret = OK;
			Crypting = ! Crypting;
			if (Crypting )
				do {
					getkey ();
					if (Key[0] == EOS)
						remark ("Empty keys are not allowed.\n");
				} while (Key[0] == EOS);
			else
				Key[0] = EOS;
		}
		else
		{
			register int j;

			ret = OK;
			(*i)++;		/* *i was the 'y' */
			while (isspace (lin[*i]) && lin[*i] != '\n')
				(*i)++;
			if (lin[*i] != '\n' && lin[*i] != EOS)
			{
				for (j = 0; lin[*i] != '\n' && lin[*i] != EOS;
				    j++, (*i)++)
					Key[j] = lin[*i];
				Key[j] = EOS;
				Crypting = YES;
			}
			else
				goto crypt_toggle;
		}
		mesg (Crypting ? "ENCRYPT" : "", CRYPT_MSG);
		break;

	default:
		Errcode = EOWHAT;

	}

	return (ret);
}


/* domark --- name lines line1 through line2 as kname */

int domark (kname)
char kname;
{
	int line;
	int ret;
	register LINEDESC *k;
	LINEDESC *getind ();

	if (Line1 <= 0)
	{
		Errcode = EORANGE;
		ret = ERR;
	}
	else
	{
		k = getind (Line1);
		for (line = Line1; line <= Line2; line++)
		{
			if (intrpt())
				return (ERR);
			k -> Markname = kname;
			k = NEXTLINE(k);
		}
		ret = OK;
	}
	return (ret);
}


/* doprnt --- set curln, locate window */

int doprnt (from, to)
int from, to;
{

	if (from <= 0)
	{
		Errcode = EORANGE;
		return (ERR);
	}

	adjust_window (from, to);
	Curln = to;
	return (OK);
}


/* doread --- read "file" after "line" */

int doread (line, file, tflag)
int line;
char *file;
int tflag;
{
	register int count, len, i;
	int ret;
	int strlen ();
	FILE *fd;
	FILE *fopen (), *crypt_open ();
	char lin1[MAXLINE], lin2[MAXLINE];
	char *fgets ();
	register LINEDESC *ptr;
	LINEDESC *sp_inject ();
	LINEDESC *getind ();
	char *expand_env ();

	file = expand_env (file);	/* expand $HOME, etc. */

	if (Savfil[0] == EOS)
	{
		strcpy (Savfil, file);
		mesg (Savfil, FILE_MSG);
	}

	if (Crypting)
		fd = crypt_open (file, "r");
	else
		fd = fopen (file, "r");

	if (fd == NULL)
	{
		ret = ERR;
		Errcode = ECANTREAD;
	}
	else
	{
		First_affected = min (First_affected, line + 1);
		ptr = getind (line);
		ret = OK;
#ifndef OLD_SCRATCH
		Curln = line;
#endif
		remark ("reading");
		for (count = 0; fgets (lin1, MAXLINE, fd) != NULL; count++)
		{
			if (intrpt ())
			{
				ret = ERR;
				break;
			}
			if (Compress == NO && tflag == NO)
				ptr = sp_inject (lin1, strlen (lin1), ptr);
			else
			{
				len = 0;
				for (i = 0; lin1[i] != EOS && len < MAXLINE - 1; i++)
					if (lin1[i] != '\t')
						lin2[len++] = lin1[i];
					else
						do
							lin2[len++] = ' ';
						while (len % 8 != 0 
						    && len < MAXLINE - 1);
				lin2[len] = EOS;
				if (len >= MAXLINE)
				{
					ret = ERR;
					Errcode = ETRUNC;
				}
				ptr = sp_inject (lin2, len, ptr);
			}
			if (ptr == NOMORE)
			{
				ret = ERR;
				break;
			}
		}
		if (Crypting)
			crypt_close (fd);
		else
			fclose (fd);
		saynum (count);
		Curln = line + count;
		svins (line, count);
	}

	return (ret);
}


/* dosopt --- set source language-related options */

int dosopt (lin)
char lin[];
{
	char lang[8];
	int i;
	int strbsr ();
	static struct {
		char *txt;
		int val;
	} ltxt[] = {    
		"",     1,
		"as",   2,
		"c",    3,
		"d",    1,
		"data", 1,
		"f",    4,
		"h",    3,
		"n",    1,
		"nr",   1,
		"nroff",1,
		"p",	3,
		"r",    3,
		"s",    2,
	};

	i = 0;
	getwrd (lin, &i, lang, 8);

	strmap (lang, 'a');

	i = strbsr ((char *)ltxt, sizeof (ltxt), sizeof (ltxt[0]), lang);
	if (i == EOF)
	{
		Errcode = ENOLANG;
		return (ERR);
	}

	switch (ltxt[i].val) {
	case 1:
		Warncol = 74;
		Rel_a = 'A';
		Rel_z = 'Z';
		Invert_case = NO;
		strcpy (Tabstr, "+4");
		settab (Tabstr);
		Compress = NO;
		break;

	case 3:
		Warncol = 74;
		Rel_a = 'A';
		Rel_z = 'Z';
		Invert_case = NO;
		strcpy (Tabstr, "+8");
		settab (Tabstr);
		Compress = YES;
		break;
	case 4:
		Warncol = 72;
		Rel_a = 'A';
		Rel_z = 'Z';
		Invert_case = NO;
		strcpy (Tabstr, "7+3");
		settab (Tabstr);
		Compress = YES;
		break;

	case 2:
		Warncol = 72;
		Rel_a = 'A';
		Rel_z = 'Z';
		Invert_case = NO;
		strcpy (Tabstr, "17+8");
		settab (Tabstr);
		Compress = YES;
		break;
	}

	mesg (Invert_case == YES ? "CASE" : "", CASE_MSG);
	mesg (Compress == YES ? "XTABS" : "", COMPRESS_MSG);

	return (OK);
}


/* dotlit --- transliterate characters */

int dotlit (sub, allbut)
char sub[];
int allbut;
{
	char new[MAXLINE];
	char kname;
	int collap, x, i, j, line, lastsub, status;
	int ret;
	LINEDESC *inx;
	LINEDESC *gettxt (), *getind ();

	ret = ERR;
	if (Line1 <= 0)
	{
		Errcode = EORANGE;
		return (ret);
	}

	if (First_affected > Line1)
		First_affected = Line1;

	lastsub = strlen (sub) - 1;
	if ((strlen (Tlpat)  - 1) > lastsub || allbut == YES)
		collap = YES;
	else
		collap = NO;

	for (line = Line1; line <= Line2; line++)
	{
		if (intrpt ())	/* check for interrupts */
			return (ERR);

		inx = gettxt (line);	/* get text of line into txt, return index */
		j = 0;
		for (i = 0; Txt[i] != EOS && Txt[i] != '\n'; i++)
		{
			x = xindex (Tlpat, Txt[i], allbut, lastsub);
			if (collap == YES && x >= lastsub && lastsub >= 0)	/* collapse */
			{
				new[j] = sub[lastsub];
				j++;
				for (i++; Txt[i] != EOS && Txt[i] != '\n'; i++)
				{
					x = xindex (Tlpat, Txt[i], allbut, lastsub);
					if (x < lastsub)
						break;
				}
			}
			if (Txt[i] == EOS || Txt[i] == '\n')
				break;
			if (x >= 0 && lastsub >= 0)	/* transliterate */
			{
				new[j] = sub[x];
				j++;
			}
			else if (x < 0)		/* copy */
			{
				new[j] = Txt[i];
				j++;
			}
			/* else
				delete */
		}

		if (Txt[i] == '\n')	/* add a newline, if necessary */
		{
			new[j] = '\n';
			j++;
		}
		new[j] = EOS;		/* add the EOS */

		kname = inx -> Markname;	/* save the markname */
		delete (line, line, &status);
		ret = inject (new);
		if (ret == ERR)
			break;
		inx = getind (Curln);
		inx -> Markname = kname;	/* set markname */
		ret = OK;
		Buffer_changed = YES;
	}

	return (ret);
}

/* doundo --- restore last set of lines deleted */

int doundo (dflg, status)
int dflg;
int *status;
{
	LINEDESC *l1, *l2, *k1, *k2;
	LINEDESC *getind ();
	int oldcnt;
	int nextln (), prevln ();

	*status = ERR;
	if (dflg == NO && Line1 <= 0)
		Errcode = EORANGE;
	else if (Limbo == NOMORE)
		Errcode = ENOLIMBO;
	else if (Line1 > Line2)
		Errcode = EBACKWARD;
	else if (Line2 > Lastln)
		Errcode = ELINE2;
	else
	{
		*status = OK;
		Curln = Line2;
#ifdef OLD_SCRATCH
		k1 = getind (Line2);
		k2 = getind (nextln (Line2));
		l1 = Limbo;
		l2 = l1 -> Prevline;
		relink (k1, l1, l2, k2);
		relink (l2, k2, k1, l1);
#else
		blkmove (Limbo - Buf, MAXBUF - 1, Line2);
#endif
		svins (Line2, Limcnt);
		oldcnt = Limcnt;
		Limcnt = 0;
		Limbo = NOMORE;
		Lastln += oldcnt;
		if (dflg == NO)
			delete (Line1, Line2, status);
		Curln += oldcnt;
		if (First_affected > Line1)
			First_affected = Line1;
	}

	return (*status);
}

/* dowrit --- write "from" through "to" into file */

int dowrit (from, to, file, aflag, fflag, tflag)
int from, to, aflag, fflag, tflag;
char *file;
{
	FILE *fd;
	FILE *fopen (), *crypt_open ();
	register int line, ret, i, j;
	int strcmp (), access ();
	char tabs[MAXLINE];
	register LINEDESC *k;
	LINEDESC *getind ();
	char *expand_env ();

	ret = ERR;
	if (from <= 0)
		Errcode = EORANGE;

	else
	{
		file = expand_env (file);	/* expand $HOME, etc. */

		if (aflag == YES)
		{
			if (Crypting)
				fd = crypt_open (file, "a");
			else
				fd = fopen (file, "a");
		}
		else if (strcmp (file, Savfil) == 0 || fflag == YES
		    || Probation == WRITECOM || access (file, 0) == -1)
		{
			if (Crypting)
				fd = crypt_open (file, "w");
			else
				fd = fopen (file, "w");
		}
		else
		{
			Errcode = EFEXISTS;
			Probation = WRITECOM;
			return (ret);
		}
		if (fd == NULL)
			Errcode = ECANTWRITE;
		else
		{
			ret = OK;
			remark ("writing");
			k = getind (from);
			for (line = from; line <= to; line++)
			{
				if (intrpt ())
					return (ERR);
				gtxt (k);
				if (Compress == NO && tflag == NO)
					fputs (Txt, fd);
				else
				{
					for (i = 0; Txt[i] == ' '; i++)
						;
					for (j = 0; j < i / 8; j++)
						tabs[j] = '\t';
					tabs[j] = EOS;
					fputs (tabs, fd);
					fputs (&Txt[j * 8], fd);
				}
				k = NEXTLINE(k);
			}
			if (Crypting)
				crypt_close (fd);
			else
				fclose (fd);
			sync ();	/* just in case the system crashes */
			saynum (line - from);
			if (from == 1 && line - 1 == Lastln)
				Buffer_changed = NO;
		}
	}
	return (ret);
}

/* expand_env --- expand environment variables in file names */

char *expand_env (file)
register char *file;
{
	register int i, j, k;	/* indices */
	char *getenv ();
	char var[MAXLINE];		/* variable name */
	char *val;			/* value of environment variable */
	static char buf[MAXLINE * 2];	/* expanded file name, static to not go away */


	i = j = k = 0;
	while (file[i] != EOS)
	{
		if (file[i] == ESCAPE)
		{
			if (file[i+1] == '$')
			{
				buf[j++] = file[++i];	/* the actual $ */
				i++;	/* for next time around the loop */
			}
			else
				buf[j++] = file[i++];	/* the \ */
		}
		else if (file[i] != '$')	/* normal char */
			buf[j++] = file[i++];
		else			/* environment var */
		{
			i++;	/* skip $ */
			k = 0;
			while (file[i] != '/' && file[i] != EOS)
				var[k++] = file[i++];	/* get var name */
			var[k] = EOS;

			if ((val = getenv (var)) != NULL)
				for (k = 0; val[k] != EOS; k++)
					buf[j++] = val[k];
					/* copy val into file name */
			/* else
				var not in enviroment; leave file
				name alone, multiple slashes are legal */
		}
	}
	buf[j] = EOS;

	return (buf);
}

/* crypt_open -- run files through crypt */

FILE *crypt_open (file, mode)
char *file, *mode;
{
	char buf[MAXLINE];
	FILE *fp, *popen ();

	if (! Crypting)
		return (NULL);

	while (Key[0] == EOS)
	{
		getkey ();
		if (Key[0] == EOS)
			printf ("The key must be non-empty!\n");
	}

	switch (mode[0]) {
	case 'r':
		sprintf (buf, "crypt %s < %s", Key, file);
		fp = popen (buf, "r");
		return (fp);		/* caller checks for NULL or not */
		break;

	case 'w':
		sprintf (buf, "crypt %s > %s", Key, file);
		fp = popen (buf, "w");
		return (fp);		/* caller checks for NULL or not */
		break;

	case 'a':
		sprintf (buf, "crypt %s >> %s", Key, file);
		fp = popen (buf, "w");
		return (fp);		/* caller checks for NULL or not */
		break;
	
	default:
		return (NULL);
	}
}

crypt_close (fp)
FILE *fp;
{
	pclose (fp);
}

/* getkey -- get an encryption key from the user */

#define repeat		do
#define until(cond)	while(!(cond))

getkey ()
{
	char *getpass ();	/* get input w/out echoing on screen */

	clrscreen ();		/* does NOT wipe out Screen_image */
	tflush ();

	ttynormal ();

	repeat
	{
		strcpy (Key, getpass ("Enter encryption key: "));
		if (strcmp (Key, getpass ("Again: ")) != 0)
		{
			Key[0] = EOS;
			fprintf (stderr, "didn't work. try again.\n");
		}
		/* else
			all ok */
	} until (Key[0] != EOS);

	ttyedit ();

	restore_screen ();
}
