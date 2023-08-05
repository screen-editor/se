/*
** docmd2.h
*/

#ifndef __DOCMD2_H
#define __DOCMD2_H

int append (int line, char str[]);
int copy (int line3);
int se_delete (int from, int to, int *status);
int join (char sub[]);
int move (int line3);
void overlay (int *status);
int subst (char sub[], int gflag, int glob);
void uniquely_name (char kname, int *status);
int draw_box (char lin[], int *i);
void dfltsopt (char name[]);
int doshell (char lin[], int *pi);

#endif
