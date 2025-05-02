1default: install
help:
	@echo "make options: install libskycalc all programs clean dist"

include ../../Makefile.System
HOME	=	$(ROOT)/src/skycalc
BIN	= 	$(HOME)/bin
LIB	=	$(HOME)/lib
SRC	=	$(HOME)/src
MAN	= 	$(HOME)/doc
INC	=	$(HOME)/include
include ../../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS) -fPIC
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = -lskycalc -lohana $(BASE_LDFLAGS)

install: $(DESTINC)/skycalc.h $(DESTLIB)/libskycalc.a
libskycalc: $(DESTLIB)/libskycalc.a $(DESTLIB)/libskycalc.$(DLLTYPE)

all: libskycalc dusktime moondata sundata sunriseset sunmoon
programs: all

INCS = $(DESTINC)/skycalc.h

MYINCS = $(INCS) $(INC)/skycalc_internal.h

OBJS = 	\
$(SRC)/time.$(ARCH).o     \
$(SRC)/geometry.$(ARCH).o \
$(SRC)/astro.$(ARCH).o    \
$(SRC)/sun.$(ARCH).o      \
$(SRC)/moon.$(ARCH).o

$(OBJS): $(MYINCS)

$(LIB)/libskycalc.$(ARCH).a: $(OBJS)
$(LIB)/libskycalc.$(ARCH).$(DLLTYPE): $(OBJS)

$(DESTLIB)/libskycalc.a:  $(LIB)/libskycalc.$(ARCH).a
$(DESTLIB)/libskycalc.$(DLLTYPE): $(LIB)/libskycalc.$(ARCH).$(DLLTYPE)

dusktime : $(BIN)/dusktime.$(ARCH)
sunriseset : $(BIN)/sunriseset.$(ARCH)
moondata : $(BIN)/moondata.$(ARCH)
sundata  : $(BIN)/sundata.$(ARCH)
sunmoon  : $(BIN)/sunmoon.$(ARCH)
sun_angle : $(BIN)/sun_angle.$(ARCH)

DUSKTIME = $(SRC)/dusktime.$(ARCH).o
SUNRISESET = $(SRC)/sunriseset.$(ARCH).o
MOONDATA = $(SRC)/moondata.$(ARCH).o
SUNDATA  = $(SRC)/sundata.$(ARCH).o
SUNMOON  = $(SRC)/sunmoon.$(ARCH).o
SUN_ANGLE = $(SRC)/sun_angle.$(ARCH).o

$(DUSKTIME) : $(MYINCS)
$(SUNRISESET) : $(MYINCS)
$(MOONDATA) : $(MYINCS)
$(SUNDATA)  : $(MYINCS)
$(SUNMOON)  : $(MYINCS)
$(SUN_ANGLE)  : $(MYINCS)

$(BIN)/dusktime.$(ARCH) : $(DUSKTIME)
$(BIN)/sunriseset.$(ARCH) : $(SUNRISESET)
$(BIN)/moondata.$(ARCH) : $(MOONDATA)
$(BIN)/sundata.$(ARCH)  : $(SUNDATA)
$(BIN)/sunmoon.$(ARCH)  : $(SUNMOON)
$(BIN)/sun_angle.$(ARCH)  : $(SUN_ANGLE)

INSTALL = dusktime sunriseset moondata sundata sunmoon sun_angle

# dependancy rules for binary code #########################
$(INSTALL): % : $(BIN)/%.$(ARCH)

%.clean :
	rm -f $(BIN)/$*.$(ARCH)

%.install:
	$(MAKE) $(DESTBIN)/$*

install:
	for i in $(INSTALL); do $(MAKE) $$i.install || exit; done
