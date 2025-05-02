#!/usr/bin/env perl

use warnings;
use strict;
use Carp;

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
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );
use File::Temp qw( tempfile );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $fpcamtool = can_run('fpcamtool') or (warn "Can't find fpcamtool" and $missing_tools = 1);
my $fpcamera = can_run('fpcamera') or (warn "Can't find fpcamera" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my @ARGS = @ARGV;

my ( $exp_tag, $fpcam_id, $camera, $outroot, $dbname, $dvodb, $reduction, $verbose, $no_update,
     $no_op, $redirect, $save_temps);
GetOptions(
    'exp_tag=s'         => \$exp_tag,   # Exposure identifier
    'fpcam_id=s'        => \$fpcam_id,  # fpcamRun identifier
    'camera|c=s'        => \$camera,    # Camera
    'dbname|d=s'        => \$dbname,    # Database name
    'outroot|w=s'       => \$outroot,   # output file base name
    'dvodb|w=s'         => \$dvodb,     # reference catalog database
    'reduction=s'       => \$reduction, # Reduction class
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update, # Update the database?
    'no-op'             => \$no_op, # Don't do any operations?
    'redirect-output'   => \$redirect,
    'save-temps'        => \$save_temps, # Save temporary files?
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
          -msg => "Required options: --exp_tag --fpcam_id --camera --outroot",
          -exitval => 3,
          ) unless
    defined $exp_tag and
    defined $fpcam_id and
    defined $outroot and
    defined $camera;

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $fpcam_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

my $replicateOutputs = 1;

$ipprc->outroot_prepare($outroot);

my $logDest;
my $traceDest;

$logDest = prepare_output("LOG.EXP", $outroot, undef, 0);
$traceDest = prepare_output("TRACE.EXP", $outroot, undef, 0);

if ($redirect) {
    $ipprc->redirect_to_logfile($logDest) or my_die( "Unable to redirect output", $fpcam_id, $PS_EXIT_SYS_ERROR );
    print "\n\n";
    print "Starting script $0 on $host\n\n";
    print "FULL COMMAND: $0 @ARGS\n\n";
}

# Recipes to use based on reduction class
$reduction = 'DEFAULT' unless defined $reduction;

my $recipe_fpcamera = $ipprc->reduction($reduction, 'FPCAMERA'); # Recipe to use
&my_die("Unrecognised FPCAMERA recipe", $fpcam_id, $PS_EXIT_CONFIG_ERROR) unless defined $recipe_fpcamera;

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

#### Get list of input chip files
my $chipfiles; # Array of input chip files
{
    # list of chips to include in this analysis
    my $command = "$fpcamtool -inputchips -fpcam_id $fpcam_id"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform camtool: $error_code", $fpcam_id, $error_code);
    }
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $fpcam_id, $PS_EXIT_PROG_ERROR);

    # extract the metadata for the files into a hash list
    $chipfiles = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $fpcam_id, $PS_EXIT_PROG_ERROR);
}

my ($listChipFile, $listChipName) = tempfile( "/tmp/$exp_tag.fp.$fpcam_id.ch.list.XXXX", UNLINK => !$save_temps );
my ($listMaskFile, $listMaskName) = tempfile( "/tmp/$exp_tag.fp.$fpcam_id.mk.list.XXXX", UNLINK => !$save_temps );
my ($listWghtFile, $listWghtName) = tempfile( "/tmp/$exp_tag.fp.$fpcam_id.wt.list.XXXX", UNLINK => !$save_temps );
my ($listPSFsFile, $listPSFsName) = tempfile( "/tmp/$exp_tag.fp.$fpcam_id.ps.list.XXXX", UNLINK => !$save_temps );

foreach my $chipfile (@$chipfiles) {
    # we perform astrometry iff photometry output exists
    next if $chipfile->{quality} != 0;

    # use the path_base as OUTPUT root and convert the filenames with ipprc->filename:
    my $class_id = $chipfile->{class_id};

    # we expect the chip analysis stage to produce psphot output (cmf file) and two binned images
    my $chipObjects = $ipprc->filename("PSPHOT.OUTPUT", $chipfile->{path_base}, $class_id);
    
    my $chipImage  = $ipprc->filename("PPIMAGE.CHIP",          $chipfile->{path_base}, $class_id);
    my $chipMask   = $ipprc->filename("PPIMAGE.CHIP.MASK",     $chipfile->{path_base}, $class_id);
    my $chipWeight = $ipprc->filename("PPIMAGE.CHIP.VARIANCE", $chipfile->{path_base}, $class_id);
    my $chipPSF    = $ipprc->filename("PSPHOT.PSF.SAVE",       $chipfile->{path_base}, $class_id);

    print $listChipFile ($chipImage  . "\n");
    print $listMaskFile ($chipMask   . "\n");
    print $listWghtFile ($chipWeight . "\n");
    print $listPSFsFile ($chipPSF    . "\n");
}
close $listChipFile;
close $listMaskFile;
close $listWghtFile;
close $listPSFsFile;

#### Get the input astrometry file
my $astromfiles; # Array of input astrom files (should be only 1)
{
    # list of chips to include in this analysis
    my $command = "$fpcamtool -inputastrom -fpcam_id $fpcam_id"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform camtool: $error_code", $fpcam_id, $error_code);
    }
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $fpcam_id, $PS_EXIT_PROG_ERROR);

    # extract the metadata for the files into a hash list
    $astromfiles = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $fpcam_id, $PS_EXIT_PROG_ERROR);
}
if (@$astromfiles > 1 or @$astromfiles == 0) {
    &my_die("Invalid response for inputastrom", $fpcam_id, $PS_EXIT_PROG_ERROR);
}

