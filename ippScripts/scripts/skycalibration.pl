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
use File::Temp qw( tempfile );
use File::Basename;
use PS::IPP::Config 1.01 qw( :standard );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $staticskytool = can_run('staticskytool') or (warn "Can't find staticskytool" and $missing_tools = 1);
my $psastro = can_run('psastro') or (warn "Can't find psastro" and $missing_tools = 1);
my $fhead = can_run('fhead') or (warn "Can't find fhead" and $missing_tools = 1);
my $fields = can_run('fields') or (warn "Can't find fields" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

sub HelpMessage {
    print "USAGE: skycalibration.pl --skycal_id (skycal_id) --outroot (root) --camera (camera) [options]\n";
    print "  --skycal_id (skycal_id) : run identifier\n";
    print "  --camera (camera)       : Camera name\n";
    print "  --dbname (dbname)       : Database name\n";
    print "  --threads (N)           : Number of threads to use\n";
    print "  --outroot (root)        : Output root name\n";
    print "  --path_base (path_base) : path_base of input\n";
    print "  --filter (filter)       : filter of input\n";
    print "  --singlefilter          : the input is the result of a single filter analysis";
    print "  --reduction (class)     : Reduction class\n";
    print "  --verbose               : Be extra verbose\n";
    print "  --no-update             : Don't update the database?\n";
    print "  --no-op                 : Don't do any operations?\n";
    print "  --redirect-output       : send output to the log file,\n";
    print "  --save-temps            : Save temporary files?\n";
    print "  --help                  : show this message\n";
    exit 2;
}

my ($skycal_id, $stack_id, $camera, $dbname, $threads, $outroot, $path_base, $filter, $singlefilter, $reduction, $verbose, $no_update, $no_op, $redirect, $save_temps);
my $run_state = 'new';
GetOptions(
    'skycal_id=s'       => \$skycal_id, # sky calibration run id
    'stack_id=s'        => \$stack_id,    # static sky run id
    'camera|c=s'        => \$camera,    # Camera name
    'dbname|d=s'        => \$dbname,    # Database name
    'threads=s'         => \$threads,   # Number of threads to use
    'outroot=s'         => \$outroot,   # Output root name
    'run-state=s'       => \$run_state, # 'new' or 'update'
    'path_base=s'       => \$path_base, # path_base of input
    'filter=s'          => \$filter,    # filter for input stack
    'reduction=s'       => \$reduction, # Reduction class
    'singlefilter'      => \$singlefilter, # input was from single filter analysis
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update, # Don't update the database?
    'no-op'             => \$no_op, # Don't do any operations?
    'redirect-output'   => \$redirect,
    'save-temps'        => \$save_temps, # Save temporary files?
    'help'              => sub { HelpMessage() },
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;

pod2usage(
    -msg => "Required options: --skycal_id --stack_id --outroot --camera --path_base",
    -exitval => 3,
          ) unless 
    defined $skycal_id and
    defined $stack_id and
    defined $path_base and
    defined $outroot and
    defined $camera;

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $skycal_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

my $logDest = $ipprc->filename("LOG.EXP", $outroot);
$ipprc->redirect_to_logfile($logDest) or my_die( "Unable to redirect output", $skycal_id, $PS_EXIT_SYS_ERROR ) if $redirect;

my $traceDest  = $ipprc->filename("TRACE.EXP", $outroot);
my $stats      = $ipprc->filename("PSASTRO.STATS", $outroot);

my $outbase = basename($outroot);

# Get list of input images to stack photometry
my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files
my $files;

# Recipes to use based on reduction class
$reduction = 'DEFAULT' unless defined $reduction;

# generate the input 
my $cmdflags = "";

unless ($run_state eq 'new' or $run_state eq 'update') {
    &my_die("$run_state is not a valid value for run-state\n", $skycal_id, $PS_EXIT_CONFIG_ERROR);
}
my $updateMode = $run_state eq 'update';

my $recipe_psastro  = $ipprc->reduction($reduction, 'STATICSKY_CALIBRATION'); # Recipe to use for psastro
unless ($recipe_psastro) {
    &my_die("Couldn't find selected reduction for STATICSKY_CALIBRATION: $reduction\n", $skycal_id, $PS_EXIT_CONFIG_ERROR);
}

# XXX: We are making the bad assumption here that the staticksky run used the same reduction class
# as this skycalRun
my $recipe_psphot = $ipprc->reduction($reduction, "STACKPHOT_PSPHOT");
unless ($recipe_psphot) {
    &my_die("Couldn't find selected reduction for STACKPHOT_PSPHOT: $reduction\n", $skycal_id, $PS_EXIT_CONFIG_ERROR);
}

print "reduction:      $reduction\n";
print "recipe_psastro: $recipe_psastro\n";
print "recipe_psphot:  $recipe_psphot\n";

my $configuration = $ipprc->filename("PSASTRO.CONFIG", $outroot);

# find input file
# First check the expected file name where the stack_id is the FILE_ID
my $file;
# if (!$singlefilter) {
if (1) {

    $file = $ipprc->filename('PSPHOT.STACK.OUTPUT', $path_base, $stack_id);
    if (! $ipprc->file_resolve($file)) {
        print "\nfailed to resolve $file\n";
        # file with proper file rule not found. Try older systems
        if ($singlefilter) {
            # input is from a single filter static sky run which used psphot instead of psphotStack
            # The file rule is different for psphot
            $file = $ipprc->filename('PSPHOT.OUT.CMF.MEF', $path_base);
            &my_die("Unable to find input for $stack_id $filter", $skycal_id, $PS_EXIT_SYS_ERROR) 
                unless $ipprc->file_exists($file);
        } else {
            # XXX: Beginning of section that can be removed eventually
            # no file with the expected name found
            # assume that the input is from an early multifilter staticsky run that did not use stack_id as the FILE_ID
            # but instead used FILE_ID = [0 .. num_filters-1]
            if (!$filter) {
                &my_die("filter not supplied unable to identify appropriate staticsky input.", $skycal_id, $PS_EXIT_SYS_ERROR);
            }
            print "Trying old style FILE_ID\n";
            my $max_filters = 5;
            for (my $i=0; $i < $max_filters; $i++) {
                my $file_id = sprintf "%03d", $i;
                $file  = $ipprc->filename('PSPHOT.STACK.OUTPUT', $path_base, $file_id);
                my $resolved = $ipprc->file_resolve($file);
                if (!$resolved) {
                    # we fail here assumming that if file_id N doesn't exist, neither to N+1
                    print "\nfailed to resolve $file\n\n";
                    &my_die("unable to identify appropriate staticsky input.", $skycal_id, $PS_EXIT_SYS_ERROR);
                }
                # Check the filter id for this file
                my $command = "$fhead $resolved | grep FILTERID";
                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => 0);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform $command: $error_code", $skycal_id, $PS_EXIT_SYS_ERROR);
                }
                # Expected output: HIERARCH FPA.FILTERID = 'r.00000 ' / Filter used (parsed, abstract name)
                my ($undef, undef, undef, $filt) = split " ", join("", @$stdout_buf);
                my $this_filter = substr $filt, 1, 7;

                # if it matches we're done
                last if $this_filter eq $filter;

                # nope loop around to try the next one
                print "Input file for $filter is not $file ($this_filter)\n";
                $file = undef;
            }
        }
        # XXX: End of section that can be removed eventually
    }
    if ($file) {
        print "\nInput file for $stack_id filter: $filter: $file\n";
    } else {
        &my_die("Unable to find input file for stack_id $stack_id.", $skycal_id, $PS_EXIT_SYS_ERROR);
    }
}

