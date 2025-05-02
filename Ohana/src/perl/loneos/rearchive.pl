#!/usr/bin/perl

# $dumpspace = "/pallas/d1/";
# $workspace = "/metis/d27/workspace/";
# $workspace = "/irene/d11/workspace/";

$database = "/hebe/d27/database/";
$lockfile = $database . "lock";
$alockfile = $database . "alock";
$clockfile = $database . "clock";
$killfile  = $database . "kill";
$haltfile  = $database . "halt";
$reffile = $database . "start.clean";

sub vsystem {
    print STDERR "@_\n";
    $status = system ("@_");
    $status;
}

sub goodbye {
    print STDERR "ending execution\n";
    vsystem ("date");
    die "@_";
}

if (@ARGV < 1) {die "USAGE: rearchive.pl dir\n"; }

$target = $ARGV[0];

print STDERR "starting execution\n";
vsystem ("date");
if (-e $killfile) { goodbye "process killed with $killfile"; }

if (-e $alockfile) { goodbye "archive lock $alockfile is set"; }
open (TEMP, ">$alockfile");
close (TEMP);

# untar directory 

if (!-e $target.tgz) { goodbye "target $target.tgz not found"; }

if (vsystem ("gunzip -c $target.tgz | tar xvf -")) { goodbye "untar failed for $target.tgz"; }
if (vsystem ("ls $target/*.cmp > $target.lst")) { goodbye "failed to get directory listing for $target"; }

open (TEMP, ">$target.log");
close (TEMP);
if (!-e $target.log) { goodbye "could not create $target.log"; }

# load in done date list
open (LISTFILE, $target.lst);
@listfile = ();
while ($line = <LISTFILE>) {
  chop ($line);
  @listfile = (@listfile,$line);
}
$Nlist = @listfile;
close (LISTFILE);

for ($i = 0; $i < Nlist; $i++) {
    if (-e $lockfile) { goodbye "database lock $lockfile is set"; }
    vsystem ("addstar -v $listfile[$i]");
}

if (-e $lockfile) { goodbye "lock file exists"; }
vsystem ("cleaning.pl");
if (-e $lockfile) { goodbye "lock file exists"; }

# vsystem ("rm $target");
exit;

