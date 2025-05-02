#!/usr/bin/perl

sub vsystem {
    print STDERR "@_\n";
    $status = system ("@_");
    $status;
}

$database = "/hebe/d27/database/";
$lockfile = $database . "lock";
$reffile = $database . "start.clean";

chdir $database;

@files = `find . -name "*.cpt" -newer $reffile`;
$Nfiles = @files;
print STDERR "N: $Nfiles\n";

if (-e $lockfile) { die "lock file exists"; }

foreach $f (@files) {
    chop ($f);
    vsystem ("markstar -v $f");
    if (-e $lockfile) { die "lock file exists"; }
    vsystem ("addusno -v $f");
    if (-e $lockfile) { die "lock file exists"; }
    vsystem ("markrock -v $f");
    if (-e $lockfile) { die "lock file exists"; }
}

