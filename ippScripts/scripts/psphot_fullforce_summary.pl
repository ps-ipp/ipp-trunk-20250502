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
use File::Temp qw( tempfile );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use Data::Dumper;
use PS::IPP::Config 1.01 qw( :standard );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $psphotFullForceSummary = can_run('psphotFullForceSummary') or (warn "Can't find psphotFullForceSummary" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
my $fftool = can_run('fftool') or (warn "Can't find fftool" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my ($ff_id, $outroot, $reduction, $camera);
my ($dbname, $threads, $verbose, $save_temps, $no_update, $no_op, $redirect);

GetOptions(
    'ff_id=s'          => \$ff_id,
    'camera=s'          => \$camera,    # camera name of sources
    'dbname|d=s'        => \$dbname,    # Database name
    'threads=s'         => \$threads,   # Number of threads to use
    'outroot=s'         => \$outroot,   # Output root name
    'reduction=s'       => \$reduction, # Reduction class
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update, # Don't update the database?
    'no-op'             => \$no_op,     # Don't do any operations?
    'redirect-output'   => \$redirect,
    'save-temps'        => \$save_temps,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
        -msg => "Required options: --ff_id --outroot --camera",
        -exitval => 3,
    )
    unless defined $ff_id,
        and defined $camera
        and defined $outroot;

my $ipprc = PS::IPP::Config->new($camera) or my_die( "Unable to set up", $ff_id, $PS_EXIT_CONFIG_ERROR );

my $neb;
my $scheme = file_scheme($outroot);
if ($scheme and $scheme eq 'neb') {
    $neb = $ipprc->nebulous();
}

my $logDest = $ipprc->filename("LOG.EXP", $outroot);

$ipprc->redirect_to_logfile($logDest) or my_die( "Unable to redirect output", 
    $ff_id, $PS_EXIT_SYS_ERROR ) if $redirect;


my ($listFile, $listName) = tempfile("/tmp/fullforce.summary.list.XXXX", UNLINK => !$save_temps );

my $cff_file;

{ 
    my $mdcParser = PS::IPP::Metadata::Config->new;
    my $results;
    {
        my $command = "$fftool -result -ff_id $ff_id -quality 0";
        $command .= " -dbname $dbname" if defined $dbname;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform fftool -result $error_code", 
                $ff_id, $error_code);
        }

        my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
            &my_die("Unable to parse metadata config doc", $ff_id, $PS_EXIT_PROG_ERROR);
        $results = parse_md_list($metadata) or
            &my_die("Unable to parse metadata list", $ff_id, $PS_EXIT_PROG_ERROR);
    }

    &my_die("result list is empty.", $ff_id, $PS_EXIT_SYS_ERROR) 
        if scalar @$results == 0;

    print $listFile "SOURCES MULTI\n";
    foreach my $result (@$results) {
        my $cmf = $ipprc->filename('PSPHOT.FULLFORCE.OUTPUT', $result->{path_base});
        &my_die("Couldn't find input cmf: $cmf", $ff_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($cmf);
        print $listFile "SOURCES STR $cmf\n";
        if (!$cff_file) {
            $cff_file = $ipprc->filename('PSPHOT.OUTPUT.CFF', $result->{sources_path_base});
            &my_die("Couldn't find input cff: $cff_file", $ff_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($cff_file);
        }
    }
    close $listFile;
}

&my_die("No CFF found in results: $cff_file", $ff_id, $PS_EXIT_PROG_ERROR) unless $cff_file;


# Recipes to use based on reduction class
$reduction = 'DEFAULT' unless defined $reduction;
my $recipe_psphot  = $ipprc->reduction($reduction, 'FULLFORCE_PSPHOT'); # Recipe to use for psphot
unless ($recipe_psphot) {
    &my_die("Couldn't find selected reduction for PSPHOT: $reduction\n", 
        $ff_id, $PS_EXIT_CONFIG_ERROR);
}

# XXX: need to figure out whether this works or not
# We probably want to create a specific recipe that looks at the results
my $recipe_ppstats = 'WARPSTATS';
my $doStats = 0;

print "reduction: $reduction\n";
print "recipe_psphot: $recipe_psphot\n";

my $dump_config = 1; 

# Get the output filenames
my $outputSources = prepare_output("PSPHOT.FULLFORCE.OUTPUT", $outroot, 1);
my $configuration = prepare_output("PSPHOT.SKY.CONFIG", $outroot, 1) if $dump_config;
my $outputStats   = prepare_output("SKYCELL.STATS", $outroot, 1) if $doStats;
my $traceDest     = prepare_output("TRACE.EXP", $outroot, 1);

my $cmdflags = "";

# Perform psphotFullForceSummary
{
    my $command = "$psphotFullForceSummary $outroot";
    $command .= " -input $listName";
    $command .= " -cff $cff_file";
    $command .= " -threads $threads" if defined $threads;
    if ($dump_config) {
        $command .= " -dumpconfig $configuration";
    }
    $command .= " -recipe PSPHOT $recipe_psphot";
    if ($doStats) {
        $command .= " -stats $outputStats";
        $command .= " -recipe PPSTATS $recipe_ppstats";
    }
#    $command .= " -F PSPHOT.OUTPUT PSPHOT.OUT.CMF.MEF";
    $command .= " -tracedest $traceDest -log $logDest";
    $command .= " -dbname $dbname" if defined $dbname;

    unless ($no_op) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform psphotFullForceSummary: $error_code", $ff_id, $error_code);
        }

        # Stats: TODO
        if ($doStats) {
            check_output($outputStats, 1);
            my $outputStatsReal = $ipprc->file_resolve($outputStats);

            # measure chip stats
            $command = "$ppStatsFromMetadata $outputStatsReal - WARP_SKYCELL";
            ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform ppStatsFromMetadata: $error_code", $ff_id, $error_code);
            }
            foreach my $line (@$stdout_buf) {
                $cmdflags .= " $line";
            }
            chomp $cmdflags;
        }
        my ($quality) = $cmdflags =~ /-quality (\d+)/; # Quality flag

        if (!$quality) {
            check_output($outputSources, 1);
        }
    } else {
        print "Not executing: $command\n";
    }
}

