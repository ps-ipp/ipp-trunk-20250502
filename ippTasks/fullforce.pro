## fullforce.pro : tasks for static sky calibration analysis : -*- sh -*-

## This file contains panTasks definitions for performing the fullforce analysis. 

# test for required global variables
check.globals

### Initialise the books containing the tasks to do
book init fullForceRun
book init fullForceSummary

### Database lists
$fullForce_DB = 0
$fullForce_revert_DB = 0
$fullForceSummary_DB = 0
$fullForceSummary_revert_DB = 0

### Check status of fullForce tasks
macro fullforce.status
  book listbook fullForceRun
end

### Reset fullForce tasks
macro fullforce.reset
  book init fullForceRun
end
macro fullforce.summary.status
  book listbook fullForceSummary
end

### Reset fullForce tasks
macro fullforce.summary.reset
  book init fullForceSummary
end

### Turn fullForce tasks on
macro fullforce.on
  task fullforce.load
    active true
  end
  task fullforce.run
    active true
  end
  task fullforce.summary.load
    active true
  end
  task fullforce.summary.run
    active true
  end
end

### Turn fullForce tasks off
macro fullforce.off
  task fullforce.load
    active false
  end
  task fullforce.run
    active false
  end
  task fullforce.summary.load
    active false
  end
  task fullforce.summary.run
    active false
  end
end

macro fullforce.revert.on
  task fullforce.revert
    active true
  end
  task fullforce.summary.revert
    active true
  end
end

macro fullforce.revert.off
  task fullforce.revert
    active false
  end
  task fullforce.summary.revert
    active false
  end
end

### Load tasks for fullforce
### Tasks are loaded into fullForceRun.
task	       fullforce.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/fullforce.log

  task.exec
    if ($LABEL:n == 0) break
    $run = fftool -todo
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$fullForce_DB
      $run = $run -dbname $DB:$fullForce_DB
      $fullForce_DB ++
      if ($fullForce_DB >= $DB:n) set fullForce_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout fullForceRun -key ff_id:warp_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook fullForceRun
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup fullForceRun
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

### Run tasks for the fullforce analysis
### Tasks are taken from fullForceRun.
task	       fullforce.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 10800

  task.exec
    # if we are unable to run use "long" exectime
    periods -exec $RUNEXEC
    book npages fullForceRun -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    if ($BURNTOOLING == 1) break


    # look for new entries in fullForceRun (pantaskState == INIT)
    book getpage fullForceRun 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword fullForceRun $pageName pantaskState RUN
    book getword fullForceRun $pageName ff_id -var FF_ID
    book getword fullForceRun $pageName warp_id -var WARP_ID
    book getword fullForceRun $pageName sources_path_base -var SOURCES_PATH_BASE
    book getword fullForceRun $pageName tess_id -var TESS_DIR
    book getword fullForceRun $pageName skycell_id -var SKYCELL_ID
    book getword fullForceRun $pageName camera -var CAMERA
    book getword fullForceRun $pageName workdir -var WORKDIR_TEMPLATE
    book getword fullForceRun $pageName warp_path_base -var WARP_PATH_BASE
    book getword fullForceRun $pageName reduction -var REDUCTION
    book getword fullForceRun $pageName dbname -var DBNAME

    # set the host and workdir based on the skycell hash
    # set.host.for.skycell $SKYCELL_ID
    host anyhost
    set.workdir.by.skycell $SKYCELL_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    basename $TESS_DIR -var TESS_ID
    sprintf outroot "%s/%s/%s/%s.%s.wrp.%s.ff.%s" $WORKDIR $TESS_ID $SKYCELL_ID $TESS_ID $SKYCELL_ID $WARP_ID $FF_ID

    stdout $LOGDIR/fullforce.log
    stderr $LOGDIR/fullforce.log

    $run = psphot_fullforce_warp.pl --ff_id $FF_ID --warp_id $WARP_ID --outroot $outroot --redirect-output --camera $CAMERA --warp_path_base $WARP_PATH_BASE --sources_path_base $SOURCES_PATH_BASE --skycell_id $SKYCELL_ID --threads @MAX_THREADS@
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
    # since we have work to do shorten exec time
    periods -exec .05
    command $run
  end

  # default exit status
  task.exit    default
    process_exit fullForceRun $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword fullForceRun $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword fullForceRun $options:0 pantaskState TIMEOUT
  end
