#!/usr/bin/env perl
$VERBOSE = 0;

if (@ARGV != 3) { die "USAGE: setsvnversion.pl (prgname) (src) (tgt)\n"; }

if (! -e $ARGV[1]) { die "setsvnversion.pl cannot find input version file\n"; }

$prgname = $ARGV[0];

$svnversion = `which svnversion`;
chomp $svnversion;
if ($?) {
    $svnversion = "NONE";
}
if ($VERBOSE) { print "svnversion: $svnversion\n";}

$svn = `which svn`;
chomp $svn;
if ($?) {
    $svn = "NONE";
}
if ($VERBOSE) {print "svn: $svn\n";}

if ($svnversion eq "NONE") {
    $VERSION = "UNKNOWN";
} else {
    $VERSION = `$svnversion`;
    chomp $VERSION;
}
if ($VERBOSE) {print "VERSION: $VERSION\n";}

if ($svn eq "NONE") {
    $BRANCH = "UNKNOWN";
    $SOURCE = "UNKNOWN";
} else {
    $fullURL = "";
    $reproot = "";
    $UUID = "";
    @lines = `$svn info ..`;
    foreach $line (@lines) {
	chomp $line;
	($key, $value) = split (":", $line, 2);
	if ($VERBOSE == 2) { print "key: $key\n";}
	if ($VERBOSE == 2) { print "value: $value\n"; }
	if ($key eq "URL")             { $fullURL = $value; }
	if ($key eq "Repository UUID") { $UUID = $value;    }
	if ($key eq "Repository Root") { $reproot = $value; }
    }
    if ($fullURL eq "") { die "failed to find URL\n"; }
    if ($reproot eq "") { die "failed to find reproot\n"; }
    if ($UUID    eq "") { die "failed to find UUID\n"; }

    # remove surrounding whitespace
    $fullURL =~ s/^\s+//;  $fullURL =~ s/\s+$//;
    $reproot =~ s/^\s+//;  $reproot =~ s/\s+$//;
    $UUID    =~ s/^\s+//;  $UUID    =~ s/\s+$//;

    ($BRANCH) = $fullURL =~ m|$reproot/(.+)|;
    if ($BRANCH eq "")  { die "failed to get branch from fullURL $fullURL\n"; }
   
    $SOURCE = $UUID;
}

if ($VERBOSE) { print "SOURCE: $SOURCE\n";}
if ($VERBOSE) { print "BRANCH: $BRANCH\n";}

open (FILE, "$ARGV[1]");
@list = <FILE>;
close (FILE);

$version_field = sprintf "@%s_VERSION@", $prgname;
$source_field = sprintf "@%s_SOURCE@", $prgname;
$branch_field = sprintf "@%s_BRANCH@", $prgname;

foreach $line (@list) {
    $line =~ s|$version_field|"$VERSION"|;
    $line =~ s|$source_field|"$SOURCE"|;
    $line =~ s|$branch_field|"$BRANCH"|;
}

$output = sprintf "%s.tmp", $ARGV[2];

open (FILE, ">$output");
foreach $line (@list) {
    print FILE $line;
}
close (FILE);

if (! -e $ARGV[2]) {
    if ($VERBOSE) { 
	print "prior $ARGV[2] not found, replacing with $output\n";
    }
    rename $output, $ARGV[2];
    exit 0;
}

$difflines = `diff $ARGV[2] $output`;
if ($difflines eq "") {
    print "  setsvnversion.pl: no change to $ARGV[2], keeping old version\n";
} else {
    if ($VERBOSE) { 
	print "$ARGV[2] changed:\n";
	print $difflines;
    }
    rename $output, $ARGV[2];
}

exit (0);
