.\" 
.\" $Header: se.m4,v 1.6 86/11/12 11:34:49 arnold Exp $
.\" 
.\" $Log:	se.m4,v $
.\" Revision 1.6  86/11/12  11:34:49  arnold
.\" Fixed use of `BSD'. Changed discussion of windows. ADR.
.\" 
.\" Revision 1.5  86/09/19  12:14:12  arnold
.\" Typo fix.
.\" 
.\" Revision 1.4  86/07/17  17:52:34  arnold
.\" Added discussion of windowing systems, and AUTHORS section.
.\" 
.\" Revision 1.3  86/07/11  15:17:50  osadr
.\" Removal of Georgia Tech specific items, and some cleaning up.
.\" 
.\" Revision 1.2  86/05/27  17:50:37  osadr
.\" Fix to quote an m4 keyword which was getting deleted.
.\" 
.\" Revision 1.1  86/05/06  13:41:04  osadr
.\" Initial revision
.\" 
.\" 
.\" 
.ie t \{ .de CW
.vs 10.5p
.ta 16m/3u 32m/3u 48m/3u 64m/3u 80m/3u 96m/3u
.nf
.ft CW
.cs CW 18
.. \}
.el \{ .de CW
.nf
.ft B
.. \}
.ie t \{ .de CN
.ta 0.5i 1i 1.5i 2i 2.5i 3i 3.5i 4i 4.5i 5i 5.5i 6i
.vs
.fi
.cs CW
.ft
.. \}
.el \{ .de CN
.fi
.ft
.. \}
changequote([])
.if n .ds lq ""
.if n .ds rq ""
.if t .ds lq ``
.if t .ds rq ''
changequote
.TH SE 1 local
.SH NAME
se \- screen editor
.SH SYNOPSIS
.B se
ifelse(HARD_TERMS,YES,
[
.B \-t
.I term
],)
[
.B \-\-acdfghiklmstuvwxyz
] [ file ... ]
.SH DESCRIPTION
.I Se
is a screen oriented version of
.IR ed (1).
It accepts the same      
commands with a few differences.
.PP
ifelse(HARD_TERMS,YES,,`divert(-1)')
.I Se
must be run from a CRT terminal and must be told what sort
of terminal it is; hence the
.I term
parameter.  The terminals
currently supported are:
.CW
	adds980   bee200   haz1510  sol      vi200
	adds100   cg       hp21     trs80    vi50
	adm31     esprit   ibm      ts1
	adm3a     fox      isc8001  tvi950
	anp       gt40     netron   tvt
	bee150    h19      sbee     vc4404
.CN
If no terminal type is specified
.I se
looks to see if you have the shell variable \*(lqTERM\*(rq set; if so,
.I se
recognizes that value as
your terminal type.
ifelse(HARD_TERMS,YES,`divert(-1)',divert)
.I Se
must be run from a CRT terminal.
It uses the
.IR ifelse(S5R2,YES,curses (3X),termlib (3))
terminal operations library, which
retrieves terminal capabilities from the
.B ifelse(S5R2,YES, /usr/lib/terminfo, /etc/termcap)
database.
For
.I se
to function, all of the following must be true:
.IP 1.
You must have the environment variable \*(lqTERM\*(rq set
to the name of your terminal type.
.IP 2.
The
.B ifelse(S5R2,YES, /usr/lib/terminfo, /etc/termcap)
database
must be accessible, and contain an entry for your terminal;
or the
ifelse(S5R2,YES,`\*`('lqTERMINFO\*`('rq',`\*`('lqTERMCAP\*`('rq')
environment variable must contain a description for your terminal.
(See
.IR ifelse(S5R2,YES, curses (3X), termlib (3))
for details.)
.IP 3.
Your terminal must have the capability to do cursor motion.
ifelse(HARD_TERMS,YES,divert,)
.PP
.I Se
first
clears the screen,
draws in its margins,
and executes the commands in the file
.BR ./.serc ,
or failing that,
.BR $HOME/.serc ,
if either file exists.
It then processes the command line,
obeying the options given there, and begins
reading your file (if you specified one).  The screen it draws
looks something like this.
(The parenthesized numerals are not part of the screen layout,
but are there to aid in the following discussion.)
.PP
.CW
	\fR(1) (2)              (3)\fP
	A     |
	B     |#include <stdio.h>
	C     |
	D    *|   register int i;
	E     |
	.  -> |   for (i = 1; i <= 12; i++)
	G     |      putc ('\en', stderr);
	$     |
	cmd>  |_  \fR(4)\fP
	11:39   myfile ........................  \fR(5)\fP
.CN
.PP
The display is divided into five parts:
(1) the line number area,
(2) the mark name area,
(3) the text area,
(4) the command line, and
(5) the status line.
The current line
is indicated by the symbol \*(lq.\*(rq in the
line number area of the screen.
In addition, a rocket
.if n (\*(lq\fB->\fP\*(rq)
.if t (\*(lq\f(CW->\fP\*(rq)
is displayed to make the current line
more obvious.  The current mark name of each line is shown in the
markname area just to the left of the vertical bar.
Other information, such as the
number of lines read in, the name of the file, and the time of day, are
displayed in the status line.
.PP
The cursor is positioned at the beginning of the command
line, showing you that
.I se
awaits your command.  You may now enter any of the standard
.I ed
commands and 
.I se
will perform them, while making sure that
the current line is always displayed on the screen.
.PP
You can set
options to control the behavior of 
.I se
on the command line, simply by using a \*(lq\-\*(rq, followed by
the option letter, and any parameters that the option may take.
These options can also be set after invoking
.I se
with the options command, \*(lqo\*(rq, explained in detail in the section
on commands. Here is a summary:
.sp
.nf
	opt = a | c | d[\fIdir\fP] | f | g | h | i[\fIa\fP | \fIindent\fP] |
		k | l[\fIlop\fP] | lm[\fIcol\fP] | m | s[\fIfiletype\fP] |
	        t[\fItabs\fP] | u[\fIchar\fP] | v[\fIcol\fP] | w[\fIcol\fP] |
	        x | y[\fIkey\fP] | z | -[\fIlnr\fP]
.fi
.sp
There are only a few other things that you need know to
successfully use
.IR se :
.IP 1.
If you make an error, 
.I se
automatically displays an error message in
the status line.  It also leaves
your command line intact so that you may change it using
in-line editing commands (see the \*(lqv\*(rq command).
If you don't want to bother with changing the command, just hit
.SM DEL
to erase the command.
.IP 2.
The \*(lqp\*(rq command has a different meaning than in
.IR ed .
When used with line
numbers, it displays as many of the lines
in the specified range as possible (always including the last line).
When used without line numbers, \*(lqp\*(rq displays  the
previous page.
.IP 3.
The \*(lq:\*(rq command positions a specified line at the
top of the screen (e.g., \*(lq12:\*(rq positions the screen so that
line 12 is at the top).  If no line number is specified, \*(lq:\*(rq
displays the next page.
.PP
Keeping these few differences in mind, you will see that
.I se
can perform all of the
functions of
.IR ed ,
while giving the advantage of a \*(lqwindow\*(rq into
the edit buffer.
.PP
Below is a summary of line number expressions, regular expressions
and commands.
Where there is no difference between
.IR se " and " ed
no explanation is given.
.SS "Line Number Expressions"
.PP
.TP
.I n
.IR n th
line.
.TP
\&.
current line.
.TP
$
last line.
.TP
^
previous line.
.TP
\-
previous line.
.TP 
.RI "capital letter " A
.IR A th
line on the screen.
.I Se
has a number of features that take advantage of the window
display to minimize keystrokes and speed editing.
In the line number area of the screen,
.I se
displays a capital letter
for each line, but in \*(lqabsolute line number\*(rq mode (controlled by the
\*(lqoa\*(rq command; see the options command)
.I se
displays the actual line number of each line.
.TP
#
number of the first line on the screen.
.TP
.RI / "regular expression" [/]
next line with pattern.
.TP
.RI ? "regular expression" [?]
previous line with pattern.
.TP
>name
number of the next line having the given markname
(search wraps around, like //).
.TP
<name
number of the previous line having the given markname
(search proceeds in reverse, like ??).
.TP
.I expression
any of the above operands may be combined with plus
or minus signs to produce a line number expression.  Plus
signs may be omitted if desired (e.g., /parse/\-5, /lexical/+2,
/lexical/2, $\-5, .+6, .6).
Unlike
.IR ed , " se"
does not recognize trailing \*(lq+\*(rq or \*(lq\-\*(rq signs.  They must always
be followed by a integer.  Successive \*(lq+\*(rq or \*(lq\-\*(rq signs
(e.g. \*(lq\-\-\*(rq) are also not allowed.
However, like
.IR vi (1), " se"
will allow you to leave off the trailing delimiter in
forward searches, backward searches, in the substitute command,
the join command,
and in the transliteration command.
.SS "Regular Expression Notation"
.PP
.TP
^
beginning of line if first character in regular expression.
.TP
\&.
any single character other than newline.
.TP
$
end of line if last character in regular expression.
.TP
.RI [ ccl "] [^" ccl ]
character set.
.TP
*
0 or more matches of the preceding regular expression element.
.TP
\e
ignore special meaning of the immediately following character
except \*(lq\e(\*(rq and \*(lq\e)\*(rq.
.TP                 
.RI \e( "regular expression" \e)
Tags the text actually matched by the sub-pattern
specified by
.I "regular expression"
for use in the replacement part
of a substitute
command.                          
.TP
&
Appearing in the replacement part of a substitute command, represents
the text actually matched by the pattern part of the command.
.TP
%
Appearing as
the only character in the replacement part,
represents the replacement part used in the previous substitute command.
(This allows an empty replacement pattern as well.)
If there are other characters in the replacement part along with the
\*(lq%\*(rq, the \*(lq%\*(rq is left alone.
.TP
.RI \e digit
Appearing in the replacement part of a substitute command,
represents the text actually matched by the tagged sub-pattern
specified by
.IR digit .
.SS File names
.PP
.I Se
will expand environment variables which appear anywhere in
a path name. Identifiers in a path name are treated as
environment variables if they start with a dollar sign \*(lq$\*(rq.
A real \*(lq$\*(rq can be used if it is escaped.
If the named environment variable is not found, it is
deleted from the path name.
The expanded path name will be placed in the status line.
.SS The .serc File
.PP
When
.I se
starts up, it tries to open the file
.B .serc
in your current directory.
If that file cannot be found, it will attempt to open the file
.B .serc
in your home directory.
If either file exists,
.I se
will read it, one line at a time, and execute each line as a command.
If a line has a \*(lq#\*(rq as the
.I first
character on the line, or if the line is empty,
the entire line is treated as a comment,
otherwise it is executed.
Here is a sample
.B .serc
file:
.PP
.CW
	# turn on tabs every 8 columns, auto indent
	ot+8
	oia
.CN
.PP
The
.B .serc
file is useful for setting up personalized options,
without having to type them on the command line every time, and without
using a special shell file in your bin (for
.IR sh (1))ifelse(BSD,YES,`,'
or a special alias (for
.IR csh (1))).
.PP
Command line options are processed
.I after
commands in the
.B .serc
file, so, in effect, command line options can be used to over-ride the
defaults in your
.B .serc
file.
.PP
.BR NOTE :
Commands in the
.B .serc
file do
.I not
go through that part of
.I se
which processes the special control characters (see below), so
.I do not
use them in your
.B .serc
file.
.PP
.B Commands
.PP
.TP
(.)\^a\^[:text]  Append
If the command is followed immediately by a colon, then
whatever text follows the colon is inserted without entering
\*(lqappend\*(rq mode.
.TP
(.,.)\^c\^[:text]  Change
If the command is followed immediately by a colon, then
whatever text follows the colon is inserted in place of the named lines
without entering \*(lqappend\*(rq mode.
.TP
(.,.)\^d  Delete
.TP
e\^[\*(lq!\*(rq\^|\^\*(lqx\*(rq] [filename]  Enter
\*(lqe!\*(rq, enter now, is the same as \*(lqE\*(rq in
.IR ed .
\*(lqex\*(rq enters the file with \*(lqXTABS\*(rq turned on, i.e. expand any tabs to
blanks.  File names with extensions \*(lqs\*(rq, \*(lqc\*(rq, \*(lqh\*(rq, \*(lqf\*(rq and \*(lqr\*(rq, are
automatically entered with \*(lqXTABS\*(rq turned on.
.TP
f [filename]  File
.TP
(.,$)\^g/\fIreg expr\fP/command  Global on pattern
.TP
none h\^[stuff] Help
This command provides access to on-line documentation on
the screen editor.
\*(lqStuff\*(rq may be used to select which information is displayed.
The help command will display information which is correct
for both UNIX and SWT modes.
.TP
(.)\^i\^[:text]  Insert
If the command is immediately followed by a colon, then
whatever text follows is inserted without entering \*(lqappend\*(rq mode.
The current line pointer is left at the last line inserted.
.TP
(^,.)\^j\^[\^/stuff\^[\^/\^]\^]     Join
Join is basically the same in
.IR se " and " ed
except if no line numbers are specified, the default
is to join the previous line with the current line (as opposed
to the current line and the next line), and
.I se
allows you to indicate what is to replace the newline(s) in \*(lqstuff\*(rq.
The default is a single blank.  If you do specify \*(lqstuff\*(rq, the trailing
delimiter is optional. \*(lqj/\*(rq is considered the same as \*(lqj//\*(rq,
i.e., the newline is deleted.
.TP
(.,.)\^k\^[m]   marK
.I Se
allows marks to be any
single character other than a newline. If \*(lqm\*(rq is not
present, the lines are marked with the default name of blank.
.RI ( Ed
allows only lower case letters to be marks.)
.TP
none  l  Locate
The Locate command places the system name into the status line
(e.g. \*(lqgatech\*(rq or \*(lqemory\*(rq).
This is so that one
can tell what machine he is using from within the screen
editor. This is particularly useful for installations with
many machines that can run the editor, where the user can
switch back and forth between them, and become confused as
to where he is at a given moment.
.TP
.RI (.,.)\^m\^ line
Move
.TP
(.,.)\^n\^[m]  Name
If \*(lqm\*(rq is present, the last line in the
specified range is marked with it and all other lines having that
mark name are given the default mark name of blank.
If \*(lqm\*(rq is not
present,
the names of all lines in the range are cleared.
.TP
none  o\^[stuff]  Option
Editing options may be queried or set.  \*(lqStuff\*(rq determines which
options are affected.
Options for 
.I se
can be specified in three ways;
in the
.B .serc
file, on the command line that invokes
.IR se ,
or with the \*(lqo\*(rq command.
To specify an option
with the \*(lqo\*(rq command, just enter \*(lqo\*(rq followed immediately by
the option letter and its parameters.  To specify an option on the
command line, just use \*(lq\-\*(rq followed by the option letter and its
parameters.
With this second method, if there are imbedded spaces in the parameter
list, the entire option should be enclosed in quotes.  For
example, to specify the \*(lqa\*(rq (absolute line number) option
and tab stops at column 8 and every fourth thereafter with the
\*(lqo\*(rq command, just enter
.sp
.CW
	oa
	ot 8 +4
.CN
.sp
when
.I se
is waiting for a command.
To enter the same options on the invoking command line, you might
use
.sp
.CW
	se myfile -a "-t 8 +4"
.CN
.sp
You may also choose to put options that you will always want into
your
.B .serc
file.  Commands in the
.B .serc
file should look exactly the same
as they would if they were typed at the
.I se
command line.
Command line options will always over-ride option
commands given in your
.B .serc
file.
.sp
The following summarizes the available
.I se
options:
.RS
.TP
a
causes absolute line numbers to be displayed in the line number area
of the screen. The default behavior is to display upper-case letters
with the letter \*(lqA\*(rq corresponding to the first line in the window.
.TP
c
inverts the case of all letters you type (i.e., converts
upper-case to lower-case and vice versa). This option causes
commands to be recognized only in upper-case and alphabetic line
numbers to be displayed and recognized only in lower-case.
In this mode,
.I se
displays the line number letters in lower case
and expects its command letters in upper case.  Unshifted letters
from the terminal are converted to upper case and shifted
letters to lower case.
.TP
.RI d\^[ dir ]
selects the placement of the current line pointer following
a \*(lqd\*(rq (delete) command.
.I Dir
must be either \*(lq>\*(rq or \*(lq<\*(rq.
If \*(lq>\*(rq is specified, the default behavior is
selected: the line following the deleted lines becomes the new
current line.  If \*(lq<\*(rq is specified, the line immediately preceding
the deleted lines becomes the new current line.  If neither is
specified, the current value of
.I dir
is displayed in the status line.
.TP
f
selects Fortran oriented options. This is equivalent to specifying
the \*(lqot7 +3\*(rq option, and \*(lqXTABS\*(rq is turned on (i.e.
tabs are expanded).
.TP
g
controls the behavior of the \*(lqs\*(rq (substitute) command
when it is under the control of a \*(lqg\*(rq (global) command.
Initially, if a substitute inside a global command fails,
.I se
will not continue with the rest of the lines which might succeed.
If \*(lqog\*(rq is given, then the global substitute will continue, and lines
which failed will not be affected.
Successive \*(lqog\*(rq commands will toggle this behavior.
An explanatory message is placed in the status line.
.TP
h
controls the use of hardware line insert/delete
on terminals that have that capability.
By default, line insert/delete will be used if available.
It is occasionally useful to turn this option off when using the
editor on a terminal which can't keep up, or if the communications
lines may be scrambling the control characters.
Each successive \*(lqoh\*(rq merely toggles a switch within the editor.
An explanatory message is placed in the status line.
.TP
.RI "i\^[ a | " indent " ]"
selects indent value for lines inserted with
\*(lqa\*(rq, \*(lqc\*(rq and \*(lqi\*(rq commands
(initially 1).
\*(lqa\*(rq selects auto-indent which sets the indent to the value which
equals the indent of the previous line.
If neither \*(lqa\*(rq nor
.I indent
are specified,
the current indent value is displayed in the status line.
.TP
k
indicates whether the current contents of your edit buffer
have been saved or not by printing either a \*(lqsaved\*(rq or
\*(lqnot saved\*(rq message on your status line.
.TP
.RI l\^[ lop ]
sets the line number display option.
Under control of this option, 
.I se
continuously displays
the value of one of three symbolic line numbers.
.I lop
may be \*(lq.\*(rq, \*(lq#\*(rq, or \*(lq$\*(rq.
If
.I lop
is omitted, the line number display is disabled.
.TP
.RI lm\^[ col ]
sets the left margin to
.I col
which must be a positive integer.
This option will `shift' your entire screen to the left,
enabling you to see characters at the end of the line that
were previously off the screen; the characters in columns
1 through
.I col
\- 1 will not be visible.  You may continue
editing in the normal fashion.  To reset your screen enter
the command \*(lqolm 1\*(rq.
If
.I col
is omitted, the current left margin column
is displayed in the status line.
.TP
m
controls notification of
the presence of existing mail
and/or
the arrival of new mail
in the user's mail file. 
The mail file is taken from the \*(lqMAIL\*(rq variable in the user's
environment.
On startup, if the mail file is not empty,
.I se
will remark,
\*(lqYou have mail.\*(rq
Then, if new mail arrives,
.I se
will remark,
\*(lqYou have new mail,\*(rq
and ring the terminal's bell.
The \*(lqm\*(rq option simply toggles a notification switch, so that
the user can turn off this notification.
The \*(lqom\*(rq command displays the current setting of the notify
switch in the status line.
.TP
s\^[d | data | as | s | c | h | n | nr | nroff | p | r | f]
sets other options for case, tabs, etc., for
data files, \*(lqd\*(rq or \*(lqdata\*(rq,
assembly files, \*(lqas\*(rq or \*(lqs\*(rq,
C files, \*(lqc\*(rq,
`include' files, \*(lqh\*(rq,
nroff files, \*(lqn\*(rq or \*(lqnr\*(rq or \*(lqnroff\*(rq,
ratfor files, \*(lqr\*(rq,
pascal files, \*(lqp\*(rq,
and fortran files, \*(lqf\*(rq.
Options set for data and nroff files are \*(lqow74\*(rq and \*(lqot+4\*(rq;
for assembly files \*(lqot 17+8\*(rq and \*(lqXTABS\*(rq is turned on;
for C, `include', pascal and ratfor files
\*(lqow74\*(rq, \*(lqot+4\*(rq and \*(lqXTABS\*(rq is turned on;
for fortran files
\*(lqot 7+3\*(rq and \*(lqXTABS\*(rq is turned on.
If \*(lqXTABS\*(rq is turned on then tabs are expanded.
If no argument is specified the options effected by this
command revert to their default value.
.TP
.RI t\^[ tabs ]
sets tab stops according to
.IR tabs .
.I Tabs
consists of
a series of numbers indicating columns in which tab stops
are to be set.  If a number is preceded by a plus sign (\*(lq+\*(rq),
it indicates that the number is an increment; stops are set
at regular intervals separated by that many columns, beginning with
the most recently specified absolute column number.  If no such
number precedes the first increment specification, the stops are
set relative to column 1.
By default, tab stops are set in every third column starting with
column 1, corresponding to a
.I tabs
specification of \*(lq+3\*(rq.
If
.I tabs
is omitted, the current tab spacing is
displayed in the status line. Examples
.sp
.CW
	ot 1 4 7 10 13 16 19 22 25 28 31 34  ...
	ot +3
	ot 7 +3
.CN
.sp
Once the tab stops are set, the control-i and control-e keys
can be used to move the cursor from its current position forward or
backward to the nearest stop, respectively.
.TP
.RI u\^[ chr ]
Normally,
.I se
displays a non-printing character (e.g. \s-1NEWLINE\s+1, \s-1TAB\s+1 ...)
as a blank.
With this option, you can
select the character that
.I se
displays in place of 
unprintable characters.
.I Chr
may be any printable character.
If
.I chr
is omitted,
.I se
displays the current replacement character on the status line.
Non-printing characters (such as
.I se
control characters), or any others for that matter,
may be entered by hitting the
.SM ESC
key followed immediately by the
key to generate the desired character.
Note, however, that the character you type is taken literally,
exactly as it is generated by your terminal, so case conversion
does not apply.
.TP
.RI v\^[ col ]
sets the default \*(lqoverlay column\*(rq.  This is the column
at which the cursor is initially positioned by the \*(lqv\*(rq command.
.I Col
must be a positive integer, or a dollar sign ($) to indicate
the end of the line.  If
.I col
is omitted, the current overlay
column is displayed in the status line.
.TP
.RI w\^[ col ]
sets the \*(lqwarning threshold\*(rq to
.I col
which must be
a positive integer. Whenever the cursor is  positioned at or
beyond this column, the column number is displayed in the status
line and the terminal's bell is sounded.
If
.I col
is omitted, the current warning threshold is displayed
in the status line.
The default warning threshold is 74, corresponding to the first column
beyond the right edge of the screen on an 80 column crt.
.TP
x
toggles tab compression and expansion (\*(lqXTABS\*(rq).
If XTABS is off, \*(lqox\*(rq
turns it on for subsequent \*(lqr\*(rq,
and \*(lqw\*(rq, commands.
Be aware that the \*(lqe\*(rq
command checks the source option for files;
use the \*(lqex\*(rq command to force
tab expansion.
.TP
.RI y\^[ key ]
allows you to edit encrypted files. \*(lqoy\*(rq followed by a key
will cause the
\*(lqe\*(rq, \*(lqr\*(rq, and \*(lqw\*(rq
commands to encrypt and decrypt files using
.IR crypt (1).
\*(lqoy\*(rq by itself will toggle the current encryption setting.
If there is no current key,
.I se
will ask you for one.
Echoing is turned off while you type your key in, and
.I se
asks you to type it in twice, just to be sure.
If encryption is turned on, and you type a plain \*(lqoy\*(rq,
it will be turned off.
Note that doing so causes
.I se
to forget the value of the encryption key.
Encryption in indicated by the message
\*(lqENCRYPT\*(rq in the status line.
The key is
.I never
shown on your screen.
.TP
z
suspends the editor (puts it in the background)
and returns to the user's shell.
(It has to be a shell that understands Berkeley job control,
or else you'll be in trouble.)
The editor will warn you if the edit buffer has not been saved.
This is the
.I only
way to suspend the editor; the editor uses control-z for its own purposes
(see the section on control characters, below).
If you normally run
.B /bin/sh
without job control,
this command has no effect at all.
.sp
On
.SM UNIX
systems without the Berkeley job control mechanism, this option
will be recognized, but will have no effect.
Instead, an explanatory message will be placed in the status line.
.TP
.RI \-[ lnr ]
splits the screen at the line specified by
.I lnr
which must
be a simple line number within the current window.  All lines above
.I lnr
remain frozen on the screen, the line specified by
.I lnr
is replaced by a row of dashes, and the space below this row becomes
the new window on the file. Further editing commands do not affect the
lines displayed in the top part of the screen.  If
.I lnr
is omitted, the screen is restored to its full size.
.RE
.TP
(.,.)\^p  Print
Prints all the lines in the given range.
As much as possible of the range is displayed, always
including the last line; if no range is given, the previous
page is displayed.
The current line pointer is left at the last line printed.
.TP
q\^[!]  Quit
\*(lqq!\*(rq, exit immediately, is the same as \*(lqQ\*(rq in 
.IR ed .
.TP
(.)r\^[x] [filename]  Read
If no line number is specified, the named file is read starting after current
line (as opposed to
.I ed
where the file is read at the end of the edit buffer).
\*(lqrx\*(rq causes tabs to be expanded in the lines read.
.TP
.RI "(.,.)\^s\^[\^/" "reg expr" / sub "\^[/]\^[g]\^[p]]     Substitute"
If no pattern and replacement are specified after the \*(lqs\*(rq,
.I se
will behave as if you had typed \*(lqs//%/\*(rq, i.e. for the
saved search pattern, substitute the saved replacement pattern.
To just delete a pattern, you may type \*(lqs/stuff\*(rq, and
.I se
will behave as if you had typed \*(lqs/stuff//\*(rq.
.TP
(.,.)t<n>   Copy
(\*(lqTo\*(rq is the
.I ed
mnemonic).
.TP
u\^[d]  Undo
\*(lqu\*(rq undoes the effects of the previous command, on the
.I last line
affected (for instance a substitute command).
\*(lqud\*(rq undoes the last delete, i.e. it inserts the last deleted line
after the current line.
.I Se
does not have a global undo capability.
.TP
(.,.)\^v   oVerlay \(em screen oriented editing
Full screen editing with
.I se
is accomplished through the
use of control characters for editing functions.
With screen oriented editing,
control characters may
be used to modify text anywhere in the buffer.
A control-v
may be used to quit overlay mode.
A control-f
may be used to restore the current line to its original state and
terminate the command.
Since
.I se
supports such a large number of
control functions, the mnemonic value of control character
assignments has dwindled to almost zero.  About the only thing
mnemonic is that most symmetric functions
have been assigned to opposing keys on the keyboard  (e.g.,
forward and backward tab to control-i and control-e, forward
and backward space to control-g and control-h, skip right
and left to control-o and control-w, and so on).
We feel pangs of conscience about this, but can find no
more satisfactory alternative.
If you feel the control character assignments are terrible and
you can find a better way, you may change them by modifying
the definitions in
.I se
and recompiling.
.sp
Except for a few special purpose ones, control characters
can be used anywhere, even on the command line.  (This is why
erroneous commands are not erased \(em you may want to edit
them.)  Most of the functions work on a single line,
but the cursor may be
positioned anywhere in the buffer.
Refer to the next section which describes each control character
in detail.
.TP
(1,$)\^w\^[+ | > | !] [filename]  Write
Write the portion of the buffer specified
to the named file.
If \*(lq+\*(rq or \*(lq>\*(rq is given, the portion of the
buffer is appended to the file; otherwise the portion of
the buffer replaces the file.
\*(lqw!\*(rq, write immediately, is the same as \*(lqW\*(rq in
.IR ed .
.TP
.RI "(1,$)\^x\^/" "reg expr" "/command     eXclude on pattern"
.TP
(.,.)\^y\^[\^/from/to\^[/]\^[p]]  TranslYterate (sic)
The range of characters
specified by \*(lqfrom\*(rq is transliterated into the range of
characters specified by \*(lqto\*(rq. The last line on which something
was transliterated is printed if the \*(lqp\*(rq option is used.
The last line in the range becomes the new current line.
As with the substitute and join commands, and pattern searches, the
trailing delimiter is optional.
.I Se
saves both the \*(lqfrom\*(rq and \*(lqto\*(rq parts of the transliterate command:
\*(lqy\*(rq is the same as \*(lqy//%/\*(rq, i.e. transliterate the saved \*(lqfrom\*(rq range
into the saved \*(lqto\*(rq range.
The \*(lq%\*(rq is special only if it is the only character
in the \*(lqto\*(rq part of the command.
.TP
.RI (.,.)\^zb\^\fIleft\fP\^[,\fIright\fP]\^[\fIchar\fP]  Draw Box
A box is drawn on the given lines, in the given columns,
using the given
.IR char .
This command can be used as an aid for preparing block diagrams,
flowcharts, or tables.
.sp
Line numbers are used to specify top and bottom row positions of the box.
.IR Left " and " right
specify left and right column positions of the box.
If second line number is omitted, the box degenerates to a horizontal line.
If right-hand column is omitted, the box degenerates to a vertical line.
If
.I char
is omitted, it defaults to blank,
allowing erasure of a previously-drawn box.
.sp
For example, \*(lq1,10zb15,25*\*(rq would draw a box 10 lines high
and 11 columns across, using asterisks.
The upper left corner of the box would be on line 1, column 15,
and the lower right corner on line 10, column 25.
.TP
(.)\^=  Equals what line number?
.TP
(1,$)\^~m\^command  global exclude on markname
Similar to the \*(lqx\*(rq prefix except that
\*(lqcommand\*(rq is performed for all lines in the range that do not have the
mark name \*(lqm\*(rq.
.TP
changequote([])
(1,$)\^'m\^command  global on markname
changequote
Similar to the \*(lqg\*(rq prefix except that
\*(lqcommand\*(rq is performed for all lines in the range that have the
mark name \*(lqm\*(rq.
.TP
(.)\^:   display next page
The next page of the
buffer is displayed and the current line pointer is placed at
the top of the window.
.TP
changequote({})
.RI "none ![" "\s-1UNIX\s+1 command" "]   escape to the shell"
The user's choice of shell is taken from the \*(lqSHELL\*(rq environment
variable (if it exists),
and is used to execute
.I "\s-1UNIX\s+1 command"
if it is present. Otherwise, an
interactive shell is created.
After an interactive shell exits, the screen is immediately redrawn.
If a command was run, the results are left on the screen, and the
user must type
.SM RETURN
to redraw the editing window.
This is how
.IR vi (1)
behaves.
If the first character of the
.I "\s-1UNIX\s+1 command"
is a `!', then the `!' is replaced with the text of
the previous shell command.
An unescaped `%' in the
.I "\s-1UNIX\s+1 command"
will be replaced with the current
saved file name.
If the shell command is expanded,
.I se
will echo it first, and then execute it.
This behavior is identical to the version of
changequote
.I ed
in
.SM UNIX
System V.
.SS Control Characters
.PP
The set of control characters defined below can be used for correcting
mistakes while typing regular editing commands, for correcting commands
that have caused an error message to be displayed, for correcting lines
typed in append mode, or for in-line editing using the \*(lqv\*(rq command.
.TP
control-a
Toggle insert mode.  The status of the insertion indicator
is inverted.
Insert mode, when enabled, causes the characters you type to be
inserted at the current cursor position in the line
instead of overwriting the characters that were there
previously.  When insert mode is in effect, \*(lqINSERT\*(rq appears
in the status line.
.TP
control-b
Scan right and erase.  The current line is scanned from the current
cursor position to the right margin until an occurrence of the
next character typed is found.  When the character is found, all
characters from the current cursor position up to (but not including)
the scanned character are deleted and the remainder of the line
is moved to the left to close the gap.  The cursor is left in
the same column which is now occupied by the scanned character.
If the line to the right of the cursor does not contain the character
being sought, the terminal's bell is sounded.
.I Se
remembers the last character that was scanned using this
or any of the other scanning keys;
if control-b is hit twice in a row, this remembered character is
used instead of a literal control-b.
.TP
control-c
Insert blank.  The characters at and to the right of
the current cursor position are moved to the right one column
and a blank is inserted to fill the gap.
.TP
control-d
Cursor up.  The effect of this key depends on
.IR se 's
current mode.  When in command mode, the current line pointer is moved
to the previous line without affecting the contents of the command
line.  If the current line pointer is at line 1, the last line
in the file becomes the new current line.
In overlay mode (viz. the \*(lqv\*(rq command), the cursor is
moved up one line while remaining in the same column.
In append mode, this key is ignored.
.TP
control-e
Tab left.  The cursor is moved to the nearest tab stop
to the left of its current position.
.TP
control-f
\*(lqFunny\*(rq return.  The effect of this key depends on the
editor's current mode. In command mode, the current command line is
entered as\-is, but is not erased upon completion of the
command; in append mode, the current line is duplicated; in
overlay mode (viz. the \*(lqv\*(rq command), the current line is restored
to its original state and command mode is reentered (except if
under control of a global prefix).
.TP
control-g
Cursor right.  The cursor is moved one column to the right.
.TP
control-h
Cursor left.  The cursor is moved one column to the left.
Note that this
.I does not
erase any characters; it simply moves the cursor.
.TP
control-i
Tab right.
The cursor is moved to the next tab stop to the right of its current
position.
Again, no characters are erased.
.TP
control-k
Cursor down.  As with the control-d key, this key's effect depends
on the current editing mode.  In command mode,  the current line pointer
is moved to the next line without changing the contents of the command
line. If the current line pointer is at the last line in the file,
line 1 becomes the new current line.  In overlay mode (viz. the
\*(lqv\*(rq command), the cursor is moved down one line while remaining in the
same column.  In append mode, control-k has no effect.
.TP
control-l
Scan left.  The cursor is positioned according to the character
typed immediately after the control-l.  In effect, the current line is
scanned, starting from the current cursor position and moving left,
for the first occurrence of this character.  If none is found before
the beginning of the line is reached, the scan resumes with the
last character in the line.  If the line does not contain the character
being looked for, the message \*(lqNOT FOUND\*(rq is printed in the status line.
.I Se
remembers the last character
that was scanned for using this key; if the control-l is hit twice in
a row, this remembered character is searched for instead of a literal
control-l.
Apart from this, however, the character typed after control-l is taken
literally, so
.IR se 's
case conversion feature does not apply.
.TP
control-m
Kill right and terminate; identical to the
.SM NEWLINE
key described below.
.TP
control-n
Scan left and erase.
The current line is scanned from the current cursor position to the
left margin until an occurrence of the next character typed is found.
Then that character and all characters to its right up to
(but not including) the character under the cursor are erased.
The remainder of the line, as well as the cursor are moved to the
left to close the gap.  If the line to the left of the cursor
does not contain the character being sought, the terminal's bell is
sounded.
If control-n is hit twice in a row, the last character scanned for is
used instead of a literal control-n.
.TP
control-o
Skip right.  The cursor is moved to the first position beyond
the current end of line.
.TP
control-p
Interrupt.  If executing any command except \*(lqa\*(rq, \*(lqc\*(rq, \*(lqi\*(rq or
\*(lqv\*(rq,
.I se
aborts the command and reenters command mode.  The command
line is not erased.
This is the only way to interrupt the editor.
.I Se
ignores the
.SM SIGQUIT
signal (see
.IR signal (2));
in fact it disables generating
quits from the terminal.  The editor uses
.SM "ASCII FS"
(control-\e) for its
own purposes, and changes the terminal driver to make control-p be the
interrupt character.
.TP
control-]
Fix screen.
The screen is reconstructed from
.IR se 's
internal representation of the screen.
.TP
control-r
Erase right.  The character at the current cursor position
is erased and
all characters to its right are moved left one position.
.TP
control-j
Scan right.  This key is identical to the control-l key
described above, except that the scan proceeds to the right from
the current cursor position.
.TP
control-t
Kill right.  The character at the current cursor position
and all those to its right are erased.
.TP
control-u
Erase left.  The character to the left of the current cursor
position is deleted and all characters to its right are moved
to the left to fill the gap.  The cursor is also moved left one
column, leaving it over the same character.
.TP
control-v
Skip right and terminate. The cursor is moved to the current
end of line and the line is terminated.
.TP
control-w
Skip left.  The cursor is positioned at column 1.
.TP
control-x
Insert tab.  The character under the cursor is moved
right to the next tab stop; the gap is filled with blanks.
The cursor is not moved.
.TP
control-y
Kill left.  All characters to the left of the cursor are
erased; those at and to the right of the cursor are moved
to the left to fill the void.  The cursor is left in column 1.
.TP
control-z
Toggle case conversion mode.  The status of the case conversion
indicator is inverted; if case inversion was on, it is turned off,
and vice versa.
Case inversion, when in effect, causes all upper case letters to
be converted to lower case, and all lower case letters to be
converted to upper case (just like the alpha-lock key on some terminals).
You can type control-z at any time
to toggle the case conversion mode.  When case inversion is in effect,
.I se
displays the word \*(lqCASE\*(rq in the status line.
Note that
.I se 
continues
to recognize alphabetic line numbers in upper case only, in contrast
to the \*(lqcase inversion\*(rq option (see the description of options under the
option command).
ifelse(BSD,NO,`divert(-1)',)
.sp
Also note that when running shell that understands Berkeley job control,
the only way to
suspend (stop) the editor is with the \*(lqoz\*(rq command
(see the options command, \*(lqoz\*(rq, above).
ifelse(BSD,NO,divert,)
.TP
control-_ (\s-1US\s+1)
Insert newline.  A newline character is inserted before
the current cursor position, and the cursor is moved one position
to the right.  The newline is displayed according to the current
non-printing replacement character (see the \*(lqu\*(rq option).
.TP
control-\e (\s-1FS\s+1)
Tab left and erase.
Characters are erased starting with the character at the nearest tab
stop to the left of the cursor up to but not including             
the character under the cursor.  The rest of the line,
including the cursor, is moved to the left to close the gap.
.sp
Use control-p to interrupt the editor.
.TP
control-^ (control-~, \s-1RS\s+1)
Tab right and erase.
Characters are erased starting with the character under the cursor
up to but not including the character at the nearest tab stop to
the right of the cursor.  The rest of the line is then
shifted to the left to close the gap.
.TP
.SM NEWLINE
Kill right and terminate.
The characters at and to the right of the current cursor
position are deleted, and the line is terminated.
.TP
.SM DEL
Kill all.  The entire line is erased, along with any error
message that appears in the status line.
.TP
.SM ESC
Escape.  The
.SM ESC
key provides a means for entering
.IR se 's
control characters literally as text into the file.  In fact,
any character that can be generated from the keyboard is
taken literally when it immediately follows the
.SM ESC
key.
If the character is non-printing (as are all of
.IR se 's
control characters),
it appears on the screen as the current non-printing replacement character
(normally a blank \(em see the options command \*(lqou\*(rq).
.SS Windowing Systems
On 4.3 `BSD', and on the AT&T Unix/PC or 3B1,
.I se
notices when its current window changes size or is repositioned,
and adjusts the screen image accordingly.
.SH FILES
.TP
.B $HOME/.serc
.I se
initialization file.
.TP
.BI /usr/tmp/ "process id" . sequence_number
for scratch file.
.TP
.B ./se.hangup
where
.I se
dumps its buffer if it catches a hang-up signal.
.TP
.B /usr/local/lib/se_h/*
help scripts for the \*(lqh\*(rq command.
.SH DIAGNOSTICS
Self explanatory diagnostics appear in the status line.
.SH CAVEATS
.I Se
will
.I never
dump its buffer into an encrypted file when it
encounters a hang-up,
even if encryption was turned on at the time.
.SH SEE ALSO
.I
Software Tools,
.I
Software Tools in Pascal,
.I
Software Tools Subsystem User's Guide,
ifelse(BSD,YES,.IR csh (1)`,',)
.IR ed (1),
.IR crypt (1),
.IR ksh (1),
.IR scriptse (1),
.IR sh (1),
.IR vi (1),
.IR signal (2),
ifelse(HARD_TERMS,YES,`divert(-1)',)
.IR ifelse(S5R2,YES, curses (3X)`,', termlib (3)`,')
ifelse(HARD_TERMS,YES,divert,)
.IR ifelse(BSD,YES, tty (4)`,', termio (7)`,')
.IR environ (5),
ifelse(HARD_TERMS,YES,`divert(-1)',)
.IR ifelse(S5R2,YES, terminfo (4), termcap (5))
ifelse(HARD_TERMS,YES,divert,)
.SH BUGS
Can only be run from a script if the script is first passed through
.IR scriptse (1).
.PP
Tabs could be handled better.  This is because
.I se
was originally written for Prime computers.
.PP
Does not check whether or not it has been put into the background
(this is to allow
.I se
to be used with the
.SM USENET
news software, which does a poor job
of signal handling for child processes).
.PP
Occasionally flakes out the screen when doing line inserts and deletes,
due to problems within the
.IR ifelse(S5R2,YES, curses (3X), termlib (3))
package in putting out the right number of padding characters.
Type a
control-]
to redraw the screen.
.PP
The auto\-indent feature does not recognize a line consisting
of just blanks and then a \*(lq.\*(rq to terminate input,
when the \*(lq.\*(rq is
not in the same position as the first non-blank character of the
previous line.
ifelse(S5R2,YES,
.PP
.I Se
does not work too well together with the
.IR shl (1)
utility`,' since if the user types a control-z`,' both of the programs
will see it`,' and
.I shl
will stop
.IR se `,'
while
.I se
will toggle the case of its input.
)
.PP
There is no global undo capability.
.PP
The help screens could use a rewrite.
.SH AUTHORS
.I Se
started out as the version of
.I ed
that came with the book \*(lqSoftware Tools,\*(rq
by Kernighan and Plauger, which was written in Ratfor. On the Pr1me
computers at the School of Information and Computer Science at Georgia Tech,
Dan Forsyth, Perry Flinn, and Alan Akin added all the enhancements suggested
in the exercises in the book, and some more of their own. Jack Waugh made
extensive modifications to turn the line editor into a screen editor;
further work was done by Dan Forsyth.
All of this was in an improved Georgia Tech version of Ratfor.
.PP
Later, Dan Forsyth, then (and now) at Medical Systems Development
Corporation, converted the Ratfor version into C, for Berkeley Unix (4.1 `BSD').
At Georgia Tech, Arnold Robbins took the C version and added many new features
and improvements, the most important of which was termlib support and System V
support. The existing help screens were edited and completed at that time, as
well. This was finished in early 1985.
.PP
Arnold Robbins is now at ...!emory!arnold, and will make every
reasonable attempt to answer any questions anyone may have about
.IR se ,
but in no way promises to support or enhance it.
