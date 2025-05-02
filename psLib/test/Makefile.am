SUBDIRS = tap pstap optime $(SRCDIRS)

TESTS = test.pl

EXTRA_DIST = test.pl

# XXX the *.out and *.config files should not be written into this directory in
# the first place
CLEANFILES = core core.* *~ *.out *.config *.fits *.md tmpImages/* tst_psTrace02_OUT

test: check

