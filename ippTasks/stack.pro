## stack.pro : tasks for image stacking : -*- sh -*-

## This file contains panTasks definitions for performing the image stacking.
## After a stack (with associated stack_id) is defined, the stack is performed
## (tasks in stackSumSkyfile).

# test for required global variables
check.globals

if ($?POLL_LIMIT_STACK == 0) set POLL_LIMIT_STACK = 40

macro set.stack.poll
  if ($0 != 2)
    echo "USAGE:set.stack.poll (value)"
    break
  end

  $POLL_LIMIT_STACK = $1
end
macro get.stack.poll
  echo $POLL_LIMIT_STACK
end

### Initialise the books containing the tasks to do
book init stackSumSkyfile
book init stackPendingSummary

### Database lists
if (not($?stackSkycell_DB)) set stackSkycell_DB = 0
if (not($?stack_revert_DB)) set stack_revert_DB = 0
if (not($?stackSummary_DB)) set stackSummary_DB = 0

if (not($?stackIgnoreWarpState))
  $stackIgnoreWarpState = 
end

# is this name ridiculous?
macro stack.ignore.warp.stack.set
  if ($0 != 2)
    echo "USAGE: stack.ignore.warp.stack.set (on/off)"
    break
  end

  if ("$1" == "on")
    $stackIgnoreWarpState = -ignore-warp-state
  else
    $stackIgnoreWarpState = 
  end
  stack.ignore.warp.stack.get
end
macro stack.ignore.warp.stack.get
  if ("$stackIgnoreWarpState" == "-ignore-warp-state")
    echo "stackIgnoreWarpState is now ON : $stackIgnoreWarpState"
    return
  end
  if ("$stackIgnoreWarpState" == "")
    echo "stackIgnoreWarpState is now OFF : $stackIgnoreWarpState"
    return
  end

  echo "stackIgnoreWarpState has an unexpected value : $stackIgnoreWarpState"
end

### Check status of stacking tasks
macro stack.status
  book listbook stackSumSkyfile
end

### Reset stacking tasks
macro stack.reset
  book init stackSumSkyfile
  book init stackPendingSummary
end

### Turn stacking tasks on
macro stack.on
  task stack.skycell.load
    active true
  end
  task stack.skycell.run
    active true
  end
  task stack.revert
    active false
  end
end

### Turn stacking tasks off
macro stack.off
  task stack.skycell.load
    active false
  end
  task stack.skycell.run
    active false
  end
  task stack.revert
    active false
  end
end

# Same for summary stages.
macro stack.summary.on
  task stack.summary.load
    active true
  end
  task stack.summary.run
    active true
  end
end

macro stack.summary.off
  task stack.summary.load
    active false
  end
  task stack.summary.run
    active false
  end
end

macro stack.revert.on
  task stack.revert
    active true
  end
end

macro stack.revert.off
  task stack.revert
    active false
  end
end


### Load tasks for doing the stack
### Tasks are loaded into stackSumSkyfile.
task	       stack.skycell.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/stack.skycell.log

  task.exec
    if ($LABEL:n == 0) break
    $run = stacktool -tosum $stackIgnoreWarpState
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$stackSkycell_DB
      $run = $run -dbname $DB:$stackSkycell_DB
      $stackSkycell_DB ++
      if ($stackSkycell_DB >= $DB:n) set stackSkycell_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    # change the limit (the last one on the command line takes precedence)
    $run = $run -limit $POLL_LIMIT_STACK
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout stackSumSkyfile -key stack_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook stackSumSkyfile
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup stackSumSkyfile
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




