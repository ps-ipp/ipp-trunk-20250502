#!/usr/bin/env perl

## this script is run on every imfile to perform the fake source
## analysis and the forced photometry analysis.

## For the fake source analysis, we load the image, inject a number of
## fake source (with a flat mag distribution), save the input source,
## then perform photometry on the image to recover the input sources.
## we should save a file with the matched input and recovered
## sources.  the output metadata should include an analysis of the
## recovery rate and measurement error as a function of magnitude

## For the forced photometry, we need to use the measured astrometry
## to select the desired locations in pixel coord.

# XXX : seeing needs to be determined from the input PSF (not currently done in ppSim)
# ppSim output -input input.fits -cmf input.cmf -psf input.psf -seeing 0.563760 -recipe PPSIM FAKEPHOT

use Carp;
use warnings;
use strict;

## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $date = `date`;
print "\n\n";
print "Starting script $0 on $host at $date\n\n";

use DateTime;
my $mjd_start = DateTime->now->mjd;   # MJD of starting script

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Config 1.01 qw( :standard );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $faketool = can_run('faketool') or (warn "Can't find faketool" and $missing_tools = 1);
my $ppSim = can_run('ppSim') or (warn "Can't find ppSim" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my @ARGS = @ARGV;

# Parse the command-line arguments
my ( $exp_id, $fake_id, $class_id, $chiproot, $camroot, $camera, $outroot,
     $dbname, $reduction, $verbose, $no_update, $no_op, $redirect  );
GetOptions(
    'exp_id=s'          => \$exp_id,    # Exposure identifier
    'fake_id=s'         => \$fake_id,   # Chiptool identifier
    'class_id=s'        => \$class_id,  # Class identifier
    'chiproot=s'        => \$chiproot,  # Input Chip files (root)
    'camroot=s'         => \$camroot,   # Input Camera files (root)
    'camera|c=s'        => \$camera,       # Camera
    'outroot|w=s'       => \$outroot,   # output file base name
    'dbname|d=s'        => \$dbname,    # Database name
    'reduction=s'       => \$reduction, # Reduction class
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update, # Don't update the database?
    'no-op'             => \$no_op,        # Don't do any operations?
    'redirect-output'   => \$redirect,
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --exp_id --fake_id --class_id --chiproot --camroot --camera --outroot",
           -exitval => 3) unless
    defined $exp_id and
    defined $fake_id and
    defined $class_id and
    defined $chiproot and
    defined $camroot and
    defined $camera and
    defined $outroot;

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $exp_id, $fake_id, $class_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration
my $logDest = $ipprc->filename("LOG.IMFILE", $outroot, $class_id) or &my_die("Missing entry from camera config", $exp_id, $fake_id, $class_id, $PS_EXIT_CONFIG_ERROR);
$ipprc->redirect_output($logDest) or my_die( "Unable to redirect output", $exp_id, $fake_id, $class_id, $PS_EXIT_SYS_ERROR ) if $redirect;
print "FULL COMMAND: $0 @ARGS\n\n";

# Recipes to use based on reduction class
$reduction = 'DEFAULT' unless defined $reduction;
my $recipe = $ipprc->reduction($reduction, 'FAKEPHOT'); # Recipe to use
unless ($recipe) {
    &my_die("Couldn't find selected reduction for FAKE: $reduction\n", $exp_id, $fake_id, $class_id, $PS_EXIT_CONFIG_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# outroot examples (HOST components must be set)
# file://data/ipp004.0/gpc1/20080130
# neb:///ipp004-v1/gpc1/20080130
# neb:///*/gpc1/20080130 (volume not specified)

# check for existing directory, generate if needed
$ipprc->outroot_prepare($outroot);

## these names are used in ppImage, and thus may be URIs
my $chipImage     = $ipprc->filename("PPIMAGE.CHIP",        $chiproot, $class_id) or &my_die("Missing entry from camera config", $exp_id, $fake_id, $class_id, $PS_EXIT_CONFIG_ERROR);
my $chipMask      = $ipprc->filename("PPIMAGE.CHIP.MASK",   $chiproot, $class_id) or &my_die("Missing entry from camera config", $exp_id, $fake_id, $class_id, $PS_EXIT_CONFIG_ERROR);
my $chipWeight    = $ipprc->filename("PPIMAGE.CHIP.VARIANCE", $chiproot, $class_id) or &my_die("Missing entry from camera config", $exp_id, $fake_id, $class_id, $PS_EXIT_CONFIG_ERROR);
my $chipPSF       = $ipprc->filename("PSPHOT.PSF.SAVE",     $chiproot, $class_id) or &my_die("Missing entry from camera config", $exp_id, $fake_id, $class_id, $PS_EXIT_CONFIG_ERROR);
my $cameraObjects = $ipprc->filename("PSASTRO.OUTPUT",      $camroot)             or &my_die("Missing entry from camera config", $exp_id, $fake_id, $class_id, $PS_EXIT_CONFIG_ERROR);
my $traceDest     = $ipprc->filename("TRACE.IMFILE",        $outroot, $class_id)  or &my_die("Missing entry from camera config", $exp_id, $fake_id, $class_id, $PS_EXIT_CONFIG_ERROR);

# XXX check for existence of input data
# &my_die("Couldn't find input file: $uri\n", $exp_id, $fake_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($uri);

## get the PPSIM recipe for this camera and FAKEPHOT reduction
my $command = "$ppConfigDump -camera $camera -dump-recipe PPSIM -recipe PPSIM $recipe -";
my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppConfigDump: $error_code", $exp_id, $fake_id, $class_id, $PS_EXIT_SYS_ERROR);
}
my $recipeData = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $exp_id, $fake_id, $class_id, $PS_EXIT_SYS_ERROR);

## allow the output images to be optional, depending on the recipe / reduction class
my $skipFake = metadataLookupBool($recipeData, 'SKIP.FAKE');

# Run ppSim
unless ($no_op || $skipFake) {
    # examine the PPSIM recipe to decide if we need to run this or opt out

    my $command = "$ppSim $outroot";
    $command .= " -input $chipImage";
    # XXX add input mask and weight to ppSim
    # $command .= " -mask $chipMask";
    # $command .= " -weight $chipWeight";
    $command .= " -cmf $cameraObjects";
    $command .= " -psf $chipPSF";
    $command .= " -recipe PPSIM $recipe";
    $command .= " -dbname $dbname" if defined $dbname;
    $command .= " -tracedest $traceDest";
    $command .= " -log $logDest";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppSim: $error_code", $exp_id, $fake_id, $class_id, $error_code);
    }

    # XXX check for output files?
    # &my_die("Couldn't find expected output file: $outputBin1\n",   $exp_id, $fake_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputBin1);

    # XXX use this to parse the output metadata : eg, detection limits, Nfakes, Nforced
    # Get the statistics on the processed image
    # my $statsFile;            # File handle
    # open $statsFile, $ipprc->file_resolve($outputStats) or &my_die("Can't open statistics file $outputStats: $!", $exp_id, $fake_id, $class_id, $PS_EXIT_SYS_ERROR);
    # my @contents = <$statsFile>; # Contents of file
    # close $statsFile;
    #
    # # parse the statistics MDC file
    # my $mdcParser = PS::IPP::Metadata::Config->new(); # Parser for metadata config files
    # my $metadata = $mdcParser->parse(join "", @contents);
    # unless ($metadata) {
    #   &my_die("Unable to parse metadata config doc", $exp_id, $fake_id, $class_id, $PS_EXIT_PROG_ERROR);
    # }
    #
    # # extract the stats from the metadata
    # unless ($stats->parse($metadata)) {
    #   &my_die("Failure extracting metadata from the statistics output file.\n", $exp_id, $fake_id, $class_id, $PS_EXIT_PROG_ERROR);
    # }
} else {
    print "skipping ppSim processing\n";
}

