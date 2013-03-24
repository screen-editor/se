# 
# $Header: makefile,v 1.2 86/07/11 15:23:24 osadr Exp $
# 
# $Log:	makefile,v $
# Revision 1.2  86/07/11  15:23:24  osadr
# Changed to ensure the proper 'ar' program is used.
# 
# Revision 1.1  86/05/06  13:31:30  osadr
# Initial revision
# 
# 
# 
# makefile for libchangetty.a -- terminal resetting functions

TARGET= libchangetty.a
SRCS= changetty.c
OBJS= changetty.o
PRINTS= $(SRCS) makefile

CFLAGS= -O `cat ../flags`

$(TARGET): $(OBJS) ../flags
	/bin/ar ruv $(TARGET) $(OBJS)
	if test -r /usr/bin/ranlib; then /usr/bin/ranlib $(TARGET); fi

clean:
	rm -fr $(OBJS)

clobber: clean
	rm -fr $(TARGET) print2

print:
	prt $(PRINTS) | lpr -b 'tty lib'
	touch print2

print2: $(PRINTS)
	prt $? | lpr -b 'new tty lib'
	touch print2
