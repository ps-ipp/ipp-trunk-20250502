#!/usr/bin/env perl

use Carp;
use warnings;
use strict;

## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $date = `date`;
print "\n\n";
print "Starting script $0 on $host at $date\n\n";

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Config 1.01 qw( :standard );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $dettool = can_run('dettool') or (warn "Can't find dettool" and $missing_tools = 1);
my $ppImage = can_run('ppImage') or (warn "Can't find ppImage" and $missing_tools = 1);
my $ppStats = can_run('ppStats') or (warn "Can't find ppStats" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my ( $det_id, $iter, $ref_det_id, $ref_iter, $exp_id, $exp_tag, $class_id, $det_type, $detrend, $input_uri, $camera, $mode, $outroot,
     $dbname, $reduction, $threads, $verbose, $no_update, $no_op, $redirect );
GetOptions(
    'det_id|d=s'        => \$det_id,
    'iteration=s'       => \$iter,
    'ref_det_id=s'      => \$ref_det_id,
    'ref_iter=s'        => \$ref_iter,
    'exp_id|e=s'        => \$exp_id,
    'exp_tag|=s'        => \$exp_tag,
    'class_id|i=s'      => \$class_id,
    'det_type|t=s'      => \$det_type,
    'detrend=s'         => \$detrend,
    'input_uri|u=s'     => \$input_uri,
    'camera|c=s'        => \$camera,
    'mode|m=s'          => \$mode,
    'outroot|w=s'       => \$outroot,   # output file base name
    'dbname|d=s'        => \$dbname, # Database name
    'reduction=s'       => \$reduction, # Reduction class
    'threads=s'         => \$threads,   # Number of threads to use for ppImage
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update,
    'no-op'             => \$no_op,
    'redirect-output'   => \$redirect,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --det_id --iteration --ref_det_id --ref_iter --exp_id --exp_tag --class_id --det_type --camera --input_uri --mode --detrend --outroot (not for 'verify' mode)",
           -exitval => 3) unless
    defined $det_id     and
    defined $iter       and
    defined $ref_det_id and
    defined $ref_iter   and
    defined $exp_id     and
    defined $exp_tag    and
    defined $class_id   and
    defined $det_type   and
    defined $input_uri  and
    defined $camera     and
    defined $mode       and
    defined $outroot    and
    defined $detrend;

# force det_type to be upper-case in this script
$det_type = uc($det_type);

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $det_id, $iter, $exp_id, $class_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration
my $logDest = $ipprc->filename("LOG.IMFILE", $outroot, $class_id) or my_die( "Unable to find LOG.IMFILE", $det_id, $iter, $exp_id, $class_id, $PS_EXIT_CONFIG_ERROR );
$ipprc->redirect_output($logDest) or my_die( "Unable to find LOG.IMFILE", $det_id, $iter, $exp_id, $class_id, $PS_EXIT_SYS_ERROR ) if $redirect;

# Recipes to use as a function of detrend type and mode
# XXX probably can drop the distinct 'verify' recipes
$reduction = 'DETREND' unless defined $reduction;
my $recipe;                     # Name of recipe to use
if ($mode eq 'master') {
    $recipe = $det_type . '_RESID';
} elsif ($mode eq 'verify') {
    $recipe = $det_type . '_VERIFY';
} else {
    &my_die("Unrecognised mode: $mode", $det_id, $iter, $exp_id, $class_id, $PS_EXIT_PROG_ERROR);
}

print "raw recipe: $recipe\n";
my $ppimage_recipe = $ipprc->reduction($reduction, $recipe);
my $jpeg_recipe = $ipprc->reduction($reduction, $det_type . '_JPEG_RESID');
print "real recipe: $recipe\n";

# Flags to specify the particular detrend to use
use constant DETRENDS => {
    'BIAS'             => '-bias',      # Specify the bias frame
    'DARK'             => '-dark',      # Specify the dark frame
    'DARK_PREMASK'     => '-dark',      # Specify the dark frame
    'DARKTEST'         => '-dark',      # Specify the dark frame
    'SHUTTER'          => '-shutter',   # Specify the shutter frame
    'FLAT_PREMASK'     => '-flat',      # Specify the flat frame
    'DOMEFLAT_PREMASK' => '-flat',      # Specify the flat frame
    'SKYFLAT_PREMASK'  => '-flat',      # Specify the flat frame
    'FLAT_RAW'         => '-flat',      # Specify the flat frame
    'DOMEFLAT_RAW'     => '-flat',      # Specify the flat frame
    'SKYFLAT_RAW'      => '-flat',      # Specify the flat frame
    'SKYFLATTEST_RAW'  => '-flat',      # Specify the flat frame
    'FLAT'             => '-flat',      # Specify the flat frame
    'FLATTEST'         => '-flat',      # Specify the flat frame
    'DOMEFLAT'         => '-flat',      # Specify the flat frame
    'SKYFLAT'          => '-flat',      # Specify the flat frame
    'FRINGE'           => '-fringe',    # Specify the fringe frame
    'MASK'             => '-mask',      # Specify the mask frame
    'DARKMASK'         => '-mask',      # Specify the mask frame
    'FLATMASK'         => '-mask',      # Specify the mask frame
    'CTEMASK'          => '-mask',      # Specify the mask frame
    'NOISEMAP'         => '-noisemap',  # Specify the noisemap frame
};

# The output file rule name depends on the detrend type
my $FILERULES = { 'FLATMASK'         => 'PPIMAGE.OUTPUT.RESID',
                  'DARKMASK'         => 'PPIMAGE.OUTPUT.RESID',
                  'CTEMASK'          => 'PPIMAGE.OUTPUT.RESID',
                  'MASK'             => 'PPIMAGE.OUTPUT.RESID',
                  'BIAS'             => 'PPIMAGE.OUTPUT.RESID',
                  'DARK'             => 'PPIMAGE.OUTPUT.RESID',
                  'DARKTEST'         => 'PPIMAGE.OUTPUT.RESID',
                  'DARK_PREMASK'     => 'PPIMAGE.OUTPUT.RESID',
                  'SHUTTER'          => 'PPIMAGE.OUTPUT.DETREND',
                  'FLAT_PREMASK'     => 'PPIMAGE.OUTPUT.DETREND',
                  'DOMEFLAT_PREMASK' => 'PPIMAGE.OUTPUT.DETREND',
                  'SKYFLAT_PREMASK'  => 'PPIMAGE.OUTPUT.DETREND',
                  'FLAT_RAW'         => 'PPIMAGE.OUTPUT.DETREND',
                  'DOMEFLAT_RAW'     => 'PPIMAGE.OUTPUT.DETREND',
                  'SKYFLAT_RAW'      => 'PPIMAGE.OUTPUT.DETREND',
                  'SKYFLATTEST_RAW'  => 'PPIMAGE.OUTPUT.DETREND',
                  'FLAT'             => 'PPIMAGE.OUTPUT.DETREND',
                  'FLATTEST'         => 'PPIMAGE.OUTPUT.DETREND',
                  'DOMEFLAT'         => 'PPIMAGE.OUTPUT.DETREND',
                  'SKYFLAT'          => 'PPIMAGE.OUTPUT.DETREND',
                  'FRINGE'           => 'PPIMAGE.OUTPUT.RESID',
		  'NOISEMAP'         => 'PPIMAGE.OUTPUT.RESID',
              };

# outroot examples (HOST components must be set)
# file://data/ipp004.0/gpc1/20080130
# neb:///ipp004-v1/gpc1/20080130
# neb:///*/gpc1/20080130 (volume not specified)

# check for existing directory, generate if needed
$ipprc->outroot_prepare($outroot);

# XXX use PPIMAGE.OUTPUT.RESID for output file (compressed, with pos & neg range allowed)
# my $outputName  = $ipprc->filename("PPIMAGE.OUTPUT", $outroot, $class_id);

my $filerule = $FILERULES->{$det_type}; # File rule to use

my $outputName  = $ipprc->filename($filerule,        $outroot, $class_id);
my $bin1Name    = $ipprc->filename("PPIMAGE.BIN1",   $outroot, $class_id);
my $bin2Name    = $ipprc->filename("PPIMAGE.BIN2",   $outroot, $class_id);
my $outputStats = $ipprc->filename("PPIMAGE.STATS",  $outroot, $class_id);
my $traceDest   = $ipprc->filename("TRACE.IMFILE",   $outroot, $class_id);

my $cmdflags;

# Run ppImage & ppStats
unless ($no_op) {
    my $command = "$ppImage -file $input_uri $outroot";
    $command .= " -recipe PPIMAGE $ppimage_recipe";
    $command .= " -recipe JPEG $jpeg_recipe";
    $command .= " -recipe PPSTATS RESIDUAL";
    $command .= " -F PPIMAGE.OUTPUT $filerule";
    $command .= " -stats $outputStats";
    $command .= " -tracedest $traceDest -log $logDest";
    $command .= " -threads $threads" if defined $threads;
    $command .= " -dbname $dbname" if defined $dbname;

    # Detrend to use in processing
    my $detFlag = DETRENDS->{$det_type};
    &my_die("Unrecognised detrend type: $det_type", $det_id, $iter, $exp_id, $class_id, $PS_EXIT_PROG_ERROR) unless defined $detFlag;
    $command .= " $detFlag $detrend";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppImage: $error_code", $det_id, $iter, $exp_id, $class_id, $error_code);
    }

    &my_die("Couldn't find expected output file: $outputName", $det_id, $iter, $exp_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputName);
    &my_die("Couldn't find expected output file: $bin1Name", $det_id, $iter, $exp_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($bin1Name);
    &my_die("Couldn't find expected output file: $bin2Name", $det_id, $iter, $exp_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($bin2Name);

    my $outputStatsReal = $ipprc->file_resolve($outputStats);
    &my_die("Couldn't find expected output file: $outputStats", $det_id, $iter, $exp_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputStatsReal);

    # ppStatsFromMetadata $outputStats - DETREND_RESID_IMFILE
    $command = "$ppStatsFromMetadata $outputStatsReal - DETREND_RESID_IMFILE";
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppStatsFromMetadata: $error_code", $det_id, $iter, $exp_id, $class_id, $error_code);
    }
    foreach my $line (@$stdout_buf) {
        $cmdflags .= " $line";
    }
    chomp $cmdflags;

    # run ppStats on the binned image
    $command = "$ppStats -recipe PPSTATS RESIDUAL $bin2Name | $ppStatsFromMetadata - - DETREND_RESID_IMFILE_BINNED";
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppStats: $error_code", $det_id, $iter, $exp_id, $class_id, $error_code);
    }
    foreach my $line (@$stdout_buf) {
        $cmdflags .= " $line";
    }
    chomp $cmdflags;
}