# command to update database
$command = "$faketool -addprocessedimfile";
$command .= " -fake_id $fake_id";
$command .= " -exp_id $exp_id";
$command .= " -class_id $class_id";
$command .= " -path_base $outroot";
$command .= " -dbname $dbname" if defined $dbname;
$command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
# XXX add this after defined
# $command .= $stats->cmdflags();

# Add the processed file to the database
unless ($no_update) {
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to perform faketool -addprocessedimfile: $error_code\n");
        exit($error_code);
    }
} else {
    print "skipping command: $command\n";
}

sub my_die
{
    my $msg = shift; # Warning message on die
    my $exp_id = shift; # rawExp identifier
    my $fake_id = shift; # fakeRun identifier
    my $class_id = shift; # Class identifier
    my $exit_code = shift; # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $exp_id and defined $fake_id and defined $class_id and not $no_update) {
        my $command = "$faketool -addprocessedimfile";
        $command .= " -fake_id $fake_id";
        $command .= " -exp_id $exp_id";
        $command .= " -class_id $class_id";
        $command .= " -path_base $outroot";
        $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
        $command .= " -fault $exit_code";
        $command .= " -dbname $dbname" if defined $dbname;
        system ($command);
    }
    exit $exit_code;
}

END {
    my $status = $?;
    system("sync") == 0
        or die "failed to execute sync: $!" ;
    $? = $status;
}

__END__
