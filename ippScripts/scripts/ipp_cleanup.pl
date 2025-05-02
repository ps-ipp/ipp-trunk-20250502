#!/usr/bin/env perl

# this script is used to cleanup the files from the different ipp
# stages.  It can be called with one of two cleanup modes: clean and
# purge.  the former removes temporary data files, leaving behind
# enough information for the results to be rebuilt.  The latter
# removes all but basic logging data.

use warnings;
use strict;
use Carp;

use IPC::Cmd 0.36 qw( can_run run );
use File::Spec;
use PS::IPP::Config 1.01 qw( :standard );
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Parse the command-line arguments
my ($stage, $camera, $stage_id, $mode, $path_base, $dbname, $verbose, $no_op, $helplist, $logfile, $check_all);
my $very_verbose = 0;

# turn this on to set data_state to error_state for components for which cleanup fails
# Note: we no longer do this, we leave the state alone.
my $set_error_state_for_file = 0;

# turn this on the set the state to error_state when errors occur cleaning up individual components
# We may stop doing this
my $set_error_state_for_run = 0;

# this gets set to 1 the first time we set the corresponding destreak run to be cleaned
#my $ds_done = 0;
# magic is dead
my $ds_done = 1;

GetOptions('stage=s'        => \$stage,     # which analysis stage to clean?
           'camera|i=s'     => \$camera,    # user-supplied camera name
           'stage_id=s'     => \$stage_id,  # id for this stage (only needed for certain stages)
           'mode|m=s'       => \$mode,      # cleanup mode (clean / purge)
           'path_base=s'    => \$path_base, # basename for files
           'check-all'      => \$check_all, # if set clean all chips regardless of data_state
           'dbname|d=s'     => \$dbname,    # Database name
           'verbose'        => \$verbose,   # Print to stdout
           'no-op'          => \$no_op,     # pretend but don't actually inject
           'helplist'       => \$helplist,  # give help listing
           'logfile=s'      => \$logfile    # destination for stdout and stderr
           ) or pod2usage( 2 );

pod2usage( -msg => "remove temporary / all data files for an IPP analysis stage",
           -exitval => 2) if defined $helplist;

pod2usage( -msg => "Usage: $0 --camera (name) --stage (stage) --stage_id (stage_id) --mode (mode) [--path_base (path)] [--dbname dbname] [--no-op] [--help]",
           -exitval => 2 ) if scalar @ARGV;

pod2usage( -msg => "Required options:--camera (name) --stage (stage) --mode (mode)",
           -exitval => 3) unless
    defined $camera and
    defined $stage and
    defined $mode;

my $ipprc = PS::IPP::Config->new( $camera ) or my_die("Unable to set up", $stage_id, $PS_EXIT_CONFIG_ERROR); # this is used for PATH, NEB filename conversions

# $mode must be one of "goto_cleaned", "goto_scrubbed", or
# "goto_purged" goto_cleaned and goto_scrubbed both result in
# 'cleaned' on success ('scrubbed' allows chips without config files
# to be cleaned; they cannot be recovered, but the small data is left
# behind). XXX make 'scrubbed' a data_state?
unless (($mode eq "goto_cleaned") || ($mode eq "goto_scrubbed") || ($mode eq "goto_purged")) {
    die "invalid cleanup mode $mode\n";
}

$ipprc->redirect_to_logfile($logfile) or 
        &my_die("Unable to redirect ouput", $stage, $stage_id, $PS_EXIT_UNKNOWN_ERROR) if $logfile;


my $hostname = `hostname`;
chomp $hostname;
print STDERR "starting cleanup for $stage $stage_id on $hostname\n" ;

my $nebulous_server;

my $bzip2 = can_run('bzip2') or die 'cannot find bzip2\n';

# set this to 1 to enable checking for files on dead nodes
# it is off for now because the implementation is a hack
# See comments below.
my $check_for_gone = 1;

my $error_state;
my $done_state;
if ($mode eq "goto_cleaned")  { $error_state = "error_cleaned"; $done_state = "cleaned"; }
if ($mode eq "goto_scrubbed") { $error_state = "error_scrubbed"; $done_state = "scrubbed";}
if ($mode eq "goto_purged")   { $error_state = "error_purged";   $done_state = "purged";}


my %stages = ( "chip" => 1, "camera" => 1, "fake" => 1, "warp" => 1, "stack" => 1, "diff"  => 1,
               "chip_bg" => 1, "warp_bg" => 1,
               "detrend.processed" => 1, "detrend.resid" => 1, "detrend.process.exp" => 0, "detrend.stack.imfile" => 0,
               "detrend.normstat.imfile" => 0, "detrend.norm.imfile" => 0, "detrend.norm.exp" => 0 );
unless (exists($stages{$stage})) {
    die "unknown stage $stage for ipp_cleanup.pl\n";
}
unless (($stages{$stage})) {
    die "unimplemented stage $stage for ipp_cleanup.pl\n";
}


my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# choice of files to delete depends on the stage
if ($stage eq "chip") {

    die "--stage_id required for stage chip\n" if !$stage_id;
    ### select the imfiles for this entry

    # this stage uses 'chiptool'
    my $chiptool = can_run('chiptool') or die "Can't find chiptool";
    my $censorObjects = can_run('censorObjects') or die "Can't find censorObjects";

    # Get list of component imfiles
    # XXX may need a different my_die for each stage
    my $imfiles;                      # Array of component files
    my $command = "$chiptool -pendingcleanupimfile -chip_id $stage_id"; # Command to run
    $command .= ' -all' if ($check_all);
    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) 
        = run(command => $command, verbose => $very_verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform chiptool: $error_code", "chip", $stage_id, $error_code);
    }

    # if there are no chipProcessedImfiles (@$stdout_buf == 0) then assume that we're done
    # it could be that there are no chipProcessedImfiles at all if say if a run was changed from drop to goto_cleaned
    # or of a run was set to update and then back to goto_cleaned before any images were processed
    if (@$stdout_buf == 0)  {
        my $command = "$chiptool -chip_id $stage_id -updaterun -set_state $done_state";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform chiptool -processedimfile: $error_code", "chip", $stage_id, $error_code);
        }
        exit 0;
    }

    # extract the metadata for the files into a hash list
    $imfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", "chip", $stage_id, $PS_EXIT_PROG_ERROR);

    my $numchips = scalar @$imfiles;
    print "Found $numchips to clean\n";

    my $clean_sources = 0;
    if ((scalar @$imfiles > 0) and ($mode eq 'goto_cleaned')) {
        # go and find the smf file(s) for the associated camRun and check the status of the file
        # if a good one is found we have the sources for this chipRun and thus can clean the cmfs
        my $command = "$chiptool -listrun -chip_id $stage_id";
        $command .= " -dbname $dbname" if defined $dbname;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $very_verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform chiptool -listrun: $error_code", "chip", $stage_id, $error_code);
        }
        my $entries = $mdcParser->parse_list(join "", @$stdout_buf) or
            &my_die("Unable to parse metadata config doc", "chip", $stage_id, $PS_EXIT_PROG_ERROR);
        my $good_smf = 0;
        foreach my $entry (@$entries) {
            my $camRun_state = $entry->{camRun_state};
            next if $camRun_state  ne 'full';
            my $cam_id = $entry->{cam_id};
            if (!$cam_id) {
                carp('no cam_id for listrun entry');
                next;
            }
            my $cam_path_base = $entry->{cam_path_base};
            if ( !defined $cam_path_base ) {
                carp("no path_base for $cam_id\n");
                next;
            }

            # XXX: This assumes that the filerules are filerules-split
            my $smf =  $ipprc->filename("PSASTRO.OUTPUT", $cam_path_base);
            if (!$ipprc->file_exists($smf)) {
                carp("smf for $cam_path_base not found");
                next;
            }
            # we run the program censorObjects in the check mode
            # If this program succeeds the smf is a valid fits file and each of the
            # extensions was succesfully read.
            # XXX: create a new program outside of magic that performs this check

            my $command = "$censorObjects -checkinputonly -file $smf";
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = 
                                        run(command => $command, verbose => $very_verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                print STDERR "censorObjects failed:\n";
                print STDERR "\nSTDOUT:\n" . join "", @$stdout_buf;
                print STDERR "\nSTDERR:\n" . join "", @$stderr_buf;
                # smf file is probably corrupt. Carry on. We will not clean sources unless another good smf is found
                # &my_die("Unable to perform censorObjects -checkinputonly: $error_code", "chip", $stage_id, $error_code);
                next;
            }
            $good_smf++;
        }
        if ($good_smf) {
            # we have a good one so we can clean the sources
            $clean_sources = 1;
            print "Found $good_smf good smf files will clean sources\n";
        } else {
            print "Unable to find good smf file will NOT clean sources\n";
        }
    }

    # loop over all of the imfiles, determine the path_base and class_id for each
    my $num_errors = 0;
    my $num_updated = 0;
    foreach my $imfile (@$imfiles) {
        my $class_id = $imfile->{class_id};
        my $path_base = $imfile->{path_base};
        my $data_state = $imfile->{data_state};
        my $status = 1;
        $status = 0 unless defined $path_base and $path_base ne "NULL";

        my $quality = $imfile->{quality};
        my $good_quality = ($quality == 0);

        print "Starting cleanup for $class_id\n";

        # don't clean up unless the data needed to update is available
        # modes goto_purged and goto_scrubbed will remove files even if the config is non-existent
        # goto_scrubbed now requires the config file to not exist.
        if ($status and $good_quality) {
            if ($mode eq "goto_cleaned") {
                my $config_file = $ipprc->filename("PPIMAGE.CONFIG", $path_base, $class_id);

                unless ($ipprc->file_exists($config_file)) {
                    my $fault = $imfile->{fault};

                    if (file_gone($config_file) or !storage_object_exists($config_file)) {
                        # config file was lost. Clean up. If the chip is ever updated a new config
                        # file will be created
                        print STDERR "forcing cleanup chip $stage_id $class_id fault: $fault quality: $quality"
                            . " because config file ($config_file) is gone\n";
                    } elsif ($fault == 0 and $quality == 0) {
                            print STDERR "skipping cleaning up chip $stage_id $class_id fault: $fault quality: $quality"
                                . " because config file ($config_file) is missing\n";
                            $status = 0;
                    } else {
                            # config file is missing but this is a bad chip anyways so clean it
                            print STDERR "cleaning up chip $stage_id $class_id fault: $fault quality: $quality"
                                . " even though config file ($config_file) is missing\n";
                    }
                }
            }
            elsif ($mode eq "goto_scrubbed") {
                my $config_file = $ipprc->filename("PPIMAGE.CONFIG", $path_base, $class_id);

                if ($ipprc->file_exists($config_file)) {
                    print STDERR "skipping scrubbed for chip $stage_id $class_id "
                        . " because config file ($config_file) is present\n";
                    $status = 0;
                }
            }
        }

        if ($status) {
            # array of actual filenames to delete
            my @files = ();

            addFilename (\@files, "PPIMAGE.CHIP", $path_base, $class_id, 1);
            addFilename (\@files, "PPIMAGE.CHIP.MASK", $path_base, $class_id, 1);
            addFilename (\@files, "PPIMAGE.CHIP.VARIANCE", $path_base, $class_id, 1);
	    addFilename (\@files, "PPIMAGE.PATTERN", $path_base, $class_id, 0);
            if ($clean_sources) {
                addFilename (\@files, "PSPHOT.OUTPUT", $path_base, $class_id);
                addFilename (\@files, "PPIMAGE.BIN1", $path_base, $class_id);
            }
            if ($mode eq "goto_purged") {
                # additional files to remove for 'purge' mode
                if (!$clean_sources) {
                    # these weren't added above but we do want to clean it
                    addFilename (\@files, "PSPHOT.OUTPUT", $path_base, $class_id);
                    addFilename (\@files, "PPIMAGE.BIN1", $path_base, $class_id);
                }
                
                # background model is needed to build stack background images so we do not remove it
                # addFilename (\@files, "PSPHOT.BACKMDL", $path_base, $class_id);

                addFilename (\@files, "PSPHOT.PSF.SAVE", $path_base, $class_id);
                addFilename (\@files, "PPIMAGE.OUTPUT.FPA1", $path_base, $class_id);
                addFilename (\@files, "PPIMAGE.OUTPUT.FPA2", $path_base, $class_id);
                addFilename (\@files, "PPIMAGE.BIN2", $path_base, $class_id);
                addFilename (\@files, "PPIMAGE.JPEG1", $path_base, $class_id);
                addFilename (\@files, "PPIMAGE.JPEG2", $path_base, $class_id);
                addFilename (\@files, "PPIMAGE.STATS", $path_base, $class_id);
                addFilename (\@files, "PPIMAGE.CONFIG", $path_base, $class_id);
            }

            # actual command to delete the files
            $status = &delete_files (\@files);
        }
        bzip2_file("LOG.IMFILE", $path_base, $class_id);
        bzip2_file("LOG.IMFILE.UPDATE", $path_base, $class_id);

        if ($status)  {
            my $update_chip = 1;
            my $command = "$chiptool -chip_id $stage_id -class_id $class_id";
            if ($mode eq "goto_purged") {
                $command .= " -topurgedimfile";
                if ($data_state eq 'purged') {
                    $update_chip = 0;
                }
            }
            elsif ($mode eq "goto_cleaned") {
                $command .= " -tocleanedimfile";
                if ($data_state eq 'cleaned') {
                    $update_chip = 0;
                }
            }
            elsif ($mode eq "goto_scrubbed") {
                $command .= " -toscrubbedimfile";
                if ($data_state eq 'scrubbed') {
                    $update_chip = 0;
                }
            }

            if ($update_chip) {
                $command .= " -dbname $dbname" if defined $dbname;

                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                        run(command => $command, verbose => $verbose);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform chiptool: $error_code", "chip", $stage_id, $error_code);
                }
                set_destreak_goto_cleaned();
                $num_updated++;
            }
        } else {
            $num_errors++;

            if ($set_error_state_for_file) {
                # if an error happens for one chip, the chipRun will stay in goto_*, but the chips will go to error_* (matching the goto_*)
                my $command = "$chiptool -updateprocessedimfile -chip_id $stage_id -class_id $class_id -set_state $error_state";
                $command .= " -dbname $dbname" if defined $dbname;

                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                        run(command => $command, verbose => $verbose);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform chiptool: $error_code", "chip", $stage_id, $error_code);
                }
            }

            if ($set_error_state_for_run) {
                # We want to flag the run as well, to avoid attempting to reprocess the same data over and over again.
                $command = "$chiptool -chip_id $stage_id -updaterun -set_state $error_state";
                $command .= " -dbname $dbname" if defined $dbname;

                ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform chiptool: $error_code", "chip", $stage_id, $error_code);
                }

            }
        }
    }
    # We no longer depend on chiptool to promote the run from goto_cleaned to cleaned based on all of
    # the imfiles being in the cleaned state.
    # Due to missing files we don't always clean individual components.
    # So unless there was an error set state to $done_state
    if (!$set_error_state_for_run or $num_errors eq 0) {
        # set state to $done_state
        my $command = "$chiptool -chip_id $stage_id -updaterun -set_state $done_state";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform chiptool: $error_code", "chip", $stage_id, $error_code);
        }
    }
    print "Cleanup completed for chip_id $stage_id. num_updated: $num_updated";
    print " num_errors: $num_errors" if $num_errors;
    print "\n";
    exit 0;
}

