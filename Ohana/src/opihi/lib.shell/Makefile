default: libshell

include ../../../Makefile.System
HOME    =       $(ROOT)/src/opihi
BIN     =       $(HOME)/bin
LIB     =       $(HOME)/lib
INC     =       $(HOME)/include
SDIR    =       $(HOME)/lib.shell
include ../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS) -fPIC
FULL_CPPFLAGS = $(BASE_CPPFLAGS) -fno-strict-aliasing
FULL_LDFLAGS  = $(BASE_LDFLAGS)

# opihi shell functions (libopihi) ########################
shell = \
$(SDIR)/BufferOps.$(ARCH).o             \
$(SDIR)/CommandOps.$(ARCH).o		\
$(SDIR)/ConfigInit.$(ARCH).o		\
$(SDIR)/ListOps.$(ARCH).o	   	\
$(SDIR)/MacroOps.$(ARCH).o		\
$(SDIR)/SocketOps.$(ARCH).o             \
$(SDIR)/VariableOps.$(ARCH).o	   	\
$(SDIR)/VectorOps.$(ARCH).o             \
$(SDIR)/VectorIO.$(ARCH).o             \
$(SDIR)/check_stack.$(ARCH).o           \
$(SDIR)/command.$(ARCH).o               \
$(SDIR)/convert_to_RPN.$(ARCH).o	\
$(SDIR)/dvomath.$(ARCH).o               \
$(SDIR)/evaluate_stack.$(ARCH).o  	\
$(SDIR)/exec_loop.$(ARCH).o             \
$(SDIR)/expand_vars.$(ARCH).o           \
$(SDIR)/expand_vectors.$(ARCH).o        \
$(SDIR)/gprint.$(ARCH).o		\
$(SDIR)/interrupt.$(ARCH).o	        \
$(SDIR)/macro_create.$(ARCH).o          \
$(SDIR)/macro_delete.$(ARCH).o          \
$(SDIR)/macro_edit.$(ARCH).o            \
$(SDIR)/macro_exec.$(ARCH).o            \
$(SDIR)/macro_funcs.$(ARCH).o           \
$(SDIR)/macro_list.$(ARCH).o            \
$(SDIR)/macro_read.$(ARCH).o            \
$(SDIR)/macro_write.$(ARCH).o		\
$(SDIR)/memstr.$(ARCH).o                \
$(SDIR)/multicommand.$(ARCH).o          \
$(SDIR)/parse.$(ARCH).o                 \
$(SDIR)/parse_commands.$(ARCH).o	\
$(SDIR)/stack_math.$(ARCH).o		\
$(SDIR)/startup.$(ARCH).o		\
$(SDIR)/string.$(ARCH).o                \
$(SDIR)/timeformat.$(ARCH).o            \
$(SDIR)/version.$(ARCH).o	   	\
$(SDIR)/opihi.$(ARCH).o

# dependancy rules for include files ########################
incs = \
$(INC)/opihi.h \
$(INC)/external.h \
$(INC)/shell.h \
$(INC)/dvomath.h \
$(INC)/display.h 

$(shell) : $(incs)

$(LIB)/libshell.$(ARCH).a: $(shell)
$(LIB)/libshell.$(ARCH).$(DLLTYPE): $(shell)

$(DESTLIB)/libshell.a: $(LIB)/libshell.$(ARCH).a
$(DESTLIB)/libshell.$(DLLTYPE): $(LIB)/libshell.$(ARCH).$(DLLTYPE)

libshell: $(DESTLIB)/libshell.a $(DESTLIB)/libshell.$(DLLTYPE)

uninstall:
	rm -f $(DESTLIB)/libshell.a
	rm -f $(DESTLIB)/libshell.$(DLLTYPE)
