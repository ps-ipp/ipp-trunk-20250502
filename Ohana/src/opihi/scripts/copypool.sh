#!/bin/csh -f

# copypool.sh -pending (indir)
# copypool.sh -copy (outdir) (path)
# copypool.sh -create (outdir) (name)

if ($#argv < 2) then
  echo "USAGE: copypool.sh -pending (indir)"
  echo "USAGE: copypool.sh -copy (outdir) (path)"
  echo "USAGE: copypool.sh -create (outdir) (name)"
  exit 2
endif

if ("$1" == "-pending") then
  set n = `ls $2 | wc -l`
  if ($n == 0) exit 0
  ls $2/* | cat
  exit 0
endif

if ("$1" == "-copy") then
  set name = `basename $3`
  rm $3
  touch $2/$name
  exit 0
endif

if ("$1" == "-create") then
  touch $2/$3
  exit 0
endif
