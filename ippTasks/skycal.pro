## skycal.pro : tasks for static sky calibration analysis : -*- sh -*-

## This file contains panTasks definitions for performing the
## skycal analysis.  After a skycalRun entry (with
## associated skycal_id) is defined, psastro is performed (via script
## skycalibration.pl) (tasks in skycalRun).

# test for required global variables
check.globals

# skcal.pro should have a more restricted polling limit (to avoid stress with getstar)
# XXX: is this necessary anymore?
if ($?POLL_LIMIT_SKYCAL == 0) set POLL_LIMIT_SKYCAL = 64

macro set.skycal.poll
  if ($0 != 2)
    echo "USAGE:set.skycal.poll (value)"
    break
  end
 
  $POLL_LIMIT_SKYCAL = $1
end

macro get.skycal.poll
  echo "skycal poll limit: $POLL_LIMIT_SKYCAL"
end

### Initialise the books containing the tasks to do
book init skycalRun

### Database lists
$skycal_DB = 0
$skycal_revert_DB = 0

### Check status of skycal tasks
macro skycal.status
  book listbook skycalRun
end

### Reset skycal tasks
macro skycal.reset
  book init skycalRun
end

### Turn skycal tasks on
macro skycal.on
  task skycal.load
    active true
  end
  task skycal.run
    active true
  end
end

### Turn skycal tasks off
macro skycal.off
  task skycal.load
    active false
  end
  task skycal.run
    active false
  end
end

macro skycal.revert.on
  task skycal.revert
    active true
  end
end

macro skycal.revert.off
  task skycal.revert
    active false
  end
end

### Load tasks for skycal
### Tasks are loaded into skycalRun.
task	       skycal.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/skycal.log

  task.exec
    if ($LABEL:n == 0) break
    $run = staticskytool -pendingskycalrun
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$skycal_DB
      $run = $run -dbname $DB:$skycal_DB
      $skycal_DB ++
      if ($skycal_DB >= $DB:n) set skycal_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    $run = $run -limit $POLL_LIMIT_SKYCAL
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout skycalRun -key skycal_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook skycalRun
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup skycalRun
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

### Run tasks for the skycal analysis
### Tasks are taken from skycalRun.
task	       skycal.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 10800

  task.exec
    # if we are unable to run use "long" exectime
    periods -exec $RUNEXEC
    book npages skycalRun -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    if ($BURNTOOLING == 1) break


    # look for new entries in skycalRun (pantaskState == INIT)
    book getpage skycalRun 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword skycalRun $pageName pantaskState RUN
    book getword skycalRun $pageName skycal_id -var SKYCAL_ID
    book getword skycalRun $pageName stack_id -var STACK_ID
    book getword skycalRun $pageName tess_id -var TESS_DIR
    book getword skycalRun $pageName skycell_id -var SKYCELL_ID
    book getword skycalRun $pageName camera -var CAMERA
    book getword skycalRun $pageName filter -var FILTER
    book getword skycalRun $pageName state -var RUN_STATE
    book getword skycalRun $pageName workdir -var WORKDIR_TEMPLATE
    book getword skycalRun $pageName path_base -var PATH_BASE
    book getword skycalRun $pageName reduction -var REDUCTION
    book getword skycalRun $pageName singlefilter -var SINGLEFILTER
    book getword skycalRun $pageName dbname -var DBNAME
    book getword skycalRun $pageName state -var RUN_STATE

    # set the host and workdir based on the skycell hash
    # set.host.for.skycell $SKYCELL_ID
    host anyhost
    set.workdir.by.skycell $SKYCELL_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    basename $TESS_DIR -var TESS_ID
    sprintf outroot "%s/%s/%s/%s.%s.stk.%s.skycal.%s" $WORKDIR $TESS_ID $SKYCELL_ID $TESS_ID $SKYCELL_ID $STACK_ID $SKYCAL_ID

    stdout $LOGDIR/skycal.log
    stderr $LOGDIR/skycal.log

    $run = skycalibration.pl --skycal_id $SKYCAL_ID --outroot $outroot --redirect-output --camera $CAMERA --path_base $PATH_BASE --stack_id $STACK_ID --filter $FILTER --run-state $RUN_STATE
    if ("$REDUCTION" != "NULL")
      $run = $run --reduction $REDUCTION
    end
    if ("$SINGLEFILTER" != "0")
        $run = $run --singlefilter
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
    process_exit skycalRun $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword skycalRun $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword skycalRun $options:0 pantaskState TIMEOUT
  end
end

task skycal.revert
  host         local

  periods      -poll 10.0
  periods      -exec 1200.0
  periods      -timeout 120.0
  npending     1
  active true
  
  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    # Only revert failures with fault=2 (SYS_ERROR), which tend to be
    # temporary filesystem problems.  Every other fault type is
    # interesting and should be kept for debugging (and so it does not
    # continue to occur).
    $run = staticskytool -revertskycal -fault 2
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$skycal_revert_DB
      $run = $run -dbname $DB:$skycal_revert_DB
      $skycal_revert_DB ++
      if ($skycal_revert_DB >= $DB:n) set skycal_revert_DB = 0
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

