/*
 * $Header: se.h,v 1.3 86/07/11 15:09:54 osadr Exp $
 */

/*
 * $Log:	se.h,v $
 * Revision 1.3  86/07/11  15:09:54  osadr
 * Removed Many Georgia Tech specific things.
 * 
 * Revision 1.2  86/05/27  17:44:15  osadr
 * Removed flexnames dependancy on PREVLINE and PREVLINE2, changed to
 * PREVLN and PREVLN2.
 * 
 * Revision 1.1  86/05/06  13:36:23  osadr
 * Initial revision
 * 
 * 
 */

/* se.h	--- definitions	for the	screen editor */

#include <stdio.h>
#include <ctype.h>
#include <signal.h>

/* some standard definitions used throughout the screen editor */
#include "constdefs.h"
#include "ascii.h"	/* definitions of ascii characters */

/* new data types */
typedef	int filedes;		/* Unix	file descriptor	*/

typedef	struct ldesc {		/* line	descriptor */
#ifdef OLD_SCRATCH
	struct	ldesc *Prevline; /* link to previous line */
	struct	ldesc *Nextline; /* link to next line */
#endif
	unsigned Seekaddr;	/* scratch file seek address / 8 */
	unsigned Lineleng : 16;	/* line length	including NL  EOS */
	unsigned Globmark : 1;	/* mark for global commands */
	unsigned Markname : 7;	/* mark name associated with line */
} LINEDESC;

/* Language extensions */
#define min(a,b)	((a)<(b)?(a):(b))
#define max(a,b)	((a)>(b)?(a):(b))
#define SKIPBL(l,i)	while (l[i] == ' ') (i)++

/* save a little procedure call overhead... */
#define remark(text)	mesg(text, REMARK_MSG)

/* Arbitrary definitions */
#define BACKWARD	-1
#define FORWARD		0
#define NOSTATUS	1
#define NOMORE		0
#define NEWLINE		'\n'
#define TAB		'\t'
#define RETERR	1
#define RETOK	0

/* Array dimensions and	other limit values */
#define MAXBUF		8192
#define MAXTOBUF	512
#define MAXCHARS	10
#define MAXROWS		200
#define MINROWS		16
#define MAXCOLS		200
#define MAXLINE		512
#define MAXPAT		512
#define GARB_FACTOR	2
#define GARB_THRESHOLD	1000
#define BUFENT		1
#define KEYSIZE		11

/* Message classes for status line at bottom of	screen */
#define NOMSG		0
#define REMARK_MSG	1
#define CHAR_MSG	2
#define CASE_MSG	3
#define INS_MSG		4
#define TIME_MSG	5
#define FILE_MSG	6
#define COL_MSG		7
#define LINE_MSG	8
#define COMPRESS_MSG	9
#define HELP_MSG	10
#define MODE_MSG	11
#define CRYPT_MSG	12

/* Characters typed by the user	*/
#define ANYWAY		'!'
#define APPENDCOM	'a'
#define UCAPPENDCOM	'A'
#define BACKSCAN	'?'
#define BACKSEARCH	'<'
#define CHANGE		'c'
#define UCCHANGE	'C'
#define COPYCOM		't'
#define UCCOPYCOM	'T'
#define CURLINE		'.'
#define DEFAULTNAME	' '
#define DELCOM		'd'
#define UCDELCOM	'D'
#define ENTER		'e'
#define UCENTER		'E'
#define ESCAPE		'\\'
#define EXCLUDE		'x'
#define UCEXCLUDE	'X'
#define GLOBAL		'g'
#define UCGLOBAL	'G'
#define GMARK		'\''
#define HELP		'h'
#define UCHELP		'H'
#define INSERT		'i'
#define UCINSERT	'I'
#define JOINCOM		'j'
#define UCJOINCOM	'J'
#define LASTLINE	'$'
#define LOCATECMD	'l'
#define UCLOCATECMD	'L'
#define MARKCOM		'k'
#define UCMARKCOM	'K'
#define MOVECOM		'm'
#define UCMOVECOM	'M'
#define NAMECOM		'n'
#define UCNAMECOM	'N'
#define NOTINCCL	'^'
#define OPTCOM		'o'
#define UCOPTCOM	'O'
#define PAGECOM		':'
#define OVERLAYCOM	'v'
#define UCOVERLAYCOM	'V'
#define PREVLN		'^'
#define PREVLN2		'-'
#define PRINT		'p'
#define UCPRINT		'P'
#define PRINTCUR	'='
#define PRINTFIL	'f'
#define UCPRINTFIL	'F'
#define QUIT		'q'
#define UCQUIT		'Q'
#define READCOM		'r'
#define UCREADCOM	'R'
#define SCAN		'/'
#define SEARCH		'>'
#define SUBSTITUTE	's'
#define UCSUBSTITUTE	'S'
#define TLITCOM		'y'
#define UCTLITCOM	'Y'
#define TOPLINE		'#'
#define UNDOCOM		'u'
#define UCUNDOCOM	'U'
#define WRITECOM	'w'
#define UCWRITECOM	'W'
#define XMARK		'~'
#define MISCCOM		'z'
#define UCMISCCOM	'Z'
#define SHELLCOM	'!'

