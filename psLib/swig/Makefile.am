
EXTRA_DIST = pslib.i Makefile.PL example/phase2.pl example/data/*

clean-generic:
	rm -rf psLibModule

# this is going into a make file one directory deeper in the tree so the
# realitive paths need to be adjusted
SWIGINC=`echo "${SRCINC}" | ${PERL} -pe "s|-I|-I../|g"`

####################################################
## PsLibModule swig build section.

CLEANFILES = psLibModule

psLibModule:
	if [ ! -d psLibModule ]; then mkdir psLibModule; fi


psLibModule/Makefile.PL: psLibModule
	cp -f $(srcdir)/Makefile.PL psLibModule/Makefile.PL

psLibModule/setup.txt: psLibModule
	echo "includes=-I../$(top_srcdir)/src $(SWIGINC) $(PSLIB_CFLAGS)" > psLibModule/setup.txt
	echo "ldflags=-L$(top_builddir)/src $(PSLIB_LIBS)" >> psLibModule/setup.txt
	echo "cflags=$(CFLAGS) -std=c99 -Wno-unused -Wno-strict-aliasing" >> psLibModule/setup.txt

psLibModule/pslib_wrap.c: psLibModule $(srcdir)/pslib.i
	$(SWIG) -perl $(SRCINC) -w451 -o $@ $(srcdir)/pslib.i

psLibModule/Makefile: psLibModule psLibModule/Makefile.PL psLibModule/setup.txt
	cd psLibModule && $(PERL) Makefile.PL PREFIX=$(PERL_PREFIX)

all: swig

swig: psLibModule/pslib_wrap.c psLibModule/setup.txt psLibModule/Makefile
	cd psLibModule && make

swig-check: swig
	export PERLLIB=psLibModule:psLibModule/blib/arch/auto/pslib && perl -Mpslib -e 'print "pslib version = " . pslib::psLibVersion() . "\n";'

swig-install: swig
	cd psLibModule && make install

install-exec-hook: swig-install


