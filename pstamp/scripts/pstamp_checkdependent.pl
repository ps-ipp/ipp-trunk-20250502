#!/bin/env perl
#
# pstamp_checkdependent.pl
#
# Check the status of a pending pstampDependent insuring that 
# the any update processing that is needed has been queued and determine check 
# whether the processing has  finished or not

use warnings;
use strict;

use Sys::Hostname;
use Getopt::Long qw( GetOptions );
use File::Basename qw(basename);
use Carp;
use POSIX;
use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config qw( :standard );
use PS::IPP::PStamp::RequestFile qw( :standard );
use Carp;

my $disable_3PI_updates = 0;
my $ra_max_3PI_hours = 22.5;  # disable updates for skycells with ra greater than this (hours)
my $ra_max_3PI = $ra_max_3PI_hours * 15 * 3.14149 / 180.; # rawExp.ra is in radians

# verbose flag for parse commands
my $parse_verbose = 0;

# XXX: put this in a module somewhere
my $IPP_DIFF_MODE_WARP_WARP   = 1;
my $IPP_DIFF_MODE_WARP_STACK  = 2;
my $IPP_DIFF_MODE_STACK_WARP  = 3;
my $IPP_DIFF_MODE_STACK_STACK = 4;

my ($dep_id, $stage, $stage_id, $component, $imagedb, $label, $rlabel, $need_magic, $fault_count, $max_fault_count, $logfile);
my ($dbname, $ps_dbserver, $verbose, $save_temps, $no_update);

GetOptions(
    'dep_id=i'      =>  \$dep_id,
    'stage=s'       =>  \$stage,
    'stage_id=i'    =>  \$stage_id,
    'component=s'   =>  \$component,
    'imagedb=s'     =>  \$imagedb,      # dbname for images lookups.
    'label=s'       =>  \$label,        # request's label
    'rlabel=s'      =>  \$rlabel,       # pstampDependent.rlabel (deprecated)
    'need_magic'    =>  \$need_magic,
    'fault_count=i' =>  \$fault_count,
    'max_fault_count=i' =>  \$max_fault_count,
    'logfile=s'	    =>  \$logfile,
    'dbname=s'      =>  \$dbname,       # postage stamp server dbname
    'dbserver=s'    =>  \$ps_dbserver,  # postage stamp server dbserver
    'verbose'       =>  \$verbose,
    'parse-verbose' =>  \$parse_verbose,
    'save-temps'    =>  \$save_temps,
    'no-update'     =>  \$no_update,
);

die "--dep_id --stage --stage_id --component --imagedb are required"
    if !(defined $dep_id and defined $stage and defined $stage_id and
        defined $component and defined $imagedb);

$max_fault_count = 5 if !$max_fault_count;
$fault_count = 0 if !defined $fault_count;

my $missing_tools;
my $chiptool = can_run('chiptool') or (warn "Can't find chiptool" and $missing_tools = 1);
my $warptool = can_run('warptool') or (warn "Can't find warptool" and $missing_tools = 1);
my $difftool = can_run('difftool') or (warn "Can't find difftool" and $missing_tools = 1);
my $stacktool = can_run('stacktool') or (warn "Can't find stacktool" and $missing_tools = 1);
my $magicdstool = can_run('magicdstool') or (warn "Can't find magicdstool" and $missing_tools = 1);
my $pstamptool = can_run('pstamptool') or (warn "Can't find pstamptool" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit ($PS_EXIT_CONFIG_ERROR);
}

my $ipprc = PS::IPP::Config->new();

if ($logfile) {
   $ipprc->redirect_output($logfile);
}

print "\n==== Starting dependency checking for $dep_id $stage $stage_id $component at " . (scalar localtime) . ". ====\n";

if ($label) {
    # rlabel is deprecated. Use one based on the supplied label parameter which is the current label 
    # for the request, which may be different than the one given to the dependent when the job was parsed.
    # XXX: having the convention that update label is 'ps_ud_' . $label of request embedded here 
    # (and formerly in pstampparse.pl) is not particularly clean but it's simple.
    my $new_rlabel = 'ps_ud_' . $label;
    $rlabel = "undefined" if !defined $rlabel;
    if ($new_rlabel ne $rlabel) {
        print "Notice: using $new_rlabel instead of $rlabel for update label.\n";
        $rlabel = $new_rlabel;
    }
}


if (!$ps_dbserver) {
    $ps_dbserver =  metadataLookupStr($ipprc->{_siteConfig}, 'PS_DBSERVER');
}
$pstamptool  .= " -dbname $dbname" if $dbname;
$pstamptool  .= " -dbserver $ps_dbserver";

# Append imagedb to the ippTools
# Note: configured DBSERVER is used for this server
$chiptool    .= " -dbname $imagedb";
$warptool    .= " -dbname $imagedb";
$difftool    .= " -dbname $imagedb";
$stacktool   .= " -dbname $imagedb";
$magicdstool .= " -dbname $imagedb";


my $tool;
my $cmd;
my $dsRun_state = "";

