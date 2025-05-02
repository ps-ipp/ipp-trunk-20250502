#!/usr/bin/env perl

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
use File::Spec;
use File::Temp qw( tempfile );
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );

# Look for programs we need
my $missing_tools;
my $bgtool = can_run('bgtool') or (warn "Can't find bgtool" and $missing_tools = 1);
my $camtool = can_run('camtool') or (warn "Can't find camtool" and $missing_tools = 1);
my $pswarp = can_run('pswarp') or (warn "Can't find pswarp" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my ($warp_bg_id, $skycell_id, $tess_dir, $reduction, $camera, $dbname, $outroot, $threads, $verbose, $no_update, $no_op, $redirect, $save_temps);
my ($astrometry, $imageName, $maskName, $weightName, $magicked);
GetOptions(
    'warp_bg_id|i=s'      => \$warp_bg_id, # Warp identifier
    'skycell_id|s=s'      => \$skycell_id, # Skycell identifier
    'tess_dir|s=s'        => \$tess_dir, # Tesselation identifier
    'camera|c=s'          => \$camera, # Camera name
    'dbname|d=s'          => \$dbname, # Database name
    'reduction=s'         => \$reduction, # Reduction class
    'outroot=s'           => \$outroot, # Output root name
    'image-list=s'        => \$imageName, # list of input image files
    'mask-list=s'         => \$maskName, # list of input mask files
    'astrometry=s'        => \$astrometry,
    'magicked=s'          => \$magicked,
    'threads=s'           => \$threads,   # Number of threads to use for pswarp
    'verbose'             => \$verbose,   # Print to stdout
    'no-update'           => \$no_update, # Don't update the database?
    'no-op'               => \$no_op, # Don't do any operations?
    'redirect-output'     => \$redirect,
    'save-temps'          => \$save_temps, # Save temporary files?
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
    -msg => "Required options: --warp_bg_id --skycell_id --tess_dir --camera --outroot",
    -exitval => 3,
) unless defined $warp_bg_id
    and defined $skycell_id
    and defined $tess_dir
    and defined $camera
    and defined $outroot;

if ($imageName) {
    # we have been invoked in a special mode where the caller tells us all about the inputs
    # This is used by the postage stamp server to make non-background corrected images without
    # the existence of a warpBackgroundRun (or chipBackgroundRun)
    # Make sure all of the inputs were supplied
    if (!(defined $maskName and defined $astrometry and defined $magicked)) {
        print STDERR "mask-list, astrometry, and magicked must be supplied if image-list is used\n";
        if ($warp_bg_id) {
            my_die( "invalid argument list.", $warp_bg_id, $skycell_id, $PS_EXIT_CONFIG_ERROR );
        }
        exit $PS_EXIT_CONFIG_ERROR;
    }
}

my $ipprc = PS::IPP::Config->new( $camera ) or 
my_die( "Unable to set up", $warp_bg_id, $skycell_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

my $logDest = $ipprc->filename("LOG.EXP", $outroot, $skycell_id) or my_die( "Unable to get log filename", $warp_bg_id, $skycell_id, $PS_EXIT_SYS_ERROR );

$ipprc->redirect_to_logfile($logDest) or my_die( "Unable to redirect output", $warp_bg_id, $skycell_id, $PS_EXIT_SYS_ERROR ) if $redirect;

# Recipes to use based on reduction class
$reduction = 'DEFAULT' unless defined $reduction;
my $recipe_pswarp = $ipprc->reduction($reduction, 'BACKGROUND_PSWARP'); # Recipe to use
my $recipe_psastro = $ipprc->reduction($reduction, 'PSASTRO'); # Recipe to use
unless ($recipe_pswarp and $recipe_psastro) {
    &my_die("Couldn't find selected reduction: $reduction\n", $warp_bg_id, $skycell_id, $PS_EXIT_CONFIG_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# Where do we get the astrometry source from?
my $astromSource;               # The astrometry source
{
    my $command = "$ppConfigDump -camera $camera -recipe PSWARP $recipe_pswarp -dump-recipe PSWARP -";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppConfigDump: $error_code", $warp_bg_id, $error_code);
    }
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $warp_bg_id, $PS_EXIT_PROG_ERROR);
    $astromSource = metadataLookupStr($metadata, 'ASTROM.SOURCE');
}

# Get list of filenames
my $tempOutRoot = "/tmp/background.warp.$warp_bg_id.$skycell_id";

if (!defined $imageName) {
    # go find our inputs
    my ($imageFile, $maskFile, $weightFile);
    ($imageFile, $imageName) = tempfile( "$tempOutRoot.image.list.XXXX",  UNLINK => !$save_temps);
    ($maskFile, $maskName) = tempfile( "$tempOutRoot.mask.list.XXXX",   UNLINK => !$save_temps);
    ($weightFile, $weightName) = tempfile( "$tempOutRoot.wt.list.XXXX",   UNLINK => !$save_temps);
    my $command = "$bgtool -warpinputs";
    $command .= " -warp_bg_id $warp_bg_id";
    $command .= " -skycell_id $skycell_id";
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to get input list: $error_code", $warp_bg_id, $skycell_id, $error_code);
    }

    my $files = $mdcParser->parse_list(join "", @$stdout_buf) or &my_die("Unable to parse metadata config doc", $warp_bg_id, $skycell_id, $PS_EXIT_PROG_ERROR);
    foreach my $file (@$files) {
        my $chip_path = $file->{chip_path_base};
        my $class_id = $file->{class_id};
        my $image = $ipprc->filename("PPBACKGROUND.OUTPUT", $chip_path, $class_id);
        my $mask = $ipprc->filename("PPBACKGROUND.OUTPUT.MASK", $chip_path, $class_id );
        my $wt = $ipprc->filename("PPBACKGROUND.OUTPUT.VARIANCE", $chip_path, $class_id );
        print $imageFile "$image\n";
        print $maskFile "$mask\n";
        print $weightFile "$wt\n";
        &my_die("Can't find input image: $image", $warp_bg_id, $skycell_id, $PS_EXIT_PROG_ERROR) unless $ipprc->file_exists($image);
        &my_die("Can't find input mask: $mask", $warp_bg_id, $skycell_id, $PS_EXIT_PROG_ERROR) unless $ipprc->file_exists($mask);
        &my_die("Can't find input mask: $wt", $warp_bg_id, $skycell_id, $PS_EXIT_PROG_ERROR) unless $ipprc->file_exists($wt);

        &my_die("Magic status don't match: $magicked vs $file->{magicked}", $warp_bg_id, $skycell_id, $PS_EXIT_PROG_ERROR) if defined $magicked and $magicked != $file->{magicked};
        $magicked = $file->{magicked};

        my $cam_path = $file->{cam_path_base};
        my $astrom = $ipprc->filename($astromSource, $cam_path);
        &my_die("Astrometry files don't match: $astrom vs $astrometry", $warp_bg_id, $skycell_id, $PS_EXIT_PROG_ERROR) if defined $astrometry and $astrom ne $astrometry;
        $astrometry = $astrom;
    }
    close $imageFile;
    close $maskFile;
    close $weightFile;
}

&my_die("Can't find input astrometry: $astrometry", $warp_bg_id, $skycell_id, $PS_EXIT_PROG_ERROR) unless $ipprc->file_exists($astrometry);

#my $out_image = $ipprc->filename("PSWARP.OUTPUT", $outroot, $skycell_id );
#my $out_mask = $ipprc->filename("PSWARP.OUTPUT.MASK", $outroot, $skycell_id);
#my $out_stats = $ipprc->filename("SKYCELL.STATS", $outroot, $skycell_id );
#my $out_config = $ipprc->filename("PSWARP.CONFIG", $outroot, $skycell_id);
#my $traceDest = $ipprc->filename("TRACE.EXP", $outroot, $skycell_id);

my $out_image = prepare_output("PSWARP.OUTPUT", $outroot, $skycell_id,  1);
my $out_mask = prepare_output("PSWARP.OUTPUT.MASK", $outroot, $skycell_id, 1);
my $out_wt = prepare_output("PSWARP.OUTPUT.VARIANCE", $outroot, $skycell_id, 1);
my $out_stats = prepare_output("SKYCELL.STATS", $outroot, $skycell_id, 1);
my $out_config = prepare_output("PSWARP.CONFIG", $outroot, $skycell_id, 1);
my $traceDest = prepare_output("TRACE.EXP", $outroot, $skycell_id, 1);

my $skyFile = prepare_output("SKYCELL.TEMPLATE", $outroot, $skycell_id, 1);

$ipprc->skycell_file( $tess_dir, $skycell_id, $skyFile, $verbose ) or &my_die("Unable to generate template skycell", $warp_bg_id, $skycell_id, $PS_EXIT_SYS_ERROR);

# Run pswarp
unless ($no_op) {
    my $command = "$pswarp";
    $command .= " -list $imageName";
    $command .= " -masklist $maskName";
    $command .= " -variancelist $weightName";
    $command .= " -astrom $astrometry";
    $command .= " $outroot $skyFile";
    $command .= " -F PSPHOT.OUTPUT PSPHOT.OUT.CMF.MEF";
    # XXX: change fits type based on recipe
    $command .= " -R PSWARP.OUTPUT FITS.TYPE NONE";
    $command .= " -recipe PSWARP $recipe_pswarp";
    $command .= " -recipe PPSTATS WARPSTATS";
    $command .= " -stats $out_stats";
    $command .= " -tracedest $traceDest -log $logDest";
    $command .= " -threads $threads" if defined $threads;
    $command .= " -dbname $dbname" if defined $dbname;
    $command .= " -dumpconfig $out_config";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform pswarp: $error_code", $warp_bg_id, $skycell_id, $error_code);
    }
}

