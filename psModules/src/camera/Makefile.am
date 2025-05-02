noinst_LTLIBRARIES = libpsmodulescamera.la

libpsmodulescamera_la_CPPFLAGS = $(SRCINC) $(PSMODULES_CFLAGS)
libpsmodulescamera_la_LDFLAGS  = -release $(PACKAGE_VERSION)
libpsmodulescamera_la_SOURCES  = \
	pmFPA.c \
	pmFPACalibration.c \
	pmFPAConstruct.c \
	pmFPACopy.c \
	pmFPAHeader.c \
	pmFPAMaskWeight.c \
	pmFPAMosaic.c \
	pmFPARead.c \
	pmFPAUtils.c \
	pmFPAWrite.c \
	pmHDU.c \
	pmHDUUtils.c \
	pmHDUGenerate.c \
	pmFPA_JPEG.c \
	pmFPAview.c \
	pmFPAfile.c \
	pmFPAfileDefine.c \
	pmFPAfileIO.c \
	pmFPAfileFitsIO.c \
	pmFPAfileFringeIO.c \
	pmFPAFlags.c \
	pmFPALevel.c \
	pmFPAExtent.c \
	pmCellSquish.c \
	pmReadoutStack.c \
	pmReadoutFake.c \
	pmFPABin.c \
	pmFPAExpNumIO.c

pkginclude_HEADERS = \
	pmFPA.h \
	pmFPACalibration.h \
	pmFPAConstruct.h \
	pmFPACopy.h \
	pmFPAHeader.h \
	pmFPAMaskWeight.h \
	pmFPAMosaic.h \
	pmFPARead.h \
	pmFPAUtils.h \
	pmFPAWrite.h \
	pmHDU.h \
	pmHDUUtils.h \
	pmHDUGenerate.h \
	pmFPA_JPEG.h \
	pmFPAview.h \
	pmFPAfile.h \
	pmFPAfileDefine.h \
	pmFPAfileIO.h \
	pmFPAfileFitsIO.h \
	pmFPAfileFringeIO.h \
	pmFPAFlags.h \
	pmFPALevel.h \
	pmFPAExtent.h \
	pmCellSquish.h \
	pmReadoutStack.h \
	pmReadoutFake.h \
	pmFPABin.h \
	pmFPAExpNumIO.h

CLEANFILES = *~
