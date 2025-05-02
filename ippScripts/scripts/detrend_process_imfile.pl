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
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
my $nebrepair = can_run('neb-repair') or (warn "Can't find neb-repair" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my ( $det_id, $exp_id, $class_id, $det_type, $exp_tag, $input_uri, $camera, $outroot, $dbname, $reduction,
     $threads, $verbose, $no_update, $no_op, $redirect );
GetOptions(
    'det_id|d=s'        => \$det_id,
    'exp_id|e=s'        => \$exp_id,
    'class_id|i=s'      => \$class_id,
    'det_type|t=s'      => \$det_type,
    'exp_tag|=s'        => \$exp_tag,
    'input_uri|u=s'     => \$input_uri,
    'camera|c=s'        => \$camera,
    'outroot|w=s'       => \$outroot, # output file base name
    'dbname|d=s'        => \$dbname, # Database name
    'reduction=s'       => \$reduction, # Reduction class
    'threads=s'         => \$threads,   # Number of threads to use for ppImage
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update,
    'no-op'             => \$no_op,
    'redirect-output'   => \$redirect,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --det_id --exp_id --class_id --det_type --exp_tag --input_uri --camera --outroot",
           -exitval => 3) unless
    defined $det_id    and
    defined $exp_id    and
    defined $class_id  and
    defined $det_type  and
    defined $exp_tag   and
    defined $input_uri and
    defined $camera    and
    defined $outroot;

# force det_type to be upper-case in this script
$det_type = uc($det_type);

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $det_id, $exp_id, $class_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration
my $logDest = $ipprc->filename("LOG.IMFILE", $outroot, $class_id) or &my_die("Missing entry from camera config", $det_id, $exp_id, $class_id, $PS_EXIT_CONFIG_ERROR);
$ipprc->redirect_output($logDest) or my_die( "Unable to redirect output", $det_id, $exp_id, $class_id, $PS_EXIT_SYS_ERROR ) if $redirect;

# Recipes to use as a function of detrend type
$reduction = "DETREND" unless defined $reduction;
my $ppimage_recipe = $ipprc->reduction($reduction, $det_type . '_PROCESS'); # Recipe name for ppImage
my $jpeg_recipe = $ipprc->reduction($reduction, $det_type . '_JPEG_IMAGE'); # Recipe name for JPEG

# The output file rule name depends on the detrend type
my $FILERULES = { 'FLATMASK'         => undef,
                  'DARKMASK'         => undef,
                  'MASK'             => undef,
                  'BIAS'             => undef,
                  'DARK'             => undef,
                  'DARK_PREMASK'     => undef,
                  'DARKTEST'         => undef,
                  'CTEMASK'          => undef,
		  'NOISEMAP'         => undef,
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
                  'FRINGE'           => 'PPIMAGE.OUTPUT.DETREND',
              };

&my_die("Couldn't find input file: $input_uri\n", $det_id, $exp_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($input_uri);

# outroot examples (HOST components must be set)
# file://data/ipp004.0/gpc1/20080130
# neb:///ipp004-v1/gpc1/20080130
# neb:///*/gpc1/20080130 (volume not specified)

# check for existing directory, generate if needed
$ipprc->outroot_prepare($outroot);

my $cmdflags;

my $filerule = $FILERULES->{$det_type}; # File rule to use for output
$filerule =  "PPIMAGE.OUTPUT" unless defined $filerule;

my $outputImage = $ipprc->filename($filerule,        $outroot, $class_id) or &my_die("Missing entry from camera config", $det_id, $exp_id, $class_id, $PS_EXIT_PROG_ERROR);
my $outputBin1  = $ipprc->filename("PPIMAGE.BIN1",   $outroot, $class_id) or &my_die("Missing entry from camera config", $det_id, $exp_id, $class_id, $PS_EXIT_PROG_ERROR);
my $outputBin2  = $ipprc->filename("PPIMAGE.BIN2",   $outroot, $class_id) or &my_die("Missing entry from camera config", $det_id, $exp_id, $class_id, $PS_EXIT_PROG_ERROR);
my $outputStats = $ipprc->filename("PPIMAGE.STATS",  $outroot, $class_id) or &my_die("Missing entry from camera config", $det_id, $exp_id, $class_id, $PS_EXIT_PROG_ERROR);
my $traceDest   = $ipprc->filename("TRACE.IMFILE",   $outroot, $class_id) or &my_die("Missing entry from camera config", $exp_id, $exp_id, $class_id, $PS_EXIT_CONFIG_ERROR);


# Run ppImage
unless ($no_op) {
    if ($input_uri =~ /neb/) {
	my $repair_cmd = "$nebrepair $input_uri";
	my ($repair_success, $repair_error_code, $repair_full_buf, $repair_stdout_buf, $repair_stderr_buf ) = run(command => $repair_cmd, verbose => $verbose);
	unless ($repair_success) {
	    &my_die("Unable to attempt repair: $input_uri $repair_error_code", $det_id, $exp_id, $class_id, $PS_EXIT_SYS_ERROR);
	}
    }


    my $command = "$ppImage -file $input_uri $outroot";
    $command .= " -recipe PPIMAGE $ppimage_recipe";
    $command .= " -recipe JPEG $jpeg_recipe";
    $command .= " -recipe PPSTATS DETSTATS";
    $command .= " -stats $outputStats";
    $command .= " -F PPIMAGE.OUTPUT $filerule" if $filerule ne "PPIMAGE.OUTPUT";
    $command .= " -tracedest $traceDest -log $logDest";
    $command .= " -threads $threads" if defined $threads;
    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppImage: $error_code", $det_id, $exp_id, $class_id, $error_code);
    }

    &my_die("Couldn't find expected output file: $outputImage", $det_id, $exp_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputImage);
    &my_die("Couldn't find expected output file: $outputBin1",  $det_id, $exp_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputBin1);
    &my_die("Couldn't find expected output file: $outputBin2",  $det_id, $exp_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputBin2);

    my $outputStatsReal = $ipprc->file_resolve($outputStats);
    &my_die("Couldn't find expected output file: $outputStats", $det_id, $exp_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputStatsReal);

    # parse stats from metadata
    $command = "$ppStatsFromMetadata $outputStatsReal - DETREND_PROCESS_IMFILE";
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppStatsFromMetadata: $error_code", $det_id, $exp_id, $class_id, $error_code);
    }
    foreach my $line (@$stdout_buf) {
        $cmdflags .= " $line";
    }
    chomp $cmdflags;
}

# command to update database
my $command = "$dettool -addprocessedimfile";
$command .= " -det_id $det_id";
$command .= " -exp_id $exp_id";
$command .= " -class_id $class_id";
$command .= " -recip $reduction";
$command .= " -uri $outputImage -path_base $outroot";
$command .= " -dbname $dbname" if defined $dbname;
$command .= " $cmdflags";

# Add the processed file to the database
unless ($no_update) {
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to perform dettool -addprocessedimfile: $error_code\n");
        exit($error_code);
    }
} else {
    print "skipping command: $command\n";
}

sub my_die
{
    my $msg = shift; # Warning message on die
    my $det_id = shift;         # Detrend identifier
    my $exp_id = shift; # Exposure tag
    my $class_id = shift; # Class identifier
    my $exit_code = shift; # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $det_id and defined $exp_id and defined $class_id and not $no_update) {
        my $command = "$dettool -addprocessedimfile";
        $command .= " -det_id $det_id";
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
    my $status = $?;
    system("sync") == 0
        or die "failed to execute sync: $!" ;
    $? = $status;
}

__END__
