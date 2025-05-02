#!/bin/bash

# This script creates the pos and neg files that the ppMops program
# needs as input.
#
# Only one argument is required: the diff_id of the exposure
#
# e.g.:
# $0 308413
#
DIFFID=$1
if [ "$DIFFID" == "" ]; then
  echo "$0 <diff_id>"
  echo "  Create the pos and neg files that the ppMops program"
  echo "  needs as input."
  exit 1
fi

OUTPUTROOT=test.$DIFFID

for f in `difftool -dbname gpc1 -diffskyfile -diff_id $DIFFID | grep path_base | sed 's/path_base//g' | sed 's/ STR //g' | tr -d ' '`; do
  neb-ls -p $f.cmf >> $OUTPUTROOT.pos
done

for f in `difftool -dbname gpc1 -diffskyfile -diff_id $DIFFID | grep path_base | sed 's/path_base//g' | sed 's/ STR //g' | tr -d ' '`; do
  neb-ls -p $f.inv.cmf >> $OUTPUTROOT.neg
done