if ($stage eq "chip") {
    $cmd = "$chiptool -processedimfile -allfiles -chip_id $stage_id -class_id $component";
} elsif ($stage eq "warp") {
    $cmd = "$warptool -warped -warp_id $stage_id -skycell_id $component";
} elsif ($stage eq "diff") {
    $cmd = "$difftool -diffskyfile -diff_id $stage_id -skycell_id $component";
} else {
    my_die("unexpected stage $stage found", $PS_EXIT_PROG_ERROR);
}

my $it = runToolAndParseExpectOne($cmd, $parse_verbose);

my_die("no components found", $PS_EXIT_PROG_ERROR) if ( !$it);

# Got "it"

my $magic_ok = 0;
if ($stage eq 'diff') {
    if ($it->{diff_mode} == $IPP_DIFF_MODE_STACK_STACK) {
        # stack stack diffs don't need magic, but since the warps need to have the chips destreaked
        # in order to be processed se set need_magic in the database for all runs
        # Now the diffs themselves don't need to be destreaked so
        $magic_ok = 1;
    }
}


# magic is no longer rquired
$need_magic = 0;
$magic_ok =  1;
my $status = 0;
if ((($it->{state} eq 'full') or ($it->{state} eq 'update')) and ($it->{data_state} eq 'full')) {

    print "\nDependency Satisfied for $stage $stage_id $component\n";
    # This Dependency is satisfied. All done. Release the pstampJobs
    #
    my $command = "$pstamptool -updatedependent -set_state full -dep_id $dep_id";
    if (!$no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
        unless ($success) {
            my_die("failed to set pstampDependent.state to 'full' dep_id: $dep_id",
                $PS_EXIT_UNKNOWN_ERROR);
        }
    } else {
        print "skipping $command\n";
    }
} elsif (($it->{state} eq 'cleaned') or ($it->{state} eq 'goto_cleaned') or ($it->{state} eq 'update')) {
    #       For warp and diff stages we need to call the 'queue_update' subroutines even if the 
    #       data_state is update in order to check the state of inputs in earlier stages in the pipeline
    #       For example if warpSkyfile is in update state but the chipRun that it depends on hasn't
    #       been updated we need to go and queue the chips for processing.

    my $fault = $it->{fault};
    if (($fault eq $PSTAMP_GONE) or (($it->{state} eq 'update') and $fault)) {
        $fault_count++;
        print "$stage $stage_id $component has fault $fault\n";
        if ($it->{fault} eq $PSTAMP_GONE) {
            faultJobs($PSTAMP_GONE);
            exit 0;
        } elsif ($fault_count >= $max_fault_count) {
            print "$stage $stage_id $component has faulted $fault_count times. Giving up\n";

            faultComponent($stage, $stage_id, $component, $PSTAMP_GONE);

            # fault the jobs
            faultJobs($PSTAMP_GONE);
            exit 0;
        }

        # hope the fault is transient.
        # fault the dependent to give the fault a chance to correct itself
        my_die("Component faulted on update dep_id: $dep_id", $PS_EXIT_SYS_ERROR);
    }

    if ($stage eq 'chip') {
        # check_states_chip takes an array so that check_states_warp can pass it a set of chips
        my $chips = [$it];
        $status = check_states_chip($it->{chip_id}, $chips, $rlabel, $need_magic);
    } elsif ($stage eq 'warp') {
        $status = check_states_warp($it, $rlabel, $need_magic);
    } elsif ($stage eq 'diff') {
        $status = check_states_diff($it, $rlabel, $need_magic);
    } else {
        my_die("Unexpected stage found $stage", $PS_EXIT_PROG_ERROR);
    }
    if ($status >= $PSTAMP_FIRST_ERROR_CODE) {
        faultJobs($status);
    }
} else {
    print "${stage}Run $stage_id state is $it->{state} $component data_state is $it->{data_state}\n";

    # detect states that cannot be updated and update the job state
    # XXX: Perhaps I should be more assertive here and check for the specific states that we know
    # that we can continue.

    my $state = $it->{state};
    my $job_fault = 0;

    if ($state eq 'error_cleaned') {
        $job_fault = $PSTAMP_NOT_AVAILABLE;
    } elsif (($state =~ /scrub/) or ($state =~ /purge/) or ($state eq 'drop')) {
        # Component state must have been changed state since dependency was inserted.
        print STDERR "Dependency cannot be satisfied\n";
        $job_fault = $PSTAMP_GONE;
    } elsif ($state =~ /new/) {
        # Dependency never should have been inserted
        my_die ("Unexpected state for ${stage}Run $stage_id $state", $PS_EXIT_PROG_ERROR);
    }

if (0) {
    if (!$job_fault and ($stage eq 'chip')) {
        # what about "error_cleaned" ?
        if (! ($it->{data_state} =~ /cleaned/) ) {
            # should only get here with data_state 'full' and perhaps destreaking not done
            my_die ("Unexpected state for ${stage}Run $stage_id $state", $PS_EXIT_PROG_ERROR)
                if $it->{data_state} ne 'full';

            # chip processing is done, start destreaking.
            my @chips;
            push @chips, $it->{class_id};
            $job_fault = check_states_magicDSRun($stage, $stage_id, \@chips, $rlabel, $need_magic, $it->{raw_magicked}, $it->{magic_ds_id}, $it->{dsRun_state});
        }
    }
}

    if ($job_fault >= $PSTAMP_FIRST_ERROR_CODE) {
        faultJobs($job_fault);
    }
}
    