# Command to update the database
my $command = "$dettool -addresidimfile";
$command .= " -det_id $det_id";
$command .= " -iteration $iter";
$command .= " -ref_det_id $ref_det_id";
$command .= " -ref_iter $ref_iter";
$command .= " -exp_id $exp_id";
$command .= " -class_id $class_id";
$command .= " -recip $recipe";
$command .= " -uri $outputName";
$command .= " -path_base $outroot";
$command .= " -dbname $dbname" if defined $dbname;
$command .= " $cmdflags";

# Add the processed file to the database
unless ($no_update) {
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to perform dettool -addresidimfile: $error_code\n");
        exit($error_code);
    }
} else {
    print "skipping command: $command\n";
}

sub my_die
{
    my $msg = shift; # Warning message on die
    my $det_id = shift;         # Detrend identifier
    my $iter = shift;           # Iteration
    my $exp_id = shift; # Exposure tag
    my $class_id = shift; # Class identifier
    my $exit_code = shift; # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $det_id and defined $iter and defined $exp_id and not $no_update) {
        my $command = "$dettool -addresidimfile";
        $command .= " -det_id $det_id";
        $command .= " -iteration $iter";
        $command .= " -ref_det_id $ref_det_id";
        $command .= " -ref_iter $ref_iter";
        $command .= " -exp_id $exp_id";
        $command .= " -class_id $class_id";
        $command .= " -path_base $outroot";
        $command .= " -fault $exit_code";
        $command .= " -dbname $dbname" if defined $dbname;
        system ($command);
    }
    exit $exit_code;
}

END {
    my $exit = $?;
    system("sync") == 0 or die "failed to execute sync: $!";
    $? = $exit;
}

__END__
