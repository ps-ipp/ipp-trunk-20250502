# copied from bison-1.875d

EXTRA_DIST = $(TESTSUITE_AT) testsuite package.m4

DISTCLEANFILES       = atconfig $(check_SCRIPTS)
MAINTAINERCLEANFILES = Makefile.in $(TESTSUITE)

## ------------ ##
## package.m4.  ##
## ------------ ##

$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	{					\
	  echo '# Signature of the current package.'; \
	  echo 'm4_define([AT_PACKAGE_NAME],      [@PACKAGE_NAME@])'; \
	  echo 'm4_define([AT_PACKAGE_TARNAME],   [@PACKAGE_TARNAME@])'; \
	  echo 'm4_define([AT_PACKAGE_VERSION],   [@PACKAGE_VERSION@])'; \
	  echo 'm4_define([AT_PACKAGE_STRING],    [@PACKAGE_STRING@])'; \
	  echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])'; \
	} >$(srcdir)/package.m4

## ------------ ##
## Test suite.  ##
## ------------ ##

TESTSUITE = $(srcdir)/testsuite

AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): package.m4 $(TESTSUITE_AT)
	$(AUTOTEST) -I $(srcdir) testsuite.at -o $@.tmp
	mv $@.tmp $@

atconfig: $(top_builddir)/config.status
	cd $(top_builddir) && ./config.status tests/$@

clean-local:
	test ! -f $(TESTSUITE) || $(SHELL) $(TESTSUITE) --clean

check-local: atconfig $(TESTSUITE)
	$(SHELL) $(TESTSUITE)

#
# END BISON
#

UNITS = \
	alloc \
	init \
	cleanup \
	createtable \
    droptable \
    insert \
    insertobject \
    insertfits \
    selectrowsfits \
    metadatafromobject \
    objectfrommetadata

AM_CPPFLAGS = -I$(top_srcdir)/src$
AM_CFLAGS   = @ippdb_CFLAGS@ $(PSLIB_CFLAGS)$
AM_LDFLAGS  = $(PSLIB_LIBS)$
LDADD       = $(top_builddir)/src/libippdb.la$

check_PROGRAMS = dbsetup dbcleanup $(UNITS)
