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
use PS::IPP::Metadata::List qw( parse_md_list );
use File::Temp qw( tempfile );

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

my ( $det_id, $exp_id, $det_type, $exp_tag, $camera, $outroot, $dbname, $reduction, $verbose, $no_update,
     $no_op, $save_temps, $redirect );
GetOptions(
    'det_id|d=s'        => \$det_id,
    'det_type|t=s'      => \$det_type,
    'exp_id|d=s'        => \$exp_id,
    'exp_tag|=s'        => \$exp_tag,
    'camera|c=s'        => \$camera,
    'outroot|w=s'       => \$outroot,   # output file base name
    'dbname|d=s'        => \$dbname, # Database name
    'reduction|=s'      => \$reduction,
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update,
    'no-op'             => \$no_op,
    'save-temps'        => \$save_temps, # Save temporary files?
    'redirect-output'   => \$redirect,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --det_id --det_type --exp_id --exp_tag --camera --outroot",
           -exitval => 3) unless
    defined $det_id   and
    defined $det_type and
    defined $exp_id   and
    defined $exp_tag  and
    defined $camera   and
    defined $outroot;

$det_type = uc($det_type);

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $det_id, $exp_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration
if ($redirect) {
    my $logDest = $ipprc->filename("LOG.EXP", $outroot, "NONE")
        or &my_die("Missing entry in file rules", $det_id, $exp_id, $PS_EXIT_CONFIG_ERROR);
    $ipprc->redirect_output($logDest) or my_die( "Unable to redirect output", $det_id, $exp_id, $PS_EXIT_SYS_ERROR );
}

# Recipes to use based on reduction class
$reduction = 'DETREND' unless defined $reduction;

my $recipe = $ipprc->reduction($reduction, uc($det_type) . '_JPEG_IMAGE'); # Recipe to use
&my_die("Unrecognised detrend type: $det_type", $det_id, $exp_id, $PS_EXIT_PROG_ERROR) unless defined $recipe;

my $cmdflags;

# Get list of component files
my ($files, $command, $success, $error_code, $full_buf, $stdout_buf, $stderr_buf);
{
    $command = "$dettool -processedimfile -det_id $det_id -exp_id $exp_id"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform dettool -processedimfile: $error_code", $det_id, $exp_id, $error_code);
    }

    # convert stdout to a metadata
    my $mdcParser = PS::IPP::Metadata::Config->new;     # Parser for metadata config files
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $det_id, $exp_id, $PS_EXIT_PROG_ERROR);

    # parse the file info in the metadata
    $files = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $det_id, $exp_id, $PS_EXIT_PROG_ERROR);

    # since I can't figure out how to do input and output within PERL, I'm writing to a temp file
    my ($statFile, $statName) = tempfile( "/tmp/$exp_tag.detproc.$det_id.stats.XXXX", UNLINK => !$save_temps );
    print "saving stats to $statName\n";
    foreach my $line (@$stdout_buf) {
        print $statFile $line;
    }
    close $statFile;

    # parse the stats in the metadata file
    $command = "$ppStatsFromMetadata $statName - DETREND_PROCESS_EXP";
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to perform ppStatsFromMetadata: $error_code\n");
        exit($error_code);
    }

    foreach my $line (@$stdout_buf) {
        $cmdflags .= " $line";
    }
    chomp $cmdflags;
}

# File lists for the two binnings
my ($list1File, $list1Name) = tempfile( "/tmp/$exp_tag.detproc.$det_id.b1.list.XXXX", UNLINK => !$save_temps );
my ($list2File, $list2Name) = tempfile( "/tmp/$exp_tag.detproc.$det_id.b2.list.XXXX", UNLINK => !$save_temps );
foreach my $file (@$files) {
    print $list1File ($ipprc->filename( "PPIMAGE.BIN1", $file->{path_base}, $file->{class_id} ) . "\n");
    print $list2File ($ipprc->filename( "PPIMAGE.BIN2", $file->{path_base}, $file->{class_id} ) . "\n");
}
close $list1File;
close $list2File;

# outroot examples (HOST components must be set)
# file://data/ipp004.0/gpc1/20080130
# neb:///ipp004-v1/gpc1/20080130
# neb:///*/gpc1/20080130 (volume not specified)

# check for existing directory, generate if needed
$ipprc->outroot_prepare($outroot);

my $jpeg1 = $ipprc->filename("PPIMAGE.JPEG1", $outroot); # Binned JPEG #1
my $jpeg2 = $ipprc->filename("PPIMAGE.JPEG2", $outroot); # Binned JPEG #2

unless ($no_op) {
    # Make the jpeg for binning 1
    $command = "$ppImage -list $list1Name $outroot"; # Command to run
    $command .= " -recipe PPIMAGE PPIMAGE_J1";
    $command .= " -recipe JPEG $recipe";
    $command .= " -dbname $dbname" if defined $dbname;
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppImage: $error_code", $det_id, $exp_id, $error_code);
    }
    &my_die("Unable to find expected output file: $jpeg1", $det_id, $exp_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($jpeg1);

    # Make the jpeg for binning 2
    $command = "$ppImage -list $list2Name $outroot"; # Command to run
    $command .= " -recipe PPIMAGE PPIMAGE_J2";
    $command .= " -recipe JPEG $recipe";
    $command .= " -dbname $dbname" if defined $dbname;
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppImage: $error_code", $det_id, $exp_id, $error_code);
    }
    &my_die("Unable to find expected output file: $jpeg2", $det_id, $exp_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($jpeg2);
}

# Command to update the database
$command  = "$dettool -addprocessedexp";
$command .= " -det_id $det_id";
$command .= " -exp_id $exp_id";
$command .= " -recip $recipe -path_base $outroot";
$command .= " -dbname $dbname" if defined $dbname;
$command .= " $cmdflags";

# Add the processed file to the database
unless ($no_update) {
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to perform dettool -addprocessedexp: $error_code");
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
    my $exit_code = shift; # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $det_id and defined $exp_id and not $no_update) {
        my $command = "$dettool -addprocessedexp";
        $command .= " -det_id $det_id";
        $command .= " -exp_id $exp_id";
        $command .= " -fault $exit_code";
        $command .= " -path_base $outroot";
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
