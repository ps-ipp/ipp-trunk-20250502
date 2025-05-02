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
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $dettool = can_run('dettool') or (warn "Can't find dettool" and $missing_tools = 1);
my $ppMerge = can_run('ppMerge') or (warn "Can't find ppMerge" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my ( $det_id, $iter, $class_id, $det_type, $camera, $outroot, $dbname, $reduction, $threads, $verbose, $save_temps,
     $no_update, $no_op, $redirect );
GetOptions(
    'det_id|d=s'        => \$det_id,
    'iteration=s'       => \$iter,
    'class_id|i=s'      => \$class_id,
    'det_type|t=s'      => \$det_type,
    'camera|c=s'        => \$camera,
    'outroot|w=s'       => \$outroot,   # output file base name
    'dbname|d=s'        => \$dbname,    # Database name
    'reduction=s'       => \$reduction, # Reduction class for processing
    'threads=s'         => \$threads,
    'verbose'           => \$verbose,   # Print to stdout
    'save-temps'        => \$save_temps, # Save temporary files?
    'no-update'         => \$no_update,
    'no-op'             => \$no_op,
    'redirect-output'   => \$redirect,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --det_id --iteration --class_id --det_type --camera --outroot",
           -exitval => 3) unless
    defined $det_id   and
    defined $iter     and
    defined $class_id and
    defined $det_type and
    defined $camera   and
    defined $outroot;

$det_type = uc($det_type);

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $det_id, $iter, $class_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration
my $logDest = $ipprc->filename("LOG.IMFILE", $outroot, $class_id)
    or &my_die("Missing entry in file rules", $det_id, $iter, $class_id, $PS_EXIT_CONFIG_ERROR);
$ipprc->redirect_output($logDest) or my_die( "Unable to redirect output", $det_id, $iter, $class_id, $PS_EXIT_SYS_ERROR ) if $redirect;

# Recipes to use as a function of detrend type
$reduction = "DETREND" unless defined $reduction;
my $recipe = $ipprc->reduction($reduction, $det_type . '_STACK') # Recipe name to use
    or &my_die("Failed to find recipe for $det_type", $det_id, $iter, $class_id, $PS_EXIT_CONFIG_ERROR);

# The output file rule name depends on the detrend type
my $FILERULES = { 'FLATMASK'         => 'PPMERGE.OUTPUT.MASK',
                  'DARKMASK'         => 'PPMERGE.OUTPUT.MASK',
                  'CTEMASK'          => 'PPMERGE.OUTPUT.MASK',
                  'MASK'             => 'PPMERGE.OUTPUT.MASK',
                  'BIAS'             => 'PPMERGE.OUTPUT.BIAS',
                  'DARK'             => 'PPMERGE.OUTPUT.DARK',
                  'DARK_PREMASK'     => 'PPMERGE.OUTPUT.DARK',
                  'DARKTEST'         => 'PPMERGE.OUTPUT.DARK',
                  'SHUTTER'          => 'PPMERGE.OUTPUT.SHUTTER',
                  'FLAT_PREMASK'     => 'PPMERGE.OUTPUT.FLAT',
                  'DOMEFLAT_PREMASK' => 'PPMERGE.OUTPUT.FLAT',
                  'SKYFLAT_PREMASK'  => 'PPMERGE.OUTPUT.FLAT',
                  'FLAT_RAW'         => 'PPMERGE.OUTPUT.FLAT',
                  'DOMEFLAT_RAW'     => 'PPMERGE.OUTPUT.FLAT',
                  'SKYFLAT_RAW'      => 'PPMERGE.OUTPUT.FLAT',
                  'SKYFLATTEST_RAW'  => 'PPMERGE.OUTPUT.FLAT',
                  'FLAT'             => 'PPMERGE.OUTPUT.FLAT',
                  'FLATTEST'         => 'PPMERGE.OUTPUT.FLAT',
                  'DOMEFLAT'         => 'PPMERGE.OUTPUT.FLAT',
                  'SKYFLAT'          => 'PPMERGE.OUTPUT.FLAT',
                  'FRINGE'           => 'PPMERGE.OUTPUT.FRINGE',
		  'NOISEMAP'         => 'PPMERGE.OUTPUT.FLAT',
              };
my $output_filerule = $FILERULES->{$det_type}; # File rule for output
&my_die("Unrecognised detrend type: $det_type", $det_id, $iter, $class_id, $PS_EXIT_SYS_ERROR) unless defined $output_filerule;

# The stats recipe depends on the detrend type
my $STATRECIPES = {'FLATMASK'         => 'DETSTATS',
                   'DARKMASK'         => 'DETSTATS',
                   'CTEMASK'          => 'DETSTATS',
                   'MASK'             => 'DETSTATS',
                   'BIAS'             => 'DETSTATS',
                   'DARK'             => 'DARKSTATS',
                   'DARK_PREMASK'     => 'DARKSTATS',
                   'DARKTEST'         => 'DARKSTATS',
                   'SHUTTER'          => 'DARKSTATS',
                   'FLAT_PREMASK'     => 'DETSTATS',
                   'DOMEFLAT_PREMASK' => 'DETSTATS',
                   'SKYFLAT_PREMASK'  => 'DETSTATS',
                   'FLAT_RAW'         => 'DETSTATS',
                   'DOMEFLAT_RAW'     => 'DETSTATS',
                   'SKYFLAT_RAW'      => 'DETSTATS',
                   'SKYFLATTEST_RAW'  => 'DETSTATS',
                   'FLAT'             => 'DETSTATS',
                   'FLATTEST'         => 'DETSTATS',
                   'DOMEFLAT'         => 'DETSTATS',
                   'SKYFLAT'          => 'DETSTATS',
                   'FRINGE'           => 'DETSTATS',
		   'NOISEMAP'         => 'DETSTATS',
              };
my $statrecipe = $STATRECIPES->{$det_type}; # File rule for output

my $cmdflags;

# Get list of files to stack
my ($files, $command, $success, $error_code, $full_buf, $stdout_buf, $stderr_buf);
{
    $command = "$dettool -processedimfile -included";
    $command .= " -det_id $det_id";
    $command .= " -class_id $class_id";
    $command .= " -dbname $dbname" if defined $dbname;

    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform dettool -processedimfile: $error_code", $det_id, $iter, $class_id, $error_code);
    }

    my $mdcParser = PS::IPP::Metadata::Config->new;     # Parser for metadata config files
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $det_id, $iter, $class_id, $PS_EXIT_PROG_ERROR);
    $files = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $det_id, $iter, $class_id, $PS_EXIT_PROG_ERROR);
}

