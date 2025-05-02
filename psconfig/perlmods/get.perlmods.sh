#!/bin/csh -f

if ($#argv != 2) then
   echo "USAGE: get.perlmods.sh (mode) (topdir)"
   exit 2
endif

# mode = bin, lib, file

# find the perl module files (*.pm) below the topdir

set mode = $1
set topdir = $2

if ("$mode" == "lib") then
  set tmpout = `mktemp`
  set list = `find $topdir -name "*.pm"`
  foreach file ($list)
    grep "^\s*use " $file | tr ';' ' ' | awk '{print $2}' >> $tmpout
  end

  sort $tmpout | uniq -c
  exit 0
endif

if ("$mode" == "bin") then
  set tmpout = `mktemp`
  foreach file ($topdir/*)
    grep "^\s*use " $file | tr ';' ' ' | awk '{print $2}' >> $tmpout
  end
  set list = `find $topdir -name "*.pl"`
  foreach file ($list)
    grep "^\s*use " $file | tr ';' ' ' | awk '{print $2}' >> $tmpout
  end

  sort $tmpout | uniq -c
  exit 0
endif

if ("$mode" == "file") then
  set tmpout = `mktemp`
  grep "^\s*use " $topdir | tr ';' ' ' | awk '{print $2}' >> $tmpout

  sort $tmpout | uniq -c
  exit 0
endif

echo "unknown mode $mode"
exit 2
