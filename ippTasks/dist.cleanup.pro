## dist.cleanup.pro : -*- sh -*-

check.globals

## dist.cleanup.pro : -*- sh -*-

book init distPendingCleanup

macro dist.cleanup.status
  book listbook distPendingCleanup
end

macro dist.cleanup.reset
  book init distPendingCleanup
end

macro dist.cleanup.on
  task dist.cleanup.load
    active true
  end
  task dist.cleanup.run
    active true
  end
end

macro dist.cleanup.off
  task dist.cleanup.load
    active false
  end
  task dist.cleanup.run
    active false
  end
end

# this variable will cycle through the known database names
$dist_cleanup_DB = 0

# select distRuns ready for cleanup

task	       dist.cleanup.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/dist.cleanup.load.log

  task.exec
    if ($LABEL:n == 0) break
    $run = disttool -pendingcleanup
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$dist_cleanup_DB
      $run = $run -dbname $DB:$dist_cleanup_DB
      $dist_cleanup_DB ++
      if ($dist_cleanup_DB >= $DB:n) set dist_cleanup_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout distPendingCleanup -key dist_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook distPendingCleanup
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup distPendingCleanup
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

# run the ipp_cleanup.pl script on pending images
task	       dist.cleanup.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 600

  host anyhost

  task.exec
    book npages distPendingCleanup -var N
    periods -exec $RUNEXEC
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in distPendingCleanup (pantaskState == INIT)
    book getpage distPendingCleanup 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break


    book setword distPendingCleanup $pageName pantaskState RUN
    book getword distPendingCleanup $pageName dist_id -var DIST_ID
    book getword distPendingCleanup $pageName stage -var STAGE
    book getword distPendingCleanup $pageName stage_id -var STAGE_ID
    book getword distPendingCleanup $pageName outdir -var OUTDIR
    book getword distPendingCleanup $pageName dbname -var DBNAME

    sprintf logfile "%s/dist.cleanup.%s.log" $OUTDIR $DIST_ID

    stdout $LOGDIR/dist.cleanup.log
    stderr $LOGDIR/dist.cleanup.log

    $run = dist_cleanup.pl --dist_id $DIST_ID --stage $STAGE --stage_id $STAGE_ID --outdir $OUTDIR --logfile $logfile
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
    process_exit distPendingCleanup $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword distPendingCleanup $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword distPendingCleanup $options:0 pantaskState TIMEOUT
  end
end

