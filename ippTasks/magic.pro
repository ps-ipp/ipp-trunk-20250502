## magic.pro : support for the streak removal : -*- sh -*-

# test for required global variables
check.globals

$LOGSUBDIR = $LOGDIR/magic
mkdir $LOGSUBDIR

### Initialise the books containing the tasks to do
book init magicToTree
book init magicToProcess

### Database lists
$magicToTree_DB = 0
$magicToProcess_DB = 0
$magicRevertTree_DB = 0
$magicRevertNode_DB = 0

### Check status of tasks
macro magic.status
  book listbook magicToTree
  book listbook magicToProcess
end

### Reset tasks
macro magic.reset
  book init magicToTree
  book init magicToProcess
end

### Turn tasks on
macro magic.revert.on
  task magic.revert.node
    active true
  end
  task magic.revert.tree
    active true
  end
end

macro magic.on
  task magic.tree.load
    active true
  end
  task magic.tree.run
    active true
  end
  task magic.process.load
    active true
  end
  task magic.process.run
    active true
  end
  magic.revert.on
end

macro magic.revert.off
  task magic.revert.node
    active false
  end
  task magic.revert.tree
    active false
  end
end

### Turn tasks off
macro magic.off
  task magic.tree.load
    active false
  end
  task magic.tree.run
    active false
  end
  task magic.process.load
    active false
  end
  task magic.process.run
    active false
  end
  magic.revert.off
end

task	       magic.tree.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/magic.tree.log

  task.exec
    $run = magictool -totree
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$magicToTree_DB
      $run = $run -dbname $DB:$magicToTree_DB
      $magicToTree_DB ++
      if ($magicToTree_DB >= $DB:n) set magicToTree_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout magicToTree -key magic_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook magicToTree
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup magicToTree
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

task	       magic.tree.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    book npages magicToTree -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images (pantaskState == INIT)
    book getpage magicToTree 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword magicToTree $pageName pantaskState RUN
    book getword magicToTree $pageName magic_id -var MAGIC_ID
    book getword magicToTree $pageName exp_id -var EXP_ID
    book getword magicToTree $pageName camera -var CAMERA
    book getword magicToTree $pageName workdir -var WORKDIR_TEMPLATE
    book getword magicToTree $pageName dbname -var DBNAME
    book getword magicToTree $pageName tess_id -var TESS_DIR
    book getword magicToTree $pageName ra -var RA
    book getword magicToTree $pageName decl -var DEC

#    set.host.for.camera $CAMERA $MAGIC_ID
#    set.workdir.by.camera $CAMERA $MAGIC_ID $WORKDIR_TEMPLATE $default_host WORKDIR
    host anyhost
    $WORKDIR = $WORKDIR_TEMPLATE

    sprintf outroot "%s/%s/%s.mgc.%s" $WORKDIR $EXP_ID $EXP_ID $MAGIC_ID

    ## generate output log based on filerule (convert the URI to a PATH)
    $logfile = `ipp_filename.pl --filerule LOG.EXP --camera $CAMERA --class_id $MAGIC_ID --basename $outroot`
    if ("$logfile" == "") 
      echo "WARNING: logfile not defined in magic.tree.run"
      break
    end

    stdout $logfile
    stderr $logfile
    dirname $logfile -var outpath
    mkdir $outpath

    $run = magic_tree.pl --magic_id $MAGIC_ID --camera $CAMERA --tess_id $TESS_DIR --ra $RA --dec $DEC --outroot $outroot --logfile $logfile
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
    process_exit magicToTree $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword magicToTree $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword magicToTree $options:0 pantaskState TIMEOUT
  end
end

task	       magic.process.load
  host         local

  periods      -poll $LOADPOLL
#  periods      -exec $LOADEXEC
  periods      -exec 30
  periods      -timeout 300
  npending     1

  stdout NULL
  stderr $LOGDIR/magic.process.log

  task.exec
    $run = magictool -toprocess
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$magicToProcess_DB
      $run = $run -dbname $DB:$magicToProcess_DB
      $magicToProcess_DB ++
      if ($magicToProcess_DB >= $DB:n) set magicToProcess_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout magicToProcess -key magic_id:node -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook magicToProcess
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup magicToProcess
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

task	       magic.process.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    periods -exec 10

    book npages magicToProcess -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images (pantaskState == INIT)
    book getpage magicToProcess 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword magicToProcess $pageName pantaskState RUN
    book getword magicToProcess $pageName magic_id -var MAGIC_ID
    book getword magicToProcess $pageName exp_id -var EXP_ID
    book getword magicToProcess $pageName node -var NODE
    book getword magicToProcess $pageName camera -var CAMERA
    book getword magicToProcess $pageName workdir -var WORKDIR_TEMPLATE
    book getword magicToProcess $pageName raw_workdir -var RAW_TEMPLATE
    book getword magicToProcess $pageName exp_name -var EXP_NAME
    book getword magicToProcess $pageName dbname -var DBNAME

#    XXX: we need new macros that select the host and workdir based on the NODE
#    we also need to make the magic scripts able to deal with the workdir being spread
#    around. That should not be too hard
#    For now select nodes by skycell

    substr $NODE 0 3 NODE_HEAD
    if ("$NODE_HEAD" == "sky")
        set.host.for.skycell $NODE
    else 
        host anyhost
    end

#    currently DetectStreaks expects that the workdir is not in nebulous
#    set.workdir.by.skycell $SKYCELL_ID $WORKDIR_TEMPLATE $default_host WORKDIR
    $WORKDIR = $WORKDIR_TEMPLATE

    sprintf baseroot "%s/%s/%s.mgc.%s" $WORKDIR $EXP_ID $EXP_ID $MAGIC_ID

    if ("$NODE" == "root")
        # This is the root node, put the streaks file into the rawExps workdir
        # which is presumably in nebulous and thus will be replicatable
        set.workdir.by.camera $CAMERA FPA $RAW_TEMPLATE $default_host RAW_WORKDIR
        sprintf final_outroot "%s/%s.%s/%s.%s.mgc.%s" $RAW_WORKDIR $EXP_NAME $EXP_ID $EXP_NAME $EXP_ID $MAGIC_ID

        $EXTRA_ARGS = --final-outroot $final_outroot
    else
        $EXTRA_ARGS = ""
    end

    $logfile = $baseroot.$NODE.log
    if ("$logfile" == "") 
      echo "WARNING: logfile not defined in magic.process.run"
      break
    end

    stdout $logfile
    stderr $logfile
    dirname $logfile -var outpath
    mkdir $outpath

    $run = magic_process.pl --magic_id $MAGIC_ID --camera $CAMERA --node $NODE --baseroot $baseroot --logfile $logfile $EXTRA_ARGS
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
    process_exit magicToProcess $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword magicToProcess $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword magicToProcess $options:0 pantaskState TIMEOUT
  end
end

task magic.revert.node
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

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
task magic.revert.tree
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGSUBDIR/reverttree.log

  task.exec
    if ($LABEL:n == 0) break
    $run = magictool -reverttree
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$magicRevertTree_DB
      $run = $run -dbname $DB:$magicRevertTree_DB
      $magicRevertTree_DB ++
      if ($magicRevertTree_DB >= $DB:n) set magicRevertTree_DB = 0
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
