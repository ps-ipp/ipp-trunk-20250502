bin_PROGRAMS = \
	addtool \
	bgtool \
	caltool \
	camtool \
	chiptool \
	detselect \
	dettool \
	difftool \
	disttool \
	flatcorr \
	dqstatstool \
	labeltool \
	magictool \
	magicdstool \
	pstamptool \
	pxadmin \
	pxinject \
	faketool \
	pzgetexp \
	pzgetimfiles \
	pztool \
	regtool \
	stacktool \
	staticskytool \
	warptool \
	receivetool \
	pubtool \
	diffphottool \
	minidvodbtool \
	mergetool \
	laptool \
	vptool \
	sctool \
	releasetool \
	fftool \
	fpcamtool \
	remotetool

pkginclude_HEADERS = \
	pxadd.h \
	pxadmin.h \
	pxcam.h \
	pxchip.h \
	pxconfig.h \
	pxdata.h \
	pxfake.h \
	pxdqstats.h
	pxmagic.h \
	pxminidvodb.h \
	pxmerge.h \
	pxregister.h \
	pxspace.h \
	pxtag.h \
	pxtools.h \
	pxtoolsErrorCodes.h \
	pxtree.h \
	pxfpcam.h \
	pxwarp.h

noinst_HEADERS = \
	addtool.h \
	bgtool.h \
	caltool.h \
	camtool.h \
	chiptool.h \
	detselect.h \
	dettool.h \
	difftool.h \
	disttool.h \
	flatcorr.h \
	faketool.h \
	dqstatstool.h \
	labeltool.h \
	magictool.h \
	magicdstool.h \
	pstamptool.h \
	pxinject.h \
	pzgetexp.h \
	pzgetimfiles.h \
	pztool.h \
	regtool.h \
	stacktool.h \
	staticskytool.h \
	warptool.h \
	pubtool.h \
	diffphottool.h \
	minidvodbtool.h \
	mergetool.h \
	laptool.h \
	vptool.h \
	sctool.h \
	releasetool.h \
	fftool.h \
	fpcamtool.h \
	remotetool.h 

lib_LTLIBRARIES = libpxtools.la
libpxtools_la_CFLAGS	= $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
libpxtools_la_LIBS      = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS)
libpxtools_la_LDFLAGS   = -release $(PACKAGE_VERSION)
libpxtools_la_SOURCES   = \
	pxadd.c \
	pxcam.c \
	pxchip.c \
	pxconfig.c \
	pxdata.c \
	pxerrors.c \
	pxfake.c \
	pxfault.c \
	pxdqstats.c \
	pxmagic.c \
	pxminidvodb.c \
	pxmerge.c \
	pxregister.c \
	pxspace.c \
	pxtag.c \
	pxtools.c \
	pxtoolsErrorCodes.c \
	pxtree.c \
	pxfpcam.c \
	pxwarp.c

# for pxtools.h
AM_CPPFLAGS = -I$(top_srcdir)/src$

pztool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS) 
pztool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
pztool_SOURCES = \
    pztool.c \
    pztoolConfig.c

pstamptool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS) 
pstamptool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
pstamptool_SOURCES = \
    pstamptool.c \
    pstamptoolConfig.c

addtool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
addtool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
addtool_SOURCES = \
    addtool.c \
    addtoolConfig.c

bgtool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
bgtool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
bgtool_SOURCES = \
    bgtool.c \
    bgtoolConfig.c

caltool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
caltool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
caltool_SOURCES = \
    caltool.c \
    caltoolConfig.c

flatcorr_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
flatcorr_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
flatcorr_SOURCES = \
    flatcorr.c \
    flatcorrConfig.c

regtool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
regtool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
regtool_SOURCES = \
    regtool.c \
    regtoolConfig.c

chiptool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
chiptool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
chiptool_SOURCES = \
    chiptool.c \
    chiptoolConfig.c

camtool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
camtool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
camtool_SOURCES = \
    camtool.c \
    camtoolConfig.c

fpcamtool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
fpcamtool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
fpcamtool_SOURCES = \
    fpcamtool.c \
    fpcamtoolConfig.c

faketool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
faketool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
faketool_SOURCES = \
    faketool.c \
    faketoolConfig.c

dqstatstool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
dqstatstool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
dqstatstool_SOURCES = \
    dqstatstool.c \
    dqstatstoolConfig.c

labeltool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
labeltool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
labeltool_SOURCES = \
    labeltool.c \
    labeltoolConfig.c

magictool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
magictool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
magictool_SOURCES = \
    magictool.c \
    magictoolConfig.c

magicdstool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
magicdstool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
magicdstool_SOURCES = \
    magicdstool.c \
    magicdstoolConfig.c

