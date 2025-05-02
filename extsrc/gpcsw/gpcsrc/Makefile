# Top-level Makefile to GigaPixelCamera project tree

default: world-message
include Make.Common
world-message:
	@echo "This is the top-level Makefile for GigaPixelCamera stuff."
	@echo "NOTE: \`make' commands will result in:"
	@echo ""
	@echo "Temporary objects : $(DIR_OBS)     (can be removed to save space)"
	@echo "Include files     : $(DIR_INC) (not required at run-time)"
	@echo "Static libraries  : $(DIR_LIB)     (not required at run-time)"
	@echo "Executables       : $(DIR_BIN)     (add this to your PATH)"
	@echo ""
	@echo "Relocate (mv) this directory tree to a place other than $(DIR_ROOT)"
	@echo "if this is not the desired place for installation."
	@echo ""
	@echo "Type \`make world' to build and install everything."
	@echo "Running \`make dep' first may also be necessary."
	@echo ""
	@/bin/false

# Add new modules here, if they are to build automatically.  See Index.
SUBDIRS=maketools network general fits facility detector

world:	install
	@echo "Congratulations!  All software successfully installed."

# Before installing, build:
install: all

# But find all libraries and install them before building any programs:
all: libinstall
dep depend: preinstall

# And before building libraries, install the header files, etc.
libinstall: preinstall
preinstall: $(DIR_INC)/missing_protos.h checkdirs

# Deal with autofs if necessary for first-time builds.
# This does not do anything if you do not use automounts.
checkdirs:
	for i in $(DIR_CONF) $(DIR_BIN) $(DIR_LIB) $(DIR_INC) $(DIR_OBS); do \
	  ls $$i > /dev/null 2>&1 ; \
	  test -L $$i && mkdir -p `/bin/ls -l $$i | sed -e 's/.*-> //'`||true;\
	done

# missing_protos.h is a hack that doesn't really belong anywhere.
#
$(DIR_INC)/missing_protos.h: missing_protos.h
	@echo "--> Installing header file $<"
	@test -d $(DIR_INC) || $(INSTALL) -m 0775 -d $(DIR_INC)
	@$(INSTALL) -m 0444 $< $@

# Create an empty missing_protos.h if none is needed
missing_protos.h:
	@test -f $@ || touch $@

include Make.Common
