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

use DateTime;
my $mjd_start = DateTime->now->mjd;   # MJD of starting script
use File::Basename;

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
my $bgtool = can_run('bgtool') or (warn "Can't find bgtool" and $missing_tools = 1);
my $ppBackground = can_run('ppBackground') or (warn "Can't find ppBackground" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
my $ppImage = can_run('ppImage') or (warn "Can't find ppImage" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my @ARGS = @ARGV;

# Parse the command-line arguments
my ( $chip_bg_id, $class_id, $camera, $outroot, $dbname, $reduction, $verbose,
     $threads, $no_update, $save_temps, $no_op, $redirect, $chip_path_base, $cam_path_base, $magicked );
GetOptions(
    'chip_bg_id=s'      => \$chip_bg_id,    # chipBackgroundRun identifier
    'class_id=s'        => \$class_id,  # Class identifier
    'camera|c=s'        => \$camera,    # Camera
    'outroot|w=s'       => \$outroot,   # output file base name
    'dbname|d=s'        => \$dbname,    # Database name
    'reduction=s'       => \$reduction, # Reduction class
    'threads=s'         => \$threads,   # Number of threads to use
    'chip_path_base=s'  => \$chip_path_base, # optional chip_path_base
    'cam_path_base=s'   => \$cam_path_base, # optional camera stage path_base
    'magicked=s'        => \$magicked,  # magicked status of input
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update, # Don't update the database?
    'no-op'             => \$no_op,     # Don't do any operations?
    'redirect-output'   => \$redirect,
    'save-temps'        => \$save_temps, # Save temporary files?
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --chip_bg_id --class_id --camera --outroot",
           -exitval => 3) unless
    defined $chip_bg_id and
    defined $class_id and
    defined $camera and
    defined $outroot;

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $chip_bg_id, $class_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

my $logDest = $ipprc->filename("LOG.IMFILE", $outroot, $class_id) or &my_die("Missing entry from camera config", $chip_bg_id, $class_id, $PS_EXIT_CONFIG_ERROR);

if ($redirect) {
    $ipprc->redirect_to_logfile($logDest) or my_die( "Unable to redirect output", $chip_bg_id, $class_id, $PS_EXIT_SYS_ERROR );
    print STDOUT "\n\n";
    print STDOUT "Starting script $0 on $host\n\n";
    print STDOUT "FULL COMMAND: $0 @ARGS\n\n";
}

# Recipes to use based on reduction class
$reduction = 'DEFAULT' unless defined $reduction;
my $recipe_ppBackground = $ipprc->reduction($reduction, 'BACKGROUND_PPBACKGROUND'); # ppBackground recipe
unless ($recipe_ppBackground) {
    &my_die("Couldn't find selected reduction for BACKGROUND_PPBACKGROUND: $reduction\n", $chip_bg_id, $class_id, $PS_EXIT_CONFIG_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# Get inputs
my $in_path;                    # Input path
my $do_stats;
my $dump_config;
if ($chip_path_base and $no_update) {
    # XXX: this path through the code has not been excercized in awhile and probably does not work

    # we are running outside of a chip_bg_run (perhaps by the postage stamp server) don't dump config
    # or do stats. Get path to input from command line.
    $in_path = $chip_path_base;
    $magicked = 0 if !defined $magicked;
    $do_stats = 0;
    $dump_config = 0;
} else {
    # normal operation. Get input parameters from the database
    $do_stats = 1;
    $dump_config = 1;
    my $command = "bgtool -chipinputs -chip_bg_id $chip_bg_id -class_id $class_id";
    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to get inputs: $error_code\n");
        exit($error_code);
    }

    my $inputs = $mdcParser->parse_list(join "", @$stdout_buf) or &my_die("Unable to parse metadata config doc", $chip_bg_id, $class_id, $PS_EXIT_PROG_ERROR);
    &my_die("Input list does not contain exactly one entry", $chip_bg_id, $class_id, $PS_EXIT_PROG_ERROR) unless scalar @$inputs == 1;
    my $input = $$inputs[0];    # Input of interest
    $in_path = $input->{path_base};
    $cam_path_base = $input->{cam_path_base};
    $magicked = $input->{magicked};
}

my $in_image = $ipprc->filename("PPIMAGE.CHIP", $in_path, $class_id);
my $in_mask;
if ($cam_path_base) {
    $in_mask = $ipprc->filename("PSASTRO.OUTPUT.MASK", $cam_path_base, $class_id);
} else {
    $in_mask = $ipprc->filename("PPIMAGE.CHIP.MASK", $in_path, $class_id);
}
my $in_wt = $ipprc->filename("PPIMAGE.CHIP.VARIANCE", $in_path, $class_id);
my $in_bg;
if (1) {
    $in_bg = $ipprc->filename("PPIMAGE.BACKMDL", $cam_path_base, $class_id);
}
else {
    $in_bg = $ipprc->filename("PSPHOT.BACKMDL", $in_path, $class_id);
}
my $in_pattern = $ipprc->filename("PPIMAGE.PATTERN", $in_path, $class_id);
my $in_config = $ipprc->filename("PPIMAGE.CONFIG", $in_path, $class_id);

# Determine what to apply
my ($apply_bg, $apply_pattern); # Apply the background and pattern?
{
    &my_die("Cannot find input file: $in_config", $chip_bg_id, $class_id, $PS_EXIT_PROG_ERROR) unless $ipprc->file_exists($in_config);

    my $command = "$ppConfigDump -ipprc $in_config -dump-recipe PPIMAGE -";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => 0);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppConfigDump: $error_code", $chip_bg_id, $class_id, $PS_EXIT_SYS_ERROR);
    }
    my $recipe = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $chip_bg_id, $class_id, $PS_EXIT_SYS_ERROR);

    $apply_bg = metadataLookupBool($recipe, "BACKGROUND");
    my $row = metadataLookupBool($recipe, "PATTERN.ROW");
    my $cell = metadataLookupBool($recipe, "PATTERN.CELL");
    $apply_pattern = ($row or $cell);
}