exit $status;


sub check_states_chip {
    my $chip_id = shift;
    my $metadatas = shift;  # an array of hashes, either from chiptool -processedimfile or warptool -scmap
    my $rlabel = shift;     # if defined a new label for the chipRun
    my $need_magic = shift; 

    my $dsRun_state;
    my $raw_all_magicked = 1; # this gets cleared if any of the inputs aren't destreaked
    my @chips;
    my $magic_ds_id;

    my $queued_update = 0;
    foreach my $chip (@$metadatas) {
        $dsRun_state = $chip->{dsRun_state};
        $raw_all_magicked &= ($chip->{raw_magicked} > 0);
        $magic_ds_id = $chip->{magic_ds_id};

        push @chips, $chip->{class_id};

        if ($disable_3PI_updates && ($chip->{data_group} =~ /ThreePi/)) {
            # we are getting close to the end of PV2. Do not update PV1 chips if RA is above the limit.
            my $ra = $chip->{ra};
            if (!defined($ra) or ($ra > $ra_max_3PI)) {
                print "3PI updates are currently disabled for RA $ra > $ra_max_3PI (radians)\n";
                return $PSTAMP_NOT_AVAILABLE;
            } else {
                print "allowing 3PI updates RA $ra\n";
            }
        } 

        my $state = $chip->{state};
        my $data_state = $chip->{data_state};
        my $fault = $chip->{fault};
        if (($state =~ /error/) or ($state =~ /purged/) or ($state =~ /scrubbed/) or ($state eq 'drop') or
            ($data_state =~ /error/) or ($data_state =~ /purged/) or ($data_state =~ /scrubbed/) or ($data_state eq 'drop') or
            ($fault eq $PSTAMP_GONE)) {

            print "chipRun state is $chip->{chip_id} has state: $state class_id $chip->{class_id} data_state: $data_state fault: $fault cannot update.\n";
            my $error_code;
            if (($state eq 'error_cleaned') or ($data_state ='error_cleaned')) {
                $error_code = $PSTAMP_NOT_AVAILABLE;
            } else {
                $error_code = $PSTAMP_GONE;
            }
            
            # caller will fault the jobs
            return $error_code;
        } elsif ($chip->{state} eq 'goto_cleaned') {

            {
                print "dependent chip is in state goto_cleaned, changing state to update.\n";

                # cleanup must not be running. Set state to update. If this chip is not 'full' it will be
                # set to be updated the next time this script is invoked
                # XXX: In the very unlikely case that this chip run is actually in the running cleanup pantasks
                # queue things may get confused but we can live with that
                my $command = "$chiptool -updaterun -set_state update -chip_id $chip_id";
                $command .= " -set_label $rlabel" if $rlabel;

                if (!$no_update) {
                    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                                run(command => $command, verbose => $verbose);
                    unless ($success) {
                        my_die("failed to change ${stage}Run $stage_id $component from goto_cleaned to update", $PS_EXIT_UNKNOWN_ERROR);
                    }
                } else {
                    print "skipping $command\n";
                }
                $queued_update = 1;
            }
        } elsif (($chip->{data_state} ne 'update') and ($chip->{data_state} ne 'full')) {

            print "Setting chip imfile $chip_id $chip->{class_id} to be updated.\n";

            # chiptool does more state checking to insure this isn't done prematurely.
            my $command = "$chiptool -setimfiletoupdate -chip_id $chip_id -class_id $chip->{class_id}";
            $command .= " -set_label $rlabel" if $rlabel;

            my $update_mode;
            if ($chip->{data_group} =~ /^LAP.ThreePi.20120706/) {
                # if this is one of the chipRuns from the PV1 LAP regenerate without using the ppImage configdump file.
                # XXX: PV2 processing is complete. Don't do this anymore
                # $update_mode = 1;
            } elsif ($chip->{data_group} =~ /^M31.rp.2013/) {
                # Version of M31 processed with recipes and auxiliary masks incompatible with the
                # current code. Run from scratch which will use compatible versions of the same masks..
                $update_mode = 1;
            } 
            $command .= " -set_update_mode $update_mode" if $update_mode;

            if (!$no_update) {
                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                            run(command => $command, verbose => $verbose);
                unless ($success) {
                    my_die("failed to queue ${stage}Run $stage_id $component for update", $PS_EXIT_UNKNOWN_ERROR);
                }
            } else {
                print "skipping $command\n";
            }
            $queued_update = 1;
        } elsif ($chip->{state} eq 'cleaned' and $chip->{data_state} eq 'update') {
            # we've had a number of runs in this limbo state

            print "Changing chip run $chip_id to state update for $chip->{class_id} which is in data_state update.\n";
            my $command = "$chiptool -updaterun -set_state update -chip_id $chip_id";
            $command .= " -set_label $rlabel" if $rlabel;

            if (!$no_update) {
                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                            run(command => $command, verbose => $verbose);
                unless ($success) {
                    my_die("failed to change ${stage}Run $stage_id $component from goto_cleaned to update", $PS_EXIT_UNKNOWN_ERROR);
                }
            } else {
                print "skipping $command\n";
            }
            $queued_update = 1;
        } elsif ($chip->{fault}) {
            $fault_count++;
            my $fault =  $chip->{fault};

            if ($fault eq $PSTAMP_GONE) {
                # caller will fault jobs
                return $PSTAMP_GONE;
            } elsif ($fault_count > $max_fault_count) {
                print "Dependency for $stage $stage_id has been found in fault state $fault_count times. Giving up. Fault: $fault\n";
                # XXX: stop faulting chips for now
#                $fault = $PSTAMP_GONE;
#                faultComponent('chip', $chip->{chip_id}, $chip->{class_id}, $PSTAMP_GONE);
                $fault = $PSTAMP_NOT_AVAILABLE;
                return $fault;
            }
            # fault the dependent
            my_die("chip $chip->{chip_id} $chip->{class_id} faulted: $chip->{fault}", $chip->{fault});
        }
    }

    return 0;
}

