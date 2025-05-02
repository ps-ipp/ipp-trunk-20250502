
PACKAGES = gpcsw

default: all

all:
	for i in $(PACKAGES); do make $$i || exit; done

install:
	for i in $(PACKAGES); do make $$i.install || exit; done

clean:
	for i in $(PACKAGES); do make $$i.clean || exit; done

update:
	for i in $(PACKAGES); do make $$i.update || exit; done

$(PACKAGES):
	if [ -d "$@" ]; then (cd $@ && make); fi

%.install:
	if [ -d "$*" ]; then make $*; fi
	if [ -d "$*" ]; then (cd $* && make install); fi

%.clean:
	if [ -d "$*" ]; then (cd $* && make clean); fi

%.update:
	if [ -d "$*" ]; then (cd $* && make update); fi


