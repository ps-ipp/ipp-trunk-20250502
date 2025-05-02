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
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use Data::Dumper;
use PS::IPP::Config 1.01 qw( :standard );
use File::Temp qw( tempfile );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $diffphottool = can_run('diffphottool') or (warn "Can't find diffphottool" and $missing_tools = 1);
my $psphot = can_run('psphot') or (warn "Can't find psphot" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my ($diff_phot_id, $skycell_id, $dbname, $threads, $outroot, $reduction, $verbose, $no_update, $no_op, $redirect, $save_temps);
GetOptions(
    'diff_phot_id=s'    => \$diff_phot_id, # Diffphot identifier
    'skycell_id=s'      => \$skycell_id, # Skycell identifier
    'dbname|d=s'        => \$dbname, # Database name
    'threads=s'         => \$threads,   # Number of threads to use
    'outroot=s'         => \$outroot, # Output root name
    'reduction=s'       => \$reduction, # Reduction class
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update, # Don't update the database?
    'no-op'             => \$no_op, # Don't do any operations?
    'redirect-output'   => \$redirect,
    'save-temps'        => \$save_temps, # Save temporary files?
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
    -msg => "Required options: --diff_phot_id --skycell_id --outroot",
    -exitval => 3,
    ) unless defined $diff_phot_id
    and defined $skycell_id
    and defined $outroot;

my $ipprc = PS::IPP::Config->new() or my_die( "Unable to set up", $diff_phot_id, $skycell_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

# XXX camera is not known here; cannot use filerules...
# my $logDest = $ipprc->filename("LOG.EXP", $outroot);
my $logDest = "$outroot.log";
$ipprc->redirect_output($logDest) or my_die( "Unable to redirect output", $diff_phot_id, $skycell_id, $PS_EXIT_SYS_ERROR ) if $redirect;

# Get input components
my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files
my $inputPath;                                  # Input path
my $camera;                                     # Camera name
my $bothways;                           # Diff was done both ways?
my $magicked;                           # Magic status
{
    my $command = "$diffphottool -input -diff_phot_id $diff_phot_id -skycell_id $skycell_id";
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform difftool -inputskyfile: $error_code", $diff_phot_id, $skycell_id, $error_code);
    }

    my $files = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $diff_phot_id, $skycell_id, $PS_EXIT_PROG_ERROR);
    &my_die("File list does not contain exactly one entry", $diff_phot_id, $skycell_id, $PS_EXIT_PROG_ERROR) unless scalar @$files == 1;
    my $file = $$files[0];      # File of interest
    $inputPath = $file->{path_base};
    $camera = $file->{camera};
    $bothways = $file->{bothways};
    $magicked = $file->{magicked};
}

