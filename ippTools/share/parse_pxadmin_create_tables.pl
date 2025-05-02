#!/usr/bin/env perl

use warnings;
use strict;

my $ctr=0;
my $old="";
while (<>) {
    s/^\s+//;    #remove whitespace from front
    s/\s*--.*\n$//;   # remove all comments
    s/\s+/ /g; # kill the middle whitespace
    s/\s*$//; # kill the whitespace at the end of the line
# if ($_ !~ /;$/ ) {chomp $_};
    s/;$/;\n/; # add back in the \n that was removed as whitespace but only for ;
    s/,$/, /; # add space at end of , if at end of line
    if (m/CREATE\s+TABLE\s+receive/) {$ctr = 1;}
    if (m/;/) {$ctr = 0;}
    if ($ctr == 0) {
	s/AUTO_INCREMENT// ;
  	if (/FOREIGN\s+KEY/) {
	    if (!/, $/) {           #if FOREIGN KEY line doesn't have ,
		$old =~ s/,\s*$//;       #remove , from previous line
		print $old;
		$old="";
	    }
	} else {
	    print $old;
	    $old=$_;
	} 
    } else { print $_; 
	 }
}
print $old;
print "-- This comment line is here to avoid empty query error.\n";
