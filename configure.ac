dnl
dnl This file is in the public domain.
dnl

dnl
dnl Initialize autoconf and automake
dnl

AC_INIT([se], [3.0.1], [tcort@se-editor.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

dnl
dnl Look for a working C compiler
dnl

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC

dnl
dnl Give the user the option of enabling usage logging.
dnl

AC_ARG_ENABLE([usage-logging],
	AS_HELP_STRING([--enable-usage-logging],[log every time se is invoked])],
	[enable_usage_logging="$enableval"],
	[enable_usage_logging="no"])

if test "x$enable_usage_logging" = "xyes" ; then
	AC_DEFINE([LOG_USAGE], [1], [log every time se is invoked])
fi

dnl
dnl curses library and header detection.
dnl

AC_SEARCH_LIBS(initscr, [ncursesw ncurses curses pdcurses], [], AC_MSG_ERROR([curses library not found.]))

AC_CHECK_HEADERS([ncursesw/ncurses.h ncursesw.h ncursesw/curses.h ncurses/ncurses.h ncurses.h ncurses/curses.h curses.h curses/curses.h pdcurses/pdcurses.h pdcurses.h pdcurses/curses.h], [CURSES_LOC="<$ac_header>";found_curses_headers=yes; break;])
AS_IF([test "x$CURSES_LOC" = "x"], [AC_MSG_ERROR([curses headers not found])])
AC_SUBST(CURSES_LOC)

dnl
dnl check for system dependent header files and functions
dnl

AC_HEADER_STDC
AC_CHECK_HEADERS([sys/wait.h unistd.h])
AC_CHECK_FUNCS([fork getpass pclose popen sync])

dnl
dnl Haiku needs -lbsd for getpass(3)
dnl

AC_CHECK_LIB(bsd, getpass)

dnl
dnl Enable some extra warnings when using gcc.
dnl
if test "x$GCC" = "xyes" ; then
	CFLAGS="$CFLAGS -Wall -pedantic"
fi

dnl
dnl generate some files
dnl

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile help/Makefile man/Makefile])
AC_OUTPUT
