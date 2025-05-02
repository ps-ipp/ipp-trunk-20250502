#!/usr/bin/env perl

# Program to convert a PS metadata config syntax (usually from one of
# the IPP tools) into a flat tab-delimited list suitable for parsing
# using "split".

# Copyright (C) 2006  Joshua Hoblitt, Paul A. Price.

use strict;
use warnings;

use PS::IPP::Metadata::Config;	# Supplies the metadata config parser.

die "Program to convert a PS metadata config file (usually from one of the IPP\n" .
    "tools) into a flat tab-delimited list suitable for parsing using \'split\'.\n\n" .
    "Usage: $0 [INFILE [OUTFILE]]\n" if ((join "", @ARGV) =~ /--help/ or scalar @ARGV > 2);

my $inFile;			# Input file
my $outFile;			# Output file
if (scalar @ARGV >= 1) {
    my $inName = shift @ARGV;
    open $inFile, $inName or die "Can't open $inName: $!\n";
    if (scalar @ARGV == 1) {
	my $outName = shift @ARGV;
	open $outFile, ">", $outName or die "Can't open $outName: $!\n";
    } else {
	$outFile = *STDOUT;
    }
} else {
    $inFile = *STDIN;
    $outFile = *STDOUT;
}

my @input = <$inFile>;		# Contents of the metadata config file
my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files
my $md = $mdcParser->parse(join "", @input)
        or die "unable to parse metadata config doc";
my $hashes = mds2hashes($md);	# An array of hashes
foreach my $pending (@$hashes) {
    foreach my $key (keys %$pending) {
	print $outFile ( $pending->{$key} . "\t");
    }
    print $outFile "\n";
}

### Pau.


# Given an array of MDs, return an array of hashes
sub mds2hashes
{
    my $mds = shift;		# Reference to the metadatas
    my @array;			# The array of hashes, to be returned
    foreach my $md (@$mds) {
        my $values = md2hash($md->{value});
        push @array, $values;
    }
    return \@array;
}

# Convert the metadata to a hash; in effect, strips out the comment, type and class fields.
sub md2hash
{
    my $values = shift;		# Reference to the metadata
    my %hash;			# Hash, to be returned
    foreach my $data (@$values) {
        $hash{$data->{name}} = $data->{value};
    }
    return \%hash;
}

END {
    my $status = $?;
    system("sync") == 0
        or die "failed to execute sync: $!" ;
    $? = $status;
}
