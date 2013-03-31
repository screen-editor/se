#!/bin/sh
#
# This file is in the public domain.
#

TESTFILE=$(basename $@)

${SCRIPTSE} < ${TESTSDIR}/${TESTFILE} | ${SE}
${DIFF} ${TESTSDIR}/${TESTFILE}.exp ${TESTFILE}.out
