#!/bin/sh

#
# pstamp_new_request.sh
#
# create a postage stamp request file and add it to the data store

# This is a simple program for testing purposes only and not
# part of the postage stamp server

# TODO: use getopt and take these configuration variables as command
# line arguments or convert this script to perl and use the ipp config

DATA_STORE=/var/www/html/ds/dsroot
PRODUCT=pstamprequest

if [[ $# == 0 ]] ; then
    echo "usage: $0 fileset_id pstamp_request_arguments"
    exit 22
fi

fileset_id=$1
shift

dir_path=${DATA_STORE}/${PRODUCT}/${fileset_id}
echo $dir_path
if [[ -e $dir_path ]] ; then
    echo fileset $fileset_id already exists in $dir_path
    exit 1
fi

if ! mkdir -p $dir_path ; then
    $status = $?
    echo failed to mkdir $dir_path
    exit $status
fi

request_file=${fileset_id}.fits

pstamprequest ${dir_path}/${request_file} -req_name $fileset_id $*
status=$?
if [[ $status != 0 ]] ; then
    echo pstamprequest failed: $status
    rm -r $dir_path
    exit $status
fi

# Invoke the data store registration script
echo $request_file\|\|\|psrequest\| | dsreg --add $fileset_id --type PSREQUEST --product $PRODUCT  --list -

status=$?
if [[ $status != 0 ]] ; then
    echo dsreg failed: $status
    rm -r $dir_path
    exit $status
fi

exit 0