# Set up files
&my_die("Couldn't find input file: $in_image\n", $chip_bg_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($in_image);
&my_die("Couldn't find input file: $in_mask\n", $chip_bg_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($in_mask);
&my_die("Couldn't find input file: $in_wt\n", $chip_bg_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($in_wt);
&my_die("Couldn't find input file: $in_bg\n", $chip_bg_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($in_bg) or !$apply_bg;
&my_die("Couldn't find input file: $in_pattern\n", $chip_bg_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($in_pattern) or !$apply_pattern;

$ipprc->outroot_prepare($outroot);

my $out_image = prepare_output("PPBACKGROUND.OUTPUT", $outroot, $class_id, 1);
my $out_mask = prepare_output("PPBACKGROUND.OUTPUT.MASK", $outroot, $class_id, 1);
my $out_wt = prepare_output("PPBACKGROUND.OUTPUT.VARIANCE", $outroot, $class_id, 1);
my $out_stats = prepare_output("PPBACKGROUND.STATS", $outroot, $class_id, 1);
my $out_config = prepare_output("PPBACKGROUND.CONFIG", $outroot, $class_id, 1);
my $traceDest = prepare_output("TRACE.IMFILE", $outroot, $class_id, 1);


# Run ppBackground
unless ($no_op) {
    my $command  = "$ppBackground $outroot";
    $command .= " -image $in_image";
    $command .= " -mask $in_mask";
    $command .= " -variance $in_wt";
    $command .= " -stats $out_stats" if $do_stats;
    $command .= " -background $in_bg" if $apply_bg;
    $command .= " -pattern $in_pattern" if $apply_pattern;
    $command .= " -recipe PPBACKGROUND $recipe_ppBackground";
    $command .= " -recipe PPSTATS CHIPSTATS" if $do_stats;
    $command .= " -dbname $dbname" if defined $dbname;
    $command .= " -dumpconfig $out_config" if $dump_config;
    $command .= " -tracedest $traceDest -log $logDest";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppBackground: $error_code", $chip_bg_id, $class_id, $error_code);
    }
}

# Gather command-line arguments from statistics
my $cmdflags = "";                  # Command-line flags to add
my $quality = 0;                    # Quality flag
if ($do_stats) {
    &my_die("Couldn't find expected output file: $out_stats", $chip_bg_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($out_stats);

    my $resolved_stats = $ipprc->file_resolve($out_stats);
    my $command = "$ppStatsFromMetadata $resolved_stats - BACKGROUND_CHIP";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppStatsFromMetadata: $error_code", $chip_bg_id, $class_id, $error_code);
    }
    foreach my $line (@$stdout_buf) {
        $cmdflags .= " $line";
    }
    chomp $cmdflags;
    ($quality) = $cmdflags =~ /-quality (\d+)/; # Quality flag
}

my $do_binned_images = 1;   # Generate the binned images that are useful for making JPEGs
if ($do_binned_images) {
    my $command = "$ppImage -file $out_image -mask $out_mask $outroot -recipe PPIMAGE PPIMAGE_PA -Db PHOTOM F";
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform ppImage: $error_code", $chip_bg_id, $class_id, $error_code);
    }
}


if (!$quality and !$no_op) {
    &my_die("Couldn't find expected output file: $out_image", $chip_bg_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($out_image);
    &my_die("Couldn't find expected output file: $out_mask", $chip_bg_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($out_mask);
    &my_die("Couldn't find expected output file: $out_config", $chip_bg_id, $class_id, $PS_EXIT_SYS_ERROR) unless !$dump_config or $ipprc->file_exists($out_config);

}

# Update database
{
    my $command = "$bgtool -addchip";
    $command .= " -chip_bg_id $chip_bg_id";
    $command .= " -class_id $class_id";
    $command .= " -path_base $outroot";
    $command .= " -set_magicked $magicked" if $magicked;
    $command .= " -hostname $host" if defined $host;
    $command .= " $cmdflags";
    $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
    $command .= " -dbname $dbname" if defined $dbname;

    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            warn("Unable to perform chiptool -addprocessedimfile: $error_code\n");
            exit($error_code);
        }
    } else {
        print "skipping command: $command\n";
    }
}


### Pau.

# Prepare to write to an output file
#   Lookup the filename in the rules.
#   Make sure that if file exists and is a nebulous file that there is only one instance
#   Deal with files that have been lost.
sub prepare_output
{
    my $filerule = shift;
    my $outroot  = shift;
    my $class_id = shift;
    my $delete = shift;
    $delete = 0 if !defined $delete;

    my $error;
    my $output = $ipprc->prepare_output($filerule, $outroot, $class_id, $delete, \$error)
                    or &my_die("failed to prepare output file for: $filerule", $chip_bg_id, $class_id, $error);
    return $output;
}



sub my_die
{
    my $msg = shift; # Warning message on die
    my $chip_bg_id = shift; # Chiptool identifier
    my $class_id = shift; # Class identifier
    my $exit_code = shift; # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $chip_bg_id and defined $class_id and not $no_update) {
        my $command = "$bgtool -addchip";
        $command .= " -chip_bg_id $chip_bg_id";
        $command .= " -class_id $class_id";
        $command .= " -fault $exit_code";
        $command .= " -path_base $outroot";
        $command .= " -hostname $host" if defined $host;
        $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
        $command .= " -dbname $dbname" if defined $dbname;
        system($command);
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
