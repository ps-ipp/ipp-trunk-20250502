default: install
help:
	@echo "make options: install libkapa clean dist"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/libkapa
LIB	= 	$(HOME)/lib
SRC	=	$(HOME)/src
MAN	=	$(HOME)/doc
INC	=	$(HOME)/include
TESTDIR	=	$(HOME)/test
TESTBIN	=	$(HOME)/test
include ../../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS) -fPIC
FULL_CPPFLAGS = $(BASE_CPPFLAGS) -fno-strict-aliasing
FULL_LDFLAGS  = $(BASE_LDFLAGS) -ldvo -lFITS -lohana

install: $(DESTINC)/kapa.h $(DESTLIB)/libkapa.a $(DESTLIB)/libkapa.$(DLLTYPE)
libkapa: $(LIB)/libkapa.$(ARCH).a $(LIB)/libkapa.$(ARCH).$(DLLTYPE)

ROTFONT:
	cd rotfont && $(MAKE) all

INCS = $(DESTINC)/kapa.h

OBJS = \
$(SRC)/IOfuncs.$(ARCH).o  \
$(SRC)/KiiPicture.$(ARCH).o \
$(SRC)/KiiOverlay.$(ARCH).o \
$(SRC)/KiiCursor.$(ARCH).o \
$(SRC)/KiiConvert.$(ARCH).o \
$(SRC)/KapaWindow.$(ARCH).o \
$(SRC)/KapaColors.$(ARCH).o \
$(SRC)/KapaStyles.$(ARCH).o \
$(SRC)/KapaOpen.$(ARCH).o

DRAW = \
$(SRC)/bDrawFuncs.$(ARCH).o     \
$(SRC)/bDrawRotFont.$(ARCH).o   \
$(SRC)/RotFont.$(ARCH).o        \
$(SRC)/DrawRotString.$(ARCH).o  \
$(SRC)/PDFRotFont.$(ARCH).o	\
$(SRC)/PSRotFont.$(ARCH).o

# $(INCS): ROTFONT
$(OBJS): $(INCS) 
$(DRAW): $(INCS)

$(LIB)/libkapa.$(ARCH).a: $(OBJS) $(DRAW)
$(LIB)/libkapa.$(ARCH).$(DLLTYPE): $(OBJS) $(DRAW)

$(DESTLIB)/libkapa.a:  $(LIB)/libkapa.$(ARCH).a  
$(DESTLIB)/libkapa.$(DLLTYPE): $(LIB)/libkapa.$(ARCH).$(DLLTYPE)

TESTPROG = 
$(TESTPROG) : % : $(TESTBIN)/% $(OBJS)
test: $(TESTPROG)
	for i in $(TESTPROG); do $(TESTBIN)/$$i 2>&1 | $(TESTHARNESS); done
test.verbose: $(TESTPROG)
	for i in $(TESTPROG); do $(TESTBIN)/$$i 2>&1 |& $(TESTHARNESS) -v; done
