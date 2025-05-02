include ../Make.Common

VERSION=0.00
CCWARN+=$(WERROR)
CCDEFS+=$(VERSIONDEFS)
$(EXECNAME): $(OBJS) libfh.a

include ../Make.Common

# Dependencies by Make.Common $Revision: 2.17 $

$(OBJ)/sort.o: sort.c

$(OBJ)/tiltystreak.o: tiltystreak.c fh/fh.h \
  fhreg/general.h fhreg/macros.h \
  fhreg/gpc_detector.h
