default: gastro2
help:
	@echo "make options: gastro2 (default)"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/gastro2
BIN	=	$(HOME)/bin
LIB	=	$(HOME)/lib
SRC	=	$(HOME)/src
MAN	=	$(HOME)/doc
INC	= 	$(HOME)/include
include ../../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS)
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = -lkapa -ldvo -lFITS -lohana $(BASE_LDFLAGS)

gastro2: $(BIN)/gastro2.$(ARCH)
coordtest: $(BIN)/coordtest.$(ARCH)
extr2mass: $(BIN)/extr2mass.$(ARCH)
install: $(DESTBIN)/gastro2

EXTR2MASS = \
$(SRC)/extr2mass.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/gregions2.$(ARCH).o

COORDTEST = \
$(SRC)/coordtest.$(ARCH).o \
$(SRC)/gpairs.$(ARCH).o \
$(SRC)/polyfit.$(ARCH).o

GASTRO2 = \
$(SRC)/plots.$(ARCH).o \
$(SRC)/gheader2.$(ARCH).o \
$(SRC)/gfit2.$(ARCH).o \
$(SRC)/gpairs.$(ARCH).o \
$(SRC)/polyfit.$(ARCH).o \
$(SRC)/plotstuff.$(ARCH).o \
$(SRC)/rotate2.$(ARCH).o \
$(SRC)/gcenter2.$(ARCH).o \
$(SRC)/gproject2.$(ARCH).o \
$(SRC)/grid.$(ARCH).o \
$(SRC)/lumfunc.$(ARCH).o \
$(SRC)/sort.$(ARCH).o \
$(SRC)/misc.$(ARCH).o \
$(SRC)/gargs.$(ARCH).o \
$(SRC)/ConfigInit.$(ARCH).o \
$(SRC)/gastro2.$(ARCH).o \
$(SRC)/gstars2.$(ARCH).o \
$(SRC)/greference2.$(ARCH).o \
$(SRC)/getusno.$(ARCH).o \
$(SRC)/getusnob.$(ARCH).o \
$(SRC)/getptolemy.$(ARCH).o \
$(SRC)/getgsc.$(ARCH).o \
$(SRC)/remove_clumps.$(ARCH).o \
$(SRC)/rfits.$(ARCH).o \
$(SRC)/rtext.$(ARCH).o

$(GASTRO2): $(INC)/gastro2.h
$(COORDTEST): $(INC)/gastro2.h
$(EXTR2MASS): $(INC)/gastro2.h

$(BIN)/gastro2.$(ARCH): $(GASTRO2)
$(BIN)/coordtest.$(ARCH): $(COORDTEST)
$(BIN)/extr2mass.$(ARCH): $(EXTR2MASS)