sub check_states_warp {
    my $metadata = shift;
    my $rlabel = shift;     # if defined a new label for the chipRun
    my $need_magic = shift; 
    
    my $exit_status = 0;

    my $raw_all_magicked = 1; # this gets cleared if any of the inputs aren't destreaked

    my $warp_id = $metadata->{warp_id};
    my $skycell_id = $metadata->{skycell_id};
    my $state = $metadata->{state};
    my $data_state = $metadata->{data_state};

    # we are getting close to the end of PV2. optionally do not update 3Pi data.
    if ($disable_3PI_updates && ($metadata->{data_group} =~ /ThreePi/)) {
        my $ra = $metadata->{ra};
        if (!defined $ra or ($ra > $ra_max_3PI)) {
            print "3PI updates are currently disabled for RA $ra > $ra_max_3PI (radians)\n";
            return $PSTAMP_NOT_AVAILABLE;
        } else {
            print "allowing 3PI updates RA $ra\n";
        }
    } 

    if (($state =~ /error/) or ($state =~ /purged/) or ($state =~ /scrubbed/) or ($state eq 'drop') or
         ($data_state =~ /error/) or ($data_state =~ /purged/) or ($data_state =~ /scrubbed/) or ($data_state eq 'drop')) {
        print STDERR "warpRun $warp_id $skycell_id has state $state $data_state faulting jobs.\n";
        my $error_code;
        if (($state eq 'error_cleaned') or ($data_state eq 'error_cleaned')) {
            $error_code = $PSTAMP_NOT_AVAILABLE;
        } else {
            $error_code = $PSTAMP_GONE;
        }
        return $error_code
    }
    if ((($data_state eq 'full') or ($data_state eq 'update')) and ($metadata->{fault})) {
        # fault dependent.
        my $fault = $metadata->{fault};
        print STDERR "warp $warp_id $skycell_id faulted with code: $fault.";
        # XXX: TODO maybe: If ($fault != 2 and $fault != $PSTAMP_GONE) revert the skycell. The warp task only reverts fault 2
        # and sometimes warps will fault due to nfs problems that end up with fault = 4;.
        return $fault;
    }
    if ($metadata->{quality} ne 0) {
        print STDERR "warp $warp_id $skycell_id bad quality on update: $metadata->{quality}.\n";
        return $PSTAMP_GONE;
    }

    my $skycell = $metadata;

    # if ($skycell->{state} eq 'goto_cleaned' and $skycell->{data_state} eq 'full') {
    if ($skycell->{state} eq 'goto_cleaned') {
        # cleanup has been queued, but hasn't finished, probably due to an error or cleanup is not running.
        # It's "safe" to set the state to update. If the skycell is not full it will be set to update
        # the next time this task runs
        print "Change state of warp $warp_id to update.\n";
        my $command = "$warptool -updaterun -set_state update -warp_id $warp_id";
        $command .= " -set_label $rlabel" if $rlabel;

        if (!$no_update) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                        run(command => $command, verbose => $verbose);
            unless ($success) {
                my_die("failed to change state of ${stage}Run $stage_id from goto_cleaned to update", $PS_EXIT_UNKNOWN_ERROR);
            }
        } else {
            print "skipping $command\n";
        }
        return 0;
    }

    # get the list of input chips for this skycell
    my $command = "$warptool -scmap -warp_id $warp_id -skycell_id $skycell_id";
    my $data = runToolAndParse($command, $parse_verbose);
    if (!$data or scalar @$data == 0) {
        # This happens if the chipProcessedImfile disappears which happened when earlier
        # versions of chiptool -revertprocessedimfile didn't check the chipRun.state before
        # deleting the row. 
        print STDERR "failed to find warpSkyCellMap for warpRun $warp_id skycell_id $skycell_id";
        return $PSTAMP_GONE;
    }

    my $chips_ready = 1;
    my @chipsToUpdate;
    my $chip_id;
    foreach my $chip (@$data) {
        # XXX: change tools to include cam_state (camRun.state). If it is not full the warp updates will never run.
        my $cam_fault = $chip->{cam_fault};
        if ($chip->{camState} eq 'cleaned') {
            print STDERR "camRun for $warp_id skycell_id $skycell_id is cleaned so warp cannot be updated.\n";
            return $PSTAMP_GONE;
        }
        if (defined $cam_fault and $cam_fault > 0) {
            print STDERR "camRun for $warp_id skycell_id $skycell_id is faulted: $cam_fault\n";
            return $PSTAMP_GONE;
        }
        $chip_id = $chip->{chip_id};
        # XXX: -scmap doesn't have ra and dec. Copy it from the warp
        $chip->{ra} = $metadata->{ra};
        $chip->{decl} = $metadata->{decl};

if (0) {
        # if chip has been magicked before require it to be magicked again
        # because the warp pending query requires it.
        if ($chip->{magicked} < 0) {
            print "Input has been destreaked so we must destreak before warping\n";
            $need_magic = 1 
        }
}

        if ($need_magic and ($chip->{magicked} eq 0)) {
            my_die("Client requires magic, but chip never magicked. How did this dependent get queued?", $PS_EXIT_PROG_ERROR);
        }

        if (($chip->{data_state} ne 'full') or ($need_magic and ($chip->{magicked} < 0))) {
            $chips_ready = 0;
            $chip->{fault} = $chip->{chip_fault};
            $chip->{data_group} = $chip->{chip_data_group};
            push @chipsToUpdate, $chip;
        } else {
            # this chip is done
        }
    }

    if ($chips_ready and $skycell->{data_state} ne 'update') {
        # the reason we defer setting the warp to update is so that we can handle error conditions at previous
        # stages more easily.
        print "Chips are ready for warp $warp_id $skycell->{skycell_id}. Setting skycell to be updated.\n";
        my $command = "$warptool -setskyfiletoupdate -warp_id $warp_id -skycell_id $skycell->{skycell_id}";
        $command .= " -set_label $rlabel" if $rlabel;

        if (!$no_update) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                        run(command => $command, verbose => $verbose);
            unless ($success) {
                my_die("failed to queue ${stage}Run $stage_id $component for update", $PS_EXIT_UNKNOWN_ERROR);
            }
        } else {
            print "skipping $command\n";
        }
    } elsif ($chips_ready and $skycell->{data_state} eq 'update' and $skycell->{state} ne 'update') {
        print "Chips are ready for warp $warp_id $skycell->{skycell_id} but run state is $skycell->{state}. Changing state to update.\n";
        my $command = "$warptool -updaterun -warp_id $warp_id -set_state update";
        $command .= " -set_label $rlabel" if $rlabel;

        if (!$no_update) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                        run(command => $command, verbose => $verbose);
            unless ($success) {
                my_die("failed to change state of ${stage}Run $stage_id to update", $PS_EXIT_UNKNOWN_ERROR);
            }
        } else {
            print "skipping $command\n";
        }

    } elsif (scalar @chipsToUpdate > 0) {
        my $fault = check_states_chip($chip_id, \@chipsToUpdate, $rlabel, $need_magic);
        if ($fault) {
            if ($fault eq $PSTAMP_GONE) {
                # chip or dsfile that this skycell depends on has faulted in a way that is not recoverable
                # fault the skycell
                faultComponent('warp', $warp_id, $skycell->{skycell_id}, $PSTAMP_GONE);
            }
            $exit_status = $fault;
        }
    }

    # return value may be used as the return status of script so we use zero as success
    return $exit_status;
}

