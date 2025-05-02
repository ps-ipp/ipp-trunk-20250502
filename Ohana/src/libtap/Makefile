default: install
help:
	@echo "make options: install libtap clean dist"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/libtap
BIN	=	$(HOME)/bin
LIB	=	$(HOME)/lib
SRC	=	$(HOME)/src
MAN	=	$(HOME)/doc
INC	= 	$(HOME)/include
TESTDIR	=	$(HOME)/test
TESTBIN	=	$(HOME)/test
include ../../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS) -fPIC
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = $(BASE_LDFLAGS)

install: $(DESTLIB)/libtap_ohana.a $(DESTLIB)/libtap_ohana.$(DLLTYPE)
libtap: $(LIB)/libtap_ohana.$(ARCH).a $(LIB)/libtap_ohana.$(ARCH).$(DLLTYPE)

INCS = $(DESTINC)/tap_ohana.h
TAP = $(SRC)/tap_ohana.$(ARCH).o

$(TAP): $(INCS)

$(LIB)/libtap_ohana.$(ARCH).a: $(TAP)
$(LIB)/libtap_ohana.$(ARCH).$(DLLTYPE): $(TAP)

$(DESTLIB)/libtap_ohana.a:  $(LIB)/libtap_ohana.$(ARCH).a
$(DESTLIB)/libtap_ohana.$(DLLTYPE): $(LIB)/libtap_ohana.$(ARCH).$(DLLTYPE)

# libtap tests are currently not compiled or run

TESTPROG = 
$(TESTPROG) : % : $(TESTBIN)/% $(OBJS)
test: $(TESTPROG)
	for i in $(TESTPROG); do $(TESTBIN)/$$i 2>&1 | $(TESTHARNESS); done
test.verbose: $(TESTPROG)
	for i in $(TESTPROG); do $(TESTBIN)/$$i 2>&1 |& $(TESTHARNESS) -v; done
