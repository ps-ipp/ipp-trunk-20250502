#!/usr/bin/env perl

use warnings;
use strict;
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );

use PS::IPP::Config;
my $ipprc = PS::IPP::Config->new();

my $touch;

GetOptions(
    'touch'     => \$touch,
);

die "No filename specified.\n" if scalar @ARGV != 1;

my $filename = shift @ARGV;

my $resolved = $ipprc->file_resolve($filename, $touch);
print "$resolved\n" if ($resolved);

1;

__END__
