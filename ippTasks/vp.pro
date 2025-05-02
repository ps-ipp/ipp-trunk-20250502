## vp.pro : tasks for video photometry : -*- sh -*-

# test for required global variables
check.globals

$LOGSUBDIR = $LOGDIR/vp
mkdir $LOGSUBDIR

### Initialise the books containing the tasks to do
book init vpPendingRun

### Database lists
$vp_DB = 0
$vp_revert_DB = 0

$VP_POLL_LIMIT = 20

### Check status of vping tasks
macro vp.status
  book listbook vpPendingRun
end

### Reset vping tasks
macro vp.reset
  book init vpPendingRun
end

### Turn vp tasks on
macro vp.on
  task vp.load
    active true
  end
  task vp.run
    active true
  end
end

### Turn vp tasks off
macro vp.off
  task vp.load
    active false
  end
  task vp.run
    active false
  end
end

macro vp.revert.on
  task vp.revert
    active true
  end
end

macro vp.revert.off
  task vp.revert
    active false
  end
end

macro set.vp.poll
    $VP_POLL_LIMIT = $1
end
macro get.vp.poll
    echo $VP_POLL_LIMIT
end


### Load tasks for doing video photometry
### Tasks are loaded into vpPendingRun.
task	       vp.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGSUBDIR/vp.load.log

  task.exec
    if ($LABEL:n == 0) break
    $run = vptool -pendingrun
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$vp_DB
      $run = $run -dbname $DB:$vp_DB
      $vp_DB ++
      if ($vp_DB >= $DB:n) set vp_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    $run = $run -limit $VP_POLL_LIMIT
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout vpPendingRun -key vp_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook vpPendingRun
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup vpPendingRun
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




### Run video photometery tasks
### Tasks are taken from vpPendingRun.
task	       vp.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 300

  task.exec
    book npages vpPendingRun -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    if ($BURNTOOLING == 1) break

    # look for new runs in vpPendingRun (pantaskState == INIT)
    book getpage vpPendingRun 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword vpPendingRun $pageName pantaskState RUN
    book getword vpPendingRun $pageName vp_id -var VP_ID
    book getword vpPendingRun $pageName camera -var CAMERA
    book getword vpPendingRun $pageName exp_tag -var EXP_TAG
    book getword vpPendingRun $pageName workdir -var WORKDIR
    book getword vpPendingRun $pageName dest_id -var DEST_ID
    book getword vpPendingRun $pageName dest_name -var DEST_NAME
    book getword vpPendingRun $pageName ds_dbname -var DS_DBNAME
    book getword vpPendingRun $pageName ds_dbhost -var DS_DBHOST
    book getword vpPendingRun $pageName dbname -var DBNAME

    # set the host and workdir based on the skycell hash
#    set.host.for.skycell $SKYCELL_ID
#    set.workdir.by.skycell $SKYCELL_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    sprintf outroot "%s/%s.vp.%d" $WORKDIR $EXP_TAG $VP_ID

    stdout $LOGSUBDIR/vp.log
    stderr $LOGSUBDIR/vp.log

    host anyhost

    $run = videophot_process.pl --vp_id $VP_ID --outroot $outroot --redirect-output --camera $CAMERA 
    if ($DEST_ID) 
     $run = $run --dest_id $DEST_ID --product $DEST_NAME --ds_dbname $DS_DBNAME --ds_dbhost $DS_DBHOST
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
    process_exit vpPendingRun $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword vpPendingRun $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword vpPendingRun $options:0 pantaskState TIMEOUT
  end
end


task vp.revert
  host         local

  periods      -poll 5
  periods      -exec 600
  periods      -exec 30
  periods      -timeout 120
  npending     1
  
  stdout NULL
  stderr $LOGSUBDIR/vp.revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = vptool -revertrun
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$vp_revert_DB
      $run = $run -dbname $DB:$vp_revert_DB
      $vp_revert_DB ++
      if ($vp_revert_DB >= $DB:n) set vp_revert_DB = 0
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
