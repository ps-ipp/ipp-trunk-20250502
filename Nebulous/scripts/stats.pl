#!/usr/bin/env perl

use strict;
use warnings;

use Statistics::Descriptive;

#my $filename = shift or die "Usage: stats.pl <filename>";
my $filename = shift || "-";

open(my $fh, "$filename") or die "can't open file $filename: $!";

my $data = do {local $/; <$fh>};

my %methods;

foreach my $line (split(/\n/, $data)) {
    my ($time, $method, $duration) = split(/\s+/, $line);

    unless (defined $time and defined $method and defined $duration) {
        warn "bad line: $line\n";
        next;
    }

    push @{$methods{$method}->{time}}, $time;
    push @{$methods{$method}->{duration}}, $duration;
}

close($fh) or die "can't close file $filename: $!";

use Data::Dumper;

my $total_stat = Statistics::Descriptive::Sparse->new();
#print Dumper \%methods;
foreach my $method (keys %methods) {
    {
        my $duration    = $methods{$method}->{duration};
        my $stat = Statistics::Descriptive::Sparse->new();
        $stat->add_data(@$duration);
        printf("%-40s : %f\n", "$method - mean duration", $stat->mean());
    }
    {
        my $times       = $methods{$method}->{time};
        my $stat = Statistics::Descriptive::Sparse->new();
        $stat->add_data(@$times);
        $total_stat->add_data(@$times);
        printf("%-40s : %f\n","$method - calls/s", $stat->count / ($stat->max - $stat->min));
    }
}

print "\n";
printf("%-40s : %f\n", "first timestamp", $total_stat->min);
printf("%-40s : %f\n", "last timestamp", $total_stat->max);
printf("%-40s : %f\n", "total wallclock seconds", ($total_stat->max - $total_stat->min));
printf("%-40s : %f\n", "total call count", $total_stat->count);
printf("%-40s : %f\n", "total calls/s:", $total_stat->count / ($total_stat->max - $total_stat->min));
