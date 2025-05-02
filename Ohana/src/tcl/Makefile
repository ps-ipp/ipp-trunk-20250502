include ../../Makefile.System
HOME 	=	$(ROOT)/src/tcl

tcl:

default: install

CODE =  edisp

install:
	@if [ ! -d $(DESTBIN) ]; then mkdir -p $(DESTBIN); fi
	for file in $(CODE); do \
	(cd $(DESTBIN); rm -f $$file; ln -s $(HOME)/src/$$file .); \
	done

# utilities #################################################
clean:	
	rm -f `find . -name "*~"`
	rm -f `find . -name "#*"`