sub check_states_diff {
    my $metadata = shift;
    my $rlabel = shift;     # if defined a new label for the chipRun
    my $need_magic = 0; 

    my $diff_id   = $metadata->{diff_id};
    my $diff_mode = $metadata->{diff_mode};
    my $skycell = $metadata;
    my $skycell_id = $skycell->{skycell_id};

    if ($metadata->{state} eq 'goto_cleaned') {
        # cleanup must not be running. Set state to update. If the skycell is not 'full' it will be
        # set to be updated the next time this script is invoked.
        print "Changing state of diffRun $diff_id from goto_cleaned to update.\n";
        my $command = "$difftool -updaterun -set_state update -diff_id $diff_id";
        $command .= " -set_label $rlabel" if $rlabel;

        if (!$no_update) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                        run(command => $command, verbose => $verbose);
            unless ($success) {
                my_die("failed to queue ${stage}Run $stage_id $component for update", $PS_EXIT_UNKNOWN_ERROR);
            }
        } else {
            print "skipping $command\n";
        }
        return 0;
    }

    if ($diff_mode == $IPP_DIFF_MODE_WARP_STACK ) {
        # check the state of the template stack
        my $command = "$stacktool -sumskyfile -stack_id $skycell->{stack2}";
        my $stack = runToolAndParseExpectOne($command, $parse_verbose);
        my_die("failed to find stackSumSkyfile for stack_id $skycell->{stack2}", $PS_EXIT_UNKNOWN_ERROR) if !$stack;

        if ($stack->{state} ne 'full') {
            print STDERR "template stack for diffRun $diff_id $skycell_id is not in full state faulting jobs\n";
            faultComponent('diff', $diff_id, $skycell_id, $PSTAMP_GONE);
            return $PSTAMP_GONE;
        }
        print " Stack $skycell->{stack2} is ready.\n";

        # now check the warp
        $command = "$warptool -warped -warp_id $skycell->{warp1} -skycell_id $skycell_id";
        my $warp = runToolAndParseExpectOne($command, $parse_verbose);
        my_die("failed to find warpSkyfile for warpRun $skycell->{warp1} skycell_id $skycell_id", $PS_EXIT_UNKNOWN_ERROR) if !$warp;

        if ($warp->{data_state} ne 'full') {
            my $warp_status = check_states_warp($warp, $rlabel, $need_magic);
            if ($warp_status eq $PSTAMP_GONE) {
                faultComponent('diff', $diff_id, $skycell_id, $PSTAMP_GONE);
            }
            return $warp_status;
        } elsif ($warp->{quality} != 0) {
            print STDERR "input warp has quality error\n";
            faultComponent('diff', $diff_id, $skycell_id, $PSTAMP_GONE);
            return $PSTAMP_GONE;
        }
        # warps are ready fall through and queue the diff update
        print " Warp $skycell->{warp1} $skycell_id is ready.\n";
    } elsif ($diff_mode eq $IPP_DIFF_MODE_WARP_WARP) {
        my $command = "$warptool -warped -warp_id $skycell->{warp1} -skycell_id $skycell_id";
        my $warp1 = runToolAndParseExpectOne($command, $parse_verbose);
        my_die("failed to find warpSkyfile for warpRun $skycell->{warp1} skycell_id $skycell_id", $PS_EXIT_UNKNOWN_ERROR) if !$warp1;

        my $warps_ready = 1;
        my $warp_status = 0;
        if ($warp1->{data_state} ne 'full') {
            $warps_ready = 0;
            $warp_status = check_states_warp($warp1, $rlabel, $need_magic);
            if ($warp_status) {
                if ($warp_status eq $PSTAMP_GONE) {
                    faultComponent('diff', $diff_id, $skycell_id, $PSTAMP_GONE);
                }
                return $warp_status;
            }
        } elsif ($warp1->{quality} != 0) {
	    print STDERR "warp $warp1->{warp_id} $skycell_id has poor quality: $warp1->{quality}\n";
	    faultComponent('diff', $diff_id, $skycell_id, $PSTAMP_GONE);
	    return $PSTAMP_GONE;
	}
        $command = "$warptool -warped -warp_id $skycell->{warp2} -skycell_id $skycell_id";
        my $warp2 = runToolAndParseExpectOne($command, $parse_verbose);
        my_die("failed to find warpSkyfile for warpRun $skycell->{warp2} skycell_id $skycell_id", $PS_EXIT_UNKNOWN_ERROR) if !$warp2;

        if ($warp2->{data_state} ne 'full') {
            $warps_ready = 0;
            $warp_status = check_states_warp($warp2, $rlabel, $need_magic);
            if ($warp_status eq $PSTAMP_GONE) {
                faultComponent('diff', $diff_id, $skycell_id, $PSTAMP_GONE);
            }
        } elsif ($warp2->{quality} != 0) {
	    print STDERR "warp $warp2->{warp_id} $skycell_id has poor quality: $warp2->{quality}\n";
	    faultComponent('diff', $diff_id, $skycell_id, $PSTAMP_GONE);
	    return $PSTAMP_GONE;
	}

        if (!$warps_ready) {
            # don't queue the diff update yet
            return $warp_status;
        }
        # inputs are ready fall through and queue the diff update

    } elsif ($diff_mode == $IPP_DIFF_MODE_STACK_STACK ) {
        # check the state of the input stack
        my $command = "$stacktool -sumskyfile -stack_id $skycell->{stack1}";
        my $stack1 = runToolAndParseExpectOne($command, $parse_verbose);
        my_die("failed to find stackSumSkyfile for stack_id $skycell->{stack1}", $PS_EXIT_UNKNOWN_ERROR) if !$stack1;

        if ($stack1->{state} ne 'full') {
            print STDERR "input stack $skycell->{stack1} for diffRun $diff_id $skycell_id is not in full state faulting jobs\n";
            faultComponent('diff', $diff_id, $skycell_id, $PSTAMP_GONE);
            return $PSTAMP_GONE;
        }
        # check the state of the template stack
        $command = "$stacktool -sumskyfile -stack_id $skycell->{stack2}";
        my $stack2 = runToolAndParseExpectOne($command, $parse_verbose);
        my_die("failed to find stackSumSkyfile for stack_id $skycell->{stack2}", $PS_EXIT_UNKNOWN_ERROR) if !$stack2;

        if ($stack2->{state} ne 'full') {
            print STDERR "template stack $skycell->{stack2} for diffRun $diff_id $skycell_id is not in full state faulting jobs\n";
            faultComponent('diff', $diff_id, $skycell_id, $PSTAMP_GONE);
            return $PSTAMP_GONE;
        }

        # inputs are ready fall through and queue the diff update
    } elsif ($diff_mode == $IPP_DIFF_MODE_STACK_WARP ) {
        # check the state of the input stack
        my $command = "$stacktool -sumskyfile -stack_id $skycell->{stack1}";
        my $stack = runToolAndParseExpectOne($command, $parse_verbose);
        my_die("failed to find stackSumSkyfile for stack_id $skycell->{stack1}", $PS_EXIT_UNKNOWN_ERROR) if !$stack;

        if ($stack->{state} ne 'full') {
            print STDERR "input stack for diffRun $diff_id $skycell_id is not in full state faulting jobs\n";
            faultComponent('diff', $diff_id, $skycell_id, $PSTAMP_GONE);
            return $PSTAMP_GONE;
        }

        # now check the template warp
        $command = "$warptool -warped -warp_id $skycell->{warp2} -skycell_id $skycell_id";
        my $warp = runToolAndParseExpectOne($command, $parse_verbose);
        my_die("failed to find warpSkyfile for warpRun $skycell->{warp2} skycell_id $skycell_id", $PS_EXIT_UNKNOWN_ERROR) if !$warp;

        if ($warp->{data_state} ne 'full') {
            my $warp_status = check_states_warp($warp, $rlabel, $need_magic);
            if ($warp_status eq $PSTAMP_GONE) {
                faultComponent('diff', $diff_id, $skycell_id, $PSTAMP_GONE);
            }
            return $warp_status;
        }
        # warps are ready fall through and queue the diff update
    } else {
        my_die("unexpected diff_mode found: $diff_mode", $PS_EXIT_PROG_ERROR);
    }

    if (($skycell->{data_state} ne 'update') or ($skycell->{state} ne 'update')) {
        my $command = "$difftool -setskyfiletoupdate -diff_id $diff_id -skycell_id $skycell_id";
        $command .= " -set_label $rlabel" if $rlabel;

        if (!$no_update) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                        run(command => $command, verbose => $verbose);
            unless ($success) {
                my_die("failed to queue ${stage}Run $stage_id $component for update", $PS_EXIT_UNKNOWN_ERROR);
            }
        } else {
            print "skipping $command\n";
        }
    } elsif (defined $rlabel and ($skycell->{label} ne $rlabel)) {
        # change the label to match this dependent's rlabel
        my $command = "$difftool -updatrun -diff_id $diff_id -set_label $rlabel";

        if (!$no_update) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                        run(command => $command, verbose => $verbose);
            unless ($success) {
                my_die("failed to queue ${stage}Run $stage_id $component for update", $PS_EXIT_UNKNOWN_ERROR);
            }
        } else {
            print "skipping $command\n";
        }
    } else {
        print " diff is in update state\n";
    }

    # return value is a unix style exit status so zero is good
    return 0;
}


