# 
# $Header: makefile,v 1.2 86/07/11 15:25:14 osadr Exp $
# 
# $Log:	makefile,v $
# Revision 1.2  86/07/11  15:25:14  osadr
# Changed to ensure that the proper 'ar' program gets used.
# 
# Revision 1.1  86/05/06  13:33:07  osadr
# Initial revision
# 
# 
# 
# makefile for pattern library for 'se'

CFLAGS=-O

PR=pr

libpat.a: pat.o
	/bin/ar ruv libpat.a pat.o
	if test -r /usr/bin/ranlib; then /usr/bin/ranlib libpat.a; fi

pat.o: pat.c

install: libpat.a
	cp libpat.a /usr/lib

print:
	$(PR) pat.c makefile | lpr -b 'pat lib'
	touch print2

print2: pat.c makefile
	$(PR) $? | lpr -b 'new pat lib'
	touch print2

clean:
	rm -f pat.o

clobber: clean
	rm -f libpat.a print2
