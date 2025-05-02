default: misc
help:
	@echo "make options: misc (default)"

include ../../Makefile.System
HOME 	=	$(ROOT)/src/misc
SRC	=	$(HOME)/src
LIB	= 	$(HOME)/lib
MAN	=	$(HOME)/doc
BIN	=	$(HOME)/bin
INC	=	$(HOME)/include

FULL_CFLAGS   =	$(CFLAGS)
FULL_CPPFLAGS = $(CPPFLAGS) -I$(INC) -I$(DESTINC) $(INCDIRS) -D$(ARCH)
FULL_LDFLAGS  = $(LDFLAGS) -L$(LIB) -L$(DESTLIB) $(LIBDIRS) -ldvo -lFITS -lohana $(LIBFLAGS)

# these are all programs which just depend on a single c file: foo : foo.c
# we use a special set of rules in this directory which expect this simplification

PROGRAMS = mknames mkgauss mkfringetable gtfringetable applyscat fiximg

misc: $(PROGRAMS)

$(PROGRAMS): % : $(BIN)/%.$(ARCH)

fakestars: $(BIN)/fakestars.$(ARCH)
$(BIN)/fakestars.$(ARCH): $(SRC)/fakestars.$(ARCH).o $(SRC)/random.$(ARCH).o

#### cfhtlog is special (requires additional libs and includes)

$(BIN)/cfhtlog.$(ARCH): $(SRC)/cfhtlog.c
	$(CC) -I/cfht/include -L/cfht/lib -o $(BIN)/cfhtlog.$(ARCH) $(SRC)/cfhtlog.c -lcfht 

$(DESTBIN)/cfhtlog: $(BIN)/cfhtlog.$(ARCH)
	@if [ ! -d $(DESTBIN) ]; then mkdir -p $(DESTBIN); fi
	cp $(SRC)/cfhtlog.$(ARCH) $(DESTBIN)/

cfhtlog:
	$(MAKE) $(DESTBIN)/cfhtlog

# copied from Makefile.Common : use a single c file: foo : foo.c
.PRECIOUS: %.$(ARCH).o
.PRECIOUS: $(BIN)/%.$(ARCH)

%.$(ARCH).o : %.c
	$(CC) $(FULL_CFLAGS) $(FULL_CPPFLAGS) -c $< -o $@

$(BIN)/%.$(ARCH) : $(SRC)/%.$(ARCH).o
	@if [ ! -d $(BIN) ]; then mkdir -p $(BIN); fi
	$(CC) $(FULL_CFLAGS) -o $@ $^ $(FULL_LDFLAGS)

$(DESTBIN)/%: $(BIN)/%.$(ARCH) 
	@if [ ! -d $(DESTBIN) ]; then mkdir -p $(DESTBIN); fi
	rm -f $(DESTBIN)/$*
	cp $(BIN)/$*.$(ARCH) $(DESTBIN)/$*

%.install:
	$(MAKE) $(DESTBIN)/$*

install:
	for i in $(PROGRAMS); do $(MAKE) $$i.install || exit; done

%.clean:
	rm -f $(SRC)/$*.$(ARCH).o
	rm -f $(BIN)/$*.$(ARCH)

clean:
	rm -f `find . -name "*.o"`
	rm -f `find . -name "*.o"`
	rm -f `find . -name "*~"`
	rm -f `find . -name "#*"`

dist: clean
	rm -rf $(BIN)
	rm -rf $(LIB)
