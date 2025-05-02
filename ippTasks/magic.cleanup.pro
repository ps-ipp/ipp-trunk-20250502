## magic.cleanup.pro : support for the cleaning magic temporary files : -*- sh -*-

# test for required global variables
check.globals

$LOGSUBDIR = $LOGDIR/magic
mkdir $LOGSUBDIR

### Initialise the books containing the tasks to do
book init magicToCleanup

### Database lists
$magicToCleanup_DB = 0

### Check status of tasks
macro magic.cleanup.status
  book listbook magicToCleanup
end

### Reset tasks
macro magic.cleanup.reset
  book init magicToCleanup
end

### Turn tasks on
macro magic.cleanup.revert.on
  task magic.cleanup.revert
    active true
  end
end

macro magic.cleanup.on
  task magic.cleanup.load
    active true
  end
  task magic.cleanup.run
    active true
  end
end

macro magic.cleanup.revert.off
  task magic.cleanup.revert
    active false
  end
end

### Turn tasks off
macro magic.cleanup.off
  task magic.cleanup.load
    active false
  end
  task magic.cleanup.run
    active false
  end
end

task	       magic.cleanup.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/magic.cleanup.load.log

  task.exec
    $run = magictool -tocleanup
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$magicToCleanup_DB
      $run = $run -dbname $DB:$magicToCleanup_DB
      $magicToCleanup_DB ++
      if ($magicToCleanup_DB >= $DB:n) set magicToCleanup_DB = 0
    end
    add_poll_args run
    # XXX: add this back in once our inital cleanup is done
#    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout magicToCleanup -key magic_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook magicToCleanup
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup magicToCleanup
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

task	       magic.cleanup.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 300

  task.exec
    periods -exec $RUNEXEC

    book npages magicToCleanup -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    book getpage magicToCleanup 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword magicToCleanup $pageName pantaskState RUN
    book getword magicToCleanup $pageName magic_id -var MAGIC_ID
    book getword magicToCleanup $pageName exp_id -var EXP_ID
    book getword magicToCleanup $pageName camera -var CAMERA
    book getword magicToCleanup $pageName workdir -var WORKDIR
    book getword magicToCleanup $pageName dbname -var DBNAME

    host anyhost

    sprintf outroot "%s/%s/%s.mgc.%s" $WORKDIR $EXP_ID $EXP_ID $MAGIC_ID

    sprintf logfile "%s.cleanup.log" $outroot

    stdout $logfile
    stderr $logfile

    $run = magic_cleanup.pl --magic_id $MAGIC_ID --exp_id $EXP_ID --camera $CAMERA --outroot $outroot --logfile $logfile
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
    process_exit magicToCleanup $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword magicToCleanup $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword magicToCleanup $options:0 pantaskState TIMEOUT
  end
end

task magic.cleanup.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1
  active false

  stdout NULL
  stderr $LOGSUBDIR/revertnode.log

  task.exec
    if ($LABEL:n == 0) break
    $run = magictool -revertnode
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$magicRevertNode_DB
      $run = $run -dbname $DB:$magicRevertNode_DB
      $magicRevertNode_DB ++
      if ($magicRevertNode_DB >= $DB:n) set magicRevertNode_DB = 0
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
