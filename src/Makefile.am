# This file is in the public domain.

AM_CPPFLAGS = -DHELP_DIR='"$(datadir)/@PACKAGE@/help"' \
	-DSHELL='"$(SHELL)"'

bin_PROGRAMS = se scriptse

se_SOURCES = \
	ascii.h				\
	changetty.c changetty.h		\
	constdefs.h			\
	docmd1.c docmd1.h		\
	docmd2.c docmd2.h		\
	edit.c edit.h			\
	extern.h			\
	main.c main.h			\
	misc.c misc.h			\
	pat.c pat.h			\
	scratch.c scratch.h		\
	screen.c screen.h		\
	se.h				\
	term.c term.h

se_LDADD = $(LIBTERMCAP)

scriptse_SOURCES = \
	scriptse.c
