#!/usr/bin/env perl
# basic ISP transmission analysis:

if (@ARGV != 1) { die "USAGE: gpc_seeing.pl (input.fits)\n"; }
$input = $ARGV[0];

# for input file /path/foo.fits, use /path/foo for output

@words = split ('\.', $input);
if (@words > 1) { pop @words; }
$output = join (".", @words);

# use constant RECIPE => 'PPIMAGE_OBDSFRA'; # Recipe to use
$RECIPE_PPIMAGE  = 'PPIMAGE_OP';
$RECIPE_PSPHOT   = 'PSPHOT.SEEING';

# recommend only processing to PSFMODEL
vsystem ("ppImage -file $input $output -recipe PPIMAGE $RECIPE_PPIMAGE -recipe PSPHOT $RECIPE_PSPHOT");
if ($status) { die "failure running ppImage\n"; }

# XXX otis can read the output psf model, or we can supply a program to interpret the model

sub vsystem {
    print STDERR "@_\n";
    my $status = system ("@_");
    $status;
}

