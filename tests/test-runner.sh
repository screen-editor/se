#!/bin/sh
#
# This file is in the public domain.
#

TESTFILE=$(basename ${1})

${SCRIPTSE} < ${TESTSDIR}/${TESTFILE} | ${SE} > /dev/tty
${DIFF} ${TESTSDIR}/${TESTFILE}.exp ${TESTFILE}.out
