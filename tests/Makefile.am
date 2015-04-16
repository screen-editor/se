# This file is in the public domain.

TESTS_ENVIRONMENT = \
	SE=$(top_builddir)/src/se \
	SCRIPTSE=$(top_builddir)/src/scriptse \
	TESTSDIR=$(top_srcdir)/tests \
	DIFF=$(DIFF) \
	TEST_RUNNER=$(top_srcdir)/tests/test-runner.sh

TESTS = \
	hello_world.sh

EXPECTED = hello_world.exp
CLEANFILES = hello_world.out

EXTRA_DIST = driver.sh $(TESTS) $(EXPECTED)
