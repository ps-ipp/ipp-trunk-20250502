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
my $psphotStack = can_run('psphotStack') or (warn "Can't find psphotStack" and $missing_tools = 1);
my $psphot = can_run('psphot') or (warn "Can't find psphot" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

sub HelpMessage {
    print "USAGE: staticsky.pl --sky_id (sky_id) --outroot (root) --camera (camera) [options]\n";
    print "  --sky_id (sky_id)   : run identifier\n";
    print "  --run-state (state) : run state (new or update)\n";
    print "  --camera (camera)   : Camera name\n";
    print "  --dbname (dbname)   : Database name\n";
    print "  --threads (N)       : Number of threads to use\n";
    print "  --outroot (root)    : Output root name\n";
    print "  --reduction (class) : Reduction class\n";
    print "  --require_sources   : Require that the stack source files exist\n";
    print "  --verbose           : Be extra verbose\n";
    print "  --no-update         : Don't update the database?\n";
    print "  --no-op             : Don't do any operations?\n";
    print "  --redirect-output   : send output to the log file,\n";
    print "  --save-temps        : Save temporary files?\n";
    print "  --help              : show this message\n";
    exit 2;
}

# XXX test:
print "run staticsky.pl @ARGV\n";

my ($sky_id, $camera, $dbname, $threads, $outroot, $reduction, $require_sources, $verbose, $no_update, $no_op, $redirect, $save_temps);
my $run_state ='new';

GetOptions(
    'sky_id=s'          => \$sky_id, # Diff identifier
    'run-state=s'       => \$run_state, 
    'camera|c=s'        => \$camera, # Camera name
    'dbname|d=s'        => \$dbname, # Database name
    'threads=s'         => \$threads,   # Number of threads to use
    'outroot=s'         => \$outroot, # Output root name
    'reduction=s'       => \$reduction, # Reduction class
    'require_sources'   => \$require_sources, # Require that the stack sources exist.
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update, # Don't update the database?
    'no-op'             => \$no_op, # Don't do any operations?
    'redirect-output'   => \$redirect,
    'save-temps'        => \$save_temps, # Save temporary files?
    'help'              => sub { HelpMessage() },
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;

pod2usage(
    -msg => "Required options: --sky_id --outroot --camera",
    -exitval => 3,
          ) unless
    defined $sky_id and
    defined $outroot and
    defined $camera;


&my_die("unexpected run state: $run_state",  $sky_id, $PS_EXIT_PROG_ERROR) unless ($run_state eq 'new') or ($run_state eq 'update');
my $updatemode = $run_state eq 'update';

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $sky_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

my $logDest = $ipprc->filename("LOG.EXP", $outroot);
$ipprc->redirect_to_logfile($logDest) or my_die( "Unable to redirect output", $sky_id, $PS_EXIT_SYS_ERROR ) if $redirect;

my $traceDest      = $ipprc->filename("TRACE.EXP", $outroot);
# my $source_id = $ipprc->source_id($dbname, $PS_TABLE_ID_STATICSKY);

my $outbase = basename($outroot);
my ($listFile, $listName) = tempfile("/tmp/$outbase.list.XXXX", UNLINK => !$save_temps );

# Get list of input images to stack photometry
my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files
my $files;
{
    my $command = "$staticskytool -inputs -sky_id $sky_id";
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform staticskytool -inputs: $error_code", $sky_id, $error_code);
    }

    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $sky_id, $PS_EXIT_PROG_ERROR);
    $files = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $sky_id, $PS_EXIT_PROG_ERROR);
}

# Recipes to use based on reduction class
$reduction = 'DEFAULT' unless defined $reduction;


# generate the input
print $listFile "INPUT   MULTI\n";
my $nInputs = @$files;

