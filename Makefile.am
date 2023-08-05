SUBDIRS = help src man tests

sampledir = $(datadir)/@PACKAGE@
sample_DATA = sample.serc

EXTRA_DIST = $(sample_DATA) COPYING ChangeLog NEWS README TODO

treeclean:
	git clean -d -f -e "*.tws" -x

reconf: treeclean
	autoreconf -i -f

list:
	git ls-tree -r --name-only --full-tree HEAD
