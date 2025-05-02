
### this is the top-level makefile for the opihi collection of programs

default: all

### the include file dependencies need to be cleaned up still

mana:     	 lib.data lib.shell cmd.basic cmd.data cmd.astro
dimm:     	 lib.data lib.shell cmd.basic cmd.data cmd.astro
dvo:      	 lib.data lib.shell cmd.basic cmd.data cmd.astro
dvo_client:    	 lib.data lib.shell cmd.basic cmd.data cmd.astro
pantasks: 	 lib.data lib.shell cmd.basic cmd.data cmd.astro
pantasks_server: lib.data lib.shell cmd.basic cmd.data cmd.astro
pantasks_client: lib.data lib.shell cmd.basic cmd.data cmd.astro
pclient:  	 lib.data lib.shell cmd.basic
pcontrol: 	 lib.data lib.shell cmd.basic

LIBS = lib.data lib.shell cmd.basic cmd.data cmd.astro

PROGRAM = mana dvo pantasks pclient pcontrol
SPECIAL = pantasks_client pantasks_server dvo_client

EXTRAS = dimm

all:
	for i in $(PROGRAM) $(SPECIAL); do $(MAKE) $$i || exit; done

libs:
	for i in $(LIBS); do $(MAKE) $$i || exit; done

extras:
	for i in $(EXTRAS); do $(MAKE) $$i || exit; done

install:
	for i in $(PROGRAM) $(SPECIAL); do $(MAKE) $$i.install || exit; done

extras-install:
	for i in $(EXTRAS); do $(MAKE) $$i.install || exit; done

clean:
	for i in $(PROGRAM) $(SPECIAL) $(EXTRAS) $(LIBS); do $(MAKE) $$i.clean || exit; done

libs-uninstall:
	for i in $(LIBS); do $(MAKE) $$i.uninstall || exit; done

dist: clean
	for i in $(PROGRAM) $(SPECIAL) $(EXTRAS) $(LIBS); do $(MAKE) $$i.dist || exit; done
	rm -rf bin
	rm -rf lib

#############################################################

pantasks_client pantasks_server:
	if [ -d pantasks ]; then (cd pantasks && $(MAKE) $@); fi

pantasks_client.install: pantasks_client
	if [ -d pantasks ]; then (cd pantasks && $(MAKE) $@); fi

pantasks_server.install: pantasks_server
	if [ -d pantasks ]; then (cd pantasks && $(MAKE) $@); fi

dvo_client:
	if [ -d dvo ]; then (cd dvo && $(MAKE) $@); fi

dvo_client.install: dvo_client
	if [ -d dvo ]; then (cd dvo && $(MAKE) $@); fi

$(PROGRAM) $(LIBS) $(EXTRAS):
	if [ -d "$@" ]; then (cd $@ && $(MAKE)); fi

%.install:
	if [ -d "$*" ]; then $(MAKE) $*; fi
	if [ -d "$*" ]; then (cd $* && $(MAKE) install); fi

%.clean:
	if [ -d "$*" ]; then (cd $* && $(MAKE) clean); fi

%.dist:
	if [ -d "$*" ]; then (cd $* && $(MAKE) dist); fi

%.uninstall:
	if [ -d "$*" ]; then (cd $* && $(MAKE) uninstall); fi

.PHONY: $(PROGRAM) $(LIBS) pantasks_server pantasks_client
