SUBDIRS = templates/psdb templates/latex

bin_SCRIPTS = glueforge
CLEANFILES = $(bin_SCRIPTS)

do_subst = sed \
	-e 's,[@]datadir[@],$(datadir),g' \
	-e 's,[@]PACKAGE_NAME[@],$(PACKAGE_NAME),g' \
	-e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g'

glueforge: glueforge.in
	$(do_subst) < $(srcdir)/glueforge.in > glueforge
	chmod +x glueforge

example_files = \
   examples/simple.md \
   examples/complex.md

glueforgedatadir = $(datadir)/@PACKAGE_NAME@
exampledir = $(glueforgedatadir)/examples
example_DATA = $(example_files)

install-data-local:
	-chmod 0755 $(glueforgedatadir) $(exampledir)

man_MANS = \
    $(top_builddir)/@PACKAGE_NAME@.1

@PACKAGE@.1: glueforge
	$(POD2MAN) glueforge > @PACKAGE@.1

clean-local:
	-rm -rf docs @PACKAGE@.1

EXTRA_DIST = autogen.sh glueforge.in $(example_files)

ACLOCAL_AMFLAGS = -I m4