# we expect the chip analysis stage to produce psphot output (cmf file) and two binned images
my $astromPath = $$astromfiles[0]->{path_base};
my $astromFile = $ipprc->filename("PSASTRO.OUTPUT", $astromPath);

my $outStats;
my $fpcameraInputArg;
$fpcameraInputArg .= " -list $listChipName";
$fpcameraInputArg .= " -masklist $listMaskName";
$fpcameraInputArg .= " -varlist $listWghtName";
$fpcameraInputArg .= " -psflist $listPSFsName"; # XXX: make PSF list optional?
$fpcameraInputArg .= " -astrom-file $astromFile";
$outStats = prepare_output("FPCAMERA.STATS", $outroot, undef, 1);

# Prepare the Output products

# the camera configurations should define the fpcamera output to be a single file (MEF), regardless of the inputs
my $outputSMF     = prepare_output("FPCAMERA.OUTPUT", $outroot, undef, 1);
my $configuration = prepare_output("FPCAMERA.CONFIG", $outroot, undef, 1);

my $cmdflags;

unless ($no_op) {

    # run fpcamera on the inputs, producing the outputs
    my $command;
    $command  = $fpcamera;
    $command .= " $fpcameraInputArg";
    $command .= " $outroot";
    $command .= " -recipe FPCAMERA $recipe_fpcamera";
    $command .= " -tracedest $traceDest -log $logDest";
    $command .= " -dbname $dbname" if defined $dbname;
    $command .= " -D FPCAMERA.CATDIR $dvodb" if defined $dvodb;
    $command .= " -dumpconfig $configuration";
    $command .= " -stats $outStats -recipe PPSTATS FPCAMERA";
    # XXX add an output stats function (not yet in fpcamera)

    ### run fpcamera command
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	print STDERR (join "\n", @$stderr_buf);
	&my_die("Unable to perform fpcamera: $error_code", $fpcam_id, $error_code);
    }

    my $quality; # Quality flag
    if (1) {
	check_output($outStats, $replicateOutputs);
	my $outStatsReal = $ipprc->file_resolve($outStats);

	# parse stats from metadata
	$command = "$ppStatsFromMetadata $outStatsReal - FPCAMERA_EXP";
	( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("Unable to perform ppStatsFromMetadata: $error_code", $fpcam_id, $error_code);
	}
	foreach my $line (@$stdout_buf) {
	    $cmdflags .= " $line";
	}
	chomp $cmdflags;
	($quality) = $cmdflags =~ /-quality (\d+)/;
    }

    if (!$quality) {
	check_output($outputSMF, $replicateOutputs);
	check_output($configuration, $replicateOutputs);
    }
}

my $dtime_script = (DateTime->now->mjd - $mjd_start) * 86400;

my $fpaCommand = "$fpcamtool -fpcam_id $fpcam_id";
  $fpaCommand .= " -addprocessedexp";
  $fpaCommand .= " -path_base $outroot";
  $fpaCommand .= " $cmdflags" if defined $cmdflags; # this defines e.g., zpt_obs, ver_pslib, etc
  $fpaCommand .= " -hostname $host" if defined $host;
  $fpaCommand .= " -dtime_script $dtime_script";
  $fpaCommand .= " -dbname $dbname" if defined $dbname;

# Add the result into the database
unless ($no_update) {
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $fpaCommand, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to add result to database: $error_code\n");
        exit($error_code);
    }
} else {
    print "skipping command: $fpaCommand\n";
}

exit 0;

sub prepare_output
{
    my $filerule = shift;
    my $outroot  = shift;
    my $class_id = shift;
    my $delete = shift;
    $delete = 0 if !defined $delete;

    my $error;
    my $output = $ipprc->prepare_output($filerule, $outroot, $class_id, $delete, \$error)
                    or &my_die("failed to prepare output file for: $filerule", $fpcam_id, $error);

    return $output;
}

sub check_output
{
    my $file = shift;
    my $replicate = shift;

    if (!defined $file) {
        return;
    }

    &my_die("Couldn't find expected output file: $file",  $fpcam_id, $PS_EXIT_SYS_ERROR) unless
        $ipprc->file_exists($file);

    # Funpack to confirm we've really made things correctly
    my $diskfile = $ipprc->file_resolve($file);
    if ($diskfile =~ /fits/) {
        my $funpack  = can_run('funpack') or &my_die ("Can't find funpack",  $fpcam_id, $PS_EXIT_SYS_ERROR);
	my $check_command = "$funpack -S $diskfile > /dev/null";
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $check_command, verbose => $verbose);
	if (!$success) {
	    &my_die("Output file not a valid fits file: $file",  $fpcam_id, $PS_EXIT_SYS_ERROR);
	}
    }
    my $scheme = file_scheme($file);
    if ($replicate and $scheme and (file_scheme($file) eq 'neb')) {
        $ipprc->replicate_file($file) or &my_die("failed to replicate: $file\n",  $fpcam_id, $PS_EXIT_SYS_ERROR);
    }
}

sub my_die
{
    my $msg = shift; # Warning message on die
    my $fpcam_id = shift; # fpcamtool identifier
    my $exit_code = shift; # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $fpcam_id and not $no_update) {
        my $command = "$fpcamtool -fpcam_id $fpcam_id";
	$command .= " -addprocessedexp";
	$command .= " -fault $exit_code";
	$command .= " -path_base $outroot" if defined $outroot;
	$command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
	$command .= " -hostname $host" if defined $host;
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
