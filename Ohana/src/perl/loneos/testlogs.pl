#!/usr/bin/perl

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

$Nline = -1;
while ($line = <STDIN>) {
    $Nline ++;
    if ($Nline == 0) {	next; }
    if ($Nline == 1) {
	@parts = split (" ",$line);
	$N1 = $parts[9];
	next;
    }
    if ($Nline == 2) {
	@parts = split (" ",$line);
	$N2 = $parts[2];
	if ($N1 != $N2) {
	    print STDERR "problem in dophot\n";
	    exit;
	}
	next;
    }
    if ($Nline == 3) {
	@parts = split (" ",$line);
	$N1 = $parts[10];
	next;
    }
    if ($Nline == 4) {
	@parts = split (" ",$line);
	$N2 = $parts[2];
        if ($N1 != $N2) {
            print STDERR "problem in fstat\n";
	    exit;
        }
	next;
    }
    if ($Nline == 5) {
	@parts = split (" ",$line);
	$N1 = $parts[8];
	next;
    }
    if ($Nline == 6) {
	@parts = split (" ",$line);
	$N2 = $parts[2];
        if ($N1 != $N2) {
            print STDERR "problem in gastro\n";
            exit;
        }
	next;
    }
    if ($Nline == 7) {
	@parts = split (" ",$line);
	$N1 = $parts[6];
	next;
    }
    if ($Nline == 8) {
	@parts = split (" ",$line);
	$N2 = $parts[2];
        if ($N1 != $N2) {
            print STDERR "problem in addstar\n";
            exit;
        }
	next;
    }
    if ($Nline > 8) {
	print STDERR "$line\n";
	print STDERR "too many lines?\n";
	exit;
    }

}

if ($Nline != 8) {
    print STDERR "not enough lines\n";
    exit;
}
