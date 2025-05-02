## diffphot.pro: (re-)photometry of diffs : -*- sh -*-

# test for required global variables
check.globals

### Initialise the books containing the tasks to do
book init diffphotRuns

### Database lists
$diffphotLoad_DB = 0
$diffphotAdvance_DB = 0
$diffphotRevert_DB = 0

### Check status of diffing tasks
macro diffphot.status
  book listbook diffphotRuns
end

### Reset diffing tasks
macro diffphot.reset
  book init diffphotRuns
end

### Turn diffing tasks on
macro diffphot.on
  task diffphot.load
    active true
  end
  task diffphot.run
    active true
  end
  task diffphot.advance
    active true
  end
  task diffphot.revert
    active false
  end
end

### Turn diffing tasks off
macro diffphot.off
  task diffphot.load
    active false
  end
  task diffphot.run
    active false
  end
  task diffphot.advance
    active false
  end
  task diffphot.revert
    active false
  end
end

macro diffphot.revert.on
  task diffphot.revert
    active true
  end
end

macro diffphot.revert.off
  task diffphot.revert
    active false
  end
end


# Load processing jobs
task	       diffphot.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/diffphot.load.log

  task.exec
    if ($LABEL:n == 0) break
    $run = diffphottool -pending
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$diffphotLoad_DB
      $run = $run -dbname $DB:$diffphotLoad_DB
      $diffphotLoad_DB ++
      if ($diffphotLoad_DB >= $DB:n) set diffphotLoad_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout diffphotRuns -key diff_phot_id:skycell_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook diffphotRuns
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup diffphotRuns
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

### Run tasks for calculating the diff overlaps
### Tasks are taken from diffSkyfile.
task	       diffphot.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC

    book npages diffphotRuns -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    if ($BURNTOOLING == 1) break

    book getpage diffphotRuns 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword diffphotRuns $pageName pantaskState RUN
    book getword diffphotRuns $pageName diff_phot_id -var DIFF_PHOT_ID
    book getword diffphotRuns $pageName skycell_id -var SKYCELL_ID
    book getword diffphotRuns $pageName tess_id -var TESS_DIR
    book getword diffphotRuns $pageName workdir -var WORKDIR_TEMPLATE
    book getword diffphotRuns $pageName state -var RUN_STATE
    book getword diffphotRuns $pageName dbname -var DBNAME
    book getword diffphotRuns $pageName reduction -var REDUCTION

    # set the host and workdir based on the skycell hash
    set.host.for.skycell $SKYCELL_ID
    set.workdir.by.skycell $SKYCELL_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    basename $TESS_DIR -var TESS_ID
    sprintf outroot "%s/%s/%s/%s.%s.dp.%s" $WORKDIR $TESS_ID $SKYCELL_ID $TESS_ID $SKYCELL_ID $DIFF_PHOT_ID

    stdout $LOGDIR/diffphot.run.log
    stderr $LOGDIR/diffphot.run.log

    $run = diffphot.pl --threads @MAX_THREADS@ --diff_phot_id $DIFF_PHOT_ID --skycell_id $SKYCELL_ID --outroot $outroot --redirect-output
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
    process_exit diffphotRuns $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword diffphotRuns $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword diffphotRuns $options:0 pantaskState TIMEOUT
  end
end


# Advance runs which have completed
task	       diffphot.advance
  host         local

  periods      -poll $LOADPOLL
#  periods      -exec $LOADEXEC
  periods      -exec 30
  periods      -timeout 60
  npending     1

  stdout NULL
  stderr $LOGDIR/diffphot.advance.log

  task.exec
    if ($LABEL:n == 0) break
    $run = diffphottool -advance
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$diffphotAdvance_DB
      $run = $run -dbname $DB:$diffphotAdvance_DB
      $diffphotAdvance_DB ++
      if ($diffphotAdvance_DB >= $DB:n) set diffphotAdvance_DB = 0
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


# Revert runs that failed
task diffphot.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1
  active false

  stdout NULL
  stderr $LOGDIR/diffphot.revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = diffphottool -revert
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$diffphotRevert_DB
      $run = $run -dbname $DB:$diffphotRevert_DB
      $diffphotRevert_DB ++
      if ($diffphotRevert_DB >= $DB:n) set diffphotRevert_DB = 0
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
