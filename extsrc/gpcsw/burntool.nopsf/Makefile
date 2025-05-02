include ../Make.Common

VERSION=0.00
# CCWARN+=$(WERROR)
CCDEFS+=$(VERSIONDEFS)
CCLINK += -lm
# $(EXECNAME): $(OBJS) libfh.a libpsf.a libpscoords.a
$(EXECNAME): $(OBJS) libfh.a libpscoords.a

include ../Make.Common

# Dependencies by Make.Common $Revision: 2.17 $

$(OBJ)/burnfix.o: burnfix.c burntool.h burnparams.h

$(OBJ)/burntool.o: burntool.c fh/fh.h fhreg/general.h \
  fhreg/macros.h fhreg/gpc_detector.h \
  burntool.h burnparams.h persist_fits.h

$(OBJ)/burnutils.o: burnutils.c burntool.h burnparams.h

$(OBJ)/cheapfits.o: cheapfits.c

$(OBJ)/persist_fits.o: persist_fits.c fh/fh.h burntool.h \
  burnparams.h persist_fits.h

$(OBJ)/persistfix.o: persistfix.c burntool.h burnparams.h

$(OBJ)/persistio.o: persistio.c burntool.h burnparams.h

$(OBJ)/psfstamp.o: psfstamp.c burntool.h burnparams.h \
  pscoords/pscoords.h

$(OBJ)/psfstats.o: psfstats.c burntool.h

$(OBJ)/sort.o: sort.c

$(OBJ)/stardetect.o: stardetect.c burntool.h burnparams.h

$(OBJ)/trailfit.o: trailfit.c burntool.h burnparams.h