{
        my $recipe_psphot  = $ipprc->reduction($reduction, 'STACKPHOT_PSPHOT'); # Recipe to use for psphot
        my $recipe_ppsub   = $ipprc->reduction($reduction, 'STACKPHOT_PPSUB'); # Recipe to use for ppsub
        my $recipe_ppstack = $ipprc->reduction($reduction, 'STACKPHOT_PPSTACK'); # Recipe to use for ppstack
        unless ($recipe_psphot) {
            &my_die("Couldn't find selected reduction for STACKPHOT: $reduction\n", $sky_id, $PS_EXIT_CONFIG_ERROR);
        }

        print "reduction:      $reduction\n";
        print "recipe_psphot:  $recipe_psphot\n";
        print "recipe_ppsub:   $recipe_ppsub\n";
        print "recipe_ppstack: $recipe_ppstack\n";
        my $configuration = $ipprc->filename("PSPHOT.STACK.CONFIG", $outroot);

	# XXX this script and psphotStack use the recipe file inconsistently:
	# XXX psphotStack respects the value of PSPHOT.STACK.USE.RAW from the config,
	# XXX but this script ignores it: if PSPHOT.STACK.USE.RAW is FALSE (needConvolvedImages = 1)
	# XXX then psphotStack will expect entries in the input mdc file (CNV:) that this script 
	# XXX is not supplying.
        my $needConvolvedImages = 0;

        foreach my $file (@$files) {
            print $listFile "INPUT   METADATA\n";

            # XXX if we take the input from 'warp', we will need to make different selections here
            my $path_base = $file->{path_base};
            print "input: $path_base\n";
            my $stack_id = $file->{stack_id};

            my ($imageCnv, $maskCnv, $weightCnv, $expnumCnv);
            if ($needConvolvedImages) {
                $imageCnv  = $ipprc->filename("PPSTACK.OUTPUT",          $path_base ); # Image name
                $maskCnv   = $ipprc->filename("PPSTACK.OUTPUT.MASK",     $path_base ); # Mask name
                $weightCnv = $ipprc->filename("PPSTACK.OUTPUT.VARIANCE", $path_base ); # Weight name
                $expnumCnv = $ipprc->filename("PPSTACK.OUTPUT.EXPNUM",   $path_base ); # Expnum name
            }

            my $imageRaw  = $ipprc->filename("PPSTACK.UNCONV",          $path_base ); # Image name
            my $maskRaw   = $ipprc->filename("PPSTACK.UNCONV.MASK",     $path_base ); # Mask name
            my $weightRaw = $ipprc->filename("PPSTACK.UNCONV.VARIANCE", $path_base ); # Weight name
            my $expnumRaw = $ipprc->filename("PPSTACK.UNCONV.EXPNUM",   $path_base ); # Expnum name

            my $sources;
            my ($psfRaw, $backmdlRaw);
            if (!$updatemode) {
                # sources from stack run. 
                # XXX: We don't need this file anymore.
                $sources   = $ipprc->filename("PSPHOT.OUT.CMF.MEF",      $path_base ); # Sources name
            } else {
                # name of output sources from this run when in new state
                $require_sources = 1 unless $no_op;
                $sources  = $ipprc->filename("PSPHOT.STACK.OUTPUT", $outroot, $stack_id);
                # During update we rename the sources file to this
                my $originalSources = "$sources.original";
                if (!$ipprc->file_exists($originalSources)) {
                    if ($ipprc->file_exists($sources)) {
                        unless ($no_op) {
                            if (!$ipprc->file_rename($sources, $originalSources)) {
                                &my_die("failed to rename $sources $originalSources",  $sky_id, $PS_EXIT_PROG_ERROR);
                            }
                        }
                    } else {
                        # maybe this should be a fault 2
                        &my_die("failed to to find either sources file to update as either $sources or $originalSources",  $sky_id, $PS_EXIT_PROG_ERROR);
                    }
                }
                $sources = $originalSources;
                $psfRaw  = $ipprc->filename("PSPHOT.STACK.PSF.SAVE", $outroot, $stack_id);
                $backmdlRaw  = $ipprc->filename("PSPHOT.STACK.BACKMDL", $outroot, $stack_id);
            }


            # XXX is this the correct PSF file?
            my $psfCnv    = $ipprc->filename("PPSTACK.TARGET.PSF",      $path_base ); # PSF name

            # XXX we could make some different choices if some inputs do not exist...
            &my_die("Couldn't find input: $imageRaw",  $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists("$imageRaw");
            &my_die("Couldn't find input: $maskRaw",   $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists("$maskRaw");
            &my_die("Couldn't find input: $weightRaw", $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists("$weightRaw");
            &my_die("Couldn't find input: $expnumRaw", $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists("$expnumRaw");
            if ($needConvolvedImages) {
                &my_die("Couldn't find input: $imageCnv",  $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists("$imageCnv");
                &my_die("Couldn't find input: $maskCnv",   $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists("$maskCnv");
                &my_die("Couldn't find input: $weightCnv", $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists("$weightCnv");
                &my_die("Couldn't find input: $expnumCnv", $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists("$expnumCnv");
                &my_die("Couldn't find input: $psfCnv",    $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists("$psfCnv");
            }
	    
	    my $have_sources = $ipprc->file_exists("$sources");
	    if ($require_sources) {
		&my_die("Couldn't find input: $sources",   $sky_id, $PS_EXIT_SYS_ERROR) unless $have_sources;
	    }
	    
            print $listFile "  STACK_ID      S64  " . $stack_id  . "\n";
            print $listFile "  RAW:IMAGE     STR  " . $imageRaw  . "\n";
            print $listFile "  RAW:MASK      STR  " . $maskRaw   . "\n";
            print $listFile "  RAW:VARIANCE  STR  " . $weightRaw . "\n";
            print $listFile "  RAW:EXPNUM    STR  " . $expnumRaw . "\n";
	    if ($have_sources) {
		print $listFile "  SOURCES       STR  " . $sources   . "\n";
	    }
            if ($updatemode) {
                &my_die("Couldn't find input: $psfRaw", $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists("$psfRaw");
                &my_die("Couldn't find input: $backmdlRaw",    $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists("$backmdlRaw");
                print $listFile "  RAW:PSF       STR  " . $psfRaw . "\n";
                print $listFile "  RAW:BACKMDL   STR  " . $backmdlRaw . "\n";
            }
            if ($needConvolvedImages) {
                print $listFile "  CNV:IMAGE     STR  " . $imageCnv  . "\n";
                print $listFile "  CNV:MASK      STR  " . $maskCnv   . "\n";
                print $listFile "  CNV:VARIANCE  STR  " . $weightCnv . "\n";
                print $listFile "  CNV:EXPNUM    STR  " . $expnumCnv . "\n";
                print $listFile "  CNV:PSF       STR  " . $psfCnv    . "\n";
            }

            print $listFile "END\n\n";
        }

        close $listFile;

        # Perform stack photometry analysis
        {
            my $command = "$psphotStack $outroot";
            $command .= " -updatemode" if $updatemode;
            $command .= " -input $listName";
            $command .= " -threads $threads" if defined $threads;
            $command .= " -recipe PSPHOT  $recipe_psphot";
            $command .= " -recipe PPSUB   $recipe_ppsub";
            $command .= " -recipe PPSTACK $recipe_ppstack";
            $command .= " -dumpconfig $configuration" if !$updatemode;
            $command .= " -tracedest $traceDest -log $logDest";
            # $command .= " -dbname $dbname" if defined $dbname;

            unless ($no_op) {
                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform psphotStack: $error_code", $sky_id, $error_code);
                }

                # my $outputStatsReal = $ipprc->file_resolve($outputStats);
                # &my_die("Couldn't find expected output file: $outputStats", $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputStatsReal);

                # measure chip stats
                # $command = "$ppStatsFromMetadata $outputStatsReal - DIFF_SKYCELL";
                # ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                #     run(command => $command, verbose => $verbose);
                # unless ($success) {
                #     $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                #     &my_die("Unable to perform ppStatsFromMetadata: $error_code", $sky_id, $error_code);
                # }
                # foreach my $line (@$stdout_buf) {
                #     $cmdflags .= " $line";
                # }
                # chomp $cmdflags;

                # my ($quality) = $cmdflags =~ /-quality (\d+)/; # Quality flag

                my $quality = 0;
                if (!$quality) {

                    # Get the output filenames
                    # we have one set of output files per input file set
                    for (my $i = 0; $i < @$files; $i++) {
                        my $stack_id = @$files[$i]->{stack_id};
                        my $outputName     = $ipprc->filename("PSPHOT.STACK.OUTPUT.IMAGE", $outroot, $stack_id);
                        my $outputMask     = $ipprc->filename("PSPHOT.STACK.OUTPUT.MASK", $outroot, $stack_id);
                        my $outputVariance = $ipprc->filename("PSPHOT.STACK.OUTPUT.VARIANCE", $outroot, $stack_id);
                        my $outputSources  = $ipprc->filename("PSPHOT.STACK.OUTPUT", $outroot, $stack_id);

                        # XXX these are optional and not generated by default
                        # &my_die("Couldn't find expected output file: $outputName", $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputName);
                        # &my_die("Couldn't find expected output file: $outputMask", $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputMask);
                        # &my_die("Couldn't find expected output file: $outputVariance", $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputVariance);
                        &my_die("Couldn't find expected output file: $outputSources", $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputSources);
                    }

                    #my $configuration  = $ipprc->filename("PPSUB.CONFIG", $outroot);
                    #my $outputStats    = $ipprc->filename("SKYCELL.STATS", $outroot);
                    #my $traceDest      = $ipprc->filename("TRACE.EXP", $outroot);

                    my $chisqName     = $ipprc->filename("PSPHOT.CHISQ.IMAGE", $outroot);
                    my $chisqMask     = $ipprc->filename("PSPHOT.CHISQ.MASK", $outroot);
                    my $chisqVariance = $ipprc->filename("PSPHOT.CHISQ.VARIANCE", $outroot);

                    # XXX check the recipe -- should we expect these to exist?
                    # &my_die("Couldn't find expected output file: $chisqName",           $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($chisqName);
                    # &my_die("Couldn't find expected output file: $chisqMask",           $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($chisqMask);
                    # &my_die("Couldn't find expected output file: $chisqVariance", $sky_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($chisqVariance);

                    if (!$updatemode) {
                        # XXX: should be checking for existence of psf and backmdl output files
                    }
                }
            } else {
                print "Not executing: $command\n";
            }
        }
}

unless ($no_update) {

    # Add the staticsky result
    {
        my $command = "$staticskytool -sky_id $sky_id";
        $command .= " -dbname $dbname" if defined $dbname;
        if ($updatemode) {
            $command .= " -updaterun -set_state full";
        } else {
            $command .= " -addresult -path_base $outroot";
            $command .= " -num_inputs $nInputs";
            $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
            $command .= " -hostname $host" if defined $host;
        }

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            my $err_message = "Unable to perform: $command\n";
            warn($err_message);
            exit $error_code;
        }
    }
}


sub my_die
{
    my $msg = shift;            # Warning message on die
    my $sky_id = shift;        # Diff identifier
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    warn($msg);
    if (defined $sky_id and not $no_update) {
        
        my $command = "$staticskytool -sky_id $sky_id -fault $exit_code";
        $command .= " -dbname $dbname" if defined $dbname;
        if ($updatemode) {
            $command .= " -updateresult";
        } else {
            $command .= " -addresult";
            $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
            $command .= " -hostname $host" if defined $host;
            $command .= " -path_base $outroot" if defined $outroot;
        }
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
