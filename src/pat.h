/*
** pat.h
**
** This file is in the public domain.
*/

#ifndef __PAT_H
#define __PAT_H

int match(char lin[], char pat[]);
int amatch(char lin[], int from, char pat[], int tagbeg[], int tagend[]);
int omatch(char lin[], char **adplin, char *ppat);
int locate(char c, char *ppat);
int patsiz(char *ppat);
int makpat(char arg[], int from, char delim, char pat[]);
int getccl(char arg[], int *pasub, char pat[], int *pindex);
void stclos(char pat[], int *ppatsub, int *plastsub);
int maksub(char arg[], int from, char delim, char sub[]);
void catsub(char lin[], int from[], int to[], char sub[], char _new[], int *k, int maxnew);
void filset(char delim, char array[], int *pasub, char set[], int *pindex, int maxset);
void dodash(char valid[], char array[], int *pasub, char set[], int *pindex, int maxset);
int addset(char c, char set[], int *pindex, int maxsiz);
char esc(char array[], int *pindex);
int start_tag(char *arg, int *argsub);
int stop_tag(char *arg, int *argsub);

#endif