if ($stage eq "camera") {
    die "--stage_id required for stage camera\n" if !$stage_id;
    # this stage uses 'camtool'
    my $camtool = can_run('camtool') or die "Can't find camtool";

    # Get list of component imfiles
    # XXX may need a different my_die for each stage
    my $exps;                      # Array of component files
    my $command = "$camtool -pendingcleanupexp -cam_id $stage_id"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform camtool: $error_code", "camera", $stage_id, $error_code);
    }
    $exps = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", "camera", $stage_id, $PS_EXIT_PROG_ERROR);

    my $n_exps = @$exps;
    &my_die("unexpected number of exposures $n_exps", "camera", $stage_id, $PS_EXIT_PROG_ERROR)
        if $n_exps != 1;

    my $exp = $exps->[0];
    my $path_base = $exp->{path_base};

    my $status = 1;
    # don't clean up unless the data needed to update is available
    # goto_scrubbed now requires the config file to not be present
    if ($mode eq "goto_cleaned") {
        my $config_file = $ipprc->filename("PSASTRO.CONFIG", $path_base);

        unless ($ipprc->file_exists($config_file)) {
            print STDERR "skipping cleanup for camRun $stage_id because config file is missing\n";
            $status = 0;
        }
    }
    elsif ($mode eq "goto_scrubbed") {
        my $config_file = $ipprc->filename("PSASTRO.CONFIG", $path_base);

        if ($ipprc->file_exists($config_file)) {
            print STDERR "skipping cleanup for camRun $stage_id because config file ($config_file) is present\n";
            $status = 0;
        }
    }
    if ($status) {
        my @files = ();
        # delete the temporary image datafiles
#        addFilename (\@files, "PSASTRO.OUTPUT", $path_base);
        if ($mode eq "goto_purged") {
            # additional files to remove for 'purge' mode
            addFilename (\@files, "PPIMAGE.JPEG1", $path_base);
            addFilename (\@files, "PPIMAGE.JPEG2", $path_base);
            addFilename (\@files, "PSASTRO.STATS", $path_base);
        }
        # actual command to delete the files
        $status = &delete_files (\@files);
    }

    if ($status)  {
        my $command;
        if ($mode eq "goto_cleaned") {
            $command = "$camtool -updaterun -cam_id $stage_id -set_state cleaned";
        }
        if ($mode eq "goto_scrubbed") {
            $command = "$camtool -updaterun -cam_id $stage_id -set_state scrubbed";
        }
        if ($mode eq "goto_purged") {
            $command = "$camtool -updaterun -cam_id $stage_id -set_state purged";
        }
        $command .= " -dbname $dbname" if defined $dbname;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform camtool: $error_code", "camera", $stage_id, $error_code);
        }

        set_destreak_goto_cleaned();

    } else {
        # since 'camera' has only a single imfile, we can just update the run
        my $command = "$camtool -updaterun -cam_id $stage_id -set_state $error_state";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform camtool: $error_code", "camera", $stage_id, $error_code);
        }
        exit $PS_EXIT_UNKNOWN_ERROR;
    }


    exit 0;
}

