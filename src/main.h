/*
** main.h
*/

#ifndef __MAIN_H
#define __MAIN_H

int main(int argc, char *argv[]);
void error(int coredump, char *msg);
void initialize(int argc, char *argv[]);
int intrpt(void);
void int_hdlr(int sig);
void hup_hdlr(int sig);
void hangup(void);
void mswait(void);
void print_verbose_err_msg(void);
void usage(void);

#endif
