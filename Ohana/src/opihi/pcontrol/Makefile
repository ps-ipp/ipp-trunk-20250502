default: pcontrol

include ../../../Makefile.System
HOME    =       $(ROOT)/src/opihi
BIN     =       $(HOME)/bin
LIB     =       $(HOME)/lib
INC     =       $(HOME)/include
SRC     =       $(HOME)/pcontrol
DATA    =       $(DESTDATA)/pcontrol
include ../Makefile.Common

# programs may add their own internal requirements here
LIBS1         = -lbasiccmd -lshell -ldata 
LIBS2         = -ldvo -lkapa -lFITS -lohana
FULL_CFLAGS   = $(BASE_CFLAGS)
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = $(LIBS1) $(LIBS2) $(BASE_LDFLAGS)

# to build the non-threaded version, remove -lpthread and comment out
# the THREADED line in include/pcontrol.h

# pcontrol user commands and support functions ########################
funcs = \
$(SRC)/init.$(ARCH).o \
$(SRC)/pcontrol.$(ARCH).o \
$(SRC)/CheckBusyJob.$(ARCH).o \
$(SRC)/CheckDoneHost.$(ARCH).o \
$(SRC)/CheckRespHost.$(ARCH).o \
$(SRC)/CheckDoneJob.$(ARCH).o \
$(SRC)/CheckHost.$(ARCH).o \
$(SRC)/CheckIdleHost.$(ARCH).o \
$(SRC)/CheckPoint.$(ARCH).o \
$(SRC)/CheckSystem.$(ARCH).o \
$(SRC)/GetJobOutput.$(ARCH).o \
$(SRC)/HostOps.$(ARCH).o \
$(SRC)/IDops.$(ARCH).o \
$(SRC)/JobOps.$(ARCH).o \
$(SRC)/MachineOps.$(ARCH).o \
$(SRC)/StackOps.$(ARCH).o \
$(SRC)/PclientCommand.$(ARCH).o \
$(SRC)/StartHost.$(ARCH).o \
$(SRC)/StopHosts.$(ARCH).o \
$(SRC)/KillJob.$(ARCH).o \
$(SRC)/StartJob.$(ARCH).o

cmds = \
$(SRC)/check.$(ARCH).o \
$(SRC)/delete.$(ARCH).o \
$(SRC)/host.$(ARCH).o \
$(SRC)/job.$(ARCH).o \
$(SRC)/jobstack.$(ARCH).o \
$(SRC)/hoststack.$(ARCH).o \
$(SRC)/kill.$(ARCH).o \
$(SRC)/pulse.$(ARCH).o \
$(SRC)/parameters.$(ARCH).o \
$(SRC)/run.$(ARCH).o \
$(SRC)/machines.$(ARCH).o \
$(SRC)/status.$(ARCH).o \
$(SRC)/stdout.$(ARCH).o \
$(SRC)/version.$(ARCH).o \
$(SRC)/verbose.$(ARCH).o

libs = \
$(DESTLIB)/libbasiccmd.a \
$(DESTLIB)/libshell.a \
$(DESTLIB)/libdata.a

pcontrol: $(BIN)/pcontrol.$(ARCH)
$(SRC)/pcontrol.$(ARCH).o : $(libs)
$(BIN)/pcontrol.$(ARCH)   : $(cmds) $(funcs)

$(cmds) $(funcs) : $(INC)/pcontrol.h

install: $(DESTBIN)/pcontrol help

help: clean-help cmd.basic.help pcontrol.help

.PHONY: pcontrol
