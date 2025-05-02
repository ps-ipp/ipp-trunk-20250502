default: elixir
help:
	@echo "make options: elixir (default)"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/elixir
BIN	=	$(HOME)/bin
LIB	=	$(HOME)/lib
SRC	=	$(HOME)/src
MAN	=	$(HOME)/doc
INC	= 	$(HOME)/include
include ../../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS)
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = -ldvo -lFITS -lohana $(BASE_LDFLAGS)

elixir: $(BIN)/elixir.$(ARCH)
install: $(DESTBIN)/elixir

ELIXIR = \
$(SRC)/elixir.$(ARCH).o			$(SRC)/ConfigInit.$(ARCH).o		\
$(SRC)/MachineOps.$(ARCH).o		$(SRC)/FifoOps.$(ARCH).o		\
$(SRC)/QueueOps.$(ARCH).o		$(SRC)/CheckCluster.$(ARCH).o		\
$(SRC)/CheckDepend.$(ARCH).o		$(SRC)/CheckEndingState.$(ARCH).o	\
$(SRC)/CheckMachineStatus.$(ARCH).o	$(SRC)/CheckProcess.$(ARCH).o		\
$(SRC)/InitProcess.$(ARCH).o		$(SRC)/StartMachine.$(ARCH).o		\
$(SRC)/SockScan.$(ARCH).o		$(SRC)/LoadPending.$(ARCH).o		\
$(SRC)/Photcodes.$(ARCH).o		$(SRC)/rconnect.$(ARCH).o		\
$(SRC)/DefineProcesses.$(ARCH).o        $(SRC)/ProcessOps.$(ARCH).o		\
$(SRC)/DumpStatus.$(ARCH).o		$(SRC)/CheckMessages.$(ARCH).o		\
$(SRC)/MsgOps.$(ARCH).o			$(SRC)/LogOpen.$(ARCH).o		\
$(SRC)/ConfigPID.$(ARCH).o		$(SRC)/RemoteOps.$(ARCH).o

$(ELIXIR): $(INC)/elixir.h
$(BIN)/elixir.$(ARCH): $(ELIXIR)
