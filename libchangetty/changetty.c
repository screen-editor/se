/*
** changetty.c
**
** Localize in one place all the data and functions
** needed to change to and from cbreak mode for
** the se screen editor.
**
** Only functions available to rest of se are:
** ttyedit(), ttynormal(), and getspeed().
**
** If USG is defined, we use the System V TTY driver.
** Otherwise (the default) we use the Berkeley terminal driver.
**
** If we are using System V, then the Release 2 version does not
** need ospeed.  If not release 2, we assume Release 1 that someone
** have moved the BSD termlib to.
*/

#include "../ascii.h"
#include "../constdefs.h"

#ifdef USG
#include <termio.h>
#else
#include <sgtty.h>
#endif

#if defined (BSD) || ! defined (S5R2)
extern short ospeed;		/* from the termlib library */
static int set_ospeed = NO;
#endif

#ifdef USG
/* all the info needed for the System V terminal driver */

typedef struct termio TTYINFO;	/* S5 control flags */
#else
/* all the info needed for the Berkeley terminal driver */

typedef struct junk {		/* terminal information */
	struct sgttyb sgttyb;	/* V7 control flags */
	struct tchars tchars;	/* V7 control characters */
	short local;		/* local mode settings */
	struct ltchars ltchars;	/* local control characters */
	} TTYINFO;
#endif

static TTYINFO OldTtyInfo;	/* initial state of terminal */
static TTYINFO NewTtyInfo;	/* modified state for editing */

static int first = YES;		/* first time anything called */


static init()
{
	if (gttyinfo(1, &OldTtyInfo) == -1)	/* get current state */
		error ("couldn't get TTY info from system. get help!\n");

	NewTtyInfo = OldTtyInfo;	/* copy it */
	mttyinfo(&NewTtyInfo);		/* change, but don't reset terminal */
	/* really should check the return value here ... */
}


ttyedit()	/* set the terminal to correct modes for editing */
{
	if (first == YES)
	{
		first = NO;
		init();
	}

	sttyinfo(1, &NewTtyInfo);	/* make the change */
	/* really should check the return value here too ... */
}

ttynormal()	/* set the terminal to correct modes for normal use */
{
	if (first)
	{
		first = NO;
		init();
	}

	sttyinfo(1, &OldTtyInfo);	/* make the change */
}

/* getspeed --- find out the terminal speed in characters/second */
/*		this routine only used if terminal types are hardwired */
/*		into se, however, since it will be in an archive, the */
/*		loader won't grab it if it isn't needed. */

int getspeed(fd)
int fd;
{
	register int i;
	TTYINFO ttybuf;
	static struct brstruct {
		int unixrate;
		int cps;
		int baudrate;
		} stab[] = {
			B0,	0,	0,
			B50,	5,	50,
			B75,	8,	75,
			B110,	10,	110,
			B134,	14,	134,
			B150,	15,	150,
			B200,	20,	200,
			B300,	30,	300,
			B600,	60,	600,
			B1200,	120,	1200,
			B1800,	180,	1800,
			B2400,	240,	2400,
			B4800,	480,	4800,
			B9600,	960,	9600
		};

	if (first)		/* might as well set it here, too */
	{
		first = NO;
		init();
	}

	if (gttyinfo(fd, &ttybuf) == -1)
		return 960;

	for (i = 0; i < sizeof(stab) / sizeof(struct brstruct); i++)
#ifdef USG
		if (stab[i].unixrate == (ttybuf.c_cflag & 017))
#else
		if (stab[i].unixrate == (ttybuf.sgttyb.sg_ospeed))
#endif
			return stab[i].cps;

	return 960;
}

/* gttyinfo --- make all necessary calls to obtain terminal status */

static int gttyinfo(fd, buf)
int fd;		/* file descriptor of terminal */
TTYINFO *buf;	/* terminal info buffer to be filled in */
{
#ifdef USG
	if (ioctl(fd, TCGETA, buf) < 0)
#else
	if (gtty(fd, &(buf->sgttyb))  < 0
		|| ioctl(fd, TIOCGETC, &(buf->tchars)) < 0
		|| ioctl(fd, TIOCLGET, &(buf->local)) < 0
		|| ioctl(fd, TIOCGLTC, &(buf->ltchars)) < 0)
#endif
	{
		return -1;
	}

#if defined (BSD) || ! defined (S5R2)
	if (set_ospeed == NO)
	{
		set_ospeed = YES;
#ifdef BSD
		ospeed = (buf->sgttyb).sg_ospeed;
#else
		ospeed = buf->c_cflag & 017;	/* tty speed, see termio(7) */
#endif
	}
#endif

	return 0;
}

/* mttyinfo --- modify a TTYINFO structure for interactive operation */

static int mttyinfo(buf)
TTYINFO *buf;		/* buffer containing TTYINFO to be modified */
{
#ifdef USG
	buf->c_cc[0] = DLE;	/* interrupt */
	buf->c_cc[1] = -1;	/* ignore quit */
	buf->c_cc[4] = 1;	/* min # chars to read */
	buf->c_cc[5] = 1;	/* min time to wait */

	buf->c_iflag &= ~(INLCR|IGNCR|ICRNL);	/* allow CR to come thru */
	buf->c_lflag |= ISIG|NOFLSH;	/* keep these bits */
	buf->c_lflag &= ~(ICANON|XCASE|ECHO|ECHOE|ECHOK|ECHONL);
#else
#ifdef GITVAX
	static struct tchars newtchars = {DLE, -1, -1, -1, EOT, -1};
	/* allows control-s and -q to be control charactes that se sees */
#else
	static struct tchars newtchars = {DLE, -1, DC1, DC3, EOT, -1};
#endif
	static struct ltchars newltchars = {-1, -1, -1, -1, -1, -1};

	buf->sgttyb.sg_flags |= (CBREAK);    /* keep these bits */
	buf->sgttyb.sg_flags &= ~(ECHO|CRMOD|LCASE|RAW|ALLDELAY);
	buf->tchars = newtchars;
	/* buf->local |= (LLITOUT);	/* Dan Forsyth says to comment out */
	buf->local &= ~(LCRTBS|LCRTERA|LPRTERA|LTOSTOP|LFLUSHO|LCRTKIL|
#ifndef BSD4_2
				LINTRUP|
#endif
				LCTLECH|LPENDIN);
	buf->ltchars = newltchars;
#endif
	return 0;
}

/* sttyinfo --- make all necessary calls to set terminal status */

static int sttyinfo(fd, buf)
int fd;		/* file descriptor of terminal */
TTYINFO *buf;	/* terminal info buffer */
{
#ifdef USG
	if (ioctl(fd, TCSETAW, buf) < 0)
#else
	if (ioctl(fd, TIOCSETN, &(buf->sgttyb)) < 0
			|| ioctl(fd, TIOCSETC, &(buf->tchars)) < 0
			|| ioctl(fd, TIOCLSET, &(buf->local)) < 0
			|| ioctl(fd, TIOCSLTC, &(buf->ltchars)) < 0)
#endif
		return	-1;

	return 0;
}
