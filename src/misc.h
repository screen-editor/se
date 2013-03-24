/*
** misc.h
**
** This file is in the public domain.
*/

#ifndef __MISC_H
#define __MISC_H

void cprow(int from, int to);
int se_index(char str[], char c);
int strbsr(char *stab, int tsize, int esize, char str[]);
int strmap(char str[], int ul);
int xindex(char array[], char c, int allbut, int lastto);
int ctoi(char str[], int *i);
void move_(char *here, char *there, int l);
int twrite(int fd, char *buf, int len);
int tflush(void);
char *basename(char *str);

#endif
