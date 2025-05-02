## lapgroup.pro : tasks for lap group management : -*- sh -*-

# test for required global variables
check.globals

$LOGSUBDIR = $LOGDIR/lapgroup
mkdir $LOGSUBDIR

### Initialise the books containing the tasks to do
book init pendinglapGroup

### Database lists
$lapgroup_DB = 0

### Check status of lapgroup tasks
macro lapgroup.status
  book listbook pendinglapGroup
end

### Reset lapgroup tasks
macro lapgroup.reset
  book init pendinglapGroup
end

### Turn lapgroup tasks on
macro lapgroup.on
  task lapgroup.load
    active true
  end
  task lapgroup.run
    active true
  end
end

### Turn lapgroup tasks off
macro lapgroup.off
  task lapgroup.load
    active false
  end
  task lapgroup.run
    active false
  end
end

### Load jobs for lapGroup
### Tasks are loaded into pendinglapGroup.
task	       lapgroup.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec 30
  periods      -timeout 60
  npending     1

  stdout NULL
  stderr $LOGSUBDIR/lapgroup.log

  task.exec
    if ($LABEL:n == 0) break
    $run = laptool -pendinggroup
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$lapgroup_DB
      $run = $run -dbname $DB:$lapgroup_DB
      $lapgroup_DB ++
      if ($lapgroup_DB >= $DB:n) set lapgroup_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout pendinglapGroup -key seq_id:tess_id:projection_cell -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook pendinglapGroup
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup pendinglapGroup
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

task	       lapgroup.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 120
  npending     1

  task.exec
    book npages pendinglapGroup -var N
    if ($N == 0) break
    if ($NETWORK == 0) break

    # look for new entries in pendinglapGroup (pantaskState == INIT)
    book getpage pendinglapGroup 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword pendinglapGroup $pageName pantaskState RUN
    book getword pendinglapGroup $pageName seq_id -var SEQ_ID
    book getword pendinglapGroup $pageName tess_id -var TESS_ID
    book getword pendinglapGroup $pageName projection_cell -var PROJECTION_CELL
    book getword pendinglapGroup $pageName label -var LABEL
    book getword pendinglapGroup $pageName dist_group -var DIST_GROUP
    book getword pendinglapGroup $pageName dbname -var DBNAME

    stdout $LOGSUBDIR/lapgroup.log
    stderr $LOGSUBDIR/lapgroup.log

    host anyhost

    $run = queuestaticsky.pl --seq_id $SEQ_ID --tess_id $TESS_ID --projection_cell $PROJECTION_CELL --label $LABEL --dist_group $DIST_GROUP
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
    process_exit pendinglapGroup $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword pendinglapGroup $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword pendinglapGroup $options:0 pantaskState TIMEOUT
  end
end

