#!/apps/gnu/bin/bash
#
# test compile current fh_registry.h

gcc -Wall t.c -c -o t.o >t.log 2>&1
grep -v "defined but not" t.log
rm -f t.o