if ($stage eq "warp") {
    die "--stage_id required for stage warp\n" if !$stage_id;
    # this stage uses 'warptool'
    my $warptool = can_run('warptool') or die "Can't find warptool";

    # Get list of component imfiles
    # XXX may need a different my_die for each stage
    my $skyfiles;                      # Array of component files
    my $command = "$warptool -pendingcleanupskyfile -warp_id $stage_id"; # Command to run
    $command .= ' -all' if $check_all;
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $very_verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform warptool: $error_code", "warp", $stage_id, $error_code);
    }

    if (@$stdout_buf == 0) {
        # No skycells were found for some reason. 
        # it could be that there are no warpSkyfiles at all if say if a run was changed from drop to goto_cleaned
        # or of a run was cleaned, set to update, and then back to goto_cleaned before any images were successfully
        # updated
        my $command = "$warptool -updaterun -warp_id $stage_id -set_state $done_state";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform warptool: $error_code", "warp", $stage_id, $error_code);
        }

        exit(0);
    }
    $skyfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", "warp", $stage_id, $PS_EXIT_PROG_ERROR);

    my $numskycells = scalar @$skyfiles;
    print "Found $numskycells to clean\n";

    my @files = ();
    my $num_errors = 0;
    my $num_updated = 0;
    foreach my $skyfile (@$skyfiles) {
        my $path_base = $skyfile->{path_base};
        my $skycell_id = $skyfile->{skycell_id};
        my $data_state = $skyfile->{data_state};

        my $status = 1;
        $status = 0 unless defined $path_base and $path_base ne "NULL";

        if ($status) {
            if ($mode eq "goto_cleaned") {
                my $config_file = $ipprc->filename("PSWARP.CONFIG", $path_base, $skycell_id);

                unless ($ipprc->file_exists($config_file)) {
                    my $fault = $skyfile->{fault};
                    my $quality = $skyfile->{quality};
                    if (file_gone($config_file) or !storage_object_exists($config_file)) {
                        print STDERR "forcing cleanup warp $stage_id $skycell_id fault: $fault quality: $quality"
                            . " because config file ($config_file) is gone\n";
                    } elsif ($fault == 0 and $quality == 0) {
                            print STDERR "skipping cleaning up warp $stage_id $skycell_id fault: $fault quality: $quality"
                                . " because config file ($config_file) is missing\n";
                            $status = 0;
                    } else {
                            # config file is missing but this is a bad warp anyways so clean it
                            print STDERR "cleaning up warp $stage_id $skycell_id fault: $fault quality: $quality"
                                . " even though config file ($config_file) is missing\n";
                    }
                }
            }
            elsif ($mode eq "goto_scrubbed") {
                my $config_file = $ipprc->filename("PSWARP.CONFIG", $path_base, $skycell_id);

                if ($ipprc->file_exists($config_file)) {
                    print STDERR "skipping scrubbed for warpRun $stage_id $skycell_id" .
                        " because config file is present\n";
                    $status = 0;
                }
            }
        }

        if ($status) {
            # XXX: what is special about quality == 8007?
            if ($skyfile->{quality} != 8007 || $check_all) {
                my @files = ();

                # delete the temporary image datafiles
                addFilename(\@files, "PSWARP.OUTPUT", $path_base, $skycell_id, 1);
                addFilename(\@files, "PSWARP.OUTPUT.MASK", $path_base, $skycell_id, 1);
                addFilename(\@files, "PSWARP.OUTPUT.VARIANCE", $path_base, $skycell_id, 1);
                # these are rebuilt during update so we can delete them here
                addFilename(\@files, "PSWARP.OUTPUT.SOURCES", $path_base, $skycell_id);
                addFilename(\@files, "SKYCELL.TEMPLATE", $path_base, $skycell_id );
		addFilename(\@files, "PSPHOT.RESID",$path_base,"fpa");
                if ($mode eq "goto_purged") {
                    # additional files to remove for 'purge' mode
                    addFilename(\@files, "PSWARP.BIN1", $path_base, $skycell_id );
                    addFilename(\@files, "PSWARP.BIN2", $path_base, $skycell_id );
                    addFilename(\@files, "SKYCELL.STATS", $path_base, $skycell_id );
                    addFilename(\@files, "SKYCELL.STATS.UPDATE", $path_base, $skycell_id );
                    addFilename(\@files, "PSWARP.CONFIG", $path_base, $skycell_id);

                    # XXX: do we want to delete these? trace file is empty
                    # addFilename(\@files, "TRACE.EXP", $path_base, $skycell_id);
                }
                # actual command to delete the files
                $status = &delete_files (\@files);
            }
        }
        bzip2_file("LOG.EXP", $path_base, $skycell_id);
        bzip2_file("LOG.EXP.UPDATE", $path_base, $skycell_id);

        if ($status)  {
            my $update_skyfile = 1;
            my $command = "$warptool -warp_id $stage_id -skycell_id $skycell_id";
            if ($mode eq "goto_purged") {
                $command .= " -topurgedskyfile";
                if ($data_state eq 'purged') {
                    $update_skyfile = 0;
                }
            }
            elsif ($mode eq "goto_cleaned") {
                $command .= " -tocleanedskyfile";
                if ($data_state eq 'cleaned') {
                    $update_skyfile = 0;
                }
            }
            elsif ($mode eq "goto_scrubbed") {
                $command .= " -toscrubbedskyfile";
                if ($data_state eq 'scrubbed') {
                    $update_skyfile = 0;
                }
            }
            $command .= " -dbname $dbname" if defined $dbname;

            if ($update_skyfile) {
                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                        run(command => $command, verbose => $verbose);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform warptool: $error_code", "warp", $stage_id, $error_code);
                }

                set_destreak_goto_cleaned();
                $num_updated++;
            }

         } else {
            $num_errors++;
            if ($set_error_state_for_file) {
                my $command = "$warptool -updateskyfile -warp_id $stage_id -skycell_id $skycell_id -set_state $error_state";
                $command .= " -dbname $dbname" if defined $dbname;

                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform warptool: $error_code", "warp", $stage_id, $error_code);
                }
            }

            if ($set_error_state_for_run) {
                # We want to flag the run as well, to avoid attempting to reprocess the same data over and over again.
                $command = "$warptool -warp_id $stage_id -updaterun -set_state $error_state";
                $command .= " -dbname $dbname" if defined $dbname;

                ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform warptool: $error_code", "warp", $stage_id, $error_code);
                }
            }
        }
    }
    if (!$set_error_state_for_run or $num_errors eq 0) {
        # Set run to done state
        my $command = "$warptool -warp_id $stage_id -updaterun -set_state $done_state";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform warptool: $error_code", "warp", $stage_id, $error_code);
        }
    }
    print "Cleanup completed for warp_id $stage_id. num_updated: $num_updated";
    print " num_errors:  $num_errors" if $num_errors;
    print "\n";
    exit 0;
}

### added for cleanup, based on warp stage entry
if ($stage eq 'stack') {
    die "--stage_id required for stage stack\n" if !$stage_id;
    # this stage uses 'stacktool'
    my $stacktool = can_run('stacktool') or die "Can't find stacktool";

    # Get list of component imfiles
    my $skyfiles;                  # Array reference of component files
    my $command = "stacktool -pendingcleanupskyfile -stack_id $stage_id"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform stacktool: $error_code", "stack", $stage_id, $error_code);
    }

    if (@$stdout_buf == 0) {
        # No skycells were found for some reason.
        # Not technically an "error," but a "you told me to do X, and I can't. Please fix this yourself."
        my $command = "$stacktool -updaterun -stack_id $stage_id -set_state $error_state";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform stacktool: $error_code", "stack", $stage_id, $error_code);
        }

        exit(0);
    }

    $skyfiles = $mdcParser->parse_list(join "", @{ $stdout_buf }) or
        &my_die("Unable to parse metadata config doc", "stack", $stage_id, $PS_EXIT_PROG_ERROR);

    my @files = ();
    foreach my $skyfile (@{ $skyfiles }) {
        my $path_base = $skyfile->{path_base};
        my $skycell_id = $skyfile->{skycell_id};

        my $status = 1;
        if ((!exists($skyfile->{path_base}))||
            (!defined($path_base))) {
            $status = 0;
        }
        if ($status) {
            if ($mode eq "goto_cleaned") {
                my $config_file = $ipprc->filename("PPSTACK.CONFIG", $path_base, $skycell_id);

                unless ($ipprc->file_exists($config_file)) {
                    print STDERR "skipping cleanup for stackRun $stage_id $skycell_id" .
                        " because config file is missing\n";
                    $status = 0;
                }
            }
            elsif ($mode eq "goto_scrubbed") {
                my $config_file = $ipprc->filename("PPSTACK.CONFIG", $path_base, $skycell_id);

                if ($ipprc->file_exists($config_file)) {
                    print STDERR "skipping scrubbed for stackRun $stage_id $skycell_id" .
                        " because config file is present\n";
                    $status = 0;
                }
            }
        }

        if ($status) {
            my @files = ();
            # delete the temporary image datafiles
            addFilename(\@files, "PPSTACK.OUTPUT", $path_base, $skycell_id);
            addFilename(\@files, "PPSTACK.OUTPUT.MASK", $path_base, $skycell_id);
            addFilename(\@files, "PPSTACK.OUTPUT.VARIANCE", $path_base, $skycell_id);
            addFilename(\@files, "PPSTACK.OUTPUT.EXP", $path_base, $skycell_id);
            addFilename(\@files, "PPSTACK.OUTPUT.EXPNUM", $path_base, $skycell_id);
            addFilename(\@files, "PPSTACK.OUTPUT.EXPWT", $path_base, $skycell_id);
            addFilename(\@files, "PPSTACK.UNCONV", $path_base, $skycell_id);
            addFilename(\@files, "PPSTACK.UNCONV.MASK", $path_base, $skycell_id);
            addFilename(\@files, "PPSTACK.UNCONV.VARIANCE", $path_base, $skycell_id);
            addFilename(\@files, "PPSTACK.UNCONV.EXP", $path_base, $skycell_id);
            addFilename(\@files, "PPSTACK.UNCONV.EXPNUM", $path_base, $skycell_id);
            addFilename(\@files, "PPSTACK.UNCONV.EXPWT", $path_base, $skycell_id);
	    
            if ($mode eq "goto_purged") {
                # additional files to remove for 'purge' mode
                addFilename(\@files, "PPSTACK.CONV.KERNEL", $path_base, $skycell_id);
                addFilename(\@files, "PPSTACK.OUTPUT.JPEG1", $path_base, $skycell_id);
                addFilename(\@files, "PPSTACK.OUTPUT.JPEG2", $path_base, $skycell_id);
                # Commented out to match warp files.
                #addFilename(\@files, "PPSTACK.TARGET.PSF", $path_base, $skycell_id);
                #addFilename(\@files, "PPSTACK.CONFIG", $path_base, $skycell_id);
            }

            $status = &delete_files(\@files);
        }

        if ($status) {
            my $command = "$stacktool -stack_id $stage_id";
            if ($mode eq "goto_purged") {
                $command .= " -updaterun -set_state purged";
            }
            elsif ($mode eq "goto_cleaned") {
                $command .= " -updaterun -set_state cleaned";
            }
            elsif ($mode eq "goto_scrubbed") {
                $command .= " -updaterun -set_state scrubbed";
            }
            $command .= " -dbname $dbname" if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform stacktool: $error_code", "stack", $stage_id, $error_code);
            }

            set_destreak_goto_cleaned();

        } else {
            my $command = "$stacktool -updaterun  -stack_id $stage_id -set_state $error_state";
            $command .= " -dbname $dbname" if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform stacktool: $error_code", "stack", $stage_id, $error_code);
            }
        }
    }
    exit 0;
}

