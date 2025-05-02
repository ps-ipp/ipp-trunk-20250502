default: dvo

include ../../../Makefile.System
HOME    =       $(ROOT)/src/opihi
BIN     =       $(HOME)/bin
LIB     =       $(HOME)/lib
INC     =       $(HOME)/include
SRC     =       $(HOME)/dvo
DATA    =       $(DESTDATA)/dvo
include ../Makefile.Common

# programs may add their own internal requirements here
LIBS1   =       -lbasiccmd -ldatacmd -lastrocmd -lshell -ldata 
LIBS2   =       -ldvo -lkapa -lFITS -lohana
FULL_CFLAGS   = $(BASE_CFLAGS)
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = $(LIBS1) $(LIBS2) $(BASE_LDFLAGS)

# dvo user commands and support functions ########################
funcs = \
$(SRC)/init.$(ARCH).o            	\
$(SRC)/cmpReadFile.$(ARCH).o		\
$(SRC)/compare.$(ARCH).o                \
$(SRC)/dvomisc.$(ARCH).o		\
$(SRC)/dvo_host_utils.$(ARCH).o		\
$(SRC)/region_list.$(ARCH).o		\
$(SRC)/mySequence.$(ARCH).o		\
$(SRC)/photcode_ops.$(ARCH).o	  	\
$(SRC)/PeriodogramOps.$(ARCH).o	  	\
$(SRC)/find_matches.$(ARCH).o           

# $(SRC)/photometry.$(ARCH).o             

broken = \
$(SRC)/calextract.$(ARCH).o      	\
$(SRC)/calmextract.$(ARCH).o     	\
$(SRC)/ccd.$(ARCH).o             	\
$(SRC)/cmd.$(ARCH).o             	\
$(SRC)/dmagaves.$(ARCH).o	  	\
$(SRC)/dmagmeas.$(ARCH).o	  	\
$(SRC)/dmags.$(ARCH).o		  	\
$(SRC)/ddmags.$(ARCH).o	  	\
$(SRC)/fitcolors.$(ARCH).o

cmds = \
$(SRC)/avextract.$(ARCH).o	  	\
$(SRC)/avmatch.$(ARCH).o	  	\
$(SRC)/avselect.$(ARCH).o	  	\
$(SRC)/avperiodogram.$(ARCH).o	  	\
$(SRC)/avperiodomatch.$(ARCH).o	  	\
$(SRC)/badimages.$(ARCH).o	  	\
$(SRC)/catdir.$(ARCH).o             	\
$(SRC)/catname.$(ARCH).o             	\
$(SRC)/catid.$(ARCH).o             	\
$(SRC)/cmatch.$(ARCH).o	  	\
$(SRC)/cmpload.$(ARCH).o	  	\
$(SRC)/cmpread.$(ARCH).o	  	\
$(SRC)/coordimage.$(ARCH).o	  	\
$(SRC)/coordmosaic.$(ARCH).o	  	\
$(SRC)/detrend.$(ARCH).o	  	\
$(SRC)/dmt.$(ARCH).o		  	\
$(SRC)/elixir.$(ARCH).o                \
$(SRC)/fitsed.$(ARCH).o                \
$(SRC)/gcat.$(ARCH).o		  	\
$(SRC)/catlist.$(ARCH).o		  	\
$(SRC)/gimages.$(ARCH).o	  	\
$(SRC)/gstar.$(ARCH).o		  	\
$(SRC)/hosts.$(ARCH).o	  	\
$(SRC)/cimages.$(ARCH).o	  	\
$(SRC)/images.$(ARCH).o	  	\
$(SRC)/imbox.$(ARCH).o		  	\
$(SRC)/imdata.$(ARCH).o	  	\
$(SRC)/imdense.$(ARCH).o	  	\
$(SRC)/imextract.$(ARCH).o	  	\
$(SRC)/imlist.$(ARCH).o	  	\
$(SRC)/imphot.$(ARCH).o	  	\
$(SRC)/imrough.$(ARCH).o	  	\
$(SRC)/imsearch.$(ARCH).o	  	\
$(SRC)/lcat.$(ARCH).o		  	\
$(SRC)/lcurve.$(ARCH).o	  	\
$(SRC)/lightcurve.$(ARCH).o	  	\
$(SRC)/mextract.$(ARCH).o	  	\
$(SRC)/mmatch.$(ARCH).o	  	\
$(SRC)/mmextract.$(ARCH).o	  	\
$(SRC)/objectcoverage.$(ARCH).o	  	\
$(SRC)/photcodes.$(ARCH).o	  	\
$(SRC)/pmeasure.$(ARCH).o	  	\
$(SRC)/paverage.$(ARCH).o	  	\
$(SRC)/procks.$(ARCH).o	  	\
$(SRC)/remote.$(ARCH).o	  	\
$(SRC)/skycat.$(ARCH).o	  	\
$(SRC)/skycoverage.$(ARCH).o	  	\
$(SRC)/skyregion.$(ARCH).o	  	\
$(SRC)/showtile.$(ARCH).o	  	\
$(SRC)/simage.$(ARCH).o	  	\
$(SRC)/subpix.$(ARCH).o  \
$(SRC)/version.$(ARCH).o

libs = \
$(DESTLIB)/libshell.a \
$(DESTLIB)/libdata.a \
$(DESTLIB)/libbasiccmd.a \
$(DESTLIB)/libastrocmd.a \
$(DESTLIB)/libdatacmd.a

dvo: $(BIN)/dvo.$(ARCH)
$(SRC)/dvo.$(ARCH).o : $(libs)
$(BIN)/dvo.$(ARCH)   : $(funcs) $(cmds) $(SRC)/dvo.$(ARCH).o

dvo_client: $(BIN)/dvo_client.$(ARCH)
$(SRC)/dvo_client.$(ARCH).o : $(libs)
$(BIN)/dvo_client.$(ARCH)   : $(funcs) $(cmds) $(SRC)/dvo_client.$(ARCH).o

dvo.install: $(DESTBIN)/dvo
dvo_client.install: $(DESTBIN)/dvo_client

install: $(DESTBIN)/dvo help modules

help: clean-help cmd.basic.help cmd.data.help cmd.astro.help dvo.help

$(funcs) $(cmds) $(libs) : $(INC)/dvoshell.h

modules: dvo.modules

.PHONY: dvo dvo_client

# deprecated functions: verify & delete
#$(SRC)/abszero.$(ARCH).o \
#$(SRC)/cals.$(ARCH).o \
#$(SRC)/dumpmags.$(ARCH).o \
#$(SRC)/extract.$(ARCH).o \
#$(SRC)/gtypes.$(ARCH).o \
#$(SRC)/photresid.$(ARCH).o \
#$(SRC)/resid.$(ARCH).o \
#$(SRC)/zeropts.$(ARCH).o
#$(SRC)/objload.$(ARCH).o \ - make sure we have vect to tv
#$(SRC)/ccdextract.$(ARCH).o \
#$(SRC)/cmdextract.$(ARCH).o \
#$(SRC)/dmagextract.$(ARCH).o \
#$(SRC)/ddmagextract.$(ARCH).o \
#$(SRC)/pcat.$(ARCH).o		  	\

# future functions, not fully implemented
#$(SRC)/detrend.$(ARCH).o \
#$(SRC)/getxtra.$(ARCH).o \
#$(SRC)/addxtra.$(ARCH).o \

# functions that need to be updated
#$(SRC)/gregions.$(ARCH).o \