# run a command that produces metadata output and parse the results into an array of objects
sub runToolAndParse {
    my $command = shift;
    my $verbose = shift;

    my ($program) = split " ", $command;
    $program = basename($program);

    print "Running $command\n" if !$verbose;
    my $start_tool = DateTime->now->mjd;
    # run the command and parse the output
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
    unless ($success) {
        print STDERR @$stderr_buf if !$verbose;
        return undef;
    }

    my $now = DateTime->now->mjd;
    my $dtime_tool = (DateTime->now->mjd - $start_tool) * 86400.;
    if ($dtime_tool > 0.1) {
        print "Time to run $program: $dtime_tool\n";
    }

    my $buf = join "", @$stdout_buf;
    if (!$buf) {
        return undef;
    }

    my $start_parse = DateTime->now->mjd;

    my $mdcParser = PS::IPP::Metadata::Config->new;
    my $results = parse_md_fast($mdcParser, $buf)
        or my_die ("Unable to parse metadata config doc", $PS_EXIT_UNKNOWN_ERROR);

    my $dtime_parse = (DateTime->now->mjd - $start_parse) * 86400.;
    if ($dtime_parse > 0.1) {
        print "Time to parse results from $program: $dtime_parse\n";
    }

    return $results;
}

# run an command returning metadata where we expect 1 entry
sub runToolAndParseExpectOne {
    my $command = shift;
    my $verbose = shift;

    my $data = runToolAndParse($command, $verbose);

    if (!$data) {
        return undef;
    }

    my $n = scalar @$data;
    if ($n > 1) {
        my_die("Unexpected number of components $n returned by $command", $PS_EXIT_PROG_ERROR);
    }

    return $data->[0];
}

