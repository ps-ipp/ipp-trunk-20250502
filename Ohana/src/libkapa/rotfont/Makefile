default: all
help:
	@echo "make options: all (generate fontsize metrics)"

TIMES = \
times8.h \
times12.h \
times14.h \
times18.h \
times24.h

COURIER = \
courier8.h \
courier12.h \
courier14.h \
courier18.h \
courier24.h

HELVETICA = \
helvetica8.h \
helvetica12.h \
helvetica14.h \
helvetica18.h \
helvetica24.h

SYMBOL = \
symbol8.h \
symbol12.h \
symbol14.h \
symbol18.h \
symbol24.h

FONTS = $(TIMES) $(COURIER) $(HELVETICA) $(SYMBOL)

$(FONTS): fixfont

all: $(FONTS)

fixfont: fixfont.c

%.h : %.bdf
	@./fontdimen.sh $<

clean:
#	rm -f times*.h courier*.h helvetica*.h symbol*.h
	rm -f times*.psx courier*.psx helvetica*.psx symbol*.psx
	rm -f fixfont

dist: clean
	rm -f times*.h courier*.h helvetica*.h symbol*.h

.PRECIOUS: .psx