&my_die("Unable to identify input", $diff_phot_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless defined $inputPath;
&my_die("Unable to identify camera", $diff_phot_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless defined $camera;
$ipprc->define_camera($camera);

# Recipes to use based on reduction class
$reduction = 'DEFAULT' unless defined $reduction;
my $recipe_psphot  = $ipprc->reduction($reduction, 'DIFF_PSPHOT'); # Recipe to use for psphot
my $recipe_ppstats = 'DIFFSTATS';
&my_die("Couldn't find selected reduction for DIFF_PSPHOT: $reduction\n", $diff_phot_id, $skycell_id, $PS_EXIT_CONFIG_ERROR) unless ($recipe_psphot);

# Input filenames
my $inputImage = $ipprc->filename("PPSUB.OUTPUT", $inputPath);
my $inputMask = $ipprc->filename("PPSUB.OUTPUT.MASK", $inputPath);
my $inputVariance = $ipprc->filename("PPSUB.OUTPUT.VARIANCE", $inputPath);
my $inputPSF = $ipprc->filename("PSPHOT.PSF.SKY.SAVE", $inputPath);
&my_die("Couldn't find input: $inputImage", $diff_phot_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($inputImage);
&my_die("Couldn't find input: $inputMask", $diff_phot_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($inputMask);
&my_die("Couldn't find input: $inputVariance", $diff_phot_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($inputVariance);
&my_die("Couldn't find input: $inputPSF", $diff_phot_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($inputPSF);

my $quality;                    # Quality flag

# Do forward photometry
{
    # Get the output filenames
    my $outputName = $ipprc->filename("PSPHOT.OUT.CMF.MEF", "$outroot.pos");
    my $outputStats = $ipprc->filename("SKYCELL.STATS", "$outroot.pos");
    my $traceDest = $ipprc->filename("TRACE.EXP", "$outroot.pos");

    my $command = "$psphot $outroot.pos";
    $command .= " -file $inputImage";
    $command .= " -mask $inputMask";
    $command .= " -variance $inputVariance";
    $command .= " -psf $inputPSF";
#    $command .= " -stats $outputStats";
    $command .= " -threads $threads" if defined $threads;
    $command .= " -recipe PSPHOT $recipe_psphot";
    $command .= " -recipe PPSTATS $recipe_ppstats";
    $command .= " -F PSPHOT.PSF.SAVE PSPHOT.PSF.SKY.SAVE";
    $command .= " -F PSPHOT.OUTPUT PSPHOT.OUT.CMF.MEF";
    $command .= " -F PSPHOT.BACKMDL PSPHOT.BACKMDL.MEF";
    $command .= " -tracedest $traceDest -log $logDest";
    $command .= " -dbname $dbname" if defined $dbname;

    unless ($no_op) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform psphot: $error_code", $diff_phot_id, $skycell_id, $error_code);
        }

        if (0) {                # psphot doesn't produce a stats file
        my $outputStatsReal = $ipprc->file_resolve($outputStats);
        &my_die("Couldn't find expected output file: $outputStats", $diff_phot_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputStatsReal);

        $command = "$ppStatsFromMetadata $outputStatsReal - DIFF_SKYCELL";
        ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform ppStatsFromMetadata: $error_code", $diff_phot_id, $skycell_id, $error_code);
        }
        my $cmdflags = "";
        foreach my $line (@$stdout_buf) {
            $cmdflags .= " $line";
        }
        chomp $cmdflags;
        if ($cmdflags =~ /-quality (\d+)/) {
            $quality = $1;
        }
        }

        if (!$quality) {
            &my_die("Couldn't find expected output file: $outputName", $diff_phot_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputName);
        }
    } else {
        print "Not executing: $command\n";
    }
}

# Do reverse photometry
if ($bothways) {
    my $inputImage = $ipprc->filename("PPSUB.INVERSE", $inputPath);
    my $inputMask = $ipprc->filename("PPSUB.INVERSE.MASK", $inputPath);
    my $inputVariance = $ipprc->filename("PPSUB.INVERSE.VARIANCE", $inputPath);
    &my_die("Couldn't find input: $inputImage", $diff_phot_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($inputImage);
    &my_die("Couldn't find input: $inputMask", $diff_phot_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($inputMask);
    &my_die("Couldn't find input: $inputVariance", $diff_phot_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($inputVariance);

    # Get the output filenames
    my $outputName = $ipprc->filename("PSPHOT.OUT.CMF.MEF", "$outroot.neg");
    my $outputStats = $ipprc->filename("SKYCELL.STATS", "$outroot.neg");
    my $traceDest = $ipprc->filename("TRACE.EXP", "$outroot.neg");

    my $command = "$psphot $outroot.neg";
    $command .= " -file $inputImage";
    $command .= " -mask $inputMask";
    $command .= " -variance $inputVariance";
    $command .= " -psf $inputPSF";
#    $command .= " -stats $outputStats";
    $command .= " -threads $threads" if defined $threads;
    $command .= " -recipe PSPHOT $recipe_psphot";
    $command .= " -recipe PPSTATS $recipe_ppstats";
    $command .= " -F PSPHOT.PSF.SAVE PSPHOT.PSF.SKY.SAVE";
    $command .= " -F PSPHOT.OUTPUT PSPHOT.OUT.CMF.MEF";
    $command .= " -F PSPHOT.BACKMDL PSPHOT.BACKMDL.MEF";
    $command .= " -tracedest $traceDest -log $logDest";
    $command .= " -dbname $dbname" if defined $dbname;

    unless ($no_op) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform psphot: $error_code", $diff_phot_id, $skycell_id, $error_code);
        }

        if (0) {                # psphot doesn't produce a stats file
        my $outputStatsReal = $ipprc->file_resolve($outputStats);
        &my_die("Couldn't find expected output file: $outputStats", $diff_phot_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputStatsReal);

        $command = "$ppStatsFromMetadata $outputStatsReal - DIFF_SKYCELL";
        ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform ppStatsFromMetadata: $error_code", $diff_phot_id, $skycell_id, $error_code);
        }
        my $cmdflags = "";
        foreach my $line (@$stdout_buf) {
            $cmdflags .= " $line";
        }
        chomp $cmdflags;
        if ($cmdflags =~ /-quality (\d+)/) {
            $quality = $1;
        }
        }

        if (!$quality) {
            &my_die("Couldn't find expected output file: $outputName", $diff_phot_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputName);
        }
    } else {
        print "Not executing: $command\n";
    }
}

{
    my $command = "$diffphottool -diff_phot_id $diff_phot_id -skycell_id $skycell_id";
    $command .= " -done -path_base $outroot";
    $command .= " -magicked $magicked" if defined $magicked;
    $command .= " -quality $quality" if defined $quality;
    $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
    $command .= " -hostname $host" if defined $host;
    $command .= " -dbname $dbname" if defined $dbname;

    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Command failed: $error_code", $diff_phot_id, $skycell_id, $error_code);
        }
    } else {
        print "Skipping update: $command\n";
    }
}


sub my_die
{
    my $msg = shift;            # Warning message on die
    my $diff_phot_id = shift;        # Diff identifier
    my $skycell_id = shift;     # Skycell identifier
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    warn($msg);
    if (defined $diff_phot_id and defined $skycell_id and not $no_update) {
        my $command = "$diffphottool -done";
        $command .= " -diff_phot_id $diff_phot_id -skycell_id $skycell_id -fault $exit_code";
        $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
        $command .= " -hostname $host" if defined $host;
        $command .= " -path_base $outroot" if defined $outroot;
        $command .= " -dbname $dbname" if defined $dbname;
        run(command => $command, verbose => $verbose);
    }
    exit $exit_code;
}

END {
    my $exit = $?;
    system("sync") == 0 or die "failed to execute sync: $!";
    $? = $exit;
}

__END__
