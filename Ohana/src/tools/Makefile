default: tools
help:
	@echo "make options: tools (default)"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/tools
SRC	=	$(HOME)/src
LIB	= 	$(HOME)/lib
MAN	=	$(HOME)/doc
BIN	=	$(HOME)/bin
INC	=	$(HOME)/include
# include ../../Makefile.Common

DVO_LIBS  = $(DESTLIB)/libdvo.a $(DESTLIB)/libFITS.a $(DESTLIB)/libohana.a
DVO_INCS  = $(DESTINC)/dvo.h $(DESTINC)/gfitsio.h $(DESTINC)/ohana.h $(DESTINC)/autocode.h 

PROGRAMS = gconfig fhead pspsconvert ftable fields list_astro glockfile \
radec mktemp precess csystem fits_insert \
medianfilter mefhead ckfits random fits_to_mysql mpcorb_predict

OTHER = testukirt 

# roc
# CZW: 2016-01-29 Removing roc from the list of programs, as I've made
# changes to it that require the nebclient library.  This complicates
# the dependency chain, and as roc isn't generally used, pulling it
# out is easier than solving that chain.

all tools: $(PROGRAMS)

BASE_CFLAGS   =	$(CFLAGS)
BASE_CPPFLAGS =	$(CPPFLAGS) -I$(INC) -I$(DESTINC) $(INCDIRS) -D$(ARCH)
BASE_LDFLAGS  = $(LDFLAGS) -L$(LIB) -L$(DESTLIB) $(LIBDIRS) $(LIBFLAGS)

FULL_CFLAGS   =	$(BASE_CFLAGS)
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = -ldvo -lFITS -lohana $(BASE_LDFLAGS)

# CZW: required library for roc to compile.
# -lnebclient

# these are all programs which just depend on a single c file: foo : foo.c
# we use a special set of rules in this directory which expect this simplification

%.$(ARCH).o : %.c $(DVO_INCS)
	$(CC) $(FULL_CFLAGS) $(FULL_CPPFLAGS) -c $< -o $@
	@echo "compiled $<"
	@echo ""

$(BIN)/%.$(ARCH) : $(SRC)/%.$(ARCH).o
	@if [ ! -d $(BIN) ]; then mkdir -p $(BIN); fi
	$(CC) $(FULL_CFLAGS) -o $@ $^ $(FULL_LDFLAGS)
	@echo "compiled $*"
	@echo ""

$(DESTBIN)/%: $(BIN)/%.$(ARCH)
	@if [ ! -d $(DESTBIN) ]; then mkdir -p $(DESTBIN); fi
	rm -f $(DESTBIN)/$*
	cp $(BIN)/$*.$(ARCH) $(DESTBIN)/$*
	@echo "installed $*"
	@echo ""

$(PROGRAMS) : % : $(BIN)/%.$(ARCH)

$(BIN)/mpcorb_predict.$(ARCH) : $(SRC)/mpcorb_predict.$(ARCH).o
	@if [ ! -d $(BIN) ]; then mkdir -p $(BIN); fi
	$(CC) $(FULL_CFLAGS) -o $@ $^ $(FULL_LDFLAGS) -lsla
	@echo "compiled $*"
	@echo ""

$(DESTBIN)/mpcorb_predict: $(BIN)/mpcorb_predict.$(ARCH)
	@if [ ! -d $(DESTBIN) ]; then mkdir -p $(DESTBIN); fi
	rm -f $(DESTBIN)/mpcorb_predict
	cp $(BIN)/mpcorb_predict.$(ARCH) $(DESTBIN)/mpcorb_predict
	@echo "installed mpcorb_predict"
	@echo ""

$(DESTLIB)/libsla.a:
	echo "missing SLALIB libsla.a: install in $(DESTLIB)"
	@exit 2

$(DESTINC)/slalib.h:
	echo "missing SLALIB slalib.h: install in $(DESTINC)"
	@exit 2

mpcorb_predict: $(DESTLIB)/libsla.a $(DESTINC)/slalib.h $(DESTBIN)/mpcorb_predict 

mpcorb: mpcorb_predict

%.clean:
	rm -f $(SRC)/$*.$(ARCH).o
	rm -f $(BIN)/$*.$(ARCH)

%.install:
	$(MAKE) $(DESTBIN)/$*

install:
	for i in $(PROGRAMS); do $(MAKE) $$i.install || exit; done


clean:
	rm -f `find . -name "*.o"`
	rm -f `find . -name "*.o"`
	rm -f `find . -name "*~"`
	rm -f `find . -name "#*"`
	rm -f $(BIN)/*.$(ARCH)

dist: clean
	rm -rf $(BIN)
	rm -rf $(LIB)

# $@ : target filename
# $* : matched word (%)
# $< : first prereq
# $^ : all prereqs

.PRECIOUS: %.$(ARCH).o
.PRECIOUS: $(BIN)/%.$(ARCH)