# Add the result to the database
{
    my $command = "$fftool -ff_id $ff_id";
    $command .= " -addsummary -path_base $outroot";
    $command .= " $cmdflags";
    $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
    $command .= " -hostname $host" if defined $host;
    $command .= " -dbname $dbname" if defined $dbname;

    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            my $err_message = "Unable to perform fftool -addwarped" ;
                &my_die("$err_message: $error_code", $ff_id, $error_code);
        }
    } else {
        print "Not executing $command\n";
    }
}

exit 0;


# Prepare to write to an output file
#   Lookup the filename in the rules.
#   Make sure that if file exists and is a nebulous file that there is only one instance
#   Deal with files that have been lost.
sub prepare_output
{
    my $filerule = shift;
    my $outroot  = shift;
    my $delete = shift;
    $delete = 0 if !defined $delete;

    my $error;
    my $output = $ipprc->prepare_output($filerule, $outroot, undef, $delete, \$error)
                    or &my_die("failed to prepare output file for: $filerule", $ff_id, $error);
    return $output;
}

sub check_output
{
    my $file = shift;
    my $replicate = shift;

    if (!defined $file) {
        return;
    }

    &my_die("Couldn't find expected output file: $file",  $ff_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($file);

    # Funpack to confirm we've really made things correctly
    my $diskfile = $ipprc->file_resolve($file);
    if ($diskfile =~ /fits/) {
        my $funpack  = can_run('funpack') or &my_die ("Can't find funpack", $ff_id, $PS_EXIT_SYS_ERROR);
	my $check_command = "$funpack -S $diskfile > /dev/null";
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $check_command, verbose => $verbose);
	if (!$success) {
	    &my_die("Output file not a valid fits file: $file", $ff_id, $PS_EXIT_SYS_ERROR);
	}
    }
    #####

    if ($replicate and $neb) {
        $ipprc->replicate_file($file) or &my_die("failed to replicate: $file\n",  $ff_id, $PS_EXIT_SYS_ERROR);
    }
}


sub my_die
{
    my $msg = shift;            # Warning message on die
    my $ff_id = shift;          # full force run identifier
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    warn($msg);
    if (defined $ff_id) {
        my $command = "$fftool -ff_id $ff_id -fault $exit_code";
        $command .= " -addsummary";
        $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
        $command .= " -hostname $host" if defined $host;
        $command .= " -path_base $outroot" if defined $outroot;
        $command .= " -dbname $dbname" if defined $dbname;
        unless ($no_update) {
            run(command => $command, verbose => $verbose);
        } else {
            print "not executing $command\n";
        }
    }
    exit $exit_code;
}

END {
    my $exit = $?;
    system("sync") == 0 or die "failed to execute sync: $!";
    $? = $exit;
}

__END__
