/*
** scratch.h
*/

#ifndef __SCRATCH_H
#define __SCRATCH_H

#include <stdio.h>

LINEDESC *alloc(LINEDESC **ptr);
void bump(int *line, LINEDESC **ix, int way);
void closef(int fd);
void clrbuf(void);
void garbage_collect(void);
LINEDESC *se_gettxt(int line);
int gtxt(LINEDESC *ptr);
int inject(char lin[]);
int maklin(char lin[], int i, LINEDESC **newind);
void makscr(int *fd, char str[], size_t strsize);
int nextln(int line);
int prevln(int line);
int readf(char buf[], int count, int fd);
int seekf(long pos, int fd);
void mkbuf(void);
LINEDESC *sp_inject(char lin[], int len, LINEDESC *line);
int writef(char buf[], int count, int fd);
LINEDESC *getind(int line);
void blkmove(int n1, int n2, int n3);
void reverse(int n1, int n2);

#endif
