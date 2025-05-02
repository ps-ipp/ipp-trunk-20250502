## detrend.stack.pro : globals and support macros : -*- sh -*-
## this file contains the tasks for running the detrend stacking stage
## these tasks use the book detPendingStackedImfile

# test for required global variables
check.globals

book init detPendingStackedImfile

macro detstack.reset
  book init detPendingStackedImfile
end

macro detstack.status
  echo detPendingStackedImfile
  book listbook detPendingStackedImfile
end

macro detstack.on
  task detrend.stack.load
    active true
  end
  task detrend.stack.run
    active true
  end
end

macro detstack.off
  task detrend.stack.load
    active false
  end
  task detrend.stack.run
    active false
  end
end

macro detstack.revert.off
  task detrend.stack.revert
    active false
  end
end

macro detstack.revert.on
  task detrend.stack.revert
    active true
  end
end

# this variable will cycle through the known database names
$detPendingStackedImfile_DB = 0
$detPendingStackedImfile_revert_DB = 0

# select images ready for detrend_stack.pl
# new entries are added to detPendingStackedImfile
# compare the new list with the ones already selected
task	       detrend.stack.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/detrend.stack.log

  task.exec
    $run = dettool -tostacked
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detPendingStackedImfile_DB
      $run = $run -dbname $DB:$detPendingStackedImfile_DB
      $detPendingStackedImfile_DB ++
      if ($detPendingStackedImfile_DB >= $DB:n) set detPendingStackedImfile_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout detPendingStackedImfile -key det_id:iteration:class_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook detPendingStackedImfile
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup detPendingStackedImfile
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

# copy new images, sending job to desired host
task	       detrend.stack.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 30

  task.exec
    book npages detPendingStackedImfile -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in detPendingStackedImfile
    book getpage detPendingStackedImfile 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword detPendingStackedImfile $pageName pantaskState RUN
    book getword detPendingStackedImfile $pageName det_id    -var DET_ID
    book getword detPendingStackedImfile $pageName iteration -var ITERATION
    book getword detPendingStackedImfile $pageName det_type  -var DET_TYPE
    book getword detPendingStackedImfile $pageName class_id  -var CLASS_ID
    book getword detPendingStackedImfile $pageName camera    -var CAMERA  
    book getword detPendingStackedImfile $pageName workdir   -var WORKDIR_TEMPLATE
    book getword detPendingStackedImfile $pageName dbname    -var DBNAME
    book getword detPendingStackedImfile $pageName reduction -var REDUCTION

    # specify choice of local or remote host based on camera and chip (class_id)
    set.host.for.camera $CAMERA $CLASS_ID

    # set workdir (interpolate host; see camera.pro for examples)
    set.workdir.by.camera $CAMERA FPA $WORKDIR_TEMPLATE $default_host WORKDIR

    ## generate outroot specific to this exposure
    sprintf outroot "%s/%s.%s.%s/%s.%s.%s.%s" $WORKDIR $CAMERA $DET_TYPE $DET_ID $CAMERA $DET_TYPE $DET_ID $ITERATION

    stdout $LOGDIR/detrend.stack.log
    stderr $LOGDIR/detrend.stack.log

    $run = detrend_stack.pl --threads @MAX_THREADS@ --det_id $DET_ID --iteration $ITERATION --class_id $CLASS_ID --det_type $DET_TYPE --camera $CAMERA --outroot $outroot --redirect-output
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
    process_exit detPendingStackedImfile $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword detPendingStackedImfile $options:0 pantaskState CRASH
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
    book setword detPendingStackedImfile $options:0 pantaskState TIMEOUT
  end
end


task detrend.stack.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
  
    $run = dettool -revertstacked -all-run 
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detPendingStackedImfile_revert_DB
      $run = $run -dbname $DB:$detPendingStackedImfile_revert_DB
      $detPendingStackedImfile_revert_DB ++
      if ($detPendingStackedImfile_revert_DB >= $DB:n) set detPendingStackedImfile_revert_DB = 0
    end
    echo $run
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
