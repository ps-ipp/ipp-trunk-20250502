<?php
// BEGIN Local configuration

$WORKDIR = "set to the location of pstamp working directory";
$dsroot = "set to the root of the data store";
$dbname = "set to the pstamp database name";
$dbserver = "set to the pstamp database server";

$PSCONFDIR = "";
$PSCONFIG  = "";
$PSBINDIR  = "$PSCONFDIR/$PSCONFIG.lin64/bin";

// END Local configuration 

# this script sets up the environment to run IPP commands with current directory
# $WORKDIR
$SCRIPT    = "$PSBINDIR/pstamp_runcommand.sh $PSCONFDIR $PSCONFIG $WORKDIR";

?>
