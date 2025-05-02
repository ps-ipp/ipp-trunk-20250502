#!/usr/bin/env perl

use strict;
use Astro::FITS::CFITSIO qw( :constants );
use File::Temp qw( tempfile );
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use Data::Dumper;
use Carp;

Astro::FITS::CFITSIO::PerlyUnpacking(1);

my $xpaset = `which xpaset`;
my $xpaget = `which xpaget`;

die "Unable to find xpaget and xpaset.\n" unless ($xpaset =~ /\S+/ and $xpaget =~ /\S+/);


my ( $filename,                 # Filename containing photometry
     $extname,                  # Extension name containing photometry
     $frame,                    # Frame number in ds9
     $colour,                   # Region colour
     $flag_colour,              # Flagged source region colour
     $flag,                     # Flags
     $mag_radius,               # Magnitude scaling for radius?
     $radius,                   # Radius for circle
     $save_temps
     );

# Defaults
$colour = "blue";
$flag_colour = "red";
$radius = 5;
$flag = (8 | # FAIL
         1024 | # BADPSF
         2048 | # DEFECT
         4096 | # SATURATED
         8192 | # CR_LIMIT
         65536 # SKY_FAILURE
         );

GetOptions(
           'file=s' => \$filename,
           'ext=s' => \$extname,
           'frame=s' => \$frame,
           'colour=s' => \$colour,
           'flag-colour=s' => \$flag_colour,
           'flag=o' => \$flag,
    'mag-radius' => \$mag_radius,
           'radius=f' => \$radius,
           'save-temps'        => \$save_temps, # Save temporary files?
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --file --ext",
           -exitval => 3)
    unless defined $filename
    and defined $extname;

my $status;                     # Status of FITSIO calls
my $fits = Astro::FITS::CFITSIO::open_file( $filename, READONLY, $status ); # FITS file handle
check_fitsio($status);
$fits->movnam_hdu(BINARY_TBL, $extname, 0, $status) and check_fitsio($status);
my $numRows;                    # Number of rows in table
$fits->get_num_rows($numRows, $status) and check_fitsio($status);

my ($xCol, $yCol, $flagCol, $magCol); # Column numbers for x,y, flag, mag
$fits->get_colnum(0, 'X_PSF', $xCol, $status) and check_fitsio($status);
$fits->get_colnum(0, 'Y_PSF', $yCol, $status) and check_fitsio($status);
$fits->get_colnum(0, 'FLAGS', $flagCol, $status) and check_fitsio($status);
$fits->get_colnum(0, 'PSF_INST_MAG', $magCol, $status) and check_fitsio($status);

my ($x, $y, $flags, $mag);     # Coordinates, flags, magnitude read from table
$fits->read_col(TFLOAT, $xCol, 1, 1, $numRows, 0, $x, undef, $status) and check_fitsio($status);
$fits->read_col(TFLOAT, $yCol, 1, 1, $numRows, 0, $y, undef, $status) and check_fitsio($status);
$fits->read_col(TINT, $flagCol, 1, 1, $numRows, 0, $flags, undef, $status) and check_fitsio($status);
$fits->read_col(TFLOAT, $magCol, 1, 1, $numRows, 0, $mag, undef, $status) and check_fitsio($status);
$fits->close_file($status);

my ($mag_min, $mag_max);        # Minimum magnitude
if ($mag_radius) {
    foreach my $m (@$mag) {
        next unless defined $m and $m != "nan" and $m < "inf" and $m > "-inf";
        unless (defined $mag_min and defined $mag_max) {
            $mag_min = $m;
            $mag_max = $m;
            next;
        }
        $mag_min = $m if $m < $mag_min;
        $mag_max = $m if $m > $mag_max;
    }
}


my ($coordFile, $coordName) = tempfile( "/tmp/ds9_cmf_regions.XXXX", UNLINK => !$save_temps );
my $numGood = 0;                # Number of good sources
my $numBad = 0;                 # Number of bad sources
my $radius_scale = $radius / ($mag_min - $mag_max + 1.0); # Scaling for radius
for (my $i = 0; $i < $numRows; $i++) {
    my $col;                    # Colour to use
    if ($$flags[$i] & $flag or not defined $$mag[$i] or $$mag[$i] eq "nan" or $$mag[$i] == "inf" or $$mag[$i] == "-inf") {
        $numBad++;
        $col = $flag_colour;
    } else {
        $numGood++;
        $col = $colour;
    }
    my $r = $radius;            # Radius of circle
    next if $mag_radius and (not defined $$mag[$i] or $$mag[$i] eq "nan" or $$mag[$i] == "inf" or $$mag[$i] == "-inf");
    $r -= ($$mag[$i] - $mag_min) * $radius_scale if defined $mag_radius;
    print $coordFile "image; circle(" . ($$x[$i] + 1) . ',' . ($$y[$i] + 1) . ",$r) \# color = $col\n";
}
close $coordFile;

my @settings = settings_save("regions format",
                             "regions system"); # Settings to save

xpaset("frame $frame") if defined $frame;
xpaset("regions format ds9");
xpaset("regions system image");
xpaset("regions load $coordName");

xpaset(@settings);

print "Plotted $numRows ($numGood good, $numBad bad) sources.\n";

### Pau.




# From Astro::FITS::CFITSIO demo
sub check_fitsio
{
    my $status = shift;         # Status of FITSIO calls

    if ($status != 0) {
        my $msg;                # Message to output
        Astro::FITS::CFITSIO::fits_get_errstatus( $status , $msg );
        croak "CFITSIO error: $msg\n";
    }
}

# Save specified settings
sub settings_save
{
    my @settings;               # Values of settings
    foreach my $setting (@_) {
        my @values = xpaget($setting);
        push @settings, $setting . ' ' . shift @values;
    }
    return @settings;
}


# XPA subroutines courtesy Derek Fox
sub xpaset {
    foreach my $cmd (@_) {
        system("xpaset -p ds9 $cmd");
    }
}
sub xpaget {
    my @out;
    foreach my $cmd (@_) {
        my $output = `xpaget ds9 $cmd`;
        push @out, $output;
    }
    return @out;
}


__END__
