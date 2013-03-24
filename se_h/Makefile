# makefile for se help scripts

DOCS= README

FILES= a abt ah arg2 args b bang bug c cc ccmisc chardel charins colon \
	com com_swt comdir comdir_swt comsyn d dir e eh elp equal f g h i \
	j k l list ln ln_swt m m_swt motion n o o- oa oc od of og oh oi ok ol \
	olm om op os ot ou ov ow ox oy oz p pat pat_swt patex patex_swt q \
	quote r rpt s s_swt scan scan_swt serc shell shell_swt specl t t_swt \
	termchar tilde u v w x y y_swt z

# destination for final	version	of help	scripts.
INSTALL=/usr/local/lib/se_h

install: existsall detab
	Install $(INSTALL) $(FILES)

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
