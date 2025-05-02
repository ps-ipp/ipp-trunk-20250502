## staticsky.pro : tasks for staticsky photometry analysis : -*- sh -*-

## This file contains panTasks definitions for performing the
## staticsky photometry analysis.  After a staticsky entry (with
## associated sky_id) is defined, psphotStack is performed (via script
## staticsky.pl) (tasks in staticskyResult).

# test for required global variables
check.globals

### Initialise the books containing the tasks to do
book init staticskyResult

### Database lists
$staticsky_DB = 0
$staticsky_revert_DB = 0

### Check status of staticsky tasks
macro staticsky.status
  book listbook staticskyResult
end

### Reset staticsky tasks
macro staticsky.reset
  book init staticskyResult
end

### Turn staticsky tasks on
macro staticsky.on
  task staticsky.load
    active true
  end
  task staticsky.run
    active true
  end
  task staticsky.revert
    active false
  end
end

### Turn staticsky tasks off
macro staticsky.off
  task staticsky.load
    active false
  end
  task staticsky.run
    active false
  end
  task staticsky.revert
    active false
  end
end

macro staticsky.revert.on
  task staticsky.revert
    active true
  end
end

macro staticsky.revert.off
  task staticsky.revert
    active false
  end
end

$RA_POLL_MAX = 0

macro set.ra.max
    if ($0 != 2) 
        echo "USAGE: set.ra.max (ra_max_deg)"
        break
    end
    $RA_POLL_MAX = $1
end
macro get.ra.max
    echo $RA_POLL_MAX
end

### Load tasks for staticsky
### Tasks are loaded into staticskyResult.
task	       staticsky.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec 30
  periods      -timeout 60
  npending     1

  stdout NULL
  stderr $LOGDIR/staticsky.log

  task.exec
    if ($LABEL:n == 0) break
    $run = staticskytool -todo
    if ($RA_POLL_MAX > 0) 
        $run = $run -ra_max $RA_POLL_MAX
    end
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$staticsky_DB
      $run = $run -dbname $DB:$staticsky_DB -limit 40
      $staticsky_DB ++
      if ($staticsky_DB >= $DB:n) set staticsky_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout staticskyResult -key sky_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook staticskyResult
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup staticskyResult
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

### Run tasks for the staticsky analysis
### Tasks are taken from staticskyResult.
task	       staticsky.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 10800

  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC

    book npages staticskyResult -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    if ($BURNTOOLING == 1) break

    # look for new entries in staticskyResult (pantaskState == INIT)
    book getpage staticskyResult 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword staticskyResult $pageName pantaskState RUN
    book getword staticskyResult $pageName sky_id -var SKY_ID
    book getword staticskyResult $pageName tess_id -var TESS_DIR
    book getword staticskyResult $pageName skycell_id -var SKYCELL_ID
    book getword staticskyResult $pageName workdir -var WORKDIR_TEMPLATE
    book getword staticskyResult $pageName path_base -var PATH_BASE
    book getword staticskyResult $pageName reduction -var REDUCTION
    book getword staticskyResult $pageName dbname -var DBNAME
    book getword staticskyResult $pageName state -var RUN_STATE

    # move this above when we get camera into the db correctly:
    # book getword staticskyResult $pageName camera -var CAMERA
    $CAMERA = GPC1
    # $CAMERA = MEGACAM

    # set the host and workdir based on the skycell hash
    # set.host.for.skycell $SKYCELL_ID
    host anyhost

    if ("$PATH_BASE" == "NULL")
        set.workdir.by.skycell $SKYCELL_ID $WORKDIR_TEMPLATE $default_host WORKDIR
        basename $TESS_DIR -var TESS_ID
        sprintf outroot "%s/%s/%s/%s.%s.sky.%s" $WORKDIR $TESS_ID $SKYCELL_ID $TESS_ID $SKYCELL_ID $SKY_ID
    else
        # run-state must be update use existing path_base
        $outroot = $PATH_BASE
    end

    stdout $LOGDIR/staticsky.log
    stderr $LOGDIR/staticsky.log

    $run = staticsky.pl --sky_id $SKY_ID --run-state $RUN_STATE --outroot $outroot --redirect-output --camera $CAMERA  --threads @MAX_THREADS@
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
    # if we are able to run the 'exec', use a short retry time
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit    default
    process_exit staticskyResult $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword staticskyResult $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword staticskyResult $options:0 pantaskState TIMEOUT
  end
end

task staticsky.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1
  active false
  
  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    # Only revert failures with fault=2 (SYS_ERROR), which tend to be
    # temporary filesystem problems.  Every other fault type is
    # interesting and should be kept for debugging (and so it doesn't
    # continue to occur).
    $run = staticskytool -revert -fault 2
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$staticsky_revert_DB
      $run = $run -dbname $DB:$staticsky_revert_DB
      $staticsky_revert_DB ++
      if ($staticsky_revert_DB >= $DB:n) set staticsky_revert_DB = 0
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