# Generate MDC file with the inputs
my ($listFile, $listName) = $ipprc->create_temp_file("$outroot.$class_id.list", $save_temps);

my $num = 0;
foreach my $file (@$files) {
    if ($file->{ignored}) { next; }

    print $listFile "INPUT$num\tMETADATA\n";
    $num++;

    my $image = $file->{uri};   # Image name
    my $mask = $ipprc->filename( "PPIMAGE.OUTPUT.MASK", $file->{path_base}, $class_id ); # Mask name
    my $weight = $ipprc->filename( "PPIMAGE.OUTPUT.VARIANCE", $file->{path_base}, $class_id ); # Weight name

    &my_die("Image $image does not exist", $det_id, $iter, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists( $image );
    print $listFile "\tIMAGE\tSTR\t" . $image . "\n";

    if ($ipprc->file_exists( $mask )) {
        print $listFile "\tMASK\tSTR\t" . $mask . "\n";
    }
    if ($ipprc->file_exists( $weight )) {
        print $listFile "\tVARIANCE\tSTR\t" . $weight . "\n";
    }

    print $listFile "END\n\n";
}
close $listFile;


# outroot examples (HOST components must be set)
# file://data/ipp004.0/gpc1/20080130
# neb:///ipp004-v1/gpc1/20080130
# neb:///*/gpc1/20080130 (volume not specified)

# check for existing directory, generate if needed
$ipprc->outroot_prepare($outroot);

my $outputStack = $ipprc->filename($output_filerule, $outroot, $class_id) or &my_die("Missing entry in file rules", $det_id, $iter, $class_id, $PS_EXIT_CONFIG_ERROR); # Output name
my $outputCount = $ipprc->filename("PPMERGE.OUTPUT.COUNT", $outroot, $class_id) or &my_die("Missing entry in file rules", $det_id, $iter, $class_id, $PS_EXIT_CONFIG_ERROR); # Count image
my $outputSigma = $ipprc->filename("PPMERGE.OUTPUT.SIGMA", $outroot, $class_id) or &my_die("Missing entry in file rules", $det_id, $iter, $class_id, $PS_EXIT_CONFIG_ERROR); # Stdev image
my $outputStats = $ipprc->filename("PPIMAGE.STATS",  $outroot, $class_id) or &my_die("Missing entry in file rules", $det_id, $iter, $class_id, $PS_EXIT_CONFIG_ERROR); # Statistics name
my $traceDest   = $ipprc->filename("TRACE.IMFILE",   $outroot, $class_id) or &my_die("Missing entry in file rules", $det_id, $iter, $class_id, $PS_EXIT_CONFIG_ERROR); # Trace messages

$command = "$ppMerge $listName $outroot"; # Command to run
$command .= " -recipe PPMERGE $recipe";
$command .= " -type $det_type"; # Type of stacking to perform
$command .= " -stats $outputStats";     # Statistics output filename
$command .= " -recipe PPSTATS $statrecipe";
$command .= " -tracedest $traceDest -log $logDest";
$command .= " -dbname $dbname" if defined $dbname;
$command .= " -threads $threads" if defined $threads;

# Stack the files
unless ($no_op) {
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppMerge: $error_code", $det_id, $iter, $class_id, $error_code);
    }
    &my_die("Unable to find expected output file: $outputStack\n", $det_id, $iter, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputStack);
    &my_die("Unable to find expected output file: $outputCount\n", $det_id, $iter, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputCount);
    &my_die("Unable to find expected output file: $outputSigma\n", $det_id, $iter, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputSigma);

    my $outputStatsReal = $ipprc->file_resolve($outputStats);
    &my_die("Couldn't find expected output file: $outputStats", $det_id, $iter, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputStatsReal);

    # parse stats from metadata
    $command = "$ppStatsFromMetadata $outputStatsReal - DETREND_STACK";
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppStatsFromMetadata: $error_code", $det_id, $iter, $class_id, $error_code);
    }
    foreach my $line (@$stdout_buf) {
        $cmdflags .= " $line";
    }
    chomp $cmdflags;
}

# Command to update the database
$command  = "$dettool -addstacked";
$command .= " -det_id $det_id";
$command .= " -iteration $iter";
$command .= " -class_id $class_id";
$command .= " -uri $outputStack";
$command .= " -recip $recipe";
$command .= " -dbname $dbname" if defined $dbname;
$command .= " $cmdflags";

# Add the resultant into the database
unless ($no_update) {
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to perform dettool -addstacked: $error_code\n");
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
    my $class_id = shift; # Class identifier
    my $exit_code = shift; # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $det_id and defined $iter and defined $class_id and not $no_update) {
        my $command = "$dettool -addstacked";
        $command .= " -det_id $det_id";
        $command .= " -iteration $iter";
        $command .= " -class_id $class_id";
        # XXX EAM : we should add this to the db : $command .= " -path_base $outroot";
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