# splits meta data config input stream into single units to work around the pathalogically
# slow parser. This is similar to and adapted from code in various ippScripts.
sub parse_md_fast {
    my $mdcParser = shift;
    my $input = shift;
    my $output = ();

    my @whole = split /\n/, $input;
    my @single = ();

    my $n;
    while ( ($n = @whole) > 0) {
        my $value = shift @whole;
        push @single, $value;
        if ($value =~ /^\s*END\s*$/) {
	    push @single, "\n";

            my $list = parse_md_list( $mdcParser->parse( join("\n", @single ) ) ) or
                print STDERR "Unable to parse metdata config doc" and return undef;
            push @$output, $list->[0];

            @single = ();
        }
    }
    return $output;
}

# Check the data_state of the magicDSFile associated with this component (currently only chip stage is supported)
# Returns zero on success.
# returns the PSTAMP fault code to use for the jobs if an unrecoverable error is detected
# And faults the dependent if transient errors occur
sub check_states_magicDSRun {
    my $stage = shift;
    my $stage_id = shift;
    my $components = shift;
    my $rlabel  = shift;
    my $need_magic = shift;
    my $input_magicked = shift;
    my $magic_ds_id = shift;
    my $dsRun_state = shift;

    # XXX: this code assumes that for update destreaking is only performed for chip stage
    my_die ("check_states_magicDSRun only implemented for chip stage", $PS_EXIT_PROG_ERROR) if $stage ne 'chip';

    # if called from check_states_warp dsRun_state is unknown. Go find it.
    if (!$dsRun_state) {
        my $command = "$chiptool -listrun -chip_id $stage_id";
        my $data = runToolAndParse($command, $parse_verbose);
        my $chipRun = $data->[0];
        $dsRun_state = $chipRun->{dsRun_state};
    }

    # if the input file is already magicked no need to queue destreaking for this chipRun
    if ($need_magic and !$input_magicked) {
        if (!defined($dsRun_state) or ($dsRun_state eq 'NULL')) {
            # it is arguably a programming error if we get here
            print "No magicDSRun for chipRun $stage_id and magic is required\n";
            return $PSTAMP_NOT_DESTREAKED;
        } elsif (($dsRun_state eq 'cleaned') or ($dsRun_state eq 'update')) {
            foreach my $c (@$components) {
                my $command = "$magicdstool -destreakedfile -magic_ds_id $magic_ds_id -component $c";
                my $dsfile = runToolAndParseExpectOne($command, $parse_verbose);
                if (!$dsfile) {
                    my_die("failed to find magicDSFile for ${stage}Run $stage_id $c", $PS_EXIT_UNKNOWN_ERROR);
                }
                if ($dsfile->{fault} eq $PSTAMP_GONE) {
                    print "magicDSFile has fault $PSTAMP_GONE\n";
                    return $PSTAMP_GONE;
                }
                #  destreak faults get cleared when the component is set to be updated
                if (($dsfile->{data_state} eq 'update') and ($dsfile->{fault} > 0)) {
                    $fault_count++;
                    if ($fault_count > $max_fault_count) {
                        print "Destreak file $magic_ds_id $component for $stage $stage_id has faulted $fault_count times. Giving up\n";
                        faultComponent('destreak', $magic_ds_id, $component, $PSTAMP_GONE);
                        return $PSTAMP_GONE;
                    }
                    # Assume fault is transient
                    my_die("faulted magicDSFile for ${stage}Run $stage_id $c fault: $dsfile->{fault}",
                        $PS_EXIT_UNKNOWN_ERROR);
                }
                if ($dsfile->{data_state} eq 'cleaned') {
                    $command = "$magicdstool -setfiletoupdate -magic_ds_id $magic_ds_id -component $c";
		    # XXX: get the recoveryroot from a config file
                    # (It isn't actually used except to check whether it is a nebulous path)
                    $command .= " -set_recoveryroot neb://any/gpc1/destreak/recover";
                    $command .= " -set_label $rlabel" if $rlabel;
                    if (!$no_update) {
                        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                            run(command => $command, verbose => $verbose);
                        unless ($success) {
                            my_die("failed to set destreaked component to 'update' for ${stage}Run $stage_id $c",
                                $PS_EXIT_UNKNOWN_ERROR);
                        }
                    } else {
                        print "skipping $command\n";
                    }
                } else {
                    print "magicDSFile for ${stage}Run $stage_id $c data_state: $dsfile->{data_state}\n";
                }
            }
        } elsif ($dsRun_state eq 'failed_revert') {
            # XXX: revert failures are rarely fixed. give up but say it's just not available not GONE 
            print "magicDSRun.state = $dsRun_state for chipRun $stage_id is in state failed_revert cannot update\n";
            return $PSTAMP_NOT_AVAILABLE;
        } else {
            print "magicDSRun.state = $dsRun_state for chipRun $stage_id";
            print " cannot update yet" if $dsRun_state ne "new";
            print ".\n";
        }
    }

    return 0;
}