warptool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
warptool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
warptool_SOURCES = \
    warptool.c \
    warptoolConfig.c

difftool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
difftool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
difftool_SOURCES = \
    difftool.c \
    difftoolConfig.c

disttool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
disttool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
disttool_SOURCES = \
    disttool.c \
    disttoolConfig.c

stacktool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
stacktool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
stacktool_SOURCES = \
    stacktool.c \
    stacktoolConfig.c

staticskytool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
staticskytool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
staticskytool_SOURCES = \
    staticskytool.c \
    staticskytoolConfig.c

pxadmin_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
pxadmin_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
pxadmin_SOURCES = \
    pxadmin.c \
    pxadminConfig.c

pzgetexp_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
pzgetexp_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
pzgetexp_SOURCES = \
    pzgetexp.c \
    pzgetexpConfig.c

pzgetimfiles_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
pzgetimfiles_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
pzgetimfiles_SOURCES = \
    pzgetimfiles.c \
    pzgetimfilesConfig.c

dettool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
dettool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
dettool_SOURCES = \
    dettool.c \
    dettoolConfig.c \
    dettool_correction.c \
    dettool_detrunsummary.c \
    dettool_normalizedexp.c \
    dettool_normalizedimfile.c \
    dettool_normalizedstat.c \
    dettool_processedexp.c \
    dettool_processedimfile.c \
    dettool_residexp.c \
    dettool_residimfile.c \
    dettool_stack.c

detselect_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
detselect_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
detselect_SOURCES = \
    detselect.c \
    detselectConfig.c

pxinject_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
pxinject_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
pxinject_SOURCES = \
    pxinject.c \
    pxinjectConfig.c


receivetool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
receivetool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
receivetool_SOURCES = \
    receivetool.c \
    receivetoolConfig.c

pubtool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
pubtool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
pubtool_SOURCES = \
    pubtool.c \
    pubtoolConfig.c

diffphottool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
diffphottool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
diffphottool_SOURCES = \
    diffphottool.c \
    diffphottoolConfig.c

minidvodbtool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
minidvodbtool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
minidvodbtool_SOURCES = \
    minidvodbtool.c \
    minidvodbtoolConfig.c

mergetool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
mergetool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
mergetool_SOURCES = \
    mergetool.c \
    mergetoolConfig.c

laptool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
laptool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
laptool_SOURCES = \
    laptool.c \
    laptoolConfig.c

vptool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
vptool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
vptool_SOURCES = \
    vptool.c \
    vptoolConfig.c

sctool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
sctool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
sctool_SOURCES = \
    sctool.c \
    sctoolConfig.c

releasetool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
releasetool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
releasetool_SOURCES = \
    releasetool.c \
    releasetoolConfig.c

fftool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
fftool_LDADD = $(PSLIB_LIBS) $(PSMODULES_LIBS) $(IPPDB_LIBS) libpxtools.la
fftool_SOURCES = \
    fftool.c \
    fftoolConfig.c

remotetool_CFLAGS = $(PSLIB_CFLAGS) $(PSMODULES_CFLAGS) $(IPPDB_CFLAGS)
remotetool_LDADD  = $(PSLIB_LIBS)   $(PSMODULES_LIBS)   $(IPPDB_LIBS)   libpxtools.la
remotetool_SOURCES = \
    remotetool.c \
    remotetoolConfig.c

clean-local:
	-rm -f TAGS
# Tags for emacs
tags:
	etags `find . -name \*.[ch] -print`

# Error codes.
BUILT_SOURCES = pxtoolsErrorCodes.h pxtoolsErrorCodes.c
CLEANFILES = pxtoolsErrorCodes.h pxtoolsErrorCodes.c
EXTRA_DIST = pxtoolsErrorCodes.dat pxtoolsErrorCodes.c.in pxtoolsErrorCodes.h.in fakemagic

pxtoolsErrorCodes.h : pxtoolsErrorCodes.dat pxtoolsErrorCodes.h.in
	$(ERRORCODES) --data=pxtoolsErrorCodes.dat --outdir=. pxtoolsErrorCodes.h

pxtoolsErrorCodes.c : pxtoolsErrorCodes.dat pxtoolsErrorCodes.c.in pxtoolsErrorCodes.h
	$(ERRORCODES) --data=pxtoolsErrorCodes.dat --outdir=. pxtoolsErrorCodes.c

BUILT_SOURCES += pxdata.c
EXTRA_DIST += pxdata.c.template

pxdata.c: $(srcdir)/pxdata.c.template $(srcdir)/Makefile
	$(PERL) -pe 's|\@PKGDATADIR\@|$(pkgdatadir)|' $< > $@