# Read the statistics
my $cmdflags;
{
    &my_die("Couldn't find expected output file: $out_stats", $warp_bg_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($out_stats);

    my $real_stats = $ipprc->file_resolve($out_stats);

    my $command = "$ppStatsFromMetadata $real_stats - BACKGROUND_WARP";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppStatsFromMetadata: $error_code", $warp_bg_id, $skycell_id, $PS_EXIT_SYS_ERROR);
    }
    foreach my $line (@$stdout_buf) {
        $cmdflags .= " $line";
    }
    chomp $cmdflags;

    my ($quality) = $cmdflags =~ /-quality (\d+)/; # Quality flag

    if (!$quality) {
        &my_die("Couldn't find expected output file: $out_image", $warp_bg_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($out_image);
        &my_die("Couldn't find expected output file: $out_mask", $warp_bg_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($out_mask);
        &my_die("Couldn't find expected output file: $out_wt", $warp_bg_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($out_wt);
        &my_die("Couldn't find expected output file: $out_config", $warp_bg_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($out_config);
    }
}

unless ($no_update) {
    my $command = "$bgtool -addwarp";
    $command .= " -warp_bg_id $warp_bg_id";
    $command .= " -skycell_id $skycell_id";
    $command .= " -path_base $outroot"; # needed for logfile lookups
    $command .= " -set_magicked $magicked" if $magicked;
    $command .= " -hostname $host";
    $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
    $command .= " $cmdflags";
    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to update database: $error_code\n");
        exit($error_code);
    }
}

### Pau.

sub prepare_output
{
    my $filerule = shift;
    my $outroot  = shift;
    my $skycell_id = shift;
    my $delete = shift;
    $delete = 0 if !defined $delete;

    my $error;
    my $output = $ipprc->prepare_output($filerule, $outroot, $skycell_id, $delete, \$error)
                    or &my_die("failed to prepare output file for: $filerule", $warp_bg_id, $skycell_id, $error);
    return $output;
}


sub my_die
{
    my $msg = shift;            # Warning message on die
    my $warp_bg_id = shift;     # warpBackgroundRun identifier
    my $skycell_id = shift;     # Skycell identifier
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    warn($msg);
    if (defined $warp_bg_id and defined $skycell_id and not $no_update) {
        my $command = "$bgtool -addwarp";
        $command .= " -warp_bg_id $warp_bg_id";
        $command .= " -skycell_id $skycell_id";
        $command .= " -path_base $outroot";
        $command .= " -hostname $host" if defined $host;
        $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
        $command .= " -fault $exit_code";
        $command .= " -dbname $dbname" if defined $dbname;
        run(command => $command, verbose => $verbose);
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
