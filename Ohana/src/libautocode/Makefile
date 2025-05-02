default: autocode
help:
	@echo "make options: install autocode latex objects"

include ../../Makefile.System
HOME	=	$(ROOT)/src/libautocode
BIN	=	$(HOME)/bin
LIB	=	$(HOME)/lib
SRC	=	$(HOME)/src
MAN	= 	$(HOME)/doc
DEF	=	$(HOME)/def
TEX	=	$(HOME)/tex
INC	=	$(HOME)/include
ASRC    =       $(SRC)
AINC    =       $(INC)
include ../../Makefile.Common

# programs may add their own internal requirements here
FULL_CFLAGS   = $(BASE_CFLAGS) -fPIC
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = $(BASE_LDFLAGS)

latex:    $(LIBTEX)
objects:  $(AOBJS)
install:  $(DESTINC)/autocode.h $(DESTLIB)/libautocode.a $(DESTLIB)/libautocode.$(DLLTYPE)
autocode: $(INC)/autocode.h $(LIB)/libautocode.$(ARCH).a $(LIB)/libautocode.$(ARCH).$(DLLTYPE)

# this library is not normally installed: the objects in the
# file included below are rolled into libdvo 
include Makefile.Targets

LIBTEX = \
$(TEX)/coords.tex \
$(TEX)/average.tex \
$(TEX)/secfilt.tex \
$(TEX)/measure.tex \
$(TEX)/missing.tex \
$(TEX)/photcode.tex \
$(TEX)/image.tex 

$(LIB)/libautocode.$(ARCH).a: $(AOBJS)
$(LIB)/libautocode.$(ARCH).$(DLLTYPE): $(AOBJS)

$(DESTLIB)/libautocode.a:  $(LIB)/libautocode.$(ARCH).a
$(DESTLIB)/libautocode.$(DLLTYPE): $(LIB)/libautocode.$(ARCH).$(DLLTYPE)
$(DESTINC)/autocode.h : $(INC)/autocode.h

# $(AOBJS): $(DEF)/autocode.c $(DESTINC)/autocode.h

# 
$(AOBJS): $(DEF)/autocode.c $(INC)/autocode.h

$(INC)/autocode.h: $(AINCS) $(DEF)/autocode.h $(DEF)/common.h
	cat $(DEF)/common.h >  $(INC)/autocode.h
	cat $(AINCS)        >> $(INC)/autocode.h
	@echo "# endif"     >> $(INC)/autocode.h
	@echo done with autocode.h

$(INC)/%.h: $(DEF)/%.d $(DEF)/autocode.h $(DEF)/common.h
	@if [ ! -d $(INC) ]; then mkdir -p $(INC); fi
	./generate $< $(DEF)/autocode.h $@

$(SRC)/%.c: $(DEF)/%.d $(DEF)/autocode.c $(INC)/%.h
	@if [ ! -d $(SRC) ]; then mkdir -p $(SRC); fi
	./generate $< $(DEF)/autocode.c $@

$(TEX)/%.tex: $(DEF)/%.d $(DEF)/autocode.tex
	@if [ ! -d $(TEX) ]; then mkdir -p $(TEX); fi
	./generate $< $(DEF)/autocode.tex $@

clean:	cleandef
cleandef:
	rm -f `find src -name "*.c"`
	rm -f `find include -name "*.h"`
# 	this is an unusual target!  it deletes .c and .h files!

.PRECIOUS: $(SRC)/%.c
.PRECIOUS: $(LIB)/%.$(ARCH).a
.PRECIOUS: $(LIB)/%.$(ARCH).$(DLLTYPE)
