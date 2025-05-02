## warp.pro : image warping tasks : -*- sh -*-
## This file contains panTasks definitions for performing the image warping.

### This is done in two (main) steps.  After a warp (with associated warp_id) is defined,
### overlaps between the exposure being warped and skycells are calculated (tasks in warpInputExp).
### Then for each skycell, the warp is made (tasks in warpPendingSkycell).

### Setups
check.globals

### Initialise the books containing the tasks to do
book init warpInputExp
book init warpPendingSkyCell
book init warpPendingCleanup
book init warpPendingSummary

### Database lists
$warpExp_DB = 0
$warpSkycell_DB = 0
$warp_revert_overlap_DB = 0
$warp_revert_warped_DB = 0
$warpSummary_DB = 0

### Check status of warping tasks
macro warp.status
  book listbook warpInputExp
  book listbook warpPendingSkyCell
end

### Reset warping tasks
macro warp.reset
  book init warpInputExp
  book init warpPendingSkyCell
  book init warpPendingSummary
end

### Turn warping tasks on
macro warp.on
  task warp.exp.load
    active true
  end
  task warp.exp.run
    active true
  end
  task warp.skycell.load
    active true
  end
  task warp.skycell.run
    active true
  end
  task warp.advancerun
    active true
  end
  task warp.revert.overlap
    active true
  end
  task warp.revert.warped
    active true
  end
end

### Turn warping tasks off
macro warp.off
  task warp.exp.load
    active false
  end
  task warp.exp.run
    active false
  end
  task warp.skycell.load
    active false
  end
  task warp.skycell.run
    active false
  end
  task warp.advancerun
    active false
  end
  task warp.revert.overlap
    active false
  end
  task warp.revert.warped
    active false
  end
end

macro warp.revert.on
  task warp.revert.warped
    active true
  end
  task warp.revert.overlap
    active true
  end
end

macro warp.revert.off
  task warp.revert.warped
    active false
  end
  task warp.revert.overlap
    active false
  end
end

macro warp.summary.on
  task warp.summary.load
    active true
  end
  task warp.summary.run
    active true
  end
end

macro warp.summary.off
  task warp.summary.load
    active false
  end
  task warp.summary.run
    active false
  end
end

macro set.warp.timing
  task	       warp.skycell.run
    periods      -poll 0.5
    periods      -exec 0.5
  end
end

### Load tasks for calculating the warp overlaps
### Tasks are loaded into warpInputExp.
task	       warp.exp.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 1200
  npending     1

  stdout NULL
  stderr $LOGDIR/warp.exp.log

  task.exec
    if ($LABEL:n == 0) break
    $run = warptool -tooverlap
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$warpExp_DB
      $run = $run -dbname $DB:$warpExp_DB
      $warpExp_DB ++
      if ($warpExp_DB >= $DB:n) set warpExp_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout warpInputExp -key warp_id:fake_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook warpInputExp
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup warpInputExp
  end

  # locked list
  task.exit    default
    showcommand failure
  end

  task.exit    crash
    showcommand crash
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
  end
end

### Run tasks for calculating the warp overlaps
### Tasks are taken from warpInputExp.
task	       warp.exp.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC

    book npages warpInputExp -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    if ($BURNTOOLING == 1) break

    # look for new images in warpInputExp (pantaskState == INIT)
    book getpage warpInputExp 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword warpInputExp $pageName pantaskState RUN
    book getword warpInputExp $pageName warp_id -var WARP_ID
    book getword warpInputExp $pageName camera -var CAMERA
    book getword warpInputExp $pageName workdir -var WORKDIR_TEMPLATE
    book getword warpInputExp $pageName dbname -var DBNAME
    # XXX change tess_id to tess_dir when schema is changed
    book getword warpInputExp $pageName tess_id -var TESS_DIR
    book getword warpInputExp $pageName exp_tag -var EXP_TAG

    # set the host and workdir (default)
    set.host.for.camera $CAMERA $WARP_ID
    set.workdir.by.camera $CAMERA $WARP_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    ## generate outroot specific to this exposure
    sprintf logfile "%s/%s/%s.wrp.%s.log" $WORKDIR $EXP_TAG $EXP_TAG $WARP_ID

    stdout $LOGDIR/warp.exp.log
    stderr $LOGDIR/warp.exp.log

    # XXX warp_overlap.pl differs from the standard script : it does not have an 'outroot' argument, and it does not take '--redirect'
    $run = warp_overlap.pl --warp_id $WARP_ID --camera $CAMERA --tess_dir $TESS_DIR --logfile $logfile
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit    default
    process_exit warpInputExp $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword warpInputExp $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword warpInputExp $options:0 pantaskState TIMEOUT
  end
