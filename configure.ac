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

AC_PROG_CC

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
dnl Try to find a terminal library
dnl

for tgetflag_library in ncurses termlib termcap curses
do
	if test "$LIBTERMCAP" = ""; then
		AC_CHECK_LIB($tgetflag_library, tgetflag, LIBTERMCAP=-l$tgetflag_library)
	fi
done

if test "$LIBTERMCAP" = ""; then
	AC_MSG_ERROR([We looked for a library containing tgetflag(), but none was found. Please install one of the following libraries: termcap, curses, ncurses, or terminfo])
fi

AC_SUBST(LIBTERMCAP)

dnl
dnl check for system dependent header files and functions
dnl

AC_CHECK_HEADERS([termio.h termios.h termcap.h])
AC_CHECK_FUNCS([tcgetattr tcsetattr])

dnl
dnl Haiku needs -lbsd for getpass(3)
dnl

AC_CHECK_LIB(bsd, getpass)

dnl
dnl generate some files
dnl

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile help/Makefile man/Makefile])
AC_OUTPUT
