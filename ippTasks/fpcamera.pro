## fpcamera.pro : globals and support macros : -*- sh -*-
## this file contains the tasks for running the forced photometry camera analysis stage
## these tasks use the book fpcamPendingExp

# test for required global variables
check.globals

# fpcamera.pro should have a more restricted polling limit (to avoid stress with getstar)
if ($?POLL_LIMIT_FPCAMERA == 0) set POLL_LIMIT_FPCAMERA = 10

macro set.fpcamera.poll
  if ($0 != 2)
    echo "USAGE:set.fpcamera.poll (value)"
    break
  end
 
  $POLL_LIMIT_FPCAMERA = $1
end

macro get.fpcamera.poll
  echo "fpcamera poll limit: $POLL_LIMIT_FPCAMERA"
end

book init fpcamPendingExp

macro fpcamera.status
  book listbook fpcamPendingExp
end

macro fpcamera.reset
  book init fpcamPendingExp
end

macro fpcamera.on
  task fpcamera.exp.load
    active true
  end
  task fpcamera.exp.run
    active true
  end
  task fpcamera.revert
    active true
  end
end

macro fpcamera.off
  task fpcamera.exp.load
    active false
  end
  task fpcamera.exp.run
    active false
  end
  task fpcamera.revert
    active false
  end
end

macro fpcamera.revert.on
  task fpcamera.revert
    active true
  end
end

macro fpcamera.revert.off
  task fpcamera.revert
    active false
  end
end

# this variable will cycle through the known database names
$fpcamera_DB = 0
$fpcamera_revert_DB = 0

# select images ready for fpcamera analysis
# new entries are added to fpcamPendingExp
# skip already-present entries
task	       fpcamera.exp.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/fpcamera.exp.log

  task.exec
    if ($LABEL:n == 0) break
    $run = fpcamtool -pendingexp
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$fpcamera_DB
      $run = $run -dbname $DB:$fpcamera_DB
      $fpcamera_DB ++
      if ($fpcamera_DB >= $DB:n) set fpcamera_DB = 0
    end
    $run = $run -limit $POLL_LIMIT_FPCAMERA
    # NOTE: we do not want to overload the dvo db machine with getstar queries, 
    # so we will limit the fpcamera stage to a smaller number than the other stages
    # add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout fpcamPendingExp -key fpcam_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook fpcamPendingExp
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup fpcamPendingExp
  end

  # default exit status
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

# run the fpcamera_exp script on pending images
task	       fpcamera.exp.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 10

  task.exec
    book npages fpcamPendingExp -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    if ($BURNTOOLING == 1) break
    
    # look for new images in fpcamPendingExp (pantaskState == INIT)
    book getpage fpcamPendingExp 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword fpcamPendingExp $pageName pantaskState RUN
    book getword fpcamPendingExp $pageName camera -var CAMERA
    book getword fpcamPendingExp $pageName exp_tag -var EXP_TAG
    book getword fpcamPendingExp $pageName fpcam_id -var FPCAM_ID
    book getword fpcamPendingExp $pageName workdir -var WORKDIR_TEMPLATE
    book getword fpcamPendingExp $pageName path_base -var PATH_BASE
    book getword fpcamPendingExp $pageName dbname -var DBNAME
    book getword fpcamPendingExp $pageName dvodb -var DVODB
    book getword fpcamPendingExp $pageName reduction -var REDUCTION

    # specify choice of remote host based on camera and chip (class_id)
    set.host.for.camera $CAMERA FPA

    # set the WORKDIR variable
    set.workdir.by.camera $CAMERA FPA $WORKDIR_TEMPLATE $default_host WORKDIR

    # notes on how this works:
    # -- raw workdir examples:
    # file://data/@HOST@.0/gpc1/20080130
    # neb:///@HOST@-vol0/gpc1/20080130 (need to supply volname?, or are we re-defining this each time?)
    # -- out workdir examples:
    # file://data/ipp004.0/gpc1/20080130
    # neb:///ipp004-vol0/gpc1/20080130

    if ("$PATH_BASE" == "NULL") 
        ## generate outroot specific to this exposure (& chip)
        sprintf outroot "%s/%s/%s.fp.%s" $WORKDIR $EXP_TAG $EXP_TAG $FPCAM_ID
    else
        $outroot = $PATH_BASE
    end

    stdout $LOGDIR/fpcamera.exp.log
    stderr $LOGDIR/fpcamera.exp.log

    $run = fpcamera_exp.pl --exp_tag $EXP_TAG --fpcam_id $FPCAM_ID --camera $CAMERA --outroot $outroot --redirect-output
    if ("$REDUCTION" != "NULL")
      $run = $run --reduction $REDUCTION
    end
    if ("$DVODB" != "NULL")
      $run = $run --dvodb $DVODB
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

  # success
  task.exit default
    process_exit fpcamPendingExp $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword fpcamPendingExp $options:0 pantaskState CRASH
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
    book setword fpcamPendingExp $options:0 pantaskState TIMEOUT
  end
end

task fpcamera.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = fpcamtool -revertprocessedexp
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$fpcamera_revert_DB
      $run = $run -dbname $DB:$fpcamera_revert_DB
      $fpcamera_revert_DB ++
      if ($fpcamera_revert_DB >= $DB:n) set fpcamera_revert_DB = 0
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