end


### Load tasks for doing the warps
### Tasks are loaded into warpPendingSkyCell.
task	       warp.skycell.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 1200
  npending     1

  stdout NULL
  stderr $LOGDIR/warp.skycell.log

  task.exec
    if ($LABEL:n == 0) break
    $run = warptool -towarped
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$warpSkycell_DB
      $run = $run -dbname $DB:$warpSkycell_DB
      $warpSkycell_DB ++
      if ($warpSkycell_DB >= $DB:n) set warpSkycell_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    # increase the poll limit for warp over the default to
    # help it keep up with chip processing
    # NOTE : it is not a problem for warptool to have multiple 
    # -limit entries: the last one is used
    $run = $run -limit {$POLL_LIMIT * 2}
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    # XXX change tess_id to tess_dir when db is updated
    ipptool2book stdout warpPendingSkyCell -key warp_id:skycell_id:tess_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook warpPendingSkyCell
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup warpPendingSkyCell
  end

  # locked list
  task.exit    default
    showcommand failure
  end

  task.exit    crash
    showcommand crash
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
  end
end

### Run tasks for calculating the warp overlaps
### Tasks are taken from warpPendingSkyCell.
task	       warp.skycell.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC

    book npages warpPendingSkyCell -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in warpPendingSkyCell (pantaskState == INIT)
    book getpage warpPendingSkyCell 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword warpPendingSkyCell $pageName pantaskState RUN
    book getword warpPendingSkyCell $pageName warp_id -var WARP_ID
    book getword warpPendingSkyCell $pageName warp_skyfile_id -var WARP_SKYFILE_ID
    book getword warpPendingSkyCell $pageName skycell_id -var SKYCELL_ID
    book getword warpPendingSkyCell $pageName camera -var CAMERA
    book getword warpPendingSkyCell $pageName workdir -var WORKDIR_TEMPLATE
    book getword warpPendingSkyCell $pageName path-base -var PATH_BASE
    book getword warpPendingSkyCell $pageName dbname -var DBNAME
    # XXX change tess_id to tess_dir when schema is changed
    book getword warpPendingSkyCell $pageName tess_id -var TESS_DIR
    book getword warpPendingSkyCell $pageName reduction -var REDUCTION
    book getword warpPendingSkyCell $pageName exp_tag -var EXP_TAG
    book getword warpPendingSkyCell $pageName state -var RUN_STATE
    book getword warpPendingSkyCell $pageName magicked -var CHIP_MAGICKED
    if ($CHIP_MAGICKED > 0)
        $MAGICKED_ARG = "--magicked $CHIP_MAGICKED"
    else
        $MAGICKED_ARG = ""
    end

    # set the host and workdir based on the skycell hash
    set.host.for.skycell $SKYCELL_ID
    set.workdir.by.skycell $SKYCELL_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    if ("$PATH_BASE" == "NULL") 
        ## generate outroot specific to this exposure
        sprintf outroot "%s/%s/%s.wrp.%s.%s" $WORKDIR $EXP_TAG $EXP_TAG $WARP_ID $SKYCELL_ID
    else 
        $outroot = $PATH_BASE
    end

    stdout $LOGDIR/warp.skycell.log
    stderr $LOGDIR/warp.skycell.log

    $run = warp_skycell.pl --threads @MAX_THREADS@ --warp_id $WARP_ID --warp_skyfile_id $WARP_SKYFILE_ID --skycell_id $SKYCELL_ID --tess_dir $TESS_DIR --camera $CAMERA --outroot $outroot --redirect-output --run-state $RUN_STATE $MAGICKED_ARG
    if ("$REDUCTION" != "NULL")
      $run = $run --reduction $REDUCTION
    end
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit    default
    process_exit warpPendingSkyCell $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword warpPendingSkyCell $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword warpPendingSkyCell $options:0 pantaskState TIMEOUT
  end
end

# this variable will cycle through the known database names
$warp_advance_DB = 0

