default: pclient

include ../../../Makefile.System
HOME    =       $(ROOT)/src/opihi
BIN     =       $(HOME)/bin
LIB     =       $(HOME)/lib
INC     =       $(HOME)/include
SDIR    =       $(HOME)/pclient
DATA    =       $(DESTDATA)/pclient
include ../Makefile.Common

# programs may add their own internal requirements here
LIBS1         = -lbasiccmd -lshell -ldata 
LIBS2         = -ldvo -lkapa -lFITS -lohana
FULL_CFLAGS   = $(BASE_CFLAGS)
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = $(LIBS1) $(LIBS2) $(BASE_LDFLAGS)

# pclient user commands and support functions ########################
funcs = \
$(SDIR)/init.$(ARCH).o \
$(SDIR)/pclient.$(ARCH).o \
$(SDIR)/ChildOps.$(ARCH).o \

cmds = \
$(SDIR)/job.$(ARCH).o \
$(SDIR)/reset.$(ARCH).o \
$(SDIR)/check.$(ARCH).o \
$(SDIR)/status.$(ARCH).o \
$(SDIR)/stdout.$(ARCH).o  \
$(SDIR)/version.$(ARCH).o

libs = \
$(DESTLIB)/libbasiccmd.a \
$(DESTLIB)/libshell.a \
$(DESTLIB)/libdata.a

pclient: $(BIN)/pclient.$(ARCH)
$(SRC)/pclient.$(ARCH).o : $(libs)
$(BIN)/pclient.$(ARCH)   : $(funcs) $(cmds)

install: $(DESTBIN)/pclient help

help: clean-help cmd.basic.help pclient.help

.PHONY: pclient
