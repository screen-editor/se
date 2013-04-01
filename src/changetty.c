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
**
** This file is in the public domain.
*/

#include "config.h"

#include "ascii.h"
#include "constdef.h"
#include "changetty.h"
#include "main.h"

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#else
/* fallback to termio */
#ifdef HAVE_TERMIO_H
#include <termio.h>
#endif
#endif

extern short ospeed;		/* from the termlib library */
static int set_ospeed = SE_NO;

static TTYINFO OldTtyInfo;	/* initial state of terminal */
static TTYINFO NewTtyInfo;	/* modified state for editing */

static int first = SE_YES;	/* first time anything called */


void init(void)
{
	if (gttyinfo(1, &OldTtyInfo) == -1)	/* get current state */
	{
		error (SE_NO, "couldn't get TTY info from system. get help!\n");
	}

	NewTtyInfo = OldTtyInfo;	/* copy it */
	mttyinfo(&NewTtyInfo);		/* change, but don't reset terminal */
	/* really should check the return value here ... */
}


void ttyedit(void)	/* set the terminal to correct modes for editing */
{
	if (first == SE_YES)
	{
		first = SE_NO;
		init();
	}

	sttyinfo(1, &NewTtyInfo);	/* make the change */
	/* really should check the return value here too ... */
}

void ttynormal(void)	/* set the terminal to correct modes for normal use */
{
	if (first)
	{
		first = SE_NO;
		init();
	}

	sttyinfo(1, &OldTtyInfo);	/* make the change */
}

/* getspeed --- find out the terminal speed in characters/second */
/*		this routine only used if terminal types are hardwired */
/*		into se, however, since it will be in an archive, the */
/*		loader won't grab it if it isn't needed. */

int getspeed(int fd)
{
	int i;
	TTYINFO ttybuf;
	static struct brstruct {
		int unixrate;
		int cps;
		int baudrate;
	} stab[] = {
		{B0,	0,	0},
		{B50,	5,	50},
		{B75,	8,	75},
		{B110,	10,	110},
		{B134,	14,	134},
		{B150,	15,	150},
		{B200,	20,	200},
		{B300,	30,	300},
		{B600,	60,	600},
		{B1200,	120,	1200},
		{B1800,	180,	1800},
		{B2400,	240,	2400},
		{B4800,	480,	4800},
		{B9600,	960,	9600}
	};

	if (first)		/* might as well set it here, too */
	{
		first = SE_NO;
		init();
	}

	if (gttyinfo(fd, &ttybuf) == -1)
	{
		return 960;
	}

	for (i = 0; i < sizeof(stab) / sizeof(struct brstruct); i++)
	{
		if (stab[i].unixrate == (ttybuf.c_cflag & 017))
		{
			return stab[i].cps;
		}
	}

	return 960;
}

/* gttyinfo --- make all necessary calls to obtain terminal status */

int gttyinfo(int fd, TTYINFO *buf)
{
#ifdef HAVE_TCGETATTR
	if (tcgetattr(fd, buf))
	{
		return -1;
	}
#else
	if (ioctl(fd, TCGETA, buf) < 0)
	{
		return -1;
	}
#endif
	if (set_ospeed == SE_NO)
	{
		set_ospeed = SE_YES;
		ospeed = buf->c_cflag & 017;
	}

	return 0;
}

/* mttyinfo --- modify a TTYINFO structure for interactive operation */

int mttyinfo(TTYINFO *buf)
{
	buf->c_cc[0] = DLE;	/* interrupt */
	buf->c_cc[1] = -1;	/* ignore quit */
	buf->c_cc[4] = 1;	/* min # chars to read */
	buf->c_cc[5] = 1;	/* min time to wait */

	buf->c_iflag &= ~(INLCR|IGNCR|ICRNL);	/* allow CR to come thru */
	buf->c_lflag |= ISIG|NOFLSH;	/* keep these bits */
	buf->c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL
#ifdef XCASE
				|XCASE
#endif
			);

	return 0;
}

/* sttyinfo --- make all necessary calls to set terminal status */

int sttyinfo(int fd, TTYINFO *buf)
{
#ifdef HAVE_TCSETATTR
	if (tcsetattr(fd, TCSANOW, buf) < 0)
#else
	if (ioctl(fd, TCSETAW, buf) < 0)
#endif
	{
		return	-1;
	} else {
		return 0;
	}
}

