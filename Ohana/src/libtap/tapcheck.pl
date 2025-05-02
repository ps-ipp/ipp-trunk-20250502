#!/usr/bin/env perl

# command line options:
# -v (pass through tap output)
# -vv (pass through comments as well)
$VERBOSE = 0;
foreach $arg (@ARGV) {
    if ($arg eq "-v") { $VERBOSE = 1; }
    # if ($arg eq "-vv") { $VERBOSE = 2; }
}

# capture stdin and generate a summary
@list = <STDIN>;

$headline = "";
$Npass = 0;
$Nfail = 0;
foreach $line (@list) {
    chomp $line;
    if ($line =~ m|^ok|)     { $Npass ++; }
    if ($line =~ m|^not ok|) { $Nfail ++; }
    if ($line =~ m|^#|) { 
	$Ncomment ++; 
	if ($headline eq "") { 
	    $headline = $line; 
	} else {
	    # skip all but the first comment (headline)
	    if ($VERBOSE) { next; }
	}
    }
    if ($VERBOSE) {
	print "$line\n";
    }
}

if (!$VERBOSE) { print "$headline\n"; }
print "Npass: $Npass, Nfail: $Nfail\n";

if ($Nfail) { exit 1; }
exit 0;

