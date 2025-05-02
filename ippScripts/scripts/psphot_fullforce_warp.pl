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

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $warptool = can_run('warptool') or (warn "Can't find warptool" and $missing_tools = 1);
my $psphotFullForce = can_run('psphotFullForce') or (warn "Can't find psphotFullForce" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
my $fftool = can_run('fftool') or (warn "Can't find fftool" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my ($ff_id, $warp_id, $skycell_id, $path_base, $sourceroot, $camera);
my ($outroot, $reduction);
my ($dbname, $threads, $verbose, $no_update, $no_op, $redirect);

GetOptions(
    'ff_id=s'          => \$ff_id,
    'warp_id=s'         => \$warp_id,   # warp identifier
    'skycell_id=s'      => \$skycell_id,# Skycell identifier
    'warp_path_base=s'  => \$path_base, # path_base of the warp skycell
    'sources_path_base=s' => \$sourceroot,# path_base of sources
    'camera=s'          => \$camera,    # camera name of sources
    'dbname|d=s'        => \$dbname,    # Database name
    'threads=s'         => \$threads,   # Number of threads to use
    'outroot=s'         => \$outroot,   # Output root name
    'reduction=s'       => \$reduction, # Reduction class
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update, # Don't update the database?
    'no-op'             => \$no_op,     # Don't do any operations?
    'redirect-output'   => \$redirect,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
    -msg => "Required options: --ff_id --warp_id --sourceroot --skycell_id --warp_path_base --outroot --camera",
    -exitval => 3,
          ) unless defined $ff_id,
    and defined $sourceroot
    and defined $path_base 
    and defined $warp_id
    and defined $skycell_id
    and defined $camera
    and defined $outroot;

my $ipprc = PS::IPP::Config->new($camera) or my_die( "Unable to set up", $ff_id, $warp_id, $skycell_id, $PS_EXIT_CONFIG_ERROR );

my $neb;
my $scheme = file_scheme($outroot);
if ($scheme and $scheme eq 'neb') {
    $neb = $ipprc->nebulous();
}

my $logDest = $ipprc->filename("LOG.EXP", $outroot, $skycell_id);

$ipprc->redirect_to_logfile($logDest) or my_die( "Unable to redirect output", 
    $ff_id, $warp_id, $skycell_id, $PS_EXIT_SYS_ERROR ) if $redirect;

if (0) { 
# if (!$path_base) {
    # If path_base is not supplied, look it up in the database.
    my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files
    my $files;
    {
        my $command = "$warptool -warped -warp_id $warp_id -skycell_id $skycell_id";
        $command .= " -dbname $dbname" if defined $dbname;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform warptool -warpskyfile -inputskyfile: $error_code", 
                $ff_id, $warp_id, $skycell_id, $error_code);
        }

        my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
            &my_die("Unable to parse metadata config doc", $ff_id, $warp_id, $skycell_id, $PS_EXIT_PROG_ERROR);
        $files = parse_md_list($metadata) or
            &my_die("Unable to parse metadata list", $ff_id, $warp_id, $skycell_id, $PS_EXIT_PROG_ERROR);
    }

    &my_die("Input list does not contain exactly one elements", $ff_id, $warp_id, $skycell_id, $PS_EXIT_SYS_ERROR) 
        unless scalar @$files == 1;

    my $warp = $files->[0];

    $path_base = $warp->{path_base};
    &my_die("Couldn't find input path in warptool output", $ff_id, $warp_id, $skycell_id, $PS_EXIT_UNKNOWN_ERROR) 
        unless defined $path_base;

}

# Recipes to use based on reduction class
$reduction = 'DEFAULT' unless defined $reduction;
my $recipe_psphot  = $ipprc->reduction($reduction, 'FULLFORCE_PSPHOT'); # Recipe to use for psphot
unless ($recipe_psphot) {
    &my_die("Couldn't find selected reduction for PSPHOT: $reduction\n", 
        $ff_id, $warp_id, $skycell_id, $PS_EXIT_CONFIG_ERROR);
}

my $doStats = 1;

print "reduction: $reduction\n";
print "recipe_psphot: $recipe_psphot\n";

# use psf measured on input warp
# XXX: get this from recipe
my $useWarpPSF = 0;

my $input         = $ipprc->filename('PSWARP.OUTPUT', $path_base);
my $inputMask     = $ipprc->filename('PSWARP.OUTPUT.MASK', $path_base);
my $inputVariance = $ipprc->filename('PSWARP.OUTPUT.VARIANCE', $path_base);
my $inputPSF      = $useWarpPSF ? $ipprc->filename('PSPHOT.PSF.SKY.SAVE', $path_base) : "";
my $inputSources  = $ipprc->filename('PSPHOT.OUTPUT.CFF', $sourceroot);

if ($verbose) {
    print "input:         $input\n";
    print "inputMask:     $inputMask\n";
    print "inputVariance: $inputVariance\n";
    print "inputPSF:      $inputPSF\n" if $inputPSF;
    print "inputSources:  $inputSources\n";
}