# advance exposures for which all imfiles have completed processing
# sets the exposure state to full and queues warp processing if requested
task	       warp.advancerun
  host         local

  periods      -poll $LOADPOLL
  periods      -exec 30
  periods      -timeout 60
  npending     1

  stdout NULL
  stderr $LOGDIR/warp.advancerun.log

  task.exec
    if ($LABEL:n == 0) break
    $run = warptool -advancerun -limit 10
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$warp_advance_DB
      $run = $run -dbname $DB:$warp_advance_DB
      $warp_advance_DB ++
      if ($warp_advance_DB >= $DB:n) set warp_advance_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
  end

  # locked list
  task.exit    default
    showcommand failure
  end

  task.exit    crash
    showcommand crash
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
  end
end

task warp.revert.overlap
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = warptool -revertoverlap
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$warp_revert_overlap_DB
      $run = $run -dbname $DB:$warp_revert_overlap_DB
      $warp_revert_overlap_DB ++
      if ($warp_revert_overlap_DB >= $DB:n) set warp_revert_overlap_DB = 0
    end
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
  end

  # locked list
  task.exit    default
    showcommand failure
  end

  task.exit    crash
    showcommand crash
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
  end
end

task warp.revert.warped
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    # Only revert failures with fault=2 (SYS_ERROR), which tend to be
    # temporary filesystem problems.  Every other fault type is
    # interesting and should be kept for debugging (and so it doesn't
    # continue to occur).
    $run = warptool -revertwarped -fault 2
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$warp_revert_warped_DB
      $run = $run -dbname $DB:$warp_revert_warped_DB
      $warp_revert_warped_DB ++
      if ($warp_revert_warped_DB >= $DB:n) set warp_revert_warped_DB = 0
    end
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
  end

  # locked list
  task.exit    default
    showcommand failure
  end

  task.exit    crash
    showcommand crash
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
  end
end

### Load tasks for doing the warps
### Tasks are loaded into warpPendingSkyCell.
task	       warp.summary.load
  host         local
  active       false
  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 1200
  npending     1

  stdout NULL
  stderr $LOGDIR/warp.summary.log

  task.exec
    if ($LABEL:n == 0) break
    $run = warptool -tosummary
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$warpSummary_DB
      $run = $run -dbname $DB:$warpSummary_DB
      $warpSummary_DB ++
      if ($warpSummary_DB >= $DB:n) set warpSummary_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    # XXX change tess_id to tess_dir when db is updated
    ipptool2book stdout warpPendingSummary -key warp_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook warpPendingSummary
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup warpPendingSummary
  end

  # locked list
  task.exit    default
    showcommand failure
  end

  task.exit    crash
    showcommand crash
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
  end
end

### Run tasks for calculating the warp overlaps
### Tasks are taken from warpPendingSkyCell.
task	       warp.summary.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60
  active       false

  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC

    book npages warpPendingSummary -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in warpPendingSkyCell (pantaskState == INIT)
    book getpage warpPendingSummary 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword warpPendingSummary $pageName pantaskState RUN
    book getword warpPendingSummary $pageName warp_id -var WARP_ID
    book getword warpPendingSummary $pageName camera -var CAMERA
    book getword warpPendingSummary $pageName workdir -var WORKDIR_TEMPLATE
    book getword warpPendingSummary $pageName dbname -var DBNAME
    book getword warpPendingSummary $pageName tess_id -var TESS_DIR
    book getword warpPendingSummary $pageName exp_tag -var EXP_TAG
    book getword warpPendingSummary $pageName state -var RUN_STATE

    # set the host and workdir based on the skycell hash
    host anyhost
    strsub $WORKDIR_TEMPLATE @HOST@.0 $default_host -var WORKDIR
#    set.workdir.by.skycell $SKYCELL_ID $WORKDIR_TEMPLATE $default_host WORKDIR

#    if ("$PATH_BASE" == "NULL") 
        ## generate outroot specific to this exposure
    basename $TESS_DIR -var TESS_ID

    sprintf outroot "%s/%s/%s.wrp.%s.%s" $WORKDIR $EXP_TAG $EXP_TAG $WARP_ID $TESS_ID


    stdout $LOGDIR/warp.summary.log
    stderr $LOGDIR/warp.summary.log

    $run = skycell_jpeg.pl --stage warp --stage_id $WARP_ID --camera $CAMERA --outroot $outroot
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit    default
    process_exit warpPendingSummary $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword warpPendingSummary $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword warpPendingSummary $options:0 pantaskState TIMEOUT
  end
end
