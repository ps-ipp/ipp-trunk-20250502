default: mana

include ../../../Makefile.System
HOME    =       $(ROOT)/src/opihi
SRC     =       $(HOME)/mana
BIN     =       $(HOME)/bin
LIB     =       $(HOME)/lib
INC     =       $(HOME)/include
DATA    =       $(DESTDATA)/mana
include ../Makefile.Common

# programs may add their own internal requirements here
LIBS1   =       -lbasiccmd -ldatacmd -lastrocmd -lshell -ldata 
LIBS2   =       -ldvo -lkapa -lFITS -lohana
FULL_CFLAGS   = $(BASE_CFLAGS)
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = $(LIBS1) $(LIBS2) $(BASE_LDFLAGS)

# mana user commands and support functions ########################
funcs = \
$(SRC)/init.$(ARCH).o \
$(SRC)/mana.$(ARCH).o \
$(SRC)/deimos_mkobj.$(ARCH).o \
$(SRC)/deimos_mkslit.$(ARCH).o \
$(SRC)/deimos_fitslit.$(ARCH).o \
$(SRC)/deimos_getobj.$(ARCH).o \
$(SRC)/deimos_fitobj.$(ARCH).o \
$(SRC)/deimos_mkmodel.$(ARCH).o \
$(SRC)/deimos_arclines.$(ARCH).o \
$(SRC)/deimos_fitarc.$(ARCH).o \
$(SRC)/deimos_fitprofile.$(ARCH).o \
$(SRC)/findrowpeaks.$(ARCH).o 

cmds = \
$(SRC)/deimos.$(ARCH).o \
$(SRC)/rawstars.$(ARCH).o \
$(SRC)/fitcontour.$(ARCH).o \
$(SRC)/starcontour.$(ARCH).o \
$(SRC)/version.$(ARCH).o \
$(SRC)/findpeaks.$(ARCH).o 

libs = \
$(DESTLIB)/libshell.a \
$(DESTLIB)/libdata.a \
$(DESTLIB)/libbasiccmd.a \
$(DESTLIB)/libastrocmd.a \
$(DESTLIB)/libdatacmd.a

mana: $(BIN)/mana.$(ARCH)
$(SRC)/mana.$(ARCH).o : $(libs)
$(BIN)/mana.$(ARCH)   : $(cmds) $(funcs)

install: $(DESTBIN)/mana help modules

help: clean-help cmd.basic.help cmd.data.help cmd.astro.help

modules: mana.modules

.PHONY: mana