if ($stage eq 'diff') {

    die "--stage_id required for stage diff\n" if !$stage_id;

    # this stage uses 'difftool'
    my $difftool = can_run('difftool') or die "Can't find difftool";

    # Get list of component imfiles
    my $skyfiles;                  # Array reference of component files
    my $command = "difftool -pendingcleanupskyfile -diff_id $stage_id"; # Command to run
    $command .= ' -all' if $check_all;
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $very_verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform difftool: $error_code", "diff", $stage_id, $error_code);
    }

    if (@$stdout_buf == 0) {
        # No skycells were found for some reason. 
        # it could be that there are no warpSkyfiles at all if say if a run was changed from drop to goto_cleaned
        # or of a run was cleaned, set to update, and then back to goto_cleaned before any images were successfully
        my $command = "$difftool -updaterun -diff_id $stage_id -set_state $done_state";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform difftool: $error_code", "diff", $stage_id, $error_code);
        }

        exit(0);
    }

    $skyfiles = $mdcParser->parse_list(join "", @{ $stdout_buf }) or
        &my_die("Unable to parse metadata config doc", "diff", $stage_id, $PS_EXIT_PROG_ERROR);

    my $num_errors = 0;
    my $num_updated = 0;
    my @files = ();
    foreach my $skyfile (@{ $skyfiles }) {
        my $path_base = $skyfile->{path_base};
        my $skycell_id = $skyfile->{skycell_id};
        my $data_state = $skyfile->{data_state};

        my $status = 1;
        if ((!exists($skyfile->{path_base}))||
            (!defined($path_base))) {
            $status = 0;
        }
        if ($status) {

            if ($mode eq "goto_cleaned") {
                my $config_file = $ipprc->filename("PPSUB.CONFIG", $path_base, $skycell_id);

            if (0) {
                unless ($ipprc->file_exists($config_file)) {
                    if (file_gone($config_file)) {
                        print STDERR "forcing cleanup for diffRun $stage_id $skycell_id" .
                            " because config file ($config_file) is gone\n";
                    } else {
                        print STDERR "skipping cleanup for diffRun $stage_id $skycell_id" .
                            " because config file ($config_file) is missing\n";
                        $status = 0;
                    }
                }
            }
                unless ($ipprc->file_exists($config_file)) {
                    my $fault = $skyfile->{fault};
                    my $quality = $skyfile->{quality};
                    if (file_gone($config_file) or !storage_object_exists($config_file)) {
                        print STDERR "forcing cleanup diff $stage_id $skycell_id fault: $fault quality: $quality"
                            . " because config file ($config_file) is gone\n";
                    } elsif ($fault == 0 and $quality == 0) {
                            print STDERR "skipping cleaning up diff $stage_id $skycell_id fault: $fault quality: $quality"
                                . " because config file ($config_file) is missing\n";
                            $status = 0;
                    } else {
                            # config file is missing but this is a bad diff anyways so clean it
                            print STDERR "cleaning up diff $stage_id $skycell_id fault: $fault quality: $quality"
                                . " even though config file ($config_file) is missing\n";
                    }
                }
            }
            elsif ($mode eq "goto_scrubbed") {
                my $config_file = $ipprc->filename("PPSUB.CONFIG", $path_base, $skycell_id);

                if ($ipprc->file_exists($config_file)) {
                    print STDERR "skipping scrubbed for diffRun $stage_id $skycell_id" .
                        " because config file ($config_file) is present\n";
                    $status = 0;
                }
            }
        }
        if ($status) {
            my @files = ();
            # delete the temporary image datafiles
            addFilename(\@files, "PPSUB.OUTPUT", $path_base, $skycell_id, 1);
            addFilename(\@files, "PPSUB.OUTPUT.MASK", $path_base, $skycell_id, 1);
            addFilename(\@files, "PPSUB.OUTPUT.VARIANCE", $path_base, $skycell_id, 1);

            addFilename(\@files, "PPSUB.INVERSE", $path_base, $skycell_id, 1);
            addFilename(\@files, "PPSUB.INVERSE.MASK", $path_base, $skycell_id, 1);
            addFilename(\@files, "PPSUB.INVERSE.VARIANCE", $path_base, $skycell_id, 1);

            addFilename(\@files, "PPSUB.INPUT.CONV", $path_base, $skycell_id);
            addFilename(\@files, "PPSUB.INPUT.CONV.MASK", $path_base, $skycell_id);
            addFilename(\@files, "PPSUB.INPUT.CONV.VARIANCE", $path_base, $skycell_id);

            addFilename(\@files, "PPSUB.REF.CONV", $path_base, $skycell_id);
            addFilename(\@files, "PPSUB.REF.CONV.MASK", $path_base, $skycell_id);
            addFilename(\@files, "PPSUB.REF.CONV.VARIANCE", $path_base, $skycell_id);

	    addFilename(\@files, "PPSUB.OUTPUT.JPEG1", $path_base, $skycell_id);

            if ($mode eq "goto_purged") {
                # additional files to remove for 'purge' mode
                addFilename(\@files, "PPSUB.OUTPUT.KERNELS", $path_base, $skycell_id);
                addFilename(\@files, "PPSUB.OUTPUT.JPEG2", $path_base, $skycell_id);
                # Commented out to match warp files.
                #addFilename(\@files, "PPSUB.CONFIG", $path_base, $skycell_id);
                addFilename(\@files, "PPSUB.OUTPUT.SOURCES", $path_base, $skycell_id);
                addFilename(\@files, "PPSUB.INVERSE.SOURCES", $path_base, $skycell_id);

            }
            $status = &delete_files(\@files);
        }

        bzip2_file("LOG.EXP", $path_base, $skycell_id);
        bzip2_file("LOG.EXP.UPDATE", $path_base, $skycell_id);

        if ($status) {
            my $command = "$difftool -diff_id $stage_id -skycell_id $skycell_id";
            my $update_skyfile = 1;

            if ($mode eq "goto_purged") {
                $command .= " -topurgedskyfile";
                if ($data_state eq 'purged') {
                    $update_skyfile = 0;
                }
            }
            elsif ($mode eq "goto_cleaned") {
                $command .= " -tocleanedskyfile";
                if ($data_state eq 'cleaned') {
                    $update_skyfile = 0;
                }
            }
            elsif ($mode eq "goto_scrubbed") {
                $command .= " -toscrubbedskyfile";
                if ($data_state eq 'scrubbed') {
                    $update_skyfile = 0;
                }
            }

            $command .= " -dbname $dbname" if defined $dbname;

            if ($update_skyfile) {
                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform difftool: $error_code", "diff", $stage_id, $error_code);
                }
                set_destreak_goto_cleaned();
                $num_updated++;
            }


        } else {
            $num_errors++;
            my $command = "$difftool -updatediffskyfile -diff_id $stage_id -skycell_id $skycell_id -set_state $error_state";
            $command .= " -dbname $dbname" if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform difftool: $error_code", "diff", $stage_id, $error_code);
            }

            $command = "$difftool -updaterun -diff_id $stage_id -set_state $error_state";

            $command .= " -dbname $dbname" if defined $dbname;

            ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform difftool: $error_code", "diff", $stage_id, $error_code);
            }
        }
    }
    if ($num_updated eq 0 and $num_errors eq 0) {
        # no skycells were updated by this procedssing so set state to $done_state
        my $command = "$difftool -diff_id $stage_id -updaterun -set_state $done_state";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform difftool: $error_code", "diff", $stage_id, $error_code);
        }
    }
    print "Cleanup completed for diff_id $stage_id. num_updated: $num_updated";
    print " num_errors: $num_errors" if $num_errors;
    print "\n";
    exit 0;
}
if ($stage eq 'fake') {
    print STDERR "This does not seem to work at present, as no files exist. Terminating quietly.\n";
    exit(0);
    die "--stage_id required for stage fake\n" if !$stage_id;
    ### select the imfiles for this entry

    # this stage uses 'chiptool'
    my $faketool = can_run('faketool') or die "Can't find faketool";

    # Get list of component imfiles
    # XXX may need a different my_die for each stage
    my $imfiles;                      # Array of component files
    my $command = "$faketool -pendingcleanupimfile -fake_id $stage_id"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform faketool: $error_code", "fake", $stage_id, $error_code);
    }

    # if there are no fakeProcessedImfiles (@$stdout_buf == 0), the reset the state to 'new'
    if (@$stdout_buf == 0)  {
        my $command = "$faketool -fake_id $stage_id -updaterun -set_state $error_state";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform faketool: $error_code", "fake", $stage_id, $error_code);
        }
        exit 0;
    }

    # extract the metadata for the files into a hash list
    $imfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", "fake", $stage_id, $PS_EXIT_PROG_ERROR);

    # loop over all of the imfiles, determine the path_base and class_id for each
    foreach my $imfile (@$imfiles) {
        my $class_id = $imfile->{class_id};
        my $path_base = $imfile->{path_base};
        my $status = 1;

        # don't clean up unless the data needed to update is available
        # modes goto_purged and goto_scrubbed will remove files even if the config is non-existent
        # goto_scrubbed now requires the config file to not exist.
        if ($mode eq "goto_cleaned") {
            my $config_file = $ipprc->filename("PPSIM.CONFIG", $path_base, $class_id);

            unless ($ipprc->file_exists($config_file)) {
                print STDERR "skipping cleanup for fakeRun $stage_id $class_id "
                    . " because config file is missing\n";
                $status = 0;
            }
        }
        elsif ($mode eq "goto_scrubbed") {
            my $config_file = $ipprc->filename("PPSIM.CONFIG", $path_base, $class_id);

            if ($ipprc->file_exists($config_file)) {
                print STDERR "skipping scrubbed for fakeRun $stage_id $class_id "
                    . " because config file is present\n";
                $status = 0;
            }
        }

        if ($status) {
            # array of actual filenames to delete
            my @files = ();

            # delete the temporary image datafiles
            addFilename (\@files, "PPSIM.OUTPUT.MEF", $path_base, $class_id);
            addFilename (\@files, "PPSIM.OUTPUT.SPL", $path_base, $class_id);
            addFilename (\@files, "PPSIM.FAKE.CHIP", $path_base, $class_id);
            addFilename (\@files, "PPSIM.FORCE.CHIP", $path_base, $class_id);
            if ($mode eq "goto_purged") {
                # additional files to remove for 'purge' mode
                addFilename (\@files, "PPSIM.SOURCES", $path_base, $class_id);
                addFilename (\@files, "PPSIM.FAKE.SOURCES", $path_base, $class_id);
                addFilename (\@files, "PPSIM.FORCE.SOURCES", $path_base, $class_id);
            }

            # actual command to delete the files
            $status = &delete_files (\@files);
        }

        if ($status)  {
            my $command = "$faketool -fake_id $stage_id -class_id $class_id";
            if ($mode eq "goto_purged") {
                $command .= " -topurgedimfile";
            }
            elsif ($mode eq "goto_cleaned") {
                $command .= " -tocleanedimfile";
            }
            elsif ($mode eq "goto_scrubbed") {
                $command .= " -toscrubbedimfile";
            }

            $command .= " -dbname $dbname" if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform faketool: $error_code", "fake", $stage_id, $error_code);
            }
        } else {

            # if an error happens for one chip, the chipRun will stay in goto_*, but the chips will go to error_* (matching the goto_*)
            my $command = "$faketool -updateprocessedimfile -fake_id $stage_id -class_id $class_id -set_state $error_state";
            $command .= " -dbname $dbname" if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform faketool: $error_code", "fake", $stage_id, $error_code);
            }
        }
    }
    exit 0;

}
# Detrend stages
if ($stage eq "detrend.processed") {

    die "--stage_id required for stage detrend.process.imfile\n" if !$stage_id;
    ### select the imfiles for this entry

    # Neither det_id nor exp_id uniquely determine a det exposure, so we pack them.
    my ($det_id,$exp_id) = split /\./, $stage_id;       #/ trailing slash for emacs;

    # this stage uses 'dettool'
    my $dettool = can_run('dettool') or die "Can't find chiptool";

    # Get list of component imfiles
    # XXX may need a different my_die for each stage
    my $imfiles;                      # Array of component files
    my $metadata;
    my $command = "$dettool -pendingcleanup_processedimfile -det_id $det_id -exp_id $exp_id"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform dettool: $error_code", "detrend.processed", $stage_id, $error_code);
    }

    # if there are no detProcessedImfiles (@$stdout_buf == 0), the reset the state to 'new'
    if (@$stdout_buf != 0)  {
#       exit 0; # Silently exit if there's nothing to do.  I don't know how we'd ever get here, but let's be safe.


        # extract the metadata for the files into a hash list
        $imfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", "$stage", $stage_id, $PS_EXIT_PROG_ERROR);
    }
    # loop over all of the imfiles, determine the path_base and class_id for each
    foreach my $imfile (@$imfiles) {
        my $iexp_id   = $imfile->{exp_id};
        my $class_id = $imfile->{class_id};
        my $path_base = $imfile->{path_base};
        my $status = 1;

        unless (defined($path_base)) {
            print STDERR "PATH BASE: >>$path_base<< didn't get defined for $iexp_id $exp_id $det_id $class_id\n";
            $status = 0;
        }
        unless (defined($class_id)) {
            print STDERR "CLASS_ID: >>$class_id<< didn't get defined for $iexp_id $exp_id $det_id $path_base\n";
            $status = 0;
        }
        # Detrends cannot be updated, so goto_cleaned and goto_scrubbed are treated as equivalent,
        # and so there is no check for config files.
        if ($status) {
            # array of actual filenames to delete
            my @files = ();
            # delete the temporary image datafiles
            addFilename (\@files, "PPIMAGE.OUTPUT", $path_base, $class_id);
            addFilename (\@files, "PPIMAGE.OUTPUT.MASK", $path_base, $class_id);
            addFilename (\@files, "PPIMAGE.OUTPUT.VARIANCE", $path_base, $class_id);
            if ($mode eq "goto_purged") {
                # additional files to remove for 'purge' mode
                addFilename (\@files, "PPIMAGE.BIN1", $path_base, $class_id);
                addFilename (\@files, "PPIMAGE.BIN2", $path_base, $class_id);
                addFilename (\@files, "PPIMAGE.STATS", $path_base, $class_id);
            }
            # actual command to delete the files
            $status = &delete_files (\@files);
        }

        if ($status)  {
            my $command = "$dettool -det_id $det_id -exp_id $iexp_id -class_id $class_id -updateprocessedimfile";
            if ($mode eq "goto_purged") {
                $command .= " -data_state purged";
            }
            elsif ($mode eq "goto_cleaned") {
                $command .= " -data_state cleaned";
            }
            elsif ($mode eq "goto_scrubbed") {
                $command .= " -data_state scrubbed";
            }
            $command .= " -dbname $dbname" if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dettool: $error_code", "$stage", $stage_id, $error_code);
            }

        } else {
            # if an error happens for one chip, the chipRun will stay in goto_*, but the chips will go to error_*
            my $command = "$dettool -det_id $det_id -exp_id $iexp_id -class_id $class_id -updateprocessedimfile ";
            $command .= " -data_state $error_state";
            $command .= " -dbname $dbname" if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dettool: $error_code", "$stage", $stage_id, $error_code);
            }
        }
    }

    # Flag the detProcessedExp as clean now as well, if it is marked to be cleaned (this is clunky, but works for now).

    $command = "$dettool -pendingcleanup_processedexp -det_id $det_id -exp_id $exp_id";
    $command .= " -dbname $dbname" if defined $dbname;
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform dettool: $error_code", "$stage (detProcessedExp)", $stage_id, $error_code);
    }
    if (@$stdout_buf != 0) {
        my $exps = $mdcParser->parse_list(join "", @$stdout_buf) or
            &my_die("Unable to parse metadata config doc", "$stage (detProcessedExp)", $stage_id, $PS_EXIT_PROG_ERROR);

        foreach my $exp (@$exps) {
            my $exp_id = $exp->{exp_id};
            my $command = "$dettool -updateprocessedexp -det_id $det_id -exp_id $exp_id ";
            if ($mode eq "goto_cleaned") {
                $command .= " -data_state cleaned ";
            }
            if ($mode eq "goto_scrubbed") {
                $command .= " -data_state scrubbed ";
            }
            if ($mode eq "goto_purged") {
                $command .= " -data_state purged ";
            }
            $command .= " -dbname $dbname" if defined $dbname;
#           print "$command\n";
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dettool: $error_code", "$stage (detProcessedExp)", $stage_id, $error_code);
            }
        }
    }

    exit 0;
}
if ($stage eq "detrend.resid") {

    die "--stage_id required for stage $stage\n" if !$stage_id;
    ### select the imfiles for this entry

    # Neither det_id nor exp_id uniquely determine the det exposure, so we pack them
    my ($det_id,$exp_id) = split /\./, $stage_id;          #/ trailing slash for emacs;

    # this stage uses 'dettool'
    my $dettool = can_run('dettool') or die "Can't find dettool";

    # Get list of component imfiles
    # XXX may need a different my_die for each stage
    my $imfiles;                      # Array of component files
    my $metadata;
    my $command = "$dettool -pendingcleanup_residimfile -det_id $det_id -exp_id $exp_id"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform dettool: $error_code", "detrend.process.imfile", $stage_id, $error_code);
    }

    # if there are no detResidImfiles (@$stdout_buf == 0), then silently exit.
    if (@$stdout_buf != 0) {
        # extract the metadata for the files into a hash list
        $imfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
            &my_die("Unable to parse metadata config doc", "$stage", $stage_id, $PS_EXIT_PROG_ERROR);
    }
    # loop over all of the imfiles, determine the path_base and class_id for each
    foreach my $imfile (@$imfiles) {
        my $iexp_id = $imfile->{exp_id};
        my $class_id = $imfile->{class_id};
        my $path_base = $imfile->{path_base};
        my $iteration = $imfile->{iteration};

        my $status = 1;

        # Detrends cannot be updated, so goto_cleaned and goto_scrubbed are treated as equivalent,
        # and so there is no check for config files.
        unless (defined($path_base)) {
            print STDERR "PATH BASE: >>$path_base<< didn't get defined for $iexp_id $exp_id $det_id $class_id\n";
            $status = 0;
        }
        unless (defined($class_id)) {
            print STDERR "CLASS_ID: >>$class_id<< didn't get defined for $iexp_id $exp_id $det_id $path_base\n";
            $status = 0;
        }
        if ($status) {
            # array of actual filenames to delete
            my @files = ();

            # delete the temporary image datafiles
            addFilename (\@files, "PPIMAGE.OUTPUT", $path_base, $class_id);
            if ($mode eq "goto_purged") {
                # additional files to remove for 'purge' mode
                addFilename (\@files, "PPIMAGE.BIN1", $path_base, $class_id);
                addFilename (\@files, "PPIMAGE.BIN2", $path_base, $class_id);
                addFilename (\@files, "PPIMAGE.STATS", $path_base, $class_id);
            }
#           foreach my $f (@files) {
#               print "RESID: $f\n";
#           }
            # actual command to delete the files
            $status = &delete_files (\@files);
        }

        if ($status)  {
            my $command = "$dettool -det_id $det_id -exp_id $iexp_id -iteration $iteration -class_id $class_id -updateresidimfile ";
            if ($mode eq "goto_purged") {
                $command .= " -data_state purged";
            }
            elsif ($mode eq "goto_cleaned") {
                $command .= " -data_state cleaned";
            }
            elsif ($mode eq "goto_scrubbed") {
                $command .= " -data_state scrubbed";
            }

            $command .= " -dbname $dbname" if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dettool: $error_code", "$stage", $stage_id, $error_code);
            }
        } else {
            my $command = "$dettool -det_id $det_id -exp_id $iexp_id -iteration $iteration -class_id $class_id -updateresidimfile ";
            $command .= " -data_state $error_state ";
            $command .= " -dbname $dbname" if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dettool: $error_code", "$stage", $stage_id, $error_code);
            }
        }
    }
    # Flag the detResidExp as clean now as well, if it is marked tobe cleaned (this is still clunky).

    $command = "$dettool -pendingcleanup_residexp -det_id $det_id -exp_id $exp_id";
    $command .= " -dbname $dbname" if defined $dbname;
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform dettool: $error_code", "$stage (detResidExp)", $stage_id, $error_code);
    }
    if (@$stdout_buf != 0) {
        my $exps = $mdcParser->parse_list(join "", @$stdout_buf) or
            &my_die("Unable to parse metadata config doc", "$stage (detResidExp)", $stage_id, $PS_EXIT_PROG_ERROR);

        foreach my $exp (@$exps) {
            my $iteration = $exp->{iteration};
            my $command = "$dettool -updateresidexp -det_id $det_id -exp_id $exp_id ";
            if ($mode eq "goto_cleaned") {
                $command .= " -data_state cleaned";
            }
            if ($mode eq "goto_scrubbed") {
                $command .= " -data_state scrubbed";
            }
            if ($mode eq "goto_purged") {
                $command .= " -data_state purged";
            }
            $command .= " -dbname $dbname" if defined $dbname;
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dettool: $error_code", "$stage (detResidExp)", $stage_id, $error_code);
            }
        }
    }

    exit 0;
}

