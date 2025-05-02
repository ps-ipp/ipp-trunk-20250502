## detrend.process.pro : globals and support macros : -*- sh -*-
## this file contains the tasks for running the detrend processing stage
## these tasks use the books detPendingProcessedImfile and detPendingProcessedExp

# test for required global variables
check.globals

book init detPendingProcessedImfile
book init detPendingProcessedExp

macro detproc.reset
  book init detPendingProcessedImfile
  book init detPendingProcessedExp
end

macro detproc.status
  echo detPendingProcessedImfile
  book listbook detPendingProcessedImfile
  echo detPendingProcessedExp
  book listbook detPendingProcessedExp
end

macro detproc.on
  task detrend.process.load
    active true
  end
  task detrend.process.run
    active true
  end
  task detrend.processexp.load
    active true
  end
  task detrend.processexp.run
    active true
  end
end

macro detproc.off
  task detrend.process.load
    active false
  end
  task detrend.process.run
    active false
  end
  task detrend.processexp.load
    active false
  end
  task detrend.processexp.run
    active false
  end
end

macro detproc.revert.off
  task detrend.process.revert
    active false
  end
  task detrend.processexp.revert
    active false
  end
end

macro detproc.revert.on
  task detrend.process.revert
    active true
  end
  task detrend.processexp.revert
    active true
  end
end

# these variables will cycle through the known database names
$detPendingProcessedImfile_DB = 0
$detPendingProcessedExp_DB = 0
$detPendingProcessedImfile_revert_DB = 0
$detPendingProcessedExp_revert_DB = 0

# select images ready for copy 
# new entries are added to detPendingProcessedImfile
# compare the new list with the ones already selected
task	       detrend.process.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/detrend.process.imfile.log

  task.exec
    $run = dettool -toprocessedimfile
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detPendingProcessedImfile_DB
      $run = $run -dbname $DB:$detPendingProcessedImfile_DB
      $detPendingProcessedImfile_DB ++
      if ($detPendingProcessedImfile_DB >= $DB:n) set detPendingProcessedImfile_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout detPendingProcessedImfile -key det_id:exp_id:class_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook detPendingProcessedImfile
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup detPendingProcessedImfile
  end

  # error
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

# run detrend_process_imfile.ps on pending images
task	       detrend.process.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 30

  task.exec
    book npages detPendingProcessedImfile -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in detPendingProcessedImfile
    book getpage detPendingProcessedImfile 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword detPendingProcessedImfile $pageName pantaskState RUN
    book getword detPendingProcessedImfile $pageName det_id   -var DET_ID   
    book getword detPendingProcessedImfile $pageName det_type -var DET_TYPE 
    book getword detPendingProcessedImfile $pageName exp_id   -var EXP_ID  
    book getword detPendingProcessedImfile $pageName class_id -var CLASS_ID 
    book getword detPendingProcessedImfile $pageName exp_tag  -var EXP_TAG  
    book getword detPendingProcessedImfile $pageName uri      -var URI      
    book getword detPendingProcessedImfile $pageName camera   -var CAMERA   
    book getword detPendingProcessedImfile $pageName workdir  -var WORKDIR_TEMPLATE
    book getword detPendingProcessedImfile $pageName dbname   -var DBNAME
    book getword detPendingProcessedImfile $pageName reduction -var REDUCTION

    # specify choice of local or remote host based on camera and chip (class_id)
    set.host.for.camera $CAMERA $CLASS_ID

    # see chip.pro for examples
    set.workdir.by.camera $CAMERA $CLASS_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    ## generate outroot specific to this exposure (& chip)
    sprintf outroot "%s/%s.%s.%s/%s/%s.detproc.%s" $WORKDIR $CAMERA $DET_TYPE $DET_ID $EXP_TAG $EXP_TAG $DET_ID

    stdout $LOGDIR/detrend.process.imfile.log
    stderr $LOGDIR/detrend.process.imfile.log

    $run = detrend_process_imfile.pl --threads @MAX_THREADS@ --det_id $DET_ID --exp_id $EXP_ID --det_type $DET_TYPE --class_id $CLASS_ID --exp_tag $EXP_TAG --input_uri $URI --camera $CAMERA --outroot $outroot --redirect-output
    if ("$REDUCTION" != "NULL")
      $run = $run --reduction $REDUCTION
    end
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create example job options as a demonstration
    if ($VERBOSE > 1)
      echo command $run
    end
    command $run
  end

  # default exit status
  task.exit    default
    process_exit detPendingProcessedImfile $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword detPendingProcessedImfile $options:0 pantaskState CRASH
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
    book setword detPendingProcessedImfile $options:0 pantaskState TIMEOUT
 end
