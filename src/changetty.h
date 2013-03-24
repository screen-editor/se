/*
** changetty.h
**
** This file is in the public domain.
*/

#ifndef __CHANGETTY_H
#define __CHANGETTY_H

#ifdef HAVE_TERMIOS_H
#include <termios.h>
typedef struct termios TTYINFO;
#else
/* fallback to termio */
#ifdef HAVE_TERMIO_H
#include <termio.h>
typedef struct termio TTYINFO;
#endif /* HAVE_TERMIO_H */
#endif /* HAVE_TERMIOS_H */

void init(void);
void ttyedit(void);
void ttynormal(void);
int getspeed(int fd);
int gttyinfo(int fd, TTYINFO *buf);
int mttyinfo(TTYINFO *buf);
int sttyinfo(int fd, TTYINFO *buf);

#endif
