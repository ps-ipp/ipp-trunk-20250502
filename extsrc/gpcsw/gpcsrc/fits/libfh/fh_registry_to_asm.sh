#!/bin/sh

cat - | \
egrep -e '^MK_(FLT|INT|BLN|STR|VAL)' | \
sed -e 's/^MK_....[^!-~]*\([0-9][0-9.]*\)[^!-~]*,[^!-~]*\([A-Z0-9_][A-Z0-9_]*\)[^"]*"\(.*\)"[^"]*$/DETCOM_\2 MACRO ARG\
    COBJ "detcom:dheader \1 \2 \\"\\ARG\\" \\"\3\\""\
    ENDM/'
