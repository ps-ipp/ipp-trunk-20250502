default: install
help:
	@echo "make options: install libohana clean dist test test.clean"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/libohana
BIN	= 	$(HOME)/bin
LIB	= 	$(HOME)/lib
SRC	=	$(HOME)/src
MAN	=	$(HOME)/doc
INC	=	$(HOME)/include
TESTDIR	=	$(HOME)/test
TESTBIN	=	$(HOME)/test
include ../../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS) -fPIC
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = $(BASE_LDFLAGS)

TEST_CFLAGS   =	$(BASE_CFLAGS)
TEST_CPPFLAGS =	$(BASE_CPPFLAGS)
TEST_LDFLAGS  = $(BASE_LDFLAGS) -lohana -ltap_ohana

install: $(DESTLIB)/libohana.a $(DESTLIB)/libohana.$(DLLTYPE)
libohana: $(LIB)/libohana.$(ARCH).a $(LIB)/libohana.$(ARCH).$(DLLTYPE)

INCS = \
$(DESTINC)/ohana_allocate.h \
$(DESTINC)/ohana_sort.h \
$(DESTINC)/ohana.h

OBJS = \
$(SRC)/ohana_allocate.$(ARCH).o  \
$(SRC)/sorts.$(ARCH).o		 \
$(SRC)/bisection.$(ARCH).o       \
$(SRC)/gaussdev.$(ARCH).o       \
$(SRC)/string.$(ARCH).o		 \
$(SRC)/findexec.$(ARCH).o	 \
$(SRC)/glockfile.$(ARCH).o	 \
$(SRC)/time.$(ARCH).o		 \
$(SRC)/gaussj.$(ARCH).o		 \
$(SRC)/spherical.$(ARCH).o       \
$(SRC)/vstats.$(ARCH).o		 \
$(SRC)/config.$(ARCH).o		 \
$(SRC)/Fread.$(ARCH).o		 \
$(SRC)/IOBufferOps.$(ARCH).o	 \
$(SRC)/memstr.$(ARCH).o	 	 \
$(SRC)/rconnect.$(ARCH).o	 \
$(SRC)/CommOps.$(ARCH).o	 \
$(SRC)/isolate_elements.$(ARCH).o	 \
$(SRC)/errors.$(ARCH).o	 \
$(SRC)/gprint.$(ARCH).o	 \
$(SRC)/sprintf_floats.$(ARCH).o	 \
$(SRC)/version.$(ARCH).o

# $(SRC)/legendre.$(ARCH).o	 \
# $(SRC)/gsl_utils.$(ARCH).o	 \

$(OBJS): $(INCS)

$(LIB)/libohana.$(ARCH).a: $(OBJS)
$(LIB)/libohana.$(ARCH).$(DLLTYPE): $(OBJS)

$(DESTLIB)/libohana.a:  $(LIB)/libohana.$(ARCH).a
$(DESTLIB)/libohana.$(DLLTYPE): $(LIB)/libohana.$(ARCH).$(DLLTYPE)

TESTPROG = sprintf_floats memtest typetest string 
TESTPROG_XTRA = aliasing

$(TESTPROG) : % : $(TESTBIN)/% $(OBJS)
test: $(TESTPROG)
	for i in $(TESTPROG); do $(TESTBIN)/$$i 2>&1 | $(TESTHARNESS); done
test.verbose: $(TESTPROG) $(TESTPROG_XTRA)
	for i in $(TESTPROG) $(TESTPROG_XTRA); do $(TESTBIN)/$$i 2>&1 |& $(TESTHARNESS) -v; done