end

# select images ready for copy 
# new entries are added to detPendingProcessedExp
# compare the new list with the ones already selected
task	       detrend.processexp.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/detrend.process.exp.log

  task.exec
    $run = dettool -toprocessedexp
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detPendingProcessedExp_DB
      $run = $run -dbname $DB:$detPendingProcessedExp_DB
      $detPendingProcessedExp_DB ++
      if ($detPendingProcessedExp_DB >= $DB:n) set detPendingProcessedExp_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout detPendingProcessedExp -key det_id:exp_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook detPendingProcessedExp
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup detPendingProcessedExp
  end

  # error
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

# copy new images, sending job to desired host
task	       detrend.processexp.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 30

  task.exec
    book npages detPendingProcessedExp -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new exposures in detPendingProcessedExp
    book getpage detPendingProcessedExp 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword detPendingProcessedExp $pageName pantaskState RUN
    book getword detPendingProcessedExp $pageName det_id    -var DET_ID
    book getword detPendingProcessedExp $pageName exp_id    -var EXP_ID
    book getword detPendingProcessedExp $pageName det_type  -var DET_TYPE
    book getword detPendingProcessedExp $pageName camera    -var CAMERA  
    book getword detPendingProcessedExp $pageName exp_tag   -var EXP_TAG
    book getword detPendingProcessedExp $pageName workdir   -var WORKDIR_TEMPLATE
    book getword detPendingProcessedExp $pageName dbname    -var DBNAME
    book getword detPendingProcessedExp $pageName reduction -var REDUCTION

    # specify choice of local or remote host based on camera and chip (class_id)
    set.host.for.camera $CAMERA FPA

    # set workdir (interpolate host; see camera.pro for examples)
    set.workdir.by.camera $CAMERA FPA $WORKDIR_TEMPLATE $default_host WORKDIR

    ## generate outroot specific to this exposure (& chip)
    sprintf outroot "%s/%s.%s.%s/%s/%s.detproc.%s" $WORKDIR $CAMERA $DET_TYPE $DET_ID $EXP_TAG $EXP_TAG $DET_ID

    stdout $LOGDIR/detrend.process.exp.log
    stderr $LOGDIR/detrend.process.exp.log

    $run = detrend_process_exp.pl --det_id $DET_ID --exp_id $EXP_ID --exp_tag $EXP_TAG --det_type $DET_TYPE --camera $CAMERA --outroot $outroot --redirect-output --verbose
    if ("$REDUCTION" != "NULL")
      $run = $run --reduction $REDUCTION
    end
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create example job options as a demonstration
    if ($VERBOSE > 1)
      echo command $run
    end
    command $run
  end

  # success
  task.exit    default
    process_exit detPendingProcessedExp $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword detPendingProcessedExp $options:0 pantaskState CRASH
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
    book setword detPendingProcessedExp $options:0 pantaskState TIMEOUT
  end
end

task detrend.process.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
 
    $run = dettool -revertprocessedimfile -all-run 
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detPendingProcessedImfile_revert_DB
      $run = $run -dbname $DB:$detPendingProcessedImfile_revert_DB
      $detPendingProcessedImfile_revert_DB ++
      if ($detPendingProcessedImfile_revert_DB >= $DB:n) set detPendingProcessedImfile_revert_DB = 0
    end
    echo $run
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

task detrend.processexp.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
  
    $run = dettool -revertprocessedexp -all-run 
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detPendingProcessedExp_revert_DB
      $run = $run -dbname $DB:$detPendingProcessedExp_revert_DB
      $detPendingProcessedExp_revert_DB ++
      if ($detPendingProcessedExp_revert_DB >= $DB:n) set detPendingProcessedExp_revert_DB = 0
    end
    echo $run
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