if ($stage eq "detrend.stack.imfile") {

    die "--stage_id required for stage $stage\n" if !$stage_id;

    # this stage uses 'dettool'
    my $dettool = can_run('dettool') or die "Can't find dettool";

    # Get list of component imfiles
    my $stacks;                  # Array reference of component files
    my $command = "$dettool -pendingcleanup_stacked -det_id $stage_id"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform dettool: $error_code", "$stage", $stage_id, $error_code);
    }
    $stacks = $mdcParser->parse_list(join "", @{ $stdout_buf }) or
        &my_die("Unable to parse metadata config doc", "$stage", $stage_id, $PS_EXIT_PROG_ERROR);

    my @files = ();
    foreach my $stack (@{ $stacks }) {
        # detStackedImfile does not have a path_base column.  This is inconvenient, as it means we need to calculate it.
        my $path_base = $stack->{uri};
        my $iteration = $stack->{iteration};
        my $class_id  = $stack->{class_id};

        $path_base =~ s/\.fits$//; # That should do it?

        my $status = 1;

        if ($status) {
            my @files = ();
            # delete the temporary image datafiles
            # There's no convenient way to get the detrend type, so I'm queueing all of them for deletion.
            # I understand that they all point to the same filename right now, but that may not be true in
            # the future.
            addFilename(\@files, "PPMERGE.OUTPUT.MASK", $path_base, $stage_id);
            addFilename(\@files, "PPMERGE.OUTPUT.BIAS", $path_base, $stage_id);
            addFilename(\@files, "PPMERGE.OUTPUT.DARK", $path_base, $stage_id);
            addFilename(\@files, "PPMERGE.OUTPUT.SHUTTER", $path_base, $stage_id);
            addFilename(\@files, "PPMERGE.OUTPUT.FLAT", $path_base, $stage_id);
            addFilename(\@files, "PPMERGE.OUTPUT.FRINGE", $path_base, $stage_id);


            addFilename(\@files, "PPMERGE.OUTPUT.SIGMA", $path_base, $stage_id);
            addFilename(\@files, "PPMERGE.OUTPUT.COUNT", $path_base, $stage_id);

            if ($mode eq "goto_purged") {
                # additional files to remove for 'purge' mode
#               addFilename(\@files, "PPMERGE.OUTPUT", $path_base, $stage_id);
            }

            $status = &delete_files(\@files);
        }

        if ($status) {
            my $command = "$dettool -det_id $stage_id -iteration $iteration -class_id $class_id";
            if ($mode eq "goto_purged") {
                $command .= " -updatestacked -data_state purged";
            }
            elsif ($mode eq "goto_cleaned") {
                $command .= " -updatestacked -data_state cleaned";
            }
            elsif ($mode eq "goto_scrubbed") {
                $command .= " -updatestacked -data_state scrubbed";
            }
            $command .= " -dbname $dbname" if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dettool: $error_code", "$stage", $stage_id, $error_code);
            }
        } else {
            my $command = "$dettool -updatestacked  -det_id $stage_id -iteration $iteration -class_id $class_id -data_state $error_state";
            $command .= " -dbname $dbname" if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dettool: $error_code", "$stage", $stage_id, $error_code);
            }
            exit $PS_EXIT_UNKNOWN_ERROR;
        }
    }
    # Check to see if we can mark the whole detRunSummary object as cleaned.

    $command = "$dettool -pendingcleanup_detrunsummary -det_id $stage_id";
    $command .= " -dbname $dbname" if defined $dbname;
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform dettool: $error_code", "$stage (detRunSummary)", $stage_id, $error_code);
    }
    if (@$stdout_buf != 0) {
        my $exps = $mdcParser->parse_list(join "", @$stdout_buf) or
            &my_die("Unable to parse metadata config doc", "$stage (detRunSummary)", $stage_id, $PS_EXIT_PROG_ERROR);

        foreach my $exp (@$exps) {
            my $iteration = $exp->{iteration};
            my $command;
            if ($mode eq "goto_cleaned") {
                $command = "$dettool -updatedetrunsummary -det_id $stage_id -iteration $iteration -data_state cleaned";
            }
            if ($mode eq "goto_scrubbed") {
                $command = "$dettool -updatedetrunsummary -det_id $stage_id -iteration $iteration -data_state scrubbed";
            }
            if ($mode eq "goto_purged") {
                $command = "$dettool -updatedetrunsummary -det_id $stage_id -iteration $iteration -data_state purged";
            }
            $command .= " -dbname $dbname" if defined $dbname;
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dettool: $error_code", "$stage (detRunSummary)", $stage_id, $error_code);
            }
        }
    }
    exit 0;
}
if ($stage eq "detrend.normstat.imfile") {
    print STDERR "I'm not convinced there's anything to clean up from stage $stage\n";
    die "--stage_id required for stage $stage\n" if !$stage_id;
    # this stage uses 'camtool'
    my $dettool = can_run('dettool') or die "Can't find dettool";

    my $command = "$dettool -pendingcleanup_normalizedstat -det_id $stage_id"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform dettool: $error_code", "$stage", $stage_id, $error_code);
    }
    my $exps = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", "$stage", $stage_id, $PS_EXIT_PROG_ERROR);

    foreach my $exp (@$exps) {
#       my $path_base = $exp->{path_base};
        my $iteration = $exp->{iteration};
        my $class_id  = $exp->{class_id};

        my $status = 1;
        if ($status)  {
            my $command = "$dettool -updatenormalizedstat -det_id $stage_id -iteration $iteration -class_id $class_id";
            if ($mode eq "goto_cleaned") {
                $command .= " -data_state cleaned";
            }
            if ($mode eq "goto_scrubbed") {
                $command .= " -data_state scrubbed";
            }
            if ($mode eq "goto_purged") {
                $command .= " -data_state purged";
            }
            $command .= " -dbname $dbname" if defined $dbname;
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dettool: $error_code", "$stage", $stage_id, $error_code);
            }
        } else {
            my $command = "$dettool -updatenormalizedstat -det_id $stage_id -iteration $iteration -class_id $class_id -data_state $error_state";
            $command .= " -dbname $dbname" if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dettool: $error_code", "$stage", $stage_id, $error_code);
            }
            exit $PS_EXIT_UNKNOWN_ERROR;
        }
    }
    # Check to see if we can mark the whole detRunSummary object as cleaned.

    $command = "$dettool -pendingcleanup_detrunsummary -det_id $stage_id";
    $command .= " -dbname $dbname" if defined $dbname;
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform dettool: $error_code", "$stage (detRunSummary)", $stage_id, $error_code);
    }
    if (@$stdout_buf != 0) {
        $exps = $mdcParser->parse_list(join "", @$stdout_buf) or
            &my_die("Unable to parse metadata config doc", "$stage (detRunSummary)", $stage_id, $PS_EXIT_PROG_ERROR);

        foreach my $exp (@$exps) {
            my $iteration = $exp->{iteration};
            my $command;
            if ($mode eq "goto_cleaned") {
                $command = "$dettool -updatedetrunsummary -det_id $stage_id -iteration $iteration -data_state cleaned";
            }
            if ($mode eq "goto_scrubbed") {
                $command = "$dettool -updatedetrunsummary -det_id $stage_id -iteration $iteration -data_state scrubbed";
            }
            if ($mode eq "goto_purged") {
                $command = "$dettool -updatedetrunsummary -det_id $stage_id -iteration $iteration -data_state purged";
            }
            $command .= " -dbname $dbname" if defined $dbname;
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dettool: $error_code", "$stage (detRunSummary)", $stage_id, $error_code);
            }
        }
    }

    exit 0;

}
if ($stage eq "detrend.norm.imfile") {
    die "--stage_id required for stage $stage\n" if !$stage_id;
    # this stage uses 'dettool'
    my $dettool = can_run('dettool') or die "Can't find dettool";

    # Get list of component imfiles
    # XXX may need a different my_die for each stage
    my $exps;                      # Array of component files
    my $command = "$dettool -pendingcleanup_normalizedimfile -det_id $stage_id"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform dettool: $error_code", "$stage", $stage_id, $error_code);
    }
    $exps = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", "$stage", $stage_id, $PS_EXIT_PROG_ERROR);

    foreach my $exp (@$exps) {
        my $path_base = $exp->{path_base};
        my $iteration = $exp->{iteration};
        my $class_id  = $exp->{class_id};

        my $status = 1;
        # don't clean up unless the data needed to update is available
        # goto_scrubbed now requires the config file to not be present
        if ($mode eq "goto_cleaned") {
            my $config_file = $ipprc->filename("PPIMAGE.CONFIG", $path_base);

            unless ($ipprc->file_exists($config_file)) {
                print STDERR "skipping cleanup for $stage $stage_id because config file is missing\n";
                $status = 0;
            }
        }
        elsif ($mode eq "goto_scrubbed") {
            my $config_file = $ipprc->filename("PPIMAGE.CONFIG", $path_base);

            if ($ipprc->file_exists($config_file)) {
                print STDERR "skipping cleanup for $stage $stage_id because config file ($config_file) is present\n";
                $status = 0;
            }
        }
        if ($status) {
            my @files = ();

            if ($mode eq "goto_purged") {
                # additional files to remove for 'purge' mode
                addFilename (\@files, "PPIMAGE.OUTPUT.FPA1", $path_base);
                addFilename (\@files, "PPIMAGE.OUTPUT.FPA2", $path_base);

                addFilename (\@files, "PPIMAGE.OUTPUT", $path_base);
                addFilename (\@files, "PPIMAGE.STATS", $path_base);
            }
            # actual command to delete the files
            $status = &delete_files (\@files);
        }

        if ($status)  {
            my $command = "$dettool -updatenormalizedimfile -det_id $stage_id -iteration $iteration -class_id $class_id";
            if ($mode eq "goto_cleaned") {
                $command .= " -data_state cleaned";
            }
            if ($mode eq "goto_scrubbed") {
                $command .= " -data_state scrubbed";
            }
            if ($mode eq "goto_purged") {
                $command .= " -data_state purged";
            }
            $command .= " -dbname $dbname" if defined $dbname;
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dettool: $error_code", "$stage", $stage_id, $error_code);
            }
        } else {
            my $command = "$dettool -updatenormalizedimfile -det_id $stage_id -iteration $iteration -class_id $class_id -data_state $error_state";
            $command .= " -dbname $dbname" if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dettool: $error_code", "$stage", $stage_id, $error_code);
            }
            exit $PS_EXIT_UNKNOWN_ERROR;
        }
    }
    # Check to see if we can mark the whole detRunSummary object as cleaned.

    $command = "$dettool -pendingcleanup_detrunsummary -det_id $stage_id";
    $command .= " -dbname $dbname" if defined $dbname;
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform dettool: $error_code", "$stage (detRunSummary)", $stage_id, $error_code);
    }
    if (@$stdout_buf != 0) {
        $exps = $mdcParser->parse_list(join "", @$stdout_buf) or
            &my_die("Unable to parse metadata config doc", "$stage (detRunSummary)", $stage_id, $PS_EXIT_PROG_ERROR);

        foreach my $exp (@$exps) {
            my $iteration = $exp->{iteration};
            my $command;
            if ($mode eq "goto_cleaned") {
                $command = "$dettool -updatedetrunsummary -det_id $stage_id -iteration $iteration -data_state cleaned";
            }
            if ($mode eq "goto_scrubbed") {
                $command = "$dettool -updatedetrunsummary -det_id $stage_id -iteration $iteration -data_state scrubbed";
            }
            if ($mode eq "goto_purged") {
                $command = "$dettool -updatedetrunsummary -det_id $stage_id -iteration $iteration -data_state purged";
            }
            $command .= " -dbname $dbname" if defined $dbname;
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dettool: $error_code", "$stage (detRunSummary)", $stage_id, $error_code);
            }
        }
    }

    exit 0;
}
if ($stage eq "detrend.norm.exp") {
    die "--stage_id required for stage $stage\n" if !$stage_id;
    # this stage uses 'dettool'
    my $dettool = can_run('dettool') or die "Can't find dettool";

    # Get list of component imfiles
    # XXX may need a different my_die for each stage
    my $exps;                      # Array of component files
    my $command = "$dettool -pendingcleanup_normalizedexp -det_id $stage_id"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform dettool: $error_code", "$stage", $stage_id, $error_code);
    }

    if (@$stdout_buf == 0) {
        exit 0;
    }
    $exps = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", "$stage", $stage_id, $PS_EXIT_PROG_ERROR);

    foreach my $exp (@$exps) {
        my $exp_id = $exp->{exp_id};
        my $iteration = $exp->{iteration};
        my $path_base = $exp->{path_base};

        my $status = 1;
        # don't clean up unless the data needed to update is available
        # goto_scrubbed now requires the config file to not be present
        if ($mode eq "goto_cleaned") {
            my $config_file = $ipprc->filename("PPIMAGE.CONFIG", $path_base);

            unless ($ipprc->file_exists($config_file)) {
                print STDERR "skipping cleanup for $stage $stage_id because config file is missing\n";
                $status = 0;
            }
        }
        elsif ($mode eq "goto_scrubbed") {
            my $config_file = $ipprc->filename("PPIMAGE.CONFIG", $path_base);

            if ($ipprc->file_exists($config_file)) {
                print STDERR "skipping cleanup for $stage $stage_id because config file ($config_file) is present\n";
                $status = 0;
            }
        }
        if ($status) {
            my @files = ();
            # delete the temporary image datafiles
            if ($mode eq "goto_purged") {
                # additional files to remove for 'purge' mode
                addFilename (\@files, "PPIMAGE.JPEG1", $path_base);
                addFilename (\@files, "PPIMAGE.JPEG2", $path_base);
            }
            # actual command to delete the files
            $status = &delete_files (\@files);
        }

        if ($status)  {
            my $command = "$dettool -updatenormalizedexp -det_id $stage_id -iteration $iteration";
            if ($mode eq "goto_cleaned") {
                $command .= " -data_state cleaned";
            }
            if ($mode eq "goto_scrubbed") {
                $command .= " -data_state scrubbed";
            }
            if ($mode eq "goto_purged") {
                $command .= " -data_state purged";
            }
            $command .= " -dbname $dbname" if defined $dbname;
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                print STDERR " residexp had an issue setting the state:? $success $error_code\n";
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dettool: $error_code", "$stage", $stage_id, $error_code);
            }
        } else {
            my $command = "$dettool -updatenormalizedexp -det_id $stage_id -exp_id $exp_id -iteration $iteration -data_state $error_state";
            $command .= " -dbname $dbname" if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dettool: $error_code", "$stage", $stage_id, $error_code);
            }
            exit $PS_EXIT_UNKNOWN_ERROR;
        }
    }
    # Check to see if we can mark the whole detRunSummary object as cleaned.

    $command = "$dettool -pendingcleanup_detrunsummary -det_id $stage_id";
    $command .= " -dbname $dbname" if defined $dbname;
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform dettool: $error_code", "$stage (detRunSummary)", $stage_id, $error_code);
    }
    if (@$stdout_buf != 0) {
        $exps = $mdcParser->parse_list(join "", @$stdout_buf) or
            &my_die("Unable to parse metadata config doc", "$stage (detRunSummary)", $stage_id, $PS_EXIT_PROG_ERROR);

        foreach my $exp (@$exps) {
            my $iteration = $exp->{iteration};
            my $command;
            if ($mode eq "goto_cleaned") {
                $command = "$dettool -updatedetrunsummary -det_id $stage_id -iteration $iteration -data_state cleaned";
            }
            if ($mode eq "goto_scrubbed") {
                $command = "$dettool -updatedetrunsummary -det_id $stage_id -iteration $iteration -data_state scrubbed";
            }
            if ($mode eq "goto_purged") {
                $command = "$dettool -updatedetrunsummary -det_id $stage_id -iteration $iteration -data_state purged";
            }
            $command .= " -dbname $dbname" if defined $dbname;
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dettool: $error_code", "$stage (detRunSummary)", $stage_id, $error_code);
            }
        }
    }
    exit 0;
}

