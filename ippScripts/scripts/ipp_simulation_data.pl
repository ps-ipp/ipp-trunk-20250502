#!/usr/bin/env perl

use warnings;
use strict;

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use Math::Trig;
use File::Spec;
use PS::IPP::Config 1.01 qw( :standard );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

my $ipprc = PS::IPP::Config->new(); # IPP configuration

my ($name,                      # Base name for output images
    $camera,                    # Name of camera to use
    $telescope,                 # Telescope name
    $tess_id,                   # Tessellation identifier
    $dbname,                    # Database name
    $path,                      # Path to data
    $workdir,                   # Working directory for data
    $no_cal,                    # Don't produce calibration files
    $no_update                  # Don't update the database
    );

GetOptions(
           'name=s'        => \$name,
           'camera=s'      => \$camera,
           'telescope=s'   => \$telescope,
           'tess_id=s'     => \$tess_id,
           'dbname=s'      => \$dbname,
           'path=s'        => \$path,
           'workdir=s'     => \$workdir,
           'no-cal'        => \$no_cal,
           'no-update'     => \$no_update,
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;

pod2usage(
          -msg => "Required options: --name --path --camera --telescope --dbname",
            -exitval => 3,
          ) unless
    defined $name and
    defined $path and
    defined $camera and
    defined $telescope and
    defined $dbname;

$workdir = $path if not defined $workdir;

# Look for programs we need
my $missing_tools;
my $ppSim = can_run('ppSim')
    or (warn "Can't find ppSim" and $missing_tools = 1);
my $inject = can_run('ipp_serial_inject_mosaic.pl')
    or (warn "Can't find ipp_serial_inject_mosaic.pl" and $missing_tools = 1);

if ($missing_tools) {
    warn ("Can't find required tools");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Number of bias images
use constant BIAS => 20;
# Dark exposure times
use constant DARK => [ 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
                       300, 300, 300, 300, 300, 300, 300, 300, 300, 300 ];
# Flat-field image characteristics
use constant FLAT => [
                      {
                          filter => 'r',
                          exptime => [ 0.1, 0.1, 0.1, 0.1, 0.1, 0.5, 1, 2, 5, 10, 20, 20, 20, 20, 20 ],
                      },
                      {
                          filter => 'i',
                          exptime => [ 20, 20, 20, 20, 20 ],
                      }
                      ];
# Object image characteristics
use constant OBJECT => [
                        {
                            filter => 'r',
                            exptime => [ 5, 10, 10, 10, 10, 10, 240 ],
                            seeing => [ 0.8, 0.4, 0.6, 0.8, 1.2, 1.5, 0.7 ],
                            ra => 150.119167,
                            dec => 2.205833,
                            pa => 0,
                            zp => 25.15,
                            sky => 20.86,
                            dither => 40,
                        },
                        {
                            filter => 'i',
                            exptime => [ 5, 30, 30, 30, 30, 30, 240 ],
                            seeing => [ 0.8, 0.4, 0.6, 0.8, 1.2, 1.5, 0.7 ],
                            ra => 150.119167,
                            dec => 2.205833,
                            pa => 0,
                            zp => 25.00,
                            sky => 20.15,
                            dither => 40,
                        },
                        ];
use constant SCALE => 0.2;      # Plate scale

#############################################################################################################
### Now do the work
#############################################################################################################

my $counter = 0;
my $tessellation = (defined $tess_id) ? "--tess_id $tess_id" : ""; # Additional switch for tessellation

# Generate bias images
for (my $i = 0; $i < BIAS; $i++) {
    my $basename;               # Output base filename
    ( $basename, $counter ) = filename( $name, $counter );
    my $filename = caturi( $path, $basename );
    unless ($no_cal) {
        run( command => "$ppSim -camera $camera -type BIAS $filename",
             verbose => 1 ) or die "Unable to run ppSim";
        unless ($no_update) {
            run( command => "$inject --camera $camera --telescope $telescope --path $path " .
                 "$tessellation --workdir $workdir --dbname $dbname $basename",
                 verbose => 1 ) or die "Unable to inject file.";
        }
    }
}

# Generate dark images
foreach my $exptime ( @{DARK()} ) {
    my $basename;               # Output base filename
    ( $basename, $counter ) = filename( $name, $counter );
    my $filename = caturi( $path, $basename );
    unless ($no_cal) {
        run ( command => "$ppSim -camera $camera -type DARK -exptime $exptime $filename",
              verbose => 1 ) or die "Unable to run ppSim";
        unless ($no_update) {
            run( command => "$inject --camera $camera --telescope $telescope --path $path " .
                 "$tessellation --workdir $workdir --dbname $dbname $basename",
                 verbose => 1 ) or die "Unable to inject file.";
        }
    }
}

# Generate flat images
foreach my $set ( @{FLAT()} ) {
    my $filter = $set->{filter}; # Name of filter
    foreach my $exptime ( @{$set->{exptime}} ) {
        my $basename;           # Output base filename
        ( $basename, $counter ) = filename( $name, $counter );
        my $filename = caturi( $path, $basename );
        unless ($no_cal) {
            run( command => "$ppSim -camera $camera -type FLAT -filter $filter -exptime $exptime $filename",
                 verbose => 1 ) or die "Unable to run ppSim";
            unless ($no_update) {
                run( command => "$inject --camera $camera --telescope $telescope --path $path " .
                     "$tessellation --workdir $workdir --dbname $dbname $basename",
                     verbose => 1 ) or die "Unable to inject file.";
            }
        }
    }
}

# Generate object images
foreach my $set ( @{OBJECT()} ) {
    my $filter = $set->{filter}; # Name of filter
    my $ra0 = $set->{ra};       # Base Right Ascension (deg)
    my $dec0 = $set->{dec};     # Base Declination (deg)
    my $pa = $set->{pa};        # Position angle (deg)
    my $zp = $set->{zp};        # Zero point
    my $scale = SCALE();        # Plate scale (arcsec/pix)
    my $sky = 10**( -0.4 * ( $set->{sky} - $zp ) ) * $scale**2; # Sky background (counts/s)
    my $dither = $set->{dither} / 3600; # Dither size (deg)

    for (my $i = 0; $i < scalar @{$set->{exptime}}; $i++) {
        my $exptime = ${$set->{exptime}}[$i]; # Exposure time
        my $seeing = ${$set->{seeing}}[$i]; # Seeing (pix)
        my $ra = $ra0 + (2*rand() - 1) * $dither * cos(deg2rad($dec0)); # RA with dither
        my $dec = $dec0 + (2*rand() - 1) * $dither; # Dec with dither

        my $basename;           # Output base filename
        ( $basename, $counter ) = filename( $name, $counter );
        my $filename = caturi( $path, $basename );
        run( command => "$ppSim -camera $camera -type OBJECT -filter $filter -exptime $exptime " .
             "-skyrate $sky -ra $ra -dec $dec -pa $pa -scale $scale -zp $zp -seeing $seeing $filename",
             verbose => 1 ) or die "Unable to run ppSim";
        unless ($no_update) {
            run( command => "$inject --camera $camera --telescope $telescope --path $path " .
                 "$tessellation --workdir $workdir --dbname $dbname $basename",
                 verbose => 1 ) or die "Unable to inject file.";
        }
    }
}


### Pau.


# Generate a filename from the base name and counter
sub filename
{
    my $base = shift;           # Base name
    my $num = shift;            # Number
    my $workdir = shift;        # Working directory
    my $name = sprintf("$base%04d", $num);
    $num++;
    return ( $name, $num );
}

__END__