end

task fullforce.revert
  host         local

  periods      -poll 10.0
  periods      -exec 1200.0
  periods      -timeout 120.0
  npending     1
  active true
  
  stdout NULL
  stderr $LOGDIR/fullforce.revert.log

  task.exec
    if ($LABEL:n == 0) break
    # Only revert failures with fault=2 (SYS_ERROR), which tend to be
    # temporary filesystem problems.  Every other fault type is
    # interesting and should be kept for debugging (and so it does not
    # continue to occur).
    $run = fftool -revert -fault 2
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$fullForce_revert_DB
      $run = $run -dbname $DB:$fullForce_revert_DB
      $fullForce_revert_DB ++
      if ($fullForce_revert_DB >= $DB:n) set fullForce_revert_DB = 0
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

### Load tasks for fullForceSummary
### Tasks are loaded into fullForceSummary.
task	       fullforce.summary.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/fullforce.summary.log

  task.exec
    if ($LABEL:n == 0) break
    $run = fftool -toadvance
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$fullForceSummary_DB
      $run = $run -dbname $DB:$fullForceSummary_DB
      $fullForceSummary_DB ++
      if ($fullForceSummary_DB >= $DB:n) set fullForceSummary_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout fullForceSummary -key ff_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook fullForceSummary
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup fullForceSummary
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

### Run tasks for the fullForceSummary analyasis
### Tasks are taken from fullForceSummary.
task	       fullforce.summary.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 10800

  task.exec
    # if we are unable to run use "long" exectime
    periods -exec $RUNEXEC
    book npages fullForceSummary -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    if ($BURNTOOLING == 1) break


    # look for new entries in fullForceSummary (pantaskState == INIT)
    book getpage fullForceSummary 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword fullForceSummary $pageName pantaskState RUN
    book getword fullForceSummary $pageName ff_id -var FF_ID
    book getword fullForceSummary $pageName tess_id -var TESS_DIR
    book getword fullForceSummary $pageName skycell_id -var SKYCELL_ID
    book getword fullForceSummary $pageName camera -var CAMERA
    book getword fullForceSummary $pageName workdir -var WORKDIR_TEMPLATE
    book getword fullForceSummary $pageName reduction -var REDUCTION
    book getword fullForceSummary $pageName dbname -var DBNAME

    # set the host and workdir based on the skycell hash
    # set.host.for.skycell $SKYCELL_ID
    host anyhost
    set.workdir.by.skycell $SKYCELL_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    basename $TESS_DIR -var TESS_ID
    sprintf outroot "%s/%s/%s/%s.%s.ff.%s" $WORKDIR $TESS_ID $SKYCELL_ID $TESS_ID $SKYCELL_ID $FF_ID

    stdout $LOGDIR/fullforce.summary.log
    stderr $LOGDIR/fullforce.summary.log

    $run = psphot_fullforce_summary.pl --ff_id $FF_ID --outroot $outroot --redirect-output --camera $CAMERA --threads @MAX_THREADS@
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
    # since we have work to do shorten exec time
    periods -exec .05
    command $run
  end

  # default exit status
  task.exit    default
    process_exit fullForceSummary $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword fullForceSummary $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword fullForceSummary $options:0 pantaskState TIMEOUT
  end
end

task fullforce.summary.revert
  host         local

  periods      -poll 10.0
  periods      -exec 1200.0
  periods      -timeout 120.0
  npending     1
  active true
  
  stdout NULL
  stderr $LOGDIR/fullforce.summary.revert.log

  task.exec
    if ($LABEL:n == 0) break
    # Only revert failures with fault=2 (SYS_ERROR), which tend to be
    # temporary filesystem problems.  Every other fault type is
    # interesting and should be kept for debugging (and so it does not
    # continue to occur).
    $run = fftool -revertsummary -fault 2
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$fullForceSummary_revert_DB
      $run = $run -dbname $DB:$fullForceSummary_revert_DB
      $fullForceSummary_revert_DB ++
      if ($fullForceSummary_revert_DB >= $DB:n) set fullForceSummary_revert_DB = 0
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

