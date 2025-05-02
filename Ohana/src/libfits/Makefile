default: install
help:
	@echo "make options: install libfits man clean dist test test.verbose test.clean"

include ../../Makefile.System
HOME	=	$(ROOT)/src/libfits
BIN	=	$(HOME)/bin
LIB	=	$(HOME)/lib
HEAD	=	$(HOME)/header
MATR	=	$(HOME)/matrix
TABL	=	$(HOME)/table
EXT	=	$(HOME)/extern
INC	=	$(HOME)/include
MAN	= 	$(HOME)/doc
TESTDIR	=	$(HOME)/test
TESTBIN	=	$(HOME)/test
include ../../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS) -fPIC
FULL_CPPFLAGS = $(BASE_CPPFLAGS) -I$(EXT)
FULL_LDFLAGS  = $(BASE_LDFLAGS) -lohana

TEST_CFLAGS   =	$(BASE_CFLAGS)
TEST_CPPFLAGS =	$(BASE_CPPFLAGS)
TEST_LDFLAGS  = $(BASE_LDFLAGS) -lFITS -lohana -ltap_ohana

TESTXTRA = tcomptiming 
TESTPROG_X = 
TESTPROG = keywords zlib ricetest imagecomp tablecomp 

$(TESTPROG) : % : $(TESTBIN)/%
$(TESTXTRA) : % : $(TESTBIN)/%
test: $(TESTPROG)
	for i in $(TESTPROG); do $(TESTBIN)/$$i 2>&1 | $(TESTHARNESS); done
test.verbose: $(TESTPROG)
	for i in $(TESTPROG); do $(TESTBIN)/$$i 2>&1 | $(TESTHARNESS) -v; done
test.programs: $(TESTPROG)

install: $(DESTLIB)/libFITS.a $(DESTLIB)/libFITS.$(DLLTYPE) $(DESTMAN)/fits.1
libfits: $(LIB)/libFITS.$(ARCH).a $(LIB)/libFITS.$(ARCH).$(DLLTYPE)
man: $(DESTMAN)/fits.1

INCS = $(DESTINC)/gfitsio.h 

HEADER_OBJ = \
$(HEAD)/F_modify.$(ARCH).o			$(HEAD)/F_create_H.$(ARCH).o  \
$(HEAD)/F_free_H.$(ARCH).o			$(HEAD)/F_H_field.$(ARCH).o   \
$(HEAD)/F_read_H.$(ARCH).o			$(HEAD)/F_write_H.$(ARCH).o   \
$(HEAD)/F_scan.$(ARCH).o			$(HEAD)/F_print.$(ARCH).o     \
$(HEAD)/F_copy_H.$(ARCH).o			$(HEAD)/F_delete.$(ARCH).o    \
$(HEAD)/F_find_H.$(ARCH).o			$(HEAD)/F_init_H.$(ARCH).o    \
$(HEAD)/F_convert_H.$(ARCH).o                   $(HEAD)/F_read_XH.$(ARCH).o   \
$(HEAD)/version.$(ARCH).o

MATRIX_OBJ = \
$(MATR)/F_add_M_value.$(ARCH).o 		$(MATR)/F_add_M.$(ARCH).o          \
$(MATR)/F_create_M.$(ARCH).o 			$(MATR)/F_divide_M.$(ARCH).o       \
$(MATR)/F_free_M.$(ARCH).o 			$(MATR)/F_copy_M.$(ARCH).o         \
$(MATR)/F_write_M.$(ARCH).o 			$(MATR)/F_get_M_value.$(ARCH).o    \
$(MATR)/F_multiply_M.$(ARCH).o 			$(MATR)/F_read_M.$(ARCH).o         \
$(MATR)/F_insert_M.$(ARCH).o 			$(MATR)/F_set_M_value.$(ARCH).o    \
$(MATR)/F_convert_format.$(ARCH).o              $(MATR)/F_read_segment.$(ARCH).o   \
$(MATR)/F_read_portion.$(ARCH).o		$(MATR)/F_load_M.$(ARCH).o	   \
$(MATR)/F_matrix.$(ARCH).o                      $(MATR)/F_compress_utils.$(ARCH).o \
$(MATR)/F_compress_M.$(ARCH).o   		$(MATR)/F_compress_data.$(ARCH).o  \
$(MATR)/F_uncompress_M.$(ARCH).o                $(MATR)/F_uncompress_data.$(ARCH).o \
$(MATR)/F_swap_raw.$(ARCH).o

TABLE_OBJ = \
$(TABL)/F_create_T.$(ARCH).o 			$(TABL)/F_create_TH.$(ARCH).o    \
$(TABL)/F_read_T.$(ARCH).o			$(TABL)/F_read_TH.$(ARCH).o      \
$(TABL)/F_write_T.$(ARCH).o			$(TABL)/F_write_TH.$(ARCH).o     \
$(TABL)/F_define_column.$(ARCH).o		$(TABL)/F_table_format.$(ARCH).o \
$(TABL)/F_set_column.$(ARCH).o			$(TABL)/F_get_column.$(ARCH).o   \
$(TABL)/F_table_row.$(ARCH).o			$(TABL)/F_free_T.$(ARCH).o       \
$(TABL)/F_table_varlength.$(ARCH).o		$(TABL)/F_copy_T.$(ARCH).o       \
$(TABL)/F_compress_T.$(ARCH).o                  $(TABL)/F_uncompress_T.$(ARCH).o

EXTERN_OBJ = \
$(EXT)/fits_hcompress.$(ARCH).o \
$(EXT)/fits_hdecompress.$(ARCH).o \
$(EXT)/pliocomp.$(ARCH).o \
$(EXT)/ricecomp.$(ARCH).o \
$(EXT)/gzip.$(ARCH).o

OBJS = $(HEADER_OBJ) $(MATRIX_OBJ) $(TABLE_OBJ) $(EXTERN_OBJ)

extern: $(EXTERN_OBJ)

$(OBJS): $(INCS)

$(LIB)/libFITS.$(ARCH).a: $(OBJS)
$(LIB)/libFITS.$(ARCH).$(DLLTYPE): $(OBJS)

$(DESTLIB)/libFITS.a:  $(LIB)/libFITS.$(ARCH).a
$(DESTLIB)/libFITS.$(DLLTYPE): $(LIB)/libFITS.$(ARCH).$(DLLTYPE)
