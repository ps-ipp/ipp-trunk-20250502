## destreak.cleanup.pro : tasks for the cleaning up destreak stage : -*- sh -*-

# test for required global variables
check.globals

$LOGSUBDIR = $LOGDIR/destreak
mkdir $LOGSUBDIR

### Initialise the books containing the tasks to do
book init magicDSToCleanup

### indexes into Database lists
$magicDSToCleanup_DB = 0

### Check status of tasks
macro destreak.cleanup.status
    book listbook magicDSToCleanup
end

### Reset tasks
macro destreak.cleanup.reset
    book init magicDSToCleanup
end

### Turn tasks on
macro destreak.cleanup.on
    task destreak.cleanup.load
        active true
    end
    task destreak.cleanup.run
        active true
    end
end

### Turn tasks off
macro destreak.cleanup.off
    task destreak.cleanup.load
        active false
    end
    task destreak.cleanup.run
        active false
    end
end

task	       destreak.cleanup.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 20
  npending     1

  stdout NULL
  stderr $LOGSUBDIR/destreak.cleanup.load.log

  task.exec
    $run = magicdstool -tocleanup
    if ($DB:n == 0)
      option DEFAULT
    else

      # save the DB name for the exit tasks
      option $DB:$magicDSToCleanup_DB
      $run = $run -dbname $DB:$magicDSToCleanup_DB

      # only bump database number after we have gone through all of the stages
      $magicDSToCleanup_DB ++
      if ($magicDSToCleanup_DB >= $DB:n) set magicDSToCleanup_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout magicDSToCleanup -key magic_ds_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook magicDSToCleanup
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup magicDSToCleanup
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

task	       destreak.cleanup.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    stdout $LOGSUBDIR/destreak.cleanup.run.log
    stderr $LOGSUBDIR/destreak.cleanup.run.log

    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC

    book npages magicDSToCleanup -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images (pantaskState == INIT)
    book getpage magicDSToCleanup 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword magicDSToCleanup $pageName pantaskState RUN
    book getword magicDSToCleanup $pageName magic_ds_id -var MAGIC_DS_ID
    book getword magicDSToCleanup $pageName camera -var CAMERA
    book getword magicDSToCleanup $pageName stage -var STAGE
    book getword magicDSToCleanup $pageName outroot -var OUTROOT
    book getword magicDSToCleanup $pageName dbname -var DBNAME

    sprintf logfile "%s/mds.%s.cleanup.log" $OUTROOT $MAGIC_DS_ID

    host anyhost

    $run = magic_destreak_cleanup.pl --magic_ds_id $MAGIC_DS_ID --stage $STAGE --camera $CAMERA --logfile $logfile

    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    # if we are ready to run, drop the retry timeout low so we fill up the queue
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit    0
    process_exit magicDSToCleanup $options:0 $JOB_STATUS
   end

  # locked list
  task.exit    default
    showcommand failure
    process_exit magicDSToCleanup $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword magicDSToCleanup $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword magicDSToCleanup $options:0 pantaskState TIMEOUT
  end
end
