## detrend.resid.pro : globals and support macros : -*- sh -*-
## this file contains the tasks for running the detrend processing stage
## these tasks use the books detPendingResidImfile and detPendingResidExp

# test for required global variables
check.globals

book init detPendingResidImfile
book init detPendingResidExp

macro detresid.reset
  book init detPendingResidImfile
  book init detPendingResidExp
end

macro detresid.status
  book listbook detPendingResidImfile
  book listbook detPendingResidExp
end

macro detresid.on
  task detrend.resid.load
    active true
  end
  task detrend.resid.run
    active true
  end
  task detrend.residexp.load
    active true
  end
  task detrend.residexp.run
    active true
  end
end

macro detresid.off
  task detrend.resid.load
    active false
  end
  task detrend.resid.run
    active false
  end
  task detrend.residexp.load
    active false
  end
  task detrend.residexp.run
    active false
  end
end

macro detresid.revert.on
  task detrend.resid.revert
    active true
  end
  task detrend.residexp.revert
    active true
  end
end

macro detresid.revert.off
  task detrend.resid.revert
    active false
  end
  task detrend.residexp.revert
    active false
  end
end

# these variables will cycle through the known database names
$detPendingResidImfile_DB = 0
$detPendingResidExp_DB = 0
$detPendingResidImfile_revert_DB = 0
$detPendingResidExp_revert_DB = 0

# select images ready for copy 
# new entries are added to detPendingResidImfile
# compare the new list with the ones already selected
task	       detrend.resid.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/detrend.resid.imfile.log

  task.exec
    $run = dettool -toresidimfile
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detPendingResidImfile_DB
      $run = $run -dbname $DB:$detPendingResidImfile_DB
      $detPendingResidImfile_DB ++
      if ($detPendingResidImfile_DB >= $DB:n) set detPendingResidImfile_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout detPendingResidImfile -key det_id:iteration:exp_id:class_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook detPendingResidImfile
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup detPendingResidImfile
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

# copy new images, sending job to desired host
task	       detrend.resid.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 30

  task.exec
    periods -exec $RUNEXEC

    book npages detPendingResidImfile -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in detPendingResidImfile
    book getpage detPendingResidImfile 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword detPendingResidImfile $pageName pantaskState RUN
    book getword detPendingResidImfile $pageName det_id     -var DET_ID   
    book getword detPendingResidImfile $pageName exp_id     -var EXP_ID  
    book getword detPendingResidImfile $pageName iteration  -var ITERATION     
    book getword detPendingResidImfile $pageName det_type   -var DET_TYPE 
    book getword detPendingResidImfile $pageName mode       -var MODE     
    book getword detPendingResidImfile $pageName exp_tag    -var EXP_TAG  
    book getword detPendingResidImfile $pageName class_id   -var CLASS_ID 
    book getword detPendingResidImfile $pageName uri        -var URI      
    book getword detPendingResidImfile $pageName det_uri    -var DET_URI  
    book getword detPendingResidImfile $pageName ref_det_id -var REF_DET_ID   
    book getword detPendingResidImfile $pageName ref_iter   -var REF_ITER
    book getword detPendingResidImfile $pageName camera     -var CAMERA   
    book getword detPendingResidImfile $pageName workdir    -var WORKDIR_TEMPLATE
    book getword detPendingResidImfile $pageName dbname     -var DBNAME
    book getword detPendingResidImfile $pageName reduction  -var REDUCTION

    # specify choice of local or remote host based on camera and chip (class_id)
    set.host.for.camera $CAMERA $CLASS_ID

    # set workdir (interpolate host; see camera.pro for examples)
    set.workdir.by.camera $CAMERA $CLASS_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    ## generate outroot specific to this exposure (& chip)
    sprintf outroot "%s/%s.%s.%s/%s/%s.detresid.%s.%s" $WORKDIR $CAMERA $DET_TYPE $DET_ID $EXP_TAG $EXP_TAG $DET_ID $ITERATION

    stdout $LOGDIR/detrend.resid.imfile.log
    stderr $LOGDIR/detrend.resid.imfile.log

    $run = detrend_resid_imfile.pl --threads @MAX_THREADS@ --det_id $DET_ID --iteration $ITERATION --ref_det_id $REF_DET_ID --ref_iter $REF_ITER --exp_id $EXP_ID --exp_tag $EXP_TAG --class_id $CLASS_ID --det_type $DET_TYPE --detrend $DET_URI --input_uri $URI --camera $CAMERA --mode $MODE --outroot $outroot --redirect-output --verbose

    if ("$REDUCTION" != "NULL")
      $run = $run --reduction $REDUCTION
    end
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create command
    if ($VERBOSE > 1)
      echo command $run
    end
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit    default
    process_exit detPendingResidImfile $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword detPendingResidImfile $options:0 pantaskState CRASH
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
    book setword detPendingResidImfile $options:0 pantaskState TIMEOUT
  end