# check that the inputs exist (and have non-zero size)
&my_die("Couldn't find input: $input", $ff_id, $warp_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($input);
&my_die("Couldn't find input: $inputMask", $ff_id, $warp_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($inputMask);
&my_die("Couldn't find input: $inputVariance", $ff_id, $warp_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($inputVariance);
&my_die("Couldn't find input: $inputPSF", $ff_id, $warp_id, $skycell_id, $PS_EXIT_SYS_ERROR) if ($inputPSF && !$ipprc->file_exists($inputPSF));
&my_die("Couldn't find input: $inputSources", $ff_id, $warp_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($inputSources);

my $dump_config = 1; 

# Get the output filenames
my $outputSources = prepare_output("PSPHOT.OUT.CMF.MEF", $outroot, 1);
my $configuration = prepare_output("PSPHOT.SKY.CONFIG", $outroot, 1) if $dump_config;
my $outputStats   = prepare_output("SKYCELL.STATS", $outroot, 1) if $doStats;
my $traceDest     = prepare_output("TRACE.EXP", $outroot, 1);

my $cmdflags = "";
my $quality = 0;

# Perform psphotFullForce
{
    my $command = "$psphotFullForce $outroot";
    $command .= " -force $inputSources";
    $command .= " -file $input";
    $command .= " -mask $inputMask";
    $command .= " -variance $inputVariance";
    $command .= " -psf $inputPSF" if $inputPSF;
    $command .= " -threads $threads" if defined $threads;
    if ($dump_config) {
        $command .= " -dumpconfig $configuration";
    }
    $command .= " -recipe PSPHOT $recipe_psphot";
    if ($doStats) {
        $command .= " -stats $outputStats";
    }
    $command .= " -F PSPHOT.OUTPUT PSPHOT.OUT.CMF.MEF";
    $command .= " -F PSPHOT.BACKMDL PSPHOT.BACKMDL.MEF";
    $command .= " -photcode-rule '{DETECTOR}.{FILTER.ID}.ForcedWarp'";
    $command .= " -tracedest $traceDest -log $logDest";
    $command .= " -dbname $dbname" if defined $dbname;

    unless ($no_op) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform psphotFullForce: $error_code", $ff_id, $warp_id, $skycell_id, $error_code);
        }

        if ($doStats) {
            check_output($outputStats, 1);
            my $outputStatsReal = $ipprc->file_resolve($outputStats);

            # analyze the stats file
            $command = "$ppStatsFromMetadata $outputStatsReal - WARP_SKYCELL";
            ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform ppStatsFromMetadata: $error_code", $ff_id, $warp_id, $skycell_id, $error_code);
            }
            foreach my $line (@$stdout_buf) {
                $cmdflags .= " $line";
            }
            chomp $cmdflags;
        }
        ($quality) = $cmdflags =~ /-quality (\d+)/; # Quality flag

    } else {
        print "Not executing: $command\n";
    }
}

if (!$quality) {
    check_output($outputSources, 1);
}

# Add the result to the database
{
    my $command = "$fftool -ff_id $ff_id -warp_id $warp_id"; 
    $command .= " -addresult -path_base $outroot";
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
                &my_die("$err_message: $error_code", $ff_id, $warp_id, $skycell_id, $error_code);
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
                    or &my_die("failed to prepare output file for: $filerule", $ff_id, $warp_id, $skycell_id, $error);
    return $output;
}

sub check_output
{
    my $file = shift;
    my $replicate = shift;

    if (!defined $file) {
        return;
    }

    &my_die("Couldn't find expected output file: $file",  $ff_id, $warp_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($file);

    # Funpack to confirm we've really made things correctly
    my $diskfile = $ipprc->file_resolve($file);
    if ($diskfile =~ /fits/) {
        my $funpack  = can_run('funpack') or &my_die ("Can't find funpack", $ff_id, $warp_id, $skycell_id, $PS_EXIT_SYS_ERROR);
	my $check_command = "$funpack -S $diskfile > /dev/null";
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $check_command, verbose => $verbose);
	if (!$success) {
	    &my_die("Output file not a valid fits file: $file", $ff_id, $warp_id, $skycell_id, $PS_EXIT_SYS_ERROR);
	}
    }
    #####

    if ($replicate and $neb) {
        $ipprc->replicate_file($file) or &my_die("failed to replicate: $file\n",  $ff_id, $warp_id, $skycell_id, $PS_EXIT_SYS_ERROR);
    }
}


sub my_die
{
    my $msg = shift;            # Warning message on die
    my $ff_id = shift;          # full force run identifier
    my $warp_id = shift;        # full force warp id
    my $skycell_id = shift;     # Skycell identifier
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    warn($msg);
    if (defined $ff_id and defined $skycell_id) {
        my $command = "$fftool -ff_id $ff_id -warp_id $warp_id -fault $exit_code";
        $command .= " -addresult";
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
