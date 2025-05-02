## release.pro : tasks for release management : -*- sh -*-

# test for required global variables
check.globals

$LOGSUBDIR = $LOGDIR/release
mkdir $LOGSUBDIR

### Initialise the books containing the tasks to do
book init pendingrelGroup

### Database lists
$relgroup_DB = 0

### Check status of release tasks
macro release.status
  book listbook pendingrelGroup
end

### Reset release tasks
macro release.reset
  book init pendingrelgroup
end

### Turn release tasks on
macro release.on
  task relgroup.load
    active true
  end
  task relgroup.run
    active true
  end
end

### Turn release tasks off
macro release.off
  task relgroup.load
    active false
  end
  task relgroup.run
    active false
  end
end

### Load jobs for relGroup
### Tasks are loaded into pendingrelGroup.
task	       relgroup.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec 30
  periods      -timeout 60
  npending     1

  stdout NULL
  stderr $LOGSUBDIR/relgroup.log

  task.exec
    if ($LABEL:n == 0) break
    $run = releasetool -pendingrelgroup
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$relgroup_DB
      $run = $run -dbname $DB:$relgroup_DB
      $relgroup_DB ++
      if ($relgroup_DB >= $DB:n) set relgroup_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout pendingrelGroup -key group_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook pendingrelGroup
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup pendingrelGroup
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

task	       relgroup.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 120
  npending     1

  task.exec
    book npages pendingrelGroup -var N
    if ($N == 0) break
    if ($NETWORK == 0) break

    # look for new entries in pendingrelGroup (pantaskState == INIT)
    book getpage pendingrelGroup 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword pendingrelGroup $pageName pantaskState RUN
    book getword pendingrelGroup $pageName group_id -var GROUP_ID
    book getword pendingrelGroup $pageName rel_id -var REL_ID
    book getword pendingrelGroup $pageName group_type -var GROUP_TYPE
    book getword pendingrelGroup $pageName lap_id -var LAP_ID
    book getword pendingrelGroup $pageName group_name -var GROUP_NAME
    book getword pendingrelGroup $pageName release_name -var RELEASE_NAME
    book getword pendingrelGroup $pageName dbname -var DBNAME

    stdout $LOGSUBDIR/relgroup.log
    stderr $LOGSUBDIR/relgroup.log

    host anyhost

    $run = relgroup_exp_list.pl --group_id $GROUP_ID --group_type $GROUP_TYPE --release_name $RELEASE_NAME --rebuild
    if ("$GROUP_TYPE" == "lap") 
        $run = $run --lap_id $LAP_ID
    else 
        $run = $run --group_name $GROUP_NAME
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
    process_exit pendingrelGroup $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword pendingrelGroup $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword pendingrelGroup $options:0 pantaskState TIMEOUT
  end
end

