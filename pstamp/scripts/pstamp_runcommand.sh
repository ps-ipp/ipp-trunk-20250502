#!/bin/sh
###
#
# pstamp_runcommand.sh: 
#
# Set up the ipp environment to run a ipp commands on behalf of the postage stamp
# server and execute the command given by the argument list 
# This script is used by cgi or php scripts.

# echo $# args are: $*

if [[ $# -lt 4 ]] ;then
    echo "usage $0 PSCONFDIR PSCONFIG WORKDIR command" >&2
    #  EINVAL = 22
    exit 22
fi

export PSCONFDIR=$1
shift

PSCONFIG=$1
shift

WORKDIR=$1
shift
export HOME=$WORKDIR


cd $WORK_DIR
status=$?
if [[ $status != 0 ]] ; then
    echo $0 cannot cd to WORK_DIR: $WORK_DIR status: $status >&2
    exit $status
fi

###
### configure the IPP
###

source $PSCONFDIR/psconfig.bash $PSCONFIG
status=$?
if [[ $status != 0 ]] ; then
    echo error setting up IPP environment >&2
    exit $status
fi

#echo path: $PATH >&2
#echo command: $* >&2

## make sure we are able to run the desired command
## XXX: This test is sort of redundant. 
## We'd get a more useful error by just going ahead and trying the command
## or by not suppressing the error output of which
if [[ ! -x `which $1 2> /dev/null` ]] ;then
    status=$?
    echo command $1 not found >&2
    exit $status
fi

#### Finally, execute the command given by the command line arguments

# make sure other users can modify files
# todo: this is of course quite dangerous, avoid this by using proper group permissions
umask 0

$*
