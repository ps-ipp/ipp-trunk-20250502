## detrend.reject.pro : globals and support macros : -*- sh -*-
## this file contains the tasks for running the detrend processing stage
## these tasks use the book detRejectExp

# test for required global variables
check.globals

book init detRejectExp

macro detreject.reset
  book init detRejectExp
end

macro detreject.status
  book listbook detRejectExp
end

macro detreject.on
  task detrend.reject.load
    active true
  end
  task detrend.reject.run
    active true
  end
end

macro detreject.off
  task detrend.reject.load
    active false
  end
  task detrend.reject.run
    active false
  end
end

# this variable will cycle through the known database names
$detRejectExp_DB = 0

# select images ready for copy 
# new entries are added to detRejectExp
# compare the new list with the ones already selected
task	       detrend.reject.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/detrend.reject.log

  task.exec
    $run = dettool -todetrunsummary
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detRejectExp_DB
      $run = $run -dbname $DB:$detRejectExp_DB
      $detRejectExp_DB ++
      if ($detRejectExp_DB >= $DB:n) set detRejectExp_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout detRejectExp -key det_id:iteration -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook detRejectExp
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup detRejectExp
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
task	       detrend.reject.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 30

  task.exec
    book npages detRejectExp -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in detRejectExp
    book getpage detRejectExp 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword detRejectExp $pageName pantaskState RUN
    book getword detRejectExp $pageName det_id    -var DET_ID   
    book getword detRejectExp $pageName iteration -var ITERATION     
    book getword detRejectExp $pageName det_type  -var DET_TYPE 
    book getword detRejectExp $pageName mode      -var MODE     
    book getword detRejectExp $pageName camera    -var CAMERA   
    book getword detRejectExp $pageName workdir   -var WORKDIR_TEMPLATE
    book getword detRejectExp $pageName dbname    -var DBNAME

    # specify choice of local or remote host based on camera and chip (class_id)
    set.host.for.camera $CAMERA FPA

    # set workdir (interpolate host; see camera.pro for examples)
    set.workdir.by.camera $CAMERA FPA $WORKDIR_TEMPLATE $default_host WORKDIR

    ## generate outroot specific to this exposure (& chip)
    sprintf outroot "%s/%s.%s.%s/%s.%s.%s.%s.detreject" $WORKDIR $CAMERA $DET_TYPE $DET_ID $CAMERA $DET_TYPE $DET_ID $ITERATION

    stdout $LOGDIR/detrend.reject.log
    stderr $LOGDIR/detrend.reject.log

    $run = detrend_reject_exp.pl --det_id $DET_ID --iteration $ITERATION --det_type $DET_TYPE --camera $CAMERA --outroot $outroot --redirect-output
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create example job options as a demonstration
    if ($VERBOSE > 1)
      echo command $run
    end
    command $run
 end

  # default exit status
  task.exit    default
    process_exit detRejectExp $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword detRejectExp $options:0 pantaskState CRASH
  end

  # operation times out?
  task.exit    timeout
    showcommand failure
    book setword detRejectExp $options:0 pantaskState TIMEOUT
  end
end