sub faultJobs {
    my ($job_fault) = @_;

    my $command = "$pstamptool -updatejob -set_state stop -set_fault $job_fault -dep_id $dep_id";
    if (!$no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
        unless ($success) {
            my_die("failed to set pstampJob.fault for dep_id: $dep_id",
                $PS_EXIT_UNKNOWN_ERROR);
        }
    } else {
        print "skipping $command\n";
    }
}

sub faultComponent {
    my ($stage, $stage_id, $component, $fault) = @_;

    # Stop faulting components. With the death of magic the likelyhood that something is truly gone
    # forever is much smaller. The false alarm rate is far too high
    return;

    my $command;
    if ($stage eq 'chip') {
        $command = "$chiptool -updateprocessedimfile -chip_id $stage_id -class_id $component";
    } elsif ($stage eq 'warp') {
        $command = "$warptool -updateskyfile -warp_id $stage_id -skycell_id $component";
    } elsif ($stage eq 'diff') {
        $command = "$difftool -updatediffskyfile -diff_id $stage_id -skycell_id $component";
    } elsif ($stage eq 'destreak') {
        $command = "$magicdstool -updatedestreakedfile -magic_ds_id $stage_id -component $component";
    } else {
        my_die("unexpected stage $stage found", $PS_EXIT_PROG_ERROR);
    }

    $command .= " -fault $fault";

    if (!$no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
        unless ($success) {
            carp "$cmd failed";
        }
    } else {
        print "skipping $command\n";
    }
}

sub my_die
{
    my $msg = shift;
    my $fault = shift;
    carp $msg;

    my $command = "$pstamptool -updatedependent -set_fault $fault -dep_id $dep_id";
    if (!$no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
        unless ($success) {
            carp "$cmd failed";
        }
    } else {
        print "skipping $command\n";
    }

    exit $fault;
}
