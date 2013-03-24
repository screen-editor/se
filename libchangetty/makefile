# makefile for libchangetty.a -- terminal resetting functions

TARGET= libchangetty.a
SRCS= changetty.c
OBJS= changetty.o
PRINTS= $(SRCS) makefile

CFLAGS= -O `cat ../flags`

$(TARGET): $(OBJS) ../flags
	ar ruv $(TARGET) $(OBJS)
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
