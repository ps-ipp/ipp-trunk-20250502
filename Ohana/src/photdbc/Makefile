default: photdbc photdbc_client dvocompress dvocompress_client dvodist
help:
	@echo "make options: photdbc photdbc_client dvocompress dvocompress_client dvodist"

include ../../Makefile.System
HOME    =       $(ROOT)/src/photdbc
BIN	=	$(HOME)/bin
LIB	=	$(HOME)/lib
SRC	=	$(HOME)/src
MAN	=	$(HOME)/doc
INC	= 	$(HOME)/include
include ../../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS)
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = -ldvo -lFITS -lohana $(BASE_LDFLAGS)

photdbc: $(BIN)/photdbc.$(ARCH)
photdbc_client: $(BIN)/photdbc_client.$(ARCH)

dvocompress: $(BIN)/dvocompress.$(ARCH)
dvocompress_client: $(BIN)/dvocompress_client.$(ARCH)


dvodist: $(BIN)/dvodist.$(ARCH)

install: $(DESTBIN)/photdbc $(DESTBIN)/photdbc_client $(DESTBIN)/dvocompress $(DESTBIN)/dvocompress_client $(DESTBIN)/dvodist

PHOTDBC = \
$(SRC)/photdbc.$(ARCH).o	   \
$(SRC)/photdbc_catalogs.$(ARCH).o  \
$(SRC)/SetSignals.$(ARCH).o 	   \
$(SRC)/initialize.$(ARCH).o	   \
$(SRC)/ConfigInit.$(ARCH).o	   \
$(SRC)/args.$(ARCH).o		   \
$(SRC)/copy_images.$(ARCH).o	   \
$(SRC)/select_images.$(ARCH).o	   \
$(SRC)/Shutdown.$(ARCH).o          \
$(SRC)/join_stars.$(ARCH).o        \
$(SRC)/make_subcatalog.$(ARCH).o        

PHOTDBC_CLIENT = \
$(SRC)/photdbc_client.$(ARCH).o	   \
$(SRC)/photdbc_catalogs.$(ARCH).o  \
$(SRC)/SetSignals.$(ARCH).o 	   \
$(SRC)/initialize.$(ARCH).o	   \
$(SRC)/ConfigInit.$(ARCH).o	   \
$(SRC)/args.$(ARCH).o		   \
$(SRC)/Shutdown.$(ARCH).o          \
$(SRC)/join_stars.$(ARCH).o        \
$(SRC)/make_subcatalog.$(ARCH).o        

DVOCOMPRESS =                         \
$(SRC)/dvocompress.$(ARCH).o	      \
$(SRC)/dvocompress_catalogs.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o 	      \
$(SRC)/args_dvocompress.$(ARCH).o     \
$(SRC)/Shutdown.$(ARCH).o 

DVOCOMPRESS_CLIENT = 	              \
$(SRC)/dvocompress_client.$(ARCH).o   \
$(SRC)/dvocompress_catalogs.$(ARCH).o \
$(SRC)/SetSignals.$(ARCH).o 	      \
$(SRC)/args_dvocompress.$(ARCH).o     \
$(SRC)/Shutdown.$(ARCH).o

DVODIST = \
$(SRC)/dvodist.$(ARCH).o	    	\
$(SRC)/initialize_dvodist.$(ARCH).o 	\
$(SRC)/Shutdown_dvodist.$(ARCH).o     	\
$(SRC)/SetSignals.$(ARCH).o             \
$(SRC)/md5_ops.$(ARCH).o                \
$(SRC)/md5.$(ARCH).o                    \
$(SRC)/memstr.$(ARCH).o                 \
$(SRC)/HostTableLoad.$(ARCH).o          \
$(SRC)/AssignSkyToHost.$(ARCH).o        \
$(SRC)/CopyBackupToHost.$(ARCH).o     \
$(SRC)/UseBackupForHost.$(ARCH).o     \
$(SRC)/CopyToHostLocation.$(ARCH).o     \
$(SRC)/CopyFromHostLocation.$(ARCH).o   \
$(SRC)/FixSkyForHost.$(ARCH).o   \
$(SRC)/PclientCommands.$(ARCH).o

OLD = \
$(SRC)/liststats.$(ARCH).o	   \
$(SRC)/name_region.$(ARCH).o	   \
$(SRC)/find_images.$(ARCH).o	   \
$(SRC)/find_regions.$(ARCH).o	   \
$(SRC)/get_regions.$(ARCH).o	   \
$(SRC)/load_catalogs.$(ARCH).o	   \
$(SRC)/gcatalog.$(ARCH).o 	   \
$(SRC)/unique_measures.$(ARCH).o   \
$(SRC)/flag_measures.$(ARCH).o     \
$(SRC)/get_mags.$(ARCH).o          \
$(SRC)/check_directory.$(ARCH).o   \
$(SRC)/overlap_funcs.$(ARCH).o     \
$(SRC)/image-db.$(ARCH).o          \
$(SRC)/check_permissions.$(ARCH).o \
$(SRC)/misc.$(ARCH).o		   \
$(SRC)/SetSignals.$(ARCH).o        \
$(SRC)/Shutdown.$(ARCH).o          \
$(SRC)/wcatalog.$(ARCH).o

$(PHOTDBC): $(INC)/photdbc.h
$(BIN)/photdbc.$(ARCH): $(PHOTDBC)

$(PHOTDBC_CLIENT): $(INC)/photdbc.h
$(BIN)/photdbc_client.$(ARCH): $(PHOTDBC_CLIENT)

$(DVOCOMPRESS): $(INC)/dvocompress.h
$(BIN)/dvocompress.$(ARCH): $(DVOCOMPRESS)

$(DVOCOMPRESS_CLIENT): $(INC)/dvocompress.h
$(BIN)/dvocompress_client.$(ARCH): $(DVOCOMPRESS_CLIENT)

$(DVODIST): $(INC)/dvodist.h
$(BIN)/dvodist.$(ARCH): $(DVODIST)