if ($stage eq "chip_bg") {

    die "--stage_id required for stage chip_bg\n" if !$stage_id;

    &my_die("only mode goto_cleaned is supported for stage chip_bg", "chip_bg", $stage_id, $PS_EXIT_PROG_ERROR)
        if $mode ne 'goto_cleaned';

    ### select the imfiles for this entry

    # this stage uses 'bgtool'
    my $bgtool = can_run('bgtool') or die "Can't find bgtool";

    # Get list of component imfiles
    # XXX may need a different my_die for each stage
    my $imfiles;                      # Array of component files
    my $command = "$bgtool -pendingcleanupchipimfile -chip_bg_id $stage_id"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform bgtool: $error_code", "chip_bg", $stage_id, $error_code);
    }

    # XXX: Is not having any files to process really a bug? NO
    if (0 and @$stdout_buf == 0)  {
        my $command = "$bgtool -chip_bg_id $stage_id -updatechip -set_state $error_state";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform bgtool: $error_code", "chip_bg", $stage_id, $error_code);
        }
        exit 0;
    }

    # extract the metadata for the files into a hash list
    $imfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", "chip_bg", $stage_id, $PS_EXIT_PROG_ERROR);

    # loop over all of the imfiles, determine the path_base and class_id for each
    my $num_errors = 0;
    my $num_updated = 0;
    foreach my $imfile (@$imfiles) {
        my $class_id = $imfile->{class_id};
        my $path_base = $imfile->{path_base};
        my $status = 1;
        $status = 0 unless defined $path_base and $path_base ne "NULL";

        # don't clean up unless the data needed to update is available
        # modes goto_purged and goto_scrubbed will remove files even if the config is non-existent
        # goto_scrubbed now requires the config file to not exist.
        if ($status) {
            if ($mode eq "goto_cleaned") {
                my $config_file = $ipprc->filename("PPBACKGROUND.CONFIG", $path_base, $class_id);

                unless ($ipprc->file_exists($config_file)) {
                    if (file_gone($config_file)) {
                        print STDERR "forcing cleanup for chipRun $stage_id $class_id "
                            . " because config file ($config_file) is gone\n";
                    } else {
                        print STDERR "skipping cleanup for chipRun $stage_id $class_id "
                            . " because config file ($config_file) is missing\n";
                        $status = 0;
                    }
                }
            }
            elsif ($mode eq "goto_scrubbed") {
                my $config_file = $ipprc->filename("PPBACKGROUND.CONFIG", $path_base, $class_id);

                if ($ipprc->file_exists($config_file)) {
                    print STDERR "skipping scrubbed for chipBackgroundRun $stage_id $class_id "
                        . " because config file ($config_file) is present\n";
                    $status = 0;
                }
            }
        }

        if ($status) {
            # array of actual filenames to delete
            my @files = ();

            # delete the image datafiles
            addFilename (\@files, "PPBACKGROUND.OUTPUT", $path_base, $class_id);
            addFilename (\@files, "PPBACKGROUND.OUTPUT.MASK", $path_base, $class_id);
            addFilename (\@files, "PPBACKGROUND.OUTPUT.VARIANCE", $path_base, $class_id);
            if ($mode eq "goto_purged") {
                # additional files to remove for 'purge' mode
                addFilename (\@files, "PPBACKGROUND.STATS", $path_base, $class_id);         #clean?
                addFilename (\@files, "PPBACKGOROUND.CONFIG", $path_base, $class_id);
            }

            # actual command to delete the files
            $status = &delete_files (\@files);
        }

        if ($status)  {
            $num_updated++;
            my $command = "$bgtool -chip_bg_id $stage_id -class_id $class_id";
            if ($mode eq "goto_cleaned") {
                $command .= " -tocleanedchipimfile";
            }
            elsif ($mode eq "goto_purged") {
                $command .= " -topurgedchipimfile";
            }
            elsif ($mode eq "goto_scrubbed") {
                $command .= " -toscrubbedchipimfile";
            }

            $command .= " -dbname $dbname" if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform chiptool: $error_code", "chip", $stage_id, $error_code);
            }

        } else {
            $num_errors++;

            if ($set_error_state_for_file) {
                # if an error happens for one chip, the chipBackgroundRun will stay in goto_*, but the chips will go to error_* (matching the goto_*)
                my $command = "$bgtool -updatechipimfile -chip_bg_id $stage_id -class_id $class_id -set_data_state $error_state";
                $command .= " -dbname $dbname" if defined $dbname;

                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                        run(command => $command, verbose => $verbose);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform bgtool: $error_code", "chip_bg", $stage_id, $error_code);
                }
            }
            if ($set_error_state_for_run) {

                # We want to flag the run as well, to avoid attempting to reprocess the same data over and over again.
                $command = "$bgtool -chip_bg_id $stage_id -updatechip -set_state $error_state";
                $command .= " -dbname $dbname" if defined $dbname;

                ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform chiptool: $error_code", "chip", $stage_id, $error_code);
                }

            }
        }
    }
    if (!$set_error_state_for_run or ($num_errors eq 0)) {
        # Set run to done state
        my $command = "$bgtool -chip_bg_id $stage_id -updatechip -set_state $done_state";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform warptool: $error_code", "warp", $stage_id, $error_code);
        }
    }
    print "Cleanup completed for chip_bg_id $stage_id. num_updated: $num_updated";
    print " num_errors:  $num_errors" if $num_errors;
    print "\n";
    exit 0;
}
if ($stage eq "warp_bg") {

    die "--stage_id required for stage warp_bg\n" if !$stage_id;

    &my_die("only mode goto_cleaned is supported for stage warp_bg", "warp_bg", $stage_id, $PS_EXIT_PROG_ERROR)
        if $mode ne 'goto_cleaned';

    ### select the imfiles for this entry

    # this stage uses 'bgtool'
    my $bgtool = can_run('bgtool') or die "Can't find bgtool";

    # Get list of component imfiles
    # XXX may need a different my_die for each stage
    my $imfiles;                      # Array of component files
    my $command = "$bgtool -pendingcleanupwarpskyfile -warp_bg_id $stage_id"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform bgtool: $error_code", "warp_bg", $stage_id, $error_code);
    }

    # XXX: Is not having any files to process really a bug? NO
    if (0 and @$stdout_buf == 0)  {
        my $command = "$bgtool -warp_bg_id $stage_id -updatewarp -set_state $error_state";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform bgtool: $error_code", "warp_bg", $stage_id, $error_code);
        }
        exit 0;
    }

    # extract the metadata for the files into a hash list
    $imfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", "warp_bg", $stage_id, $PS_EXIT_PROG_ERROR);

    # loop over all of the imfiles, determine the path_base and skycell_id for each
    my $num_errors = 0;
    my $num_updated = 0;
    foreach my $imfile (@$imfiles) {
        my $skycell_id = $imfile->{skycell_id};
        my $path_base = $imfile->{path_base};
        my $status = 1;
        $status = 0 unless defined $path_base and $path_base ne "NULL";

        # don't clean up unless the data needed to update is available
        # modes goto_purged and goto_scrubbed will remove files even if the config is non-existent
        # goto_scrubbed now requires the config file to not exist.
        if ($status) {
            if ($mode eq "goto_cleaned") {
                my $config_file = $ipprc->filename("PSWARP.CONFIG", $path_base, $skycell_id);

                unless ($ipprc->file_exists($config_file)) {
                    if (file_gone($config_file)) {
                        print STDERR "forcing cleanup for warpBackgroundSkyfile $stage_id $skycell_id "
                            . " because config file ($config_file) is gone\n";
                    } elsif ($imfile->{fault}) {
                        print STDERR "forcing cleanup for faulted warpBackgroundSkyfile\n";
                    } else {
                        print STDERR "skipping cleanup for warpBackgroundSkyfile $stage_id $skycell_id "
                            . " because config file ($config_file) is missing\n";
                        $status = 0;
                    }
                }
            }
        }

        if ($status) {
            # array of actual filenames to delete
            my @files = ();

            # delete the image datafiles
            addFilename (\@files, "PSWARP.OUTPUT", $path_base, $skycell_id);
            addFilename (\@files, "PSWARP.OUTPUT.MASK", $path_base, $skycell_id);
            addFilename (\@files, "PSWARP.OUTPUT.VARIANCE", $path_base, $skycell_id);
            if ($mode eq "goto_purged") {
                # additional files to remove for 'purge' mode
                addFilename (\@files, "PSWARP.STATS", $path_base, $skycell_id);         #clean?
                addFilename (\@files, "PSWARP.CONFIG", $path_base, $skycell_id);
            }

            # actual command to delete the files
            $status = &delete_files (\@files);
        }

        if ($status)  {
            $num_updated++;
            my $command = "$bgtool -warp_bg_id $stage_id -skycell_id $skycell_id";
            # recall that only goto_cleaned is supported (currently)
            if ($mode eq "goto_cleaned") {
                $command .= " -tocleanedwarpskyfile";
            }
            elsif ($mode eq "goto_purged") {
                $command .= " -topurgedwarpskyfile";
            }
            elsif ($mode eq "goto_scrubbed") {
                $command .= " -toscrubbedwarpskyfile";
            }

            $command .= " -dbname $dbname" if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform bgtool: $error_code", "warp", $stage_id, $error_code);
            }

        } else {

          $num_errors++;

          # NOTE: bgtool does not support -updatewarpskyfile
            if (0) {
                # if an error happens for one skycell, the warpBackgroundRun will stay in goto_*, but the skycells will go to error_* (matching the goto_*)
                my $command = "$bgtool -updatewarpskyfile -warp_bg_id $stage_id -skycell_id $skycell_id -set_state $error_state";
                $command .= " -dbname $dbname" if defined $dbname;

                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                        run(command => $command, verbose => $verbose);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform bgtool: $error_code", "warp_bg", $stage_id, $error_code);
                }
            }

            if ($set_error_state_for_run) {

                # We want to flag the run as well, to avoid attempting to reprocess the same data over and over again.
                $command = "$bgtool -warp_bg_id $stage_id -updatewarp -set_state $error_state";
                $command .= " -dbname $dbname" if defined $dbname;

                ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform bgtool: $error_code", "warp", $stage_id, $error_code);
                }
            }
        }
    }
    if (($num_errors == 0) || !$set_error_state_for_run) {
        # Set run to done state
        my $command = "$bgtool -warp_bg_id $stage_id -updatewarp -set_state $done_state";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform warptool: $error_code", "warp", $stage_id, $error_code);
        }
    }
    print "Cleanup completed for warp_bg_id $stage_id. num_updated: $num_updated";
    print " num_errors:  $num_errors" if $num_errors;
    print "\n";
    exit 0;
}

