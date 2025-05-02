#!/usr/bin/env perl

use warnings;
use strict;

use Astro::FITS::CFITSIO qw( :constants );
Astro::FITS::CFITSIO::PerlyUnpacking(1);
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use Math::Trig;
use Data::Dumper;
use PS::IPP::Config 1.01 qw( :standard );

my $ipprc = PS::IPP::Config->new();

use constant EXTNAME => 'MOPS_TRANSIENT_DETECTIONS'; # Extension name for output table
use constant ZERO_POINT => 25;  # Magnitude zero point
use constant OBSERVATORY_CODE => 566; # IAU Observatory Code
use constant FAKE_LIMITING_MAG => 23.0; # Fake limiting magnitude to report
use constant FAKE_DETECTION_EFFICIENCY => 0.0; # Fake detection efficiency to report

my ( $input,                    # Name of input file with IPP photometry
     $extname,                  # Name of extension containing photometry
     $skycell,                  # Skycell file with WCS
     $output,                   # Name of output file
     $save_temps,               # Save temporary files?
     );

GetOptions(
           'input=s'    => \$input,
           'extname=s'  => \$extname,
           'output=s'   => \$output,
           'save-temps' => \$save_temps,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --input --extname --output",
           -exitval => 3)
    unless defined $input
    and defined $extname
    and defined $output;

# Specification of columns to write
my $columns = [ { name => 'RA_DEG',   type => 'D' }, # Right ascension
                { name => 'RA_SIG',   type => 'D' }, # Error in right ascension
                { name => 'DEC_DEG',  type => 'D' }, # Declination
                { name => 'DEC_SIG',  type => 'D' }, # Error in declination
                { name => 'FLUX',     type => 'D' }, # Flux
                { name => 'FLUX_SIG', type => 'D' }, # Error in flux
                { name => 'STARPSF',  type => 'D' }, # probability that the PSF matches a starlike PSF
                { name => 'ANG',      type => 'D' }, # Angle
                { name => 'ANG_SIG',  type => 'D' }, # Error in angle
                { name => 'LEN',      type => 'D' }, # Length
                { name => 'LEN_SIG',  type => 'D' }, # Error in length
                ];

# Header translation table
my $headers = {
    # IPP name     => MOPS name, type, comment
    'FPA.RA'       => { name => 'RA',       type => TSTRING, comment => 'Right ascension of boresight'},
    'FPA.DEC'      => { name => 'DEC',      type => TSTRING, comment => 'Declination of boresight' },
    'FPA.FILTER'   => { name => 'FILTER',   type => TSTRING, comment => 'Filter name' },
    'EXPTIME'      => { name => 'EXPTIME',  type => TDOUBLE, comment => 'Exposure time' },
    'FPA.POSANGLE' => { name => 'ROTANGLE', type => TDOUBLE, comment => 'Position angle' },
    'FPA.ALT'      => { name => 'TEL_ALT',  type => TDOUBLE, comment => 'Telescope altitude' },
    'FPA.AZ'       => { name => 'TEL_AZ',   type => TDOUBLE, comment => 'Telescope azimuth' },
    'IMAGEID'      => { name => 'DIFFIMID', type => TINT,    comment => 'Difference image identifier'},
    'FPA.OBS'      => { name => 'FPA_ID',   type => TSTRING, comment => 'Exposure identifier' },
};


# Read header
my $inputResolved = $ipprc->file_resolve($input); # Resolved filename
my $status = 0;                 # CFITSIO status
my $inFits = Astro::FITS::CFITSIO::open_file( $inputResolved, READONLY, $status ); # FITS file handle
die("failed to open input file: $inputResolved: $status") if $status;
my $inHeader = $inFits->read_header(); # Header for input
check_fitsio($status);

# Read table data
$inFits->movnam_hdu(BINARY_TBL, $extname, 0, $status) and check_fitsio($status);
my $numRows;                    # Number of rows in table
$inFits->get_num_rows($numRows, $status) and check_fitsio($status);