end

# select images ready for copy 
# new entries are added to detPendingResidExp
# compare the new list with the ones already selected
task	       detrend.residexp.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/detrend.resid.exp.log

  task.exec
    $run = dettool -toresidexp
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detPendingResidExp_DB
      $run = $run -dbname $DB:$detPendingResidExp_DB
      $detPendingResidExp_DB ++
      if ($detPendingResidExp_DB >= $DB:n) set detPendingResidExp_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout detPendingResidExp -key det_id:iteration:exp_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook detPendingResidExp
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup detPendingResidExp
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

# copy new images, sending job to desired host
task	       detrend.residexp.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 30

  task.exec
    book npages detPendingResidExp -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in detPendingResidExp
    book getpage detPendingResidExp 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword detPendingResidExp $pageName pantaskState RUN
    book getword detPendingResidExp $pageName det_id    -var DET_ID  
    book getword detPendingResidExp $pageName exp_id    -var EXP_ID 
    book getword detPendingResidExp $pageName iteration -var ITERATION    
    book getword detPendingResidExp $pageName det_type  -var DET_TYPE
    book getword detPendingResidExp $pageName mode      -var MODE    
    book getword detPendingResidExp $pageName exp_tag   -var EXP_TAG 
    book getword detPendingResidExp $pageName include   -var INCLUDE 
    book getword detPendingResidExp $pageName camera    -var CAMERA  
    book getword detPendingResidExp $pageName workdir   -var WORKDIR_TEMPLATE
    book getword detPendingResidExp $pageName dbname    -var DBNAME

    # specify choice of local or remote host based on camera and chip (class_id)
    set.host.for.camera $CAMERA FPA

    # set workdir (interpolate host; see camera.pro for examples)
    set.workdir.by.camera $CAMERA FPA $WORKDIR_TEMPLATE $default_host WORKDIR

    ## generate outroot specific to this exposure (& chip)
    sprintf outroot "%s/%s.%s.%s/%s/%s.detresid.%s.%s" $WORKDIR $CAMERA $DET_TYPE $DET_ID $EXP_TAG $EXP_TAG $DET_ID $ITERATION

    stdout $LOGDIR/detrend.resid.exp.log
    stderr $LOGDIR/detrend.resid.exp.log

    $run = detrend_resid_exp.pl --det_id $DET_ID --iteration $ITERATION --exp_id $EXP_ID --exp_tag $EXP_TAG --det_mode $MODE --det_type $DET_TYPE --camera $CAMERA --outroot $outroot --redirect-output --verbose

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
    process_exit detPendingResidExp $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword detPendingResidExp $options:0 pantaskState CRASH
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
    book setword detPendingResidExp $options:0 pantaskState TIMEOUT
  end
end

task detrend.resid.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec

    $run = dettool -revertresidimfile -all-run 
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detPendingResidImfile_revert_DB
      $run = $run -dbname $DB:$detPendingResidImfile_revert_DB
      $detPendingResidImfile_revert_DB ++
      if ($detPendingResidImfile_revert_DB >= $DB:n) set detPendingResidImfile_revert_DB = 0
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

task detrend.residexp.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
  

    $run = dettool -revertresidexp -all-run 
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detPendingResidExp_revert_DB
      $run = $run -dbname $DB:$detPendingResidExp_revert_DB
      $detPendingResidExp_revert_DB ++
      if ($detPendingResidExp_revert_DB >= $DB:n) set detPendingResidExp_revert_DB = 0
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
