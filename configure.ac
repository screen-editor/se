dnl
dnl This file is in the public domain.
dnl

dnl
dnl Initialize autoconf and automake
dnl

AC_INIT([se],[3.0.1],[tcort@se-editor.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_SILENT_RULES([yes])

dnl
dnl Look for a working C compiler
dnl

AC_PROG_CC
AC_SEARCH_LIBS([strerror],[cposix])

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

AC_SEARCH_LIBS(tgetnum, [ncursesw ncurses curses termlib termcap pdcurses], [], AC_MSG_ERROR([curses library not found.]))

AC_CHECK_HEADERS([ncursesw/ncurses.h ncursesw.h ncursesw/curses.h ncurses/ncurses.h ncurses.h ncurses/curses.h curses.h curses/curses.h pdcurses/pdcurses.h pdcurses.h pdcurses/curses.h], [CURSES_LOC="<$ac_header>";found_curses_headers=yes; break;])
AS_IF([test "x$CURSES_LOC" = "x"], [AC_MSG_ERROR([curses headers not found])])
AC_SUBST(CURSES_LOC)

dnl
dnl check for crypt(1) program
dnl

AC_CHECK_PROG(CRYPT_PROG, crypt, yes)
if test x"${CRYPT_PROG}" != x"" ; then
	AC_DEFINE([HAVE_CRYPT_PROG], [1], [crypt(1) is available])
fi
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_LIB(bsd, getpass)

dnl
dnl check for system dependent header files and functions
dnl

AC_CHECK_HEADERS([sys/ioctl.h sys/utsname.h sys/wait.h termio.h termios.h termcap.h unistd.h])
AC_CHECK_FUNCS([fork getpass mkstemp pclose popen sync tcgetattr tcsetattr uname])

dnl
dnl Discover gethostname on mingw32
dnl

AC_CHECK_HEADERS([Winsock2.h])
AC_CHECK_LIB(Ws2_32, gethostname)
AC_CHECK_FUNCS([gethostname])

dnl
dnl Enable some extra warnings when using gcc.
dnl
if test "x$GCC" = "xyes" ; then
	CFLAGS="$CFLAGS -Wall -pedantic"
fi

dnl
dnl check for diff(1)
dnl

AC_ARG_VAR([DIFF],[the 'diff' program to use for test output comparison])
AC_PATH_PROG([DIFF],[diff],[])
if test "x$DIFF" = "x"; then
	AC_MSG_NOTICE([==> diff command not found!])
	AC_MSG_NOTICE([==> Set DIFF variable if present in non-standard path!])
	AC_MSG_NOTICE([diff is mandatory to run the tests])
fi

dnl
dnl generate some files
dnl

dnl short filename for DOS
AC_CONFIG_HEADERS([config.h:config.in])

AC_CONFIG_FILES([Makefile src/Makefile help/Makefile man/Makefile tests/Makefile])
AC_OUTPUT
m4_unquote(
  _m4_defn([_m4_wrap_text])_m4_popdef([_m4_wrap_text]))