SHELL=/bin/sh
GLUEFORGE=`which glueforge`

all: ippdb.mdc

ippdb.mdc : ipp.m4 *.md
	m4 ipp.m4 > ippdb.mdc

## please leave the output target as ippdb.src and
## move to ippdb by hand for cvs import
install: ippdb.mdc
	rm -rf ../ippdb.src
	$(GLUEFORGE) -i ippdb.mdc --output ../ippdb.src
	chmod +x ../ippdb.src/autogen.sh

src: ippdb.mdc
	$(GLUEFORGE) -i ippdb.mdc --output ../ippdb
	chmod +x ../ippdb/autogen.sh

foo: src
	$(MAKE) -C ../ippdb install

build : ippdb ippdb.mdc

clean:
	@rm -f *~
