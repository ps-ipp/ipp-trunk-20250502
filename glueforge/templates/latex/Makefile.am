template_files = \
    config.pl \
    tables.tt 

templatedir = $(datadir)/@PACKAGE_NAME@/templates
latextemplatedir = $(templatedir)/latex
latextemplate_DATA = $(template_files)

install-data-hook:
	chmod 0755 $(templatedir) $(latextemplatedir)

EXTRA_DIST = $(template_files)
