#!/usr/bin/perl

$topdir  = "/pallas/";
$scriptdir = "/hebe/d27/logs/";
$donelogfile = $scriptdir . "done.log";
$scriptfile = $scriptdir . "maestro.dat";
$flatdir = "/hebe/d27/references/config/";

$workspace = "/hebe/d27/workspace/";
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

$Narg = @ARGV;
if ($Narg > 1) {
    goodbye "USAGE: locks.pl [-reset] [-halt] [-kill] [-dB] [-controller] [-archive]";
}

unless ($ARGV[0] cmp "-reset") {
    unlink ($lockfile);
    unlink ($alockfile);
    unlink ($clockfile);
    unlink ($killfile);
    unlink ($haltfile);
    exit;
}

unless ($ARGV[0] cmp "-kill") {
    open (TEMP, ">$killfile");
    close (TEMP);
    exit;
}

unless ($ARGV[0] cmp "-halt") {
    open (TEMP, ">$haltfile");
    close (TEMP);
    exit;
}

unless ($ARGV[0] cmp "-dB") {
    open (TEMP, ">$lockfile");
    close (TEMP);
    exit;
}

unless ($ARGV[0] cmp "-controller") {
    open (TEMP, ">$clockfile");
    close (TEMP);
    exit;
}

unless ($ARGV[0] cmp "-archive") {
    open (TEMP, ">$alockfile");
    close (TEMP);
    exit;
}

@locks = ();
if (-e $lockfile) { @locks = (@locks,$lockfile); }
if (-e $alockfile) { @locks = (@locks,$alockfile); }
if (-e $clockfile) { @locks = (@locks,$clockfile); }
if (-e $killfile) { @locks = (@locks,$killfile); }
if (-e $haltfile) { @locks = (@locks,$haltfile); }

$Nlocks = @locks;

if ($Nlocks > 0) {
    print STDERR "locks set:\n";
    for ($i = 0; $i < $Nlocks; $i++) {
	print STDERR "$locks[$i]\n";
    }
} else {
    print STDERR "no locks currently set\n";
}

print STDERR "\n";
print STDERR "USAGE: locks.pl [-reset] [-kill] [-dB] [-controller] [-archive]\n";
