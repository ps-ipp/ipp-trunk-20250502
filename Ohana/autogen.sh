#!/bin/sh

for arg in $*; do
    case $arg in
        --no-configure)
	    exit 0
            ;;
        *)
            ;;
    esac
done

./configure.tcsh $*
