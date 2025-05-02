TGT  = $(PSCONFDIR)/$(PSCONFIG)/etc/compress.mgc

SUBDIRS = \
	recipes \
	isp \
	gpc1 \
	gpc1-tdi \
	gpc2 \
	cfh12k \
	megacam \
	mosaic2 \
	simple \
	simmosaic \
	simtest \
	vysos5 \
	vysos20 \
	tc3 \
	skycell \
	sdssmosaic \
	sdss \
	esowfi \
	lbc_red \
	lulin \
	ssp \
	uh8k \
	suprime  \
	hsc

install_files = \
	system.config \
	GSCregions.tbl \
	dvo.photcodes

built_sources = \
	ipprc.config.in \
	dvo.site.in

built_files = \
	ipprc.config \
	dvo.site \
	$(TGT)

installdir = $(datadir)/ippconfig

install_DATA = $(install_files) $(built_files)

install-data-hook:
	chmod 0755 $(installdir) 

EXTRA_DIST = $(install_files) $(built_sources)

BUILT_SOURCES = $(built_files)

#ACLOCAL_AMFLAGS = -I m4

# From the autoconf manual, section 4.7.2 "Installation Directory Variables"
edit = sed \
	-e 's|@pkgdatadir[@]|$(pkgdatadir)|g' \
	-e 's|@prefix[@]|$(prefix)|g'

ipprc.config dvo.site: Makefile
	rm -f $@
	$(edit) '$(srcdir)/$@.in' > $@

ipprc.config: $(srcdir)/ipprc.config.in
dvo.site: $(srcdir)/dvo.site.in



if HAVE_LINUX
MES = YesLINUX
MGC = compress.mgc
else
MES = StillNoLINUX
MGC = gentoo
endif

$(TGT): $(MGC)
	cp $(MGC) $(TGT)     

test: $(TGT)
	file o60523g0045o.2132764.wrp.2688740.skycell.1566.004.pswarp.mdc
	file -m $(TGT) o60523g0045o.2132764.wrp.2688740.skycell.1566.004.pswarp.mdc

prt: 
	@echo $(MES)