my $resolved = $ipprc->file_resolve($file);
if (!$resolved) {
    &my_die("Unable to find input file: $file", $skycal_id, $PS_EXIT_SYS_ERROR);
}

# XXX: The following is for compatability with psphotStack cmfs created prior to adding NDET
# read the input header to find the number of detections
my $compatability_flags = "";
my $n_detections = 0;
{
    # NSTARS is buggy. It only counts sources with a model, and counts them twice.
    # We want the actual number of sources (real + matched). Use the length of the psf table
    my $command = "echo $resolved | $fields -n SkyChip.psf NAXIS2";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => 0);
        # fields does not return 0 on success
    (undef, $n_detections)  = split " ", join "", @$stdout_buf;
    chomp $n_detections;
    &my_die("Unable to find number of detections from $file: ", $skycal_id, $PS_EXIT_SYS_ERROR)
        unless defined $n_detections;

    $compatability_flags .= " -n_detections $n_detections";

}


# Perform psastro analysis
my $output_sources_filerule =  'PSASTRO.OUTPUT.CMF';
my $quality = 0;
{
    my $command = "$psastro $outroot";
    $command .= " -file $file";
#     $command .= " -threads $threads" if defined $threads;  # psastro doesn't currently accept -threads
    $command .= " -recipe PSASTRO  $recipe_psastro";
    $command .= " -recipe PSPHOT   $recipe_psphot";
    $command .= " -F PSASTRO.OUTPUT $output_sources_filerule";
    $command .= " -Db PSASTRO.SAVE.CFF F" if $updateMode;
    $command .= " -stats $stats";
    # $command .= " -recipe PPSTATS CAMSTATS";
    $command .= " -recipe PPSTATS SKYCALSTATS";
    $command .= " -dumpconfig $configuration" if $configuration;
    $command .= " -tracedest $traceDest";
    $command .= " -dbname $dbname" if defined $dbname;

    unless ($no_op) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform psastro: $error_code", $skycal_id, $error_code);
        }

        &my_die("Couldn't find stats file: $stats", $skycal_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($stats);

        {
            my $resolved = $ipprc->file_resolve($stats) 
                or  &my_die("failed to resolve stats file: $stats", $skycal_id, $PS_EXIT_SYS_ERROR) ;
            my $command = "$ppStatsFromMetadata $resolved - STATICSKY_CAL";
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform psastro: $error_code", $skycal_id, $error_code);
            }
            $cmdflags = join "", @$stdout_buf;
            chomp $cmdflags;
        }

        # if ppStats didn't get n_detections use the compatability version extracted above
        $cmdflags .= $compatability_flags unless ($cmdflags =~ /-n_detections/);

        ($quality) =  $cmdflags =~ /-quality (\d+)/;
        if (!$quality) {
            my $outputSources  = $ipprc->filename($output_sources_filerule, $outroot);
            &my_die("Couldn't find expected output file: $outputSources", $skycal_id, $PS_EXIT_SYS_ERROR)
                unless $ipprc->file_exists($outputSources);

            &my_die("Couldn't find expected output file: $configuration", $skycal_id, $PS_EXIT_SYS_ERROR) 
                unless $ipprc->file_exists($configuration);
        }
    } else {
        print "Not executing: $command\n";
    }
}


