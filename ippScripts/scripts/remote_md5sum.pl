#! /usr/bin/env perl

use warnings;
use strict;

#my $time = time();
my $filename = shift(@ARGV);

unless (defined($filename)) { 
    die;
}

open(LA,"/proc/loadavg"); # possibly not portable.
my $load = (split /\s+/, <LA>)[0];
close(LA);

if ($load > 30) {
    my $sleep_dur = int($load - 8);
    
#    print STDERR "$load $sleep_dur $time\n";
#    $time = time();
#    print STDERR "$time\n";
    sleep($sleep_dur);
#    $time = time();
#    print STDERR "RESUME $time\n";
}
my $response = `md5sum $filename`;
print $response;
#$time = time();
#print STDERR "END $time\n";
