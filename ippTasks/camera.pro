## camera.pro : globals and support macros : -*- sh -*-
## this file contains the tasks for running the camera analysis stage
## these tasks use the book camPendingExp

# test for required global variables
check.globals

# camera.pro should have a more restricted polling limit (to avoid stress with getstar)
if ($?POLL_LIMIT_CAMERA == 0) set POLL_LIMIT_CAMERA = 10

macro set.camera.poll
  if ($0 != 2)
    echo "USAGE:set.camera.poll (value)"
    break
  end
 
  $POLL_LIMIT_CAMERA = $1
end

macro get.camera.poll
  echo "camera poll limit: $POLL_LIMIT_CAMERA"
end

book init camPendingExp

macro camera.status
  book listbook camPendingExp
end

macro camera.reset
  book init camPendingExp
end

macro camera.on
  task camera.exp.load
    active true
  end
  task camera.exp.run
    active true
  end
  task camera.revert
    active true
  end
end

macro camera.off
  task camera.exp.load
    active false
  end
  task camera.exp.run
    active false
  end
  task camera.revert
    active false
  end
end

macro camera.revert.on
  task camera.revert
    active true
  end
end

macro camera.revert.off
  task camera.revert
    active false
  end
end

# this variable will cycle through the known database names
$camera_DB = 0
$camera_revert_DB = 0

# select images ready for camera analysis
# new entries are added to camPendingExp
# skip already-present entries
task	       camera.exp.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/camera.exp.log

  task.exec
    if ($LABEL:n == 0) break
    $run = camtool -pendingexp
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$camera_DB
      $run = $run -dbname $DB:$camera_DB
      $camera_DB ++
      if ($camera_DB >= $DB:n) set camera_DB = 0
    end
    $run = $run -limit $POLL_LIMIT_CAMERA
    # NOTE: we do not want to overload the dvo db machine with getstar queries, 
    # so we will limit the camera stage to a smaller number than the other stages
    # add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout camPendingExp -key cam_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook camPendingExp
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup camPendingExp
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

# run the cameraexposure script on pending images
task	       camera.exp.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 10

  task.exec
    book npages camPendingExp -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    if ($BURNTOOLING == 1) break
    
    # look for new images in camPendingExp (pantaskState == INIT)
    book getpage camPendingExp 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword camPendingExp $pageName pantaskState RUN
    book getword camPendingExp $pageName camera -var CAMERA
    book getword camPendingExp $pageName exp_tag -var EXP_TAG
    book getword camPendingExp $pageName cam_id -var CAM_ID
    book getword camPendingExp $pageName workdir -var WORKDIR_TEMPLATE
    book getword camPendingExp $pageName path_base -var PATH_BASE
    book getword camPendingExp $pageName dvodb  -var DVODB
    book getword camPendingExp $pageName dbname -var DBNAME
    book getword camPendingExp $pageName reduction -var REDUCTION
    book getword camPendingExp $pageName state -var RUN_STATE

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
        sprintf outroot "%s/%s/%s.cm.%s" $WORKDIR $EXP_TAG $EXP_TAG $CAM_ID
    else
        $outroot = $PATH_BASE
    end

    stdout $LOGDIR/camera.exp.log
    stderr $LOGDIR/camera.exp.log

    $run = camera_exp.pl --exp_tag $EXP_TAG --cam_id $CAM_ID --camera $CAMERA --outroot $outroot --redirect-output --run-state $RUN_STATE
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
    process_exit camPendingExp $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword camPendingExp $options:0 pantaskState CRASH
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
    book setword camPendingExp $options:0 pantaskState TIMEOUT
  end
end

task camera.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = camtool -revertprocessedexp
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$camera_revert_DB
      $run = $run -dbname $DB:$camera_revert_DB
      $camera_revert_DB ++
      if ($camera_revert_DB >= $DB:n) set camera_revert_DB = 0
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