# Add the result to the database
if (!$updateMode) {
    my $command = "$staticskytool -skycal_id $skycal_id";
    $command .= " -addskycalresult -path_base $outroot";
    $command .= " $cmdflags";
    $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
    $command .= " -hostname $host" if defined $host;
    $command .= " -dbname $dbname" if defined $dbname;

    unless($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            my $err_message = "Unable to perform $command";
	    warn($err_message);
	    exit $error_code;
        }
    } else {
        print "skipping $command\n";
    }
} else {
    if ($quality) {
        # First update the result
        my $command = "$staticskytool -updateskycal -skycal_id $skycal_id -set_fault 0";
        $command .= " -set_quality $quality";
        $command .= " -dbname $dbname" if defined $dbname;

        unless($no_update) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                my $err_message = "Unable to perform $command";
                warn($err_message);
                exit $error_code;
            }
        } else {
            print "skipping $command\n";
        }
    }
    my $command = "$staticskytool -skycal_id $skycal_id";
    $command .= ' -updateskycalrun -set_state full';
    $command .= " -dbname $dbname" if defined $dbname;

    unless($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            my $err_message = "Unable to perform $command";
	    warn($err_message);
	    exit $error_code;
        }
    } else {
        print "skipping $command\n";
    }
}

exit 0;


sub my_die
{
    my $msg = shift;            # Warning message on die
    my $skycal_id = shift;        # Diff identifier
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    warn($msg);
    if (defined $skycal_id and not $no_update) {
        my $command = "$staticskytool -skycal_id $skycal_id";
        if ($updateMode) {
            $command .= " -updateskycal -set_fault $exit_code";
        } else {
            $command .= " -addskycalresult -fault $exit_code";
            $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
            $command .= " -hostname $host" if defined $host;
            $command .= " -path_base $outroot" if defined $outroot;
        }
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