### Run tasks for calculating the stack overlaps
### Tasks are taken from stackSumSkyfile.
task	       stack.skycell.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 10800

  task.exec
    book npages stackSumSkyfile -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    if ($BURNTOOLING == 1) break

    # look for new images in stackSumSkyfile (pantaskState == INIT)
    book getpage stackSumSkyfile 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword stackSumSkyfile $pageName pantaskState RUN
    book getword stackSumSkyfile $pageName stack_id -var STACK_ID
    book getword stackSumSkyfile $pageName tess_id -var TESS_DIR
    book getword stackSumSkyfile $pageName skycell_id -var SKYCELL_ID
    book getword stackSumSkyfile $pageName workdir -var WORKDIR_TEMPLATE
    book getword stackSumSkyfile $pageName path_base -var PATH_BASE
    book getword stackSumSkyfile $pageName reduction -var REDUCTION
    book getword stackSumSkyfile $pageName dbname -var DBNAME
    book getword stackSumSkyfile $pageName state -var RUN_STATE

    # set the host and workdir based on the skycell hash
    set.host.for.skycell $SKYCELL_ID
    set.workdir.by.skycell $SKYCELL_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    # XXX old code:
    # host anyhost
    # $WORKDIR = $WORKDIR_TEMPLATE

    basename $TESS_DIR -var TESS_ID
    if ("$PATH_BASE" == "NULL")
        sprintf outroot "%s/%s/%s/%s.%s.stk.%s" $WORKDIR $TESS_ID $SKYCELL_ID $TESS_ID $SKYCELL_ID $STACK_ID
    else
        $outroot = $PATH_BASE
    end

    stdout $LOGDIR/stack.skycell.log
    stderr $LOGDIR/stack.skycell.log

    $run = stack_skycell.pl --threads @MAX_THREADS@ --stack_id $STACK_ID --outroot $outroot --redirect-output --run-state $RUN_STATE
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
    command $run
  end

  # default exit status
  task.exit    default
    process_exit stackSumSkyfile $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword stackSumSkyfile $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword stackSumSkyfile $options:0 pantaskState TIMEOUT
  end
end


task stack.revert
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
    $run = stacktool -revertsumskyfile -fault 2
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$stack_revert_DB
      $run = $run -dbname $DB:$stack_revert_DB
      $stack_revert_DB ++
      if ($stack_revert_DB >= $DB:n) set stack_revert_DB = 0
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

### Load tasks for doing the stack summary
### Tasks are loaded into stackPendingSummary
task           stack.summary.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 1200
  active       false
#  trange       07:00:00 08:00:00 -nmax 1
  npending     1

  stdout       NULL
  stderr       $LOGDIR/stack.summary.load

  task.exec
    if ($LABEL:n == 0) break
    $run = stacktool -tosummary
    if ($DB:n == 0) 
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$stackSummary_DB
      $run = $run -dbname $DB:$stackSummary_DB
      $stackSummary_DB ++
      if ($stackSummary_DB >= $DB:n) set stackSummary_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit     0
    # convert 'stdout' to book format
    ipptool2book stdout stackPendingSummary -key sass_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook stackPendingSummary
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup stackPendingSummary
  end

  # locked list
  task.exit     default
    showcommand failure
  end

  task.exit     crash
    showcommand crash
  end

  # operation times out?
  task.exit     timeout
    showcommand timeout
  end
end

### Run tasks for doing the stack summary
task           stack.summary.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60
  active       false

  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC

    book npages stackPendingSummary -var N
    if ($N == 0) break
    if ($NETWORK == 0) break

    # look for new images in stackPendingSummary (pantaskState == INIT)
    book getpage stackPendingSummary 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword stackPendingSummary $pageName pantaskState RUN
    book getword stackPendingSummary $pageName sass_id -var SASS_ID
    book getword stackPendingSummary $pageName camera -var CAMERA
    book getword stackPendingSummary $pageName workdir -var WORKDIR_TEMPLATE
    book getword stackPendingSummary $pageName dbname -var DBNAME
    book getword stackPendingSummary $pageName tess_id -var TESS_DIR
    book getword stackPendingSummary $pageName state -var RUN_STATE
    book getword stackPendingSummary $pageName projection_cell -var projection_cell

    # set the host and workdir
    host anyhost
    strsub $WORKDIR_TEMPLATE @HOST@.0 $default_host -var WORKDIR

    basename $TESS_DIR -var TESS_ID

    ## generate outroot specific to this association
    sprintf outroot "%s/%s/%s.stk.%s.summary" $WORKDIR $TESS_ID $TESS_ID $SASS_ID

    stdout $LOGDIR/stack.summary.log
    stderr $LOGDIR/stack.summary.log

    $run = skycell_jpeg.pl --stage stack --stage_id $SASS_ID --camera $CAMERA --outroot $outroot
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
  task.exit       default
    process_exit stackPendingSummary $options:0 $JOB_STATUS
  end

  # locked list
  task.exit       crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword stackPendingSummary $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit       timeout
    showcommand timeout
    book setword stackPendingSummary $options:0 pantaskState TIMEOUT
  end
end

