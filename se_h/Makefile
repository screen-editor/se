# 
# $Header: Makefile,v 1.2 86/10/07 15:15:04 arnold Exp $
# 
# $Log:	Makefile,v $
# Revision 1.2  86/10/07  15:15:04  arnold
# Updated to reflect current set of help files.
# 
# Revision 1.1  86/05/06  14:04:17  osadr
# Initial revision
# 
# 
# 
# makefile for se help scripts

DOCS= README

FILES= a abt ah arg2 args b bang bug c cc ccmisc chardel charins colon \
	com comdir comsyn d dir e eh elp equal f g h i j k l list ln m \
	motion n o o- oa oc od of og oh oi ok ol olm om os ot ou ov ow \
	ox oy oz p pat patex q quote r rpt s scan serc shell specl t \
	termchar tilde u v w x y z

# destination for final	version	of help	scripts.
INSTALL=/usr/local/lib/se_h

install: existsall detab
	./Install $(INSTALL) $(FILES)

existsall: $(FILES) $(DOCS) Install
	@echo this message indicates all the files exist, otherwise make would die.

detab: detab.c
	cc -O -o detab detab.c

clean:
	rm -f detab print2

clobber: clean


print:
	prt $(DOCS) Makefile Install $(FILES) | lpr -b 'se help'
	touch print2

print2:	$(DOCS)	Makefile Install $(FILES)
	prt $? | lpr -b	'new se	help'
	touch print2