die "ipp_cleanup.pl -stage $stage not yet implemented\n";

sub delete_files
{
    my $files = shift; # reference to a list of files to unlink

    foreach my $file (@$files) {
        print STDERR "unlinking $stage $stage_id $file\n" if $very_verbose;

        my $error_code = $ipprc->kill_file($file);

        &my_die("failed to kill $file", $stage, $stage_id, $PS_EXIT_CONFIG_ERROR) if $error_code;
    }

    return 1;
}

my $whichnode;
sub file_gone
{
    # if $check_for_gone check whether the only instance of file is on a lost volume
    # XXX: we don't have a proper interface for this. 
    # For now try to use Bill's hack the script 'whichnode'
    return 0 if !$check_for_gone;

    my $file = shift;

    if (file_scheme($file) ne 'neb') {
        return 0;
    }

    if (!$whichnode) {
        $whichnode = can_run('whichnode') or
            &my_die("Can't find whichnode", "chip", $stage_id, $PS_EXIT_CONFIG_ERROR);
    }

    my $command = "$whichnode $file";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform whichnode: $error_code", "chip", $stage_id, $error_code);
    }

    my @lines = split "\n", (join "", @$stdout_buf);
    my $numGone = 0;
    my $numNotGone = 0;
    foreach my $line (@lines) {
        chomp $line;

        # output lines are either
        #   "volume available"
        # or 
        #   "volume not available"

        my ($volume, $answer, undef) = split " ", $line;
        # our hack is if the volume has an X in the name it's gone
        if ($volume =~ /X/) {
            print STDERR "$file is on $volume which is gone\n";
            $numGone++;
        } elsif ($answer eq 'available') {
            $numNotGone++;
        } elsif ($answer eq 'not') {
            print STDERR "$file is on $volume which is not available\n";
            $numNotGone++;
        } else {
            print STDERR "unexpected output from whichnode: $line\n";
        }
    }
    # if there are any instances that are not on a gone node return 0
    if ($numNotGone == 0 and $numGone > 0) {
        return 1;
    } else {
        return 0;
    }
}

