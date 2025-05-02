include Configure

all: idx install

idx: 
	mkidx html

install:
	rsync -auvz --delete --delete-excluded --exclude=CVS/ --exclude="*.htm" --exclude="*.idx" html/ $(WWW)

clean:	
	rm -f `find . -name "index.idx"`
	rm -f `find . -name "*.html"`
	rm -f `find . -name "*~"`
	rm -f `find . -name "#*"`
