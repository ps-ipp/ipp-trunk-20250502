#!/bin/csh -f

foreach ball (`grep -v "^#" ipp-test.perl | prcol 3`)
  tar xvzf $ball
  set topdir = `echo $ball | sed s/.tar.gz//`
  get.perlmods.sh lib $topdir > $topdir.mods.txt
end
