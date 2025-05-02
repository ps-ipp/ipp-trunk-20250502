include ../../Makefile.System
HOME 	=	$(ROOT)/src/perl

perl: default

default: install

DADS = \
dads.detrend dads.jpeg dads.keywords grab.keywords

DETREND = \
darktime.cfh12k darktime.linear \
demodemap flatten.mana flatten.flips defringe getfringe defringe.ccd

GENERAL = \
cfht.names checkconfig mkrun megacenter megacoords \
update.astrometry.iraf wfi.keywords mefheadunstrip cadc2qso \
mkhtml mkidx

IMSTATS = \
imstatqso imstats \
irstats seeingstats

MKDETREND = \
create.subset dt.select fixscat fixscat.mef mkscat \
mkdetrend mosaic.merge mosaic.stats \
normalize split.mef \
detflips merge.lists

MKFRINGE = \
fr.mkmodes fr.modesave fr.modestats \
fr.ckfile fr.detrend fr.frstats \
fr.getfringe fr.imdata fr.jpegs \
fr.medbin.spline fr.medbin.interp fr.mkhtml fr.mklists \
fr.select mkfringe fr.defringe fr.smooth

POSTRUN = \
ckastrom ckdetrend ckphotom \
ckrunid ckvalid elixir.photreport \
elixir.postrun mktrans mktreport \
validate rerun.complete elixir.rerun

PTOLEMY = \
cmp.header imclean.cfht imclean.subaru gastro.raw elixir.export

REALTIME = \
ckimregdb elixir.link elixir.reload

CODE = $(DADS) $(DETREND) $(GENERAL) $(IMSTATS) $(MKDETREND) $(MKFRINGE) $(POSTRUN) $(PTOLEMY) $(REALTIME)

install:
	@if [ ! -d $(DESTBIN) ]; then mkdir -p $(DESTBIN); fi
	for file in $(CODE); do \
	(chmod +x src/$$file && cd $(DESTBIN) && rm -f $$file && ln -s $(HOME)/src/$$file .); \
	done

# utilities #################################################
clean:	
	rm -f `find . -name "*~"`
	rm -f `find . -name "#*"`
