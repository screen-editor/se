/*
** screen.h
**
** This file is in the public domain.
*/

#ifndef __SCREEN_H
#define __SCREEN_H

void clrrow(int row);
int display_message(FILE *fp);
void getcmd(char *lin, int col1, int *curpos, char *termchar);
int cread(void);
int scan_char(char chr, int wrap, int cursor, int nlpos, char *lin, int *status);
int scan_tab(char chr, int cursor, int *status);
void gobble(int len, int cursor, int *status, int *nlpos, char *lin);
void insert(int len, int *nlpos, int *status, int cursor, char *lin);
void set_cursor(int pos, int *status, int *cursor, int *nlpos, char *lin);
void litnnum(char *lit, int num, int type);
void load(char chr, int row, int col);
void loadstr(char *str, int row, int stcol, int endcol);
void mesg(char *s, int t);
void prompt(char *str);
void restore_screen(void);
void saynum(int n);
void updscreen(void);
void warn_deleted(int from, int to);
void watch(void);
void adjust_window(int from, int to);
void svdel(int ln, int n);
void svins(int ln, int n);
void fixscreen(void);

#endif
