/*
** edit.h
**
** This file is in the public domain.
*/

#ifndef __EDIT_H
#define __EDIT_H

#include <stdio.h>

void edit(int argc, char *argv[]);
int getlst(char lin[], int *i, int *status);
int getnum(char lin[], int *i, int *pnum, int *status);
int getone(char lin[], int *i, int *num, int *status);
int ckglob(char lin[], int *i, int *status);
int doglob(char lin[], int *i, int *cursav, int *status);
int ckchar(char ch, char altch, char lin[], int *i, int *flag, int *status);
int ckp(char lin[], int i, int *pflag, int *status);
int ckupd(char lin[], int *i, char cmd, int *status);
void defalt(int def1, int def2);
int getfn(char lin[], int i, char filename[], size_t filenamesize);
int getkn(char lin[], int *i, char *kname, char dfltnm);
int getrange(char array[], int *k, char set[], int size, int *allbut);
int getrhs(char lin[], int *i, char sub[], size_t subsize, int *gflag);
int getstr(char lin[], int *i, char dst[], int maxdst);
int getwrd(char line[], int *i, char word[], int size);
int knscan(int way, int *num);
int makset(char array[], int *k, char set[], size_t size);
int optpat(char lin[], int *i);
int ptscan(int way, int *num);
int settab(char str[]);
void serc(void);
int serc_safe (char *path);
char *sysname(void);
void log_usage(void);

#endif
