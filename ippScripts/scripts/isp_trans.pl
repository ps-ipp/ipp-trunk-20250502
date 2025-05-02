#!/usr/bin/env perl
# basic ISP transmission analysis:

if (@ARGV != 1) { die "USAGE: isp_trans.pl (input.fits)\n"; }
$input = $ARGV[0];

# for input file /path/foo.fits, use /path/foo for output

@words = split ('\.', $input);
if (@words > 1) { pop @words; }
$output = join (".", @words);

# use constant RECIPE => 'PPIMAGE_OBDSFRA'; # Recipe to use
$RECIPE_PPIMAGE  = 'PPIMAGE_OA'; # Recipe to use (switch to OBDSFRA when detrend images are ready)
$RECIPE_PSPHOT   = 'PSPHOT.SUMMIT'; 
$CALDIR  = '/data/alala.0/ipp/ippRefs/catdir.synth.bright'; # source of photometric calibration data
$IMTABLE = 'images.dat'; # source of photometric calibration data

vsystem ("ppImage -file $input $output -recipe PPIMAGE $RECIPE_PPIMAGE -recipe PSPHOT $RECIPE_PSPHOT");
if ($status) { die "failure running ppImage\n"; }

vsystem ("addstar -incal -image -D CAMERA isp -D IMAGE_TABLE $IMTABLE -D CATDIR $CALDIR $output.smf");
if ($status) { die "failure getting calibration from addstar\n"; }

sub vsystem {
    print STDERR "@_\n";
    my $status = system ("@_");
    $status;
}