/* Error message numbers.  Arbitrary so	long as	they are different. */
#define EBACKWARD	1
#define ENOPAT		2
#define EBADPAT		3
#define EBADSTR		4
#define EBADSUB		5
#define ECANTREAD	6
#define EEGARB		7
#define EFILEN		8
#define EBADTABS	9
#define EINSIDEOUT	10
#define EKNOTFND	11
#define ELINE1		12
#define E2LONG		13
#define ENOERR		14
#define ENOLIMBO	15
#define EODLSSGTR	16
#define EORANGE		17
#define EOWHAT		18
#define EPNOTFND	19
#define ESTUPID		20
#define EWHATZAT	21
#define EBREAK		22
#define ELINE2		23
#define ECANTWRITE	24
#define ECANTINJECT	25
#define ENOMATCH	26
#define ENOFN		27
#define EBADLIST	28
#define ENOLIST		29
#define ENONSENSE	30
#define ENOHELP		31
#define EBADLNR		32
#define EFEXISTS	33
#define EBADCOL		34
#define ENOLANG		35
#define ETRUNC		36
#define ENOSHELL	37
#define ECANTFORK	38
#define EHANGUP		39
#define ENOSUB		40
#define ENOCMD		41

#ifdef HARD_TERMS
/* Terminal types */
#define NOTERM		0	/* type	not known yet */
#define ADDS980		1	/* ADDS	Consul 980 */
#define ADDS100		2	/* ADDS	Regent 100, 20,	40, etc	*/
#define FOX		3	/* Perkin-Elmer	1100 */
#define TVT		4	/* Allen's SWTPC T. V. Typewriter II */
#define GT40		5	/* DEC GT40 with Waugh terminal	program	*/
#define BEE150		6	/* Beehive 150 */
#define BEE200		7	/* Beehive 200 */
#define SBEE		8	/* Beehive Super-Bee */
#define SOL		9	/* Sol emulating Beehive 200 */
#define HAZ1510		10	/* Hazeltine 1500 series */
#define CG		11	/* Chromatics CG */
#define ISC8001		12	/* ISC 8001 color terminal */
#define ADM3A		13	/* Lear-Siegler	ADM 3A */
#define IBM		14	/* IBM 3101 */
#define ANP		15	/* Allen & Paul	model 1	*/
#define NETRON		16	/* Netronics */
#define H19		17	/* Heath H19/Zenith Z19	*/
#define TRS80		18	/* Radio Shaft TRS80 */
#define HP21		19	/* Hewlett-Packard 2621A/P */
#define ADM31		20	/* Lear-Siegler	ADM 31 */
#define VI200		21	/* VIsual 200 terminal */
#define VC4404		22	/* Volker-Craig	4404 */
#define ESPRIT		23	/* Hazeltine Esprit (Hazeltine mode) */
#define TS1		24	/* Falco TS-1 */
#define TVI950		25	/* Televideo 950 */
#define VI50		26	/* Visual 50 */
#define VI300		27	/* Visual 300	*/
#endif

/* Screen design positions */
#define NAMECOL		5	/* column to put mark name in */
#define BARCOL		6	/* column for "|" divider */
#define POOPCOL		7	/* column for text to start in */

/* Control characters */

/* Leftward cursor motion */
#define CURSOR_LEFT		CTRL_H	/* left	one column */
#define TAB_LEFT		CTRL_E	/* left	one tab	stop */
#define SKIP_LEFT		CTRL_W	/* go to column	1 */
#define SCAN_LEFT		CTRL_L	/* scan	left for a char	*/
#define G_LEFT			CTRL_U	/* erase char to left */
#define G_TAB_LEFT		FS	/* erase to prev tab stop */
#define KILL_LEFT		CTRL_Y	/* erase to column 1 */
#define G_SCAN_LEFT		CTRL_N	/* scan	left and erase */

/* Rightward cursor motion */
#define CURSOR_RIGHT		CTRL_G	/* right one column */
#define TAB_RIGHT		CTRL_I	/* right one tab stop */
#define SKIP_RIGHT		CTRL_O	/* go to end of	line */
#define SCAN_RIGHT		CTRL_J	/* scan	right for char */
#define G_RIGHT			CTRL_R	/* erase over cursor */
#define G_TAB_RIGHT		RS	/* erase to next tab */
#define KILL_RIGHT		CTRL_T	/* erase to end	of line	*/
#define G_SCAN_RIGHT		CTRL_B	/* scan	right and erase	*/

/* Line	termination */
#define T_SKIP_RIGHT		CTRL_V	/* skip	to end and terminate */
#define T_KILL_RIGHT		CR	/* KILL_RIGHT,	SKIP_RIGHT_AND_TERM */
#define FUNNY			CTRL_F	/* take	funny return */
#define CURSOR_UP		CTRL_D	/* move	up one line */
#define CURSOR_DOWN		CTRL_K	/* move	down one line */
#define CURSOR_SAME		CTRL_J	/* leave cursor	on same	line */

/* Insertion */
#define INSERT_BLANK		CTRL_C	/* insert one blank */
#define INSERT_TAB		CTRL_X	/* insert blanks to next tab */
#define INSERT_NEWLINE		US	/* insert a newline */

/* Miscellany */
#define TOGGLE_INSERT_MODE	CTRL_A	/* toggle insert mode flag */
#define SHIFT_CASE		CTRL_Z	/* toggle case mapping flag */
#define KILL_ALL		DEL	/* erase entire	line */
#define FIX_SCREEN		GS	/* clear and restore screen */

/* Function for moving around the buffer, either style line handling: */
#ifdef OLD_SCRATCH
#define NEXTLINE(k)	((k) -> Nextline)
#else
#define NEXTLINE(k)	(((k) < &Buf[Lastln]) ? (k) + 1 : Line0)
#endif
