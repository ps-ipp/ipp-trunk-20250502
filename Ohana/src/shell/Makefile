include ../../Makefile.System
HOME 	=	$(ROOT)/src/shell

shell:

default: install

DADS = \
dads.cleanup

GENERAL = \
block checkdisks dumptape \
fits2jpeg gopsfphot gosexphot \
setpath scattercorrection \
vncstart vnccheck gpc.cell.zoom

IMSTATS = \
irsex sexstats

MKDETREND = \
detstats rm.elixir

REALTIME = \
el_plots elixir.fork elixir.launch \
getfocus getseeing gettemps mkjpeg

SKYPROBE = \
sp_command sp_phot \
sp_plots sp_sexphot

CODE = $(DADS) $(GENERAL) $(IMSTATS) $(MKDETREND) $(POSTRUN) $(PTOLEMY) $(REALTIME) $(SKYPROBE)

install:
	@if [ ! -d $(DESTBIN) ]; then mkdir -p $(DESTBIN); fi
	for file in $(CODE); do \
	(chmod +x src/$$file && cd $(DESTBIN) && rm -f $$file && ln -s $(HOME)/src/$$file .); \
	done

# utilities #################################################
clean:	
	rm -f `find . -name "*~"`
	rm -f `find . -name "#*"`
