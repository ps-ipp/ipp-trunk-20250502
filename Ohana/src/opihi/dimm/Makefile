default: dimm

include ../../../Makefile.System
HOME    =       $(ROOT)/src/opihi
BIN     =       $(HOME)/bin
LIB     =       $(HOME)/lib
INC     =       $(HOME)/include
SRC     =       $(HOME)/dimm
DATA    =       $(DESTDATA)/dimm
include ../Makefile.Common

# programs may add their own internal requirements here
LIBS1   =       -lbasiccmd -ldatacmd -lastrocmd -lshell -ldata -lsbig
LIBS2   =       -ldvo -lkapa -lFITS -lohana
FULL_CFLAGS   = $(BASE_CFLAGS)
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = $(LIBS1) $(LIBS2) $(BASE_LDFLAGS)

# dimm user commands and support functions #####################
funcs = \
$(SRC)/Camera.$(ARCH).o		\
$(SRC)/Serial.$(ARCH).o		\
$(SRC)/Telescope.$(ARCH).o             

cmds = \
$(SRC)/init.$(ARCH).o		  	\
$(SRC)/dimm.$(ARCH).o		  	\
$(SRC)/camera_cmds.$(ARCH).o	  	\
$(SRC)/findstars.$(ARCH).o	  	\
$(SRC)/telescope_cmds.$(ARCH).o   \
$(SRC)/version.$(ARCH).o

libs = \
$(DESTLIB)/libshell.a \
$(DESTLIB)/libdata.a \
$(DESTLIB)/libbasiccmd.a \
$(DESTLIB)/libastrocmd.a \
$(DESTLIB)/libdatacmd.a \
$(DESTLIB)/libsbig.a

dimm: sbig $(BIN)/dimm.$(ARCH)
$(SRC)/dimm.$(ARCH).o : $(libs)
$(BIN)/dimm.$(ARCH)   : $(funcs) $(cmds)

install: $(DESTBIN)/dimm help modules

help: cmd.basic.help cmd.data.help cmd.astro.help dimm.help

modules: dimm.modules

# SBIG install functions
sbig: $(DESTINC)/sbig.h $(DESTLIB)/libsbig.a
	@echo sbig code installed

$(DESTINC)/sbig.h:	$(SRC)/sbig/sbig.h
	cp $(SRC)/sbig/sbig.h $(DESTINC)/sbig.h

$(DESTLIB)/libsbig.a:	$(SRC)/sbig/sbig.a
	cp $(SRC)/sbig/sbig.a $(DESTLIB)/libsbig.a

$(SRC)/sbig/sbig.a: $(SRC)/sbig/sbig.a.src
	cp $(SRC)/sbig/sbig.a.src $(SRC)/sbig/sbig.a

.PHONY: dimm

# these have not been finished: should be used for analysis of the extracted images
# $(SRC)/Analysis.$(ARCH).o            \
# $(SRC)/Image.$(ARCH).o		\
