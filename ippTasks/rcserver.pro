## rcserver.pro : tasks for managing the server side of the ipp data distribution system : -*- sh -*-

# test for required global variables
check.globals

$LOGSUBDIR = $LOGDIR/rcserver
mkdir $LOGSUBDIR

### Initialise the books containing the tasks to do
book init rcPendingFS

### Database lists
$rcPendingFS_DB = 0
$rcRevertFS_DB = 0

### Check status of tasks
macro rcserver.status
  book listbook rcPendingFS
end

### Reset tasks
macro rcserver.reset
  book init rcPendingFS
end

### Turn tasks on
macro rcserver.on
  task rcserver.makefileset.load
    active true
  end
  task rcserver.makefileset.run
    active true
  end
end
macro rcserver.off
  task rcserver.makefileset.load
    active false
  end
  task rcserver.makefileset.run
    active false
  end
end

macro rcserver.revert.on
  task rcserver.revert
    active true
  end
end

macro rcserver.revert.off
  task rcserver.revert
    active false
  end
end

task	       rcserver.makefileset.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGSUBDIR/rcserver.makefileset.load.log

  task.exec
    $run = disttool -pendingfileset
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$rcPendingFS_DB
      $run = $run -dbname $DB:$rcPendingFS_DB
      $rcPendingFS_DB ++
      if ($rcPendingFS_DB >= $DB:n) set rcPendingFS_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout rcPendingFS -key dist_id:dest_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook rcPendingFS
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup rcPendingFS
  end

  # locked list
  task.exit    default
    showcommand failure
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
  end
end

task	       rcserver.makefileset.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60
  npending     5

  task.exec
    periods -exec 10

    book npages rcPendingFS -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new components to process (pantaskState == INIT)
    book getpage rcPendingFS 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book getword rcPendingFS $pageName dist_id -var DIST_ID
    book getword rcPendingFS $pageName target_id -var TARGET_ID
    book getword rcPendingFS $pageName stage -var STAGE
    book getword rcPendingFS $pageName stage_id -var STAGE_ID
    book getword rcPendingFS $pageName data_group -var DATA_GROUP
    book getword rcPendingFS $pageName filter -var FILTER
    book getword rcPendingFS $pageName dist_dir -var DIST_DIR
    book getword rcPendingFS $pageName dest_id -var DEST_ID
    book getword rcPendingFS $pageName product_name -var PRODUCT_NAME
    book getword rcPendingFS $pageName ds_dbhost -var DS_DBHOST
    book getword rcPendingFS $pageName ds_dbname -var DS_DBNAME
    book getword rcPendingFS $pageName dbname -var DBNAME

    host anyhost

    sprintf logfile "%s/makefs.%s.%s.log" $DIST_DIR $DIST_ID $DEST_ID

    book setword rcPendingFS $pageName pantaskState RUN

    $run = dist_make_fileset.pl --dist_id $DIST_ID --target_id $TARGET_ID --stage $STAGE --stage_id $STAGE_ID --data_group $DATA_GROUP --filter $FILTER --dest_id $DEST_ID --product_name $PRODUCT_NAME  --ds_dbhost $DS_DBHOST --ds_dbname $DS_DBNAME --dist_dir $DIST_DIR --logfile $logfile

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
    process_exit rcPendingFS $options:0 $JOB_STATUS
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword rcPendingFS $options:0 pantaskState TIMEOUT
  end
end


task rcserver.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGSUBDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = disttool -revertfileset
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$rcRevertFS_DB
      $run = $run -dbname $DB:$rcRevertFS_DB
      $rcRevertFS_DB ++
      if ($rcRevertFS_DB >= $DB:n) set rcRevertFS_DB = 0
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