my $ra = column($inFits, 'RA_PSF', $numRows); # Right Ascension, degrees
my $dec = column($inFits, 'DEC_PSF', $numRows); # Declination, degrees
my $mag = column($inFits, 'PSF_INST_MAG', $numRows); # Magnitude
my $magErr = column($inFits, 'PSF_INST_MAG_SIG', $numRows); # Magnitude error
my $ens = column($inFits, 'EXT_NSIGMA', $numRows); # Significance of extension
my $xErr = column($inFits, 'X_PSF_SIG', $numRows); # Error in x position, pixels
my $yErr = column($inFits, 'Y_PSF_SIG', $numRows); # Error in y position, pixels
my $scale = column($inFits, 'PLTSCALE', $numRows); # Plate scale, arcsec/pixel
my $angle = column($inFits, 'POSANGLE', $numRows); # Position angle, degrees

$inFits->close_file( $status );

my ($raErr, $decErr);           # Error in ra, dec
for (my $i = 0; $i < $numRows; $i++) {
    my $cosPA = cos($$angle[$i]);
    my $sinPA = sin($$angle[$i]);

    # XXX Not sure about the transformation here --- check
    $$raErr[$i] = $$scale[$i] * ($cosPA * $xErr + $sinPA * $yErr) / 3600;
    $$decErr[$i] = $$scale[$i] * ($sinPA * $xErr - $cosPA * $yErr) / 3600;
}

# Plate scales
#my $cdelt1 = $$inHeader{'CDELT1'} or die("Can't find CDELT1");
#my $cdelt2 = $$inHeader{'CDELT2'} or die("Can't find CDELT2");
### XXX WCS wasn't being set in inverse diffs, but it's available elsewhere
my $cdelt1 = $$scale[0] / 3600;
my $cdelt2 = $$scale[1] / 3600;

# Parse the list of columns
my @colNames;                   # Names of columns
my @colTypes;                   # Types of columns
my %colData;                    # Data for each column
foreach my $colSpec ( @$columns) {
    push @colNames, $colSpec->{name};
    push @colTypes, $colSpec->{type};
    $colData{$colSpec->{name}} = [];
}


# Convert the input data into the output formats
for (my $i = 0; $i < $numRows; $i++) {
    push @{$colData{'RA_DEG'}}, $ra;
    push @{$colData{'DEC_DEG'}}, $dec;
    push @{$colData{'RA_SIG'}}, $raErr;
    push @{$colData{'DEC_SIG'}}, $decErr;
    push @{$colData{'FLUX'}}, $$mag[$i] + ZERO_POINT;
    push @{$colData{'FLUX_SIG'}}, $magErr;
    push @{$colData{'ANG'}}, 0.0;
    push @{$colData{'ANG_SIG'}}, 0.0;
    push @{$colData{'LEN'}}, 0.0;
    push @{$colData{'LEN_SIG'}}, 0.0;
    push @{$colData{'STARPSF'}}, $$ens[$i];     # for now set this to the value of EXT_NSIGMA
}


# Write the output
my $outputResolved = $ipprc->file_resolve($output, 1);
unlink "$outputResolved";
my $outFits = Astro::FITS::CFITSIO::create_file( $outputResolved, $status ); # Output file handle
check_fitsio( $status );

# Write the table
$outFits->create_tbl( BINARY_TBL(), $numRows, scalar @colNames, \@colNames, \@colTypes, undef, EXTNAME,
                      $status );
check_fitsio( $status );

# Write the header keywords
foreach my $keyword ( keys %$headers ) {
    my $value = $inHeader->{$keyword}; # Header keyword value
    unless (defined $value) {
        print "Can't find header keyword $keyword\n";
        next;
    }
    $value =~ s/\'//g;
    my $name = $headers->{$keyword}->{name}; # New name
    my $type = $headers->{$keyword}->{type}; # Type
    my $comment = $headers->{$keyword}->{comment}; # Comment
    $outFits->write_key( $type, $name, $value, $comment, $status );
    check_fitsio( $status );
}

