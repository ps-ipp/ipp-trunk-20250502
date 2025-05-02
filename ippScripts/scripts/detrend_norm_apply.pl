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
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line
my ( $det_id, $iter, $class_id, $value, $input_uri, $camera, $det_type, $outroot, $dbname, $verbose,
     $no_update, $no_op, $redirect );
GetOptions(
    'det_id|d=s'        => \$det_id,     # Detrend ID
    'iteration|n=s'     => \$iter,       # Iteration
    'class_id|i=s'      => \$class_id,   # Class ID
    'value|v=s'         => \$value,      # Value to apply (for normalisation)
    'input_uri|u=s'     => \$input_uri,  # Input file
    'camera|c=s'        => \$camera,     # Camera
    'det_type|t=s'      => \$det_type,   # Detrend type
    'outroot|w=s'       => \$outroot,    # output file base name
    'dbname|d=s'        => \$dbname,     # Database name
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update,  # Don't update the database
    'no-op'             => \$no_op,      # Don't do any operations
    'redirect-output'   => \$redirect,   # send output from script to LOG.IMFILE
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --det_id --iteration --class_id --value --input_uri --camera --det_type --outroot",
           -exitval => 3) unless
    defined $det_id    and
    defined $iter      and
    defined $class_id  and
    defined $value     and
    defined $input_uri and
    defined $camera    and
    defined $det_type  and
    defined $outroot;

# force det_type to be upper-case in this script
$det_type = uc($det_type);

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $det_id, $iter, $class_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

my $logDest = $ipprc->filename("LOG.IMFILE", $outroot, $class_id)
        or &my_die("Missing entry from camera config", $det_id, $iter, $class_id, $PS_EXIT_CONFIG_ERROR);
$ipprc->redirect_output($logDest) or my_die( "Unable to redirect output", $det_id, $iter, $class_id, $PS_EXIT_SYS_ERROR ) if $redirect;

my $RECIPE_PPIMAGE = 'PPIMAGE_N'; # Recipe to use with ppImage

# Define which detrend types we normalise
use constant DETTYPE => {
    'BIAS'             => 'BIAS',
    'DARK'             => 'DARK',
    'DARK_PREMASK'     => 'DARK',
    'SHUTTER'          => 'SHUTTER',
    'FLAT_PREMASK'     => 'FLAT',
    'DOMEFLAT_PREMASK' => 'FLAT',
    'SKYFLAT_PREMASK'  => 'FLAT',
    'FLAT_RAW'         => 'FLAT',
    'DOMEFLAT_RAW'     => 'FLAT',
    'SKYFLAT_RAW'      => 'FLAT',
    'SKYFLATTEST_RAW'  => 'FLAT',
    'FLAT'             => 'FLAT',
    'FLATTEST'         => 'FLAT',
    'DOMEFLAT'         => 'FLAT',
    'SKYFLAT'          => 'FLAT',
    'FRINGE'           => 'FRINGE',
    'MASK'             => 'MASK',
    'DARKMASK'         => 'MASK',
    'FLATMASK'         => 'MASK',
    'CTEMASK'          => 'MASK',
    'DARKTEST'         => 'DARK',
    'NOISEMAP'         => 'MASK',
    };

# convert supplied detrend type to a controlled namespace
&my_die("Unrecognised detrend type: $det_type", $det_id, $iter, $class_id, $PS_EXIT_PROG_ERROR) unless exists DETTYPE()->{$det_type};
my $det_type_real = DETTYPE()->{$det_type};

