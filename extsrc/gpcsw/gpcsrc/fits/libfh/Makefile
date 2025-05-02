# `Makefile' - Use with GNU make to build libfh
#
#   This file is part of version 1 of the FITS Handling Library.
#   Read the `License' file for terms of use and distribution.
#   Copyright 2001, Canada-France-Hawaii Telescope, daprog@cfht.hawaii.edu.
#
# ___This header automatically generated from `Index'. Do not edit it here!___
# Makefile for libfh

include ../Make.Common

VERSION = 2.03
CCWARN += $(WERROR)
CCDEFS += -DHAVE_FH_VALIDATE
SRCS = fh.c fh_table.c
HDRS = fh.h fh_registry.h fh_registry.asm

fh_registry.asm: fh_registry.h fh_registry_to_asm.sh
	./fh_registry_to_asm.sh < $< > $@

include ../Make.Common

# Dependencies by Make.Common $Revision: 2.17 $

$(OBJ)/fh.o: fh.c fh.h fh_validate.c
