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
my $dvoApplyCorr = can_run('dvoApplyCorr') or (warn "Can't find dvoApplyCorr" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my ( $det_id, $class_id, $det_type, $input_uri, $camera, $dbname,
     $verbose, $no_update, $no_op, $outroot, $redirect, $corr_uri );
GetOptions(
    'det_id|d=s'        => \$det_id,
    'class_id|i=s'      => \$class_id,
    'det_type|t=s'      => \$det_type,
    'input_uri|u=s'     => \$input_uri,
    'corr_uri|u=s'      => \$corr_uri,
    'outroot|o=s'       => \$outroot,
    'camera|c=s'        => \$camera,
    'dbname|d=s'        => \$dbname, # Database name
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update,
    'no-op'             => \$no_op,
    'redirect-output'   => \$redirect,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --det_id --class_id --det_type --input_uri --camera --outroot",
           -exitval => 3)
    unless defined $det_id
    and defined $class_id
    and defined $det_type
    and defined $input_uri
    and defined $outroot
    and defined $camera;

# force det_type to be upper-case in this script
$det_type = uc($det_type);

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $det_id, $class_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration
if ($redirect) {
    my $logDest = $ipprc->filename("LOG.IMFILE", $outroot, $class_id)
       or &my_die("Missing entry from camera config", $det_id, $class_id, $PS_EXIT_CONFIG_ERROR);
    $ipprc->redirect_output($logDest) or my_die( "Unable to redirect output", $det_id, $class_id, $PS_EXIT_SYS_ERROR );
}

# Recipes to use as a function of detrend type
# XXX there are no recipe options for dvoApplyCorr...
# XXX in the future, we may define corrections other than the flat-field correction
# $reduction = "DETREND" unless defined $reduction;
# my $recipe = $ipprc->reduction($reduction, uc($det_type) . '_CORRECT'); # Recipe name to use

# XXX for now, check and warn, but don't exit on unexpected det_types
# det_type is type of the CORRECTED (output) file
if ($det_type ne "FLAT") { warn ("unexpected input detrend type: correcting data of type $det_type"); }

&my_die("Couldn't find input file: $input_uri\n", $det_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($input_uri);

my $outputImage = $ipprc->filename("DVOFLAT.OUTPUT", $outroot, $class_id) or &my_die("Missing entry from camera config", $det_id, $class_id, $PS_EXIT_PROG_ERROR);

# Run dvoApplyCorr
unless ($no_op) {
    # unless explicitly supplied, the correction image is determined by dvoApplyCorr via look up to the detrend database
    my $command = "$dvoApplyCorr -file $input_uri $outroot";
    $command .= " -corr $corr_uri" if defined $corr_uri;
    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppImage: $error_code", $det_id, $class_id, $error_code);
    }

    &my_die("Couldn't find expected output file: $outputImage", $det_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputImage);
}

# command to update database
my $command = "$dettool -addcorrectimfile";
$command .= " -det_id $det_id";
$command .= " -class_id $class_id";
$command .= " -uri $outputImage";
$command .= " -path_base $outroot";
$command .= " -dbname $dbname" if defined $dbname;

# Add the processed file to the database
unless ($no_update) {
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to perform dettool -addcorrectimfile: $error_code\n");
        exit($error_code);
    }
} else {
    print "skipping command: $command\n";
}

sub my_die
{
    my $msg = shift; # Warning message on die
    my $det_id = shift;         # Detrend identifier
    my $class_id = shift; # Class identifier
    my $exit_code = shift; # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $det_id and defined $class_id and not $no_update) {
        my $command = "$dettool -addcorrectimfile";
        $command .= " -det_id $det_id";
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