# Adjust the time from start to mid-point
{
    my $mjd = $inHeader->{'MJD-OBS'} or die "Can't find MJD.\n"; # Modified Julian Date
    my $exptime = $inHeader->{'EXPTIME'} or die "Can't find EXPTIME.\n"; # Exposure time, seconds
    $mjd += $exptime / 2.0 / 3600 / 24;     # start --> mid-point
    $outFits->write_key( TDOUBLE, 'MJD-OBS', $mjd, 'Time of exposure mid-point', $status);
    check_fitsio( $status );
}

# Stellar PSF
my $fwhm = 0.5 * 3600 * ($inHeader->{'FWHM_MAJ'} * $cdelt1 + $inHeader->{'FWHM_MIN'} * $cdelt2); # FWHM for star
$outFits->write_key( TSTRING, 'STARPSF', $fwhm, 'Stellar PSF (arcsec)', $status );
check_fitsio( $status );

# Observatory code
$outFits->write_key( TINT, 'OBSCODE', OBSERVATORY_CODE, 'IAU observatory code', $status );

# Limiting magnitude
$outFits->write_key( TINT, 'LIMITMAG', FAKE_LIMITING_MAG, 'Limiting magnitude (FAKE)', $status );

# Detection efficiency
$outFits->write_key( TINT, 'DE1', FAKE_DETECTION_EFFICIENCY, 'Detection efficiency (FAKE)', $status );
$outFits->write_key( TINT, 'DE2', FAKE_DETECTION_EFFICIENCY, 'Detection efficiency (FAKE)', $status );
$outFits->write_key( TINT, 'DE3', FAKE_DETECTION_EFFICIENCY, 'Detection efficiency (FAKE)', $status );
$outFits->write_key( TINT, 'DE4', FAKE_DETECTION_EFFICIENCY, 'Detection efficiency (FAKE)', $status );
$outFits->write_key( TINT, 'DE5', FAKE_DETECTION_EFFICIENCY, 'Detection efficiency (FAKE)', $status );
$outFits->write_key( TINT, 'DE6', FAKE_DETECTION_EFFICIENCY, 'Detection efficiency (FAKE)', $status );
$outFits->write_key( TINT, 'DE7', FAKE_DETECTION_EFFICIENCY, 'Detection efficiency (FAKE)', $status );
$outFits->write_key( TINT, 'DE8', FAKE_DETECTION_EFFICIENCY, 'Detection efficiency (FAKE)', $status );
$outFits->write_key( TINT, 'DE9', FAKE_DETECTION_EFFICIENCY, 'Detection efficiency (FAKE)', $status );
$outFits->write_key( TINT, 'DE10', FAKE_DETECTION_EFFICIENCY, 'Detection efficiency (FAKE)', $status );

# Write the data
for (my $i = 0; $i < scalar @colNames; $i++) {
    my $colName = $colNames[$i];# Column name
    $outFits->write_col( TDOUBLE, $i + 1, 1, 1, $numRows, $colData{$colName}, $status );
    check_fitsio( $status );
}

$outFits->close_file( $status );

### Pau.


# Read a column specified by name
sub column
{
    my $fits = shift;           # FITS file
    my $name = shift;           # Name of column
    my $rows = shift;           # Number of rows
    my $status = 0;             # Status of CFITSIO

    my $num;                    # Column number
    $fits->get_colnum(0, $name, $num, $status) and check_fitsio($status);
    my $type;                   # Type of column
    $inFits->get_coltype($num, $type, undef, undef, $status) and check_fitsio($status);
    my $data;                   # Array with data
    $inFits->read_col($type, $num, 1, 1, $rows, 0, $data, undef, $status) and check_fitsio($status);

    return $data;
}




# From Astro::FITS::CFITSIO demo
sub check_fitsio
{
    my $status = shift;         # Status of FITSIO calls

    if ($status != 0) {
        my $msg;                # Message to output
        Astro::FITS::CFITSIO::fits_get_errstatus( $status , $msg );
        die "CFITSIO error: $msg\n";
    }
}

__END__