&my_die("Couldn't find input file: $input_uri\n", $det_id, $iter, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($input_uri);

# outroot examples (HOST components must be set)
# file://data/ipp004.0/gpc1/20080130
# neb:///ipp004-v1/gpc1/20080130
# neb:///*/gpc1/20080130 (volume not specified)

# check for existing directory, generate if needed
$ipprc->outroot_prepare($outroot);

my $outFile = ($det_type_real eq "MASK") ? 'PPIMAGE.OUTPUT.DETMASK' : 'PPIMAGE.OUTPUT';; # XXXX something of a hack (too many places to control things...)

my $RECIPE_PPSTATS = ($det_type_real eq "DARK") ? 'DARKSTATS' : 'DETSTATS';; # XXXX something of a hack (too many places to control things...)

my $output    = $ipprc->filename($outFile,        $outroot, $class_id) or &my_die("Missing entry from camera config", $det_id, $iter, $class_id, $PS_EXIT_CONFIG_ERROR);
my $b1name    = $ipprc->filename("PPIMAGE.BIN1",  $outroot, $class_id) or &my_die("Missing entry from camera config", $det_id, $iter, $class_id, $PS_EXIT_CONFIG_ERROR);
my $b2name    = $ipprc->filename("PPIMAGE.BIN2",  $outroot, $class_id) or &my_die("Missing entry from camera config", $det_id, $iter, $class_id, $PS_EXIT_CONFIG_ERROR);
my $statsName = $ipprc->filename("PPIMAGE.STATS", $outroot, $class_id) or &my_die("Missing entry from camera config", $det_id, $iter, $class_id, $PS_EXIT_CONFIG_ERROR);
my $traceDest = $ipprc->filename("TRACE.IMFILE",  $outroot, $class_id) or &my_die("Missing entry from camera config", $det_id, $iter, $class_id, $PS_EXIT_CONFIG_ERROR);

my $cmdflags;

# Run normalisation
unless ($no_op) {

    # we cannot use ppImage to load a normalized mask : just copy it and build the jpeg images
    if ($det_type_real eq 'MASK') {
        $RECIPE_PPIMAGE = 'PPIMAGE_BIN';
        my $input_real = $ipprc->file_resolve($input_uri, 0);
        my $output_real = $ipprc->file_resolve($output, 1);
        system ("cp $input_real $output_real");
    }

    my $command = "$ppImage -file $input_uri $outroot";
    $command .= " -norm $value -stats $statsName";
    $command .= " -recipe PPIMAGE $RECIPE_PPIMAGE";
    $command .= " -recipe PPSTATS $RECIPE_PPSTATS";
    $command .= " -F PPIMAGE.OUTPUT $outFile" unless $outFile eq 'PPIMAGE.OUTPUT';
    $command .= ' -isfringe' if $det_type_real eq 'FRINGE';
    $command .= ' -isdark' if $det_type_real eq 'DARK';
    $command .= " -tracedest $traceDest -log $logDest";
    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppImage: $error_code", $det_id, $iter, $class_id, $error_code);
    }
    &my_die("Can't find expected output file: $output",    $det_id, $iter, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($output);
    &my_die("Can't find expected output file: $b1name",    $det_id, $iter, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($b1name);
    &my_die("Can't find expected output file: $b2name",    $det_id, $iter, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($b2name);

    my $statsNameReal = $ipprc->file_resolve($statsName);
    &my_die("Can't find expected output file: $statsName", $det_id, $iter, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($statsNameReal);

    # parse stats from metadata
    $command = "$ppStatsFromMetadata $statsNameReal - DETREND_NORM_APPLY";
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

    # replicate output file
    my $scheme = file_scheme ($output);
    if ($scheme eq "neb") { 
	$ipprc->replicate_file($output) or &my_die("failed to replicate: $output\n", $det_id,$iter,$class_id,$PS_EXIT_SYS_ERROR);
    } else {
	print "output $output is a file or path, not a nebulous key: skipping replication\n"; 
    }
}

# Command to update the database
my $command = "$dettool -addnormalizedimfile";
$command .= " -det_id $det_id";
$command .= " -iteration $iter";
$command .= " -class_id $class_id";
$command .= " -uri $output";
$command .= " -path_base $outroot";
$command .= " -dbname $dbname" if defined $dbname;
$command .= " $cmdflags";

# Add the processed file to the database
unless ($no_update) {
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to perform dettool -addnormalizedimfile: $error_code\n");
        exit($error_code);
    }
} else {
    print "skipping command: $command\n";
}

sub my_die
{
    my $msg = shift;            # Warning message on die
    my $det_id = shift;         # Detrend identifier
    my $iter = shift;           # Iteration
    my $class_id = shift;       # Class identifier
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $det_id and defined $iter and defined $class_id and not $no_update) {
        my $command = "$dettool -addnormalizedimfile";
        $command .= " -det_id $det_id";
        $command .= " -iteration $iter";
        $command .= " -class_id $class_id";
        $command .= " -path_base $outroot";
        $command .= " -fault $exit_code";
        $command .= " -dbname $dbname" if defined $dbname;
        system ($command);
    }
    exit $exit_code;
}

# Return the scheme used for a filename (copied from PS-IPP-Config/lib/PS/IPP/Config.pm
sub file_scheme
{
    my $name = shift;                # Filename for which to get the scheme
    my ($scheme) = $name =~ /^(path|neb|file):/; # The scheme, e.g., file://, path://
    # $scheme may be undef if the input doesn't contain one of the above recognised schemes
    unless (defined($scheme)) { $scheme = "none"; }
    return $scheme;
}

END {
    my $status = $?;
    system("sync") == 0
        or die "failed to execute sync: $!" ;
    $? = $status;
}

__END__
