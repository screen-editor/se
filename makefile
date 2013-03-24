# makefile for the Georgia Tech Screen Editor, 'se'

HEADERS= ascii.h constdefs.h extern.h se.h

SRCS= docmd1.c docmd2.c edit.c main.c misc.c scratch.c screen.c term.c
OBJS= docmd1.o docmd2.o edit.o main.o misc.o scratch.o screen.o term.o

LIBRARIES= libchangetty/libchangetty.a pat/libpat.a

LIBS=`echo $(LIBRARIES); if egrep 'DBSD|US5R2' flags > /dev/null; then echo -ltermlib; else echo -lcurses; fi`

DOCS= makefile README changes.made
MANS= scriptse.1 se.1

CFLAGS= -O `cat flags`
LDFLAGS=

# On BSD systems, force make to use the right shell for commands
SHELL=/bin/sh

###########################################################################
# Begin system dependant macro definitions

# PR is to print the files nicely.  Use pr -n if available, or else just pr
# I use a private utility called 'prt'
PR=prt

# NROFF is for nroffing.  we use the System V nroff. 
NROFF=/usr/5bin/nroff

# MANSEC is where to put the manual pages. Use 'l' for local, otherwise '1'.
MANSEC=l

# DESTBIN is where se and scriptse will go
DESTBIN= /usr/local/bin

# OWNER and GROUP are the owner and group respectively
OWNER= root
GROUP= admin

# INSTALL is the program to do the installation, use cp for real work
INSTALL= cp

# CHOWN changes the owner.
CHOWN= /etc/chown

# CHGRP changes the group.
CHGRP= chgrp

# CHMOD will change permissions.
CHMOD= chmod

########
# other things to change:
#
# on non-BSD systems, change the 'lpr' below to 'lp'
########

# Begin list of dependencies

all: se scriptse se.1
	@echo all done

se: $(OBJS) $(LIBRARIES)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

$(OBJS): $(HEADERS) flags

flags: where
	where > flags

libchangetty/libchangetty.a: libchangetty/changetty.c
	cd libchangetty; make

pat/libpat.a: pat/pat.c
	cd pat; make

scriptse: scriptse.c
	$(CC) -O scriptse.c -o scriptse

se.1: se.m4 flags
	(m4munge $(CFLAGS) ; cat se.m4) | m4 | sed '/^$$/d' > se.1

install: all $(MANS)
	$(INSTALL) se $(DESTBIN)
	$(CHOWN) $(OWNER) $(DESTBIN)/se
	$(CHGRP) $(GROUP) $(DESTBIN)/se
	$(CHMOD) 711 $(DESTBIN)/se 
	$(INSTALL) scriptse $(DESTBIN)
	$(CHOWN) $(OWNER) $(DESTBIN)/scriptse
	$(CHGRP) $(GROUP) $(DESTBIN)/scriptse
	$(CHMOD) 711 $(DESTBIN)/scriptse 
	$(INSTALL) se.1 /usr/man/man$(MANSEC)/se.$(MANSEC)
	$(CHOWN) $(OWNER) /usr/man/man$(MANSEC)/se.$(MANSEC)
	$(CHGRP) $(GROUP) /usr/man/man$(MANSEC)/se.$(MANSEC)
	$(CHMOD) 644 /usr/man/man$(MANSEC)/se.$(MANSEC)
	$(INSTALL) scriptse.1 /usr/man/man$(MANSEC)/scriptse.$(MANSEC)
	$(CHOWN) $(OWNER) /usr/man/man$(MANSEC)/scriptse.$(MANSEC)
	$(CHGRP) $(GROUP) /usr/man/man$(MANSEC)/scriptse.$(MANSEC)
	$(CHMOD) 644 /usr/man/man$(MANSEC)/scriptse.$(MANSEC)
	cd se_h; make install
	
print:
	$(PR) $(HEADERS) $(SRCS) $(DOCS) $(MANS) | lpr

printman: $(MANS)
	$(NROFF) -man $(MANS) | col | lpr

print2:	$(HEADERS) $(SRCS) $(DOCS) $(MANS)
	$(PR) $? | lpr

printall: printman print
	cd pat; make print
	cd libchangetty; make print
	cd se_h; make print

clean:
	rm -f *.o print2
	cd pat; make clean
	cd libchangetty; make clean

clobber: clean
	rm -f se scriptse flags se.1
	cd pat; make clobber
	cd libchangetty; make clobber
	cd se_h; make clobber