sub addFilename
{
    my $files      = shift; # reference to a list of files to unlink
    my $filerule   = shift; # filerule to add
    my $path_base  = shift; # base filename
    my $class_id   = shift; # class_id, if needed
    my $recovery   = shift; # is there is a recovery file to clean?

    my $file = $ipprc->filename($filerule, $path_base, $class_id);

    push @$files, $file;
    
    if ($recovery) {
        # need to clean up the recovery file (the pixels censored by streaksremove)
        $file = $ipprc->recovery_filename($file);
        push @$files, $file;
    }
    return 1;
}

sub set_destreak_goto_cleaned {

    return if $ds_done;

    # Tell magicdstool that we've cleaned up this data, so it needs to do the same if it needs to do the same.
    my $magicdstool = can_run('magicdstool') or die "Can't find magicdstool";
    my $command = "$magicdstool -stage $stage -stage_id $stage_id -updaterun -set_state goto_cleaned -set_label goto_cleaned";
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform magicdstool: $error_code", "$stage", $stage_id, $error_code);
    }
    $ds_done = 1;
}

sub bzip2_file {
    my $filerule = shift;
    my $path_base = shift;
    my $component = shift;

    my $filename = $ipprc->filename($filerule, $path_base, $component);
    if (!$ipprc->file_exists($filename)) {
        return 1;
    }
    if (my $resolved = $ipprc->file_resolve($filename)) {
        my $bzip2_filename = $filename . '.bz2';
	if (file_scheme($bzip2_filename) eq 'neb') {
	    # XXX: GRRRR $ipprc->file_exists() returns false if storage object exists but instance has
	    # size of zero
	    $nebulous_server = $ipprc->nebulous() if !$nebulous_server;
	    if ($nebulous_server->storage_object_exists($bzip2_filename)) {
		print STDERR "$bzip2_filename exists, killing\n";
                $ipprc->kill_file($bzip2_filename);
	    } else {
	        print STDERR "$bzip2_filename does not exist\n" if $very_verbose;
	    }
	} else {
	    if ($ipprc->file_exists($bzip2_filename)) {
	        print STDERR "$bzip2_filename exists, killing\n";
	        $ipprc->kill_file($bzip2_filename);
	    } else {
	        print STDERR "$bzip2_filename does not exist\n" if $very_verbose;
	    }
	}
        my $bzip2_file = $ipprc->file_create($bzip2_filename);
        my_die("Unable to create $bzip2_filename", $stage_id, $stage_id, $PS_EXIT_SYS_ERROR) unless $bzip2_file;

        my $command = "$bzip2 < $resolved > $bzip2_file";
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $very_verbose);
        if ($success) {
            # success delete the original file
            my $error_code = $ipprc->kill_file($filename);
        } else {
            # if bzip2 failed. Carry on but don't delete the existing file
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            print STDERR "Failed to bzip2 $filename: $error_code\n";
            return 0;
        }
    }
    return 1;
}

sub storage_object_exists {
    my $filename = shift;
    $nebulous_server = $ipprc->nebulous() if !$nebulous_server;
    return $nebulous_server->storage_object_exists($filename);
}

# XXX we currently do not set the error state in the db on my_die
sub my_die
{
    my $msg = shift; # Warning message on die
    my $stage = shift; # stage name
    my $stage_id = shift; #  identifier
    my $exit_code = shift; # Exit code
    # outputImage and path_base are globals

    carp($msg);
    exit $exit_code;
}

__END__
