default: libbasiccmd

include ../../../Makefile.System
HOME    =       $(ROOT)/src/opihi
SRC     =       $(HOME)/cmd.basic
BIN     =       $(HOME)/bin
LIB     =       $(HOME)/lib
INC     =       $(HOME)/include
include ../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS) -fPIC
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = $(BASE_LDFLAGS)

# basic user commands ########################

srcs = \
$(SRC)/init.$(ARCH).o       \
$(SRC)/break.$(ARCH).o	     \
$(SRC)/cd.$(ARCH).o	     \
$(SRC)/config.$(ARCH).o     \
$(SRC)/continue.$(ARCH).o   \
$(SRC)/basename.$(ARCH).o     \
$(SRC)/dirname.$(ARCH).o     \
$(SRC)/date.$(ARCH).o	     \
$(SRC)/echo.$(ARCH).o	     \
$(SRC)/file.$(ARCH).o	     \
$(SRC)/getchr.$(ARCH).o     \
$(SRC)/getargs.$(ARCH).o     \
$(SRC)/help.$(ARCH).o	     \
$(SRC)/input.$(ARCH).o	     \
$(SRC)/inthash.$(ARCH).o     \
$(SRC)/list.$(ARCH).o	     \
$(SRC)/list_help.$(ARCH).o  \
$(SRC)/list_vars.$(ARCH).o  \
$(SRC)/local.$(ARCH).o	     \
$(SRC)/macro.$(ARCH).o	     \
$(SRC)/memory.$(ARCH).o     \
$(SRC)/mkdir.$(ARCH).o     \
$(SRC)/module.$(ARCH).o     \
$(SRC)/nop.$(ARCH).o        \
$(SRC)/opihi.$(ARCH).o     \
$(SRC)/output.$(ARCH).o     \
$(SRC)/quit.$(ARCH).o	     \
$(SRC)/run_for.$(ARCH).o    \
$(SRC)/run_foreach.$(ARCH).o    \
$(SRC)/run_if.$(ARCH).o     \
$(SRC)/run_while.$(ARCH).o  \
$(SRC)/scan.$(ARCH).o	     \
$(SRC)/scannext.$(ARCH).o	     \
$(SRC)/shell.$(ARCH).o	     \
$(SRC)/sprintf.$(ARCH).o    \
$(SRC)/fprintf.$(ARCH).o    \
$(SRC)/strlen.$(ARCH).o     \
$(SRC)/substr.$(ARCH).o     \
$(SRC)/strstr.$(ARCH).o     \
$(SRC)/strhash.$(ARCH).o     \
$(SRC)/strmatch.$(ARCH).o     \
$(SRC)/strpop.$(ARCH).o     \
$(SRC)/strsub.$(ARCH).o     \
$(SRC)/usleep.$(ARCH).o     \
$(SRC)/sleep.$(ARCH).o	     \
$(SRC)/wait.$(ARCH).o	     \
$(SRC)/which.$(ARCH).o

# dependancy rules for include files ########################
incs = \
$(INC)/opihi.h \
$(INC)/external.h \
$(INC)/shell.h \
$(INC)/dvomath.h \
$(INC)/display.h 

$(srcs): $(incs)

$(LIB)/libbasiccmd.$(ARCH).a: $(srcs)
$(LIB)/libbasiccmd.$(ARCH).$(DLLTYPE): $(srcs)

$(DESTLIB)/libbasiccmd.a: $(LIB)/libbasiccmd.$(ARCH).a
$(DESTLIB)/libbasiccmd.$(DLLTYPE): $(LIB)/libbasiccmd.$(ARCH).$(DLLTYPE)

libbasiccmd: $(DESTLIB)/libbasiccmd.a $(DESTLIB)/libbasiccmd.$(DLLTYPE)

uninstall:
	rm -f $(DESTLIB)/libbasiccmd.a
	rm -f $(DESTLIB)/libbasiccmd.$(DLLTYPE)
