/*
** docmd1.h
**
** This file is in the public domain.
*/

#ifndef __DOCMD1_H
#define __DOCMD1_H

#include <stdio.h>

int docmd(char lin[], int i, int glob, int *status);
void dohelp(char lin[], int *i, int *status);
int doopt(char lin[], int *i);
int domark(char kname);
int doprnt(int from, int to);
int doread(int line, char *file, int tflag);
int dosopt(char lin[]);
int dotlit(char sub[], int allbut);
int doundo(int dflg, int *status);
int dowrit(int from, int to, char *file, int aflag, int fflag, int tflag);
char *expand_env(char *file);
FILE *crypt_open(char *file, char *mode);
void crypt_close(FILE *fp);
void getkey(void);

#endif
