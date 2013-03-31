# This file is in the public domain.

TESTS_ENVIRONMENT = SE=$(top_builddir)/src/se \
			SCRIPTSE=$(top_builddir)/src/scriptse \
			TESTSDIR=$(top_srcdir)/tests \
			DIFF=$(DIFF) \
			$(SHELL) $(top_srcdir)/tests/driver.sh

TESTS = hello_world
EXPECTED = hello_world.exp
CLEANFILES = hello_world.out

EXTRA_DIST = driver.sh $(TESTS) $(EXPECTED)
