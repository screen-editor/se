/*
** term.h
**
** This file is in the public domain.
*/

#ifndef __TERM_H
#define __TERM_H

int outc(int c);
void getdescrip(void);
int setcaps(char *term);
void t_init(void);
void t_exit(void);
void winsize(int sig);
void send(char chr);
void clrscreen(void);
void position_cursor(int row, int col);
void setscreen(void);
void inslines(int row, int n);
void dellines(int row, int n);
int hwinsdel(void);
void clear_to_eol(int row, int col);
int se_set_term(char *type);
void brighton(void);
void brightoff(void);

#endif
