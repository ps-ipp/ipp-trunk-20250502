## detrend.norm.pro : globals and support macros : -*- sh -*-
## this file contains the tasks for running the detrend normalization stages
## these tasks use the books detPendingNormStatImfile detPendingNormImfile detPendingNormExp

# test for required global variables
check.globals

book init detPendingNormStatImfile
book init detPendingNormImfile
book init detPendingNormExp

macro detnorm.reset
  book init detPendingNormStatImfile
  book init detPendingNormImfile
  book init detPendingNormExp
end

macro detnorm.status
  book listbook detPendingNormStatImfile
  book listbook detPendingNormImfile
  book listbook detPendingNormExp
end

macro detnorm.on
  task detrend.norm.load
    active true
  end
  task detrend.norm.run
    active true
  end
  task detrend.normexp.load
    active true
  end
  task detrend.normexp.run
    active true
  end
  task detrend.normstat.load
    active true
  end
  task detrend.normstat.run
    active true
  end
end

macro detnorm.off
  task detrend.norm.load
    active false
  end
  task detrend.norm.run
    active false
  end
  task detrend.normexp.load
    active false
  end
  task detrend.normexp.run
    active false
  end
  task detrend.normstat.load
    active false
  end
  task detrend.normstat.run
    active false
  end
end

macro detnorm.revert.off
  task detrend.norm.revert
    active false
  end
  task detrend.normexp.revert
    active false
  end
  task detrend.normstat.revert
    active false
  end
end

macro detnorm.revert.on
  task detrend.norm.revert
    active true
  end
  task detrend.normexp.revert
    active true
  end
  task detrend.normstat.revert
    active true
  end
end


# these variables will cycle through the known database names
$detPendingNormStatImfile_DB = 0
$detPendingNormImfile_DB = 0
$detPendingNormExp_DB = 0
$detPendingNormStatImfile_DB_revert = 0
$detPendingNormImfile_DB_revert = 0
$detPendingNormExp_DB_revert = 0

# select images ready for copy 
# new entries are added to detPendingNormStatImfile
# compare the new list with the ones already selected
task	       detrend.normstat.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/detrend.normstat.log

  task.exec
    $run = dettool -tonormalizedstat
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detPendingNormStatImfile_DB
      $run = $run -dbname $DB:$detPendingNormStatImfile_DB
      $detPendingNormStatImfile_DB ++
      if ($detPendingNormStatImfile_DB >= $DB:n) set detPendingNormStatImfile_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout detPendingNormStatImfile -key det_id:iteration -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook detPendingNormStatImfile
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup detPendingNormStatImfile
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
task	       detrend.normstat.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 30

  task.exec
    book npages detPendingNormStatImfile -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in detPendingNormStatImfile
    book getpage detPendingNormStatImfile 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword detPendingNormStatImfile $pageName pantaskState RUN
    book getword detPendingNormStatImfile $pageName det_id    -var DET_ID
    book getword detPendingNormStatImfile $pageName det_type  -var DET_TYPE
    book getword detPendingNormStatImfile $pageName iteration -var ITERATION
    book getword detPendingNormStatImfile $pageName camera    -var CAMERA  
    book getword detPendingNormStatImfile $pageName workdir   -var WORKDIR_TEMPLATE
    book getword detPendingNormStatImfile $pageName dbname    -var DBNAME

    # specify choice of local or remote host based on camera and chip (class_id)
    set.host.for.camera $CAMERA FPA

    # set workdir (interpolate host; see camera.pro for examples)
    set.workdir.by.camera $CAMERA FPA $WORKDIR_TEMPLATE $default_host WORKDIR

    ## generate outroot specific to this exposure (& chip)
    sprintf outroot "%s/%s.%s.%s/%s.%s.normstat.%s.%s" $WORKDIR $CAMERA $DET_TYPE $DET_ID $CAMERA $DET_TYPE $DET_ID $ITERATION

    stdout $LOGDIR/detrend.normstat.log
    stderr $LOGDIR/detrend.normstat.log

    $run = detrend_norm_calc.pl --det_id $DET_ID --iteration $ITERATION --det_type $DET_TYPE --outroot $outroot --redirect-output
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
    process_exit detPendingNormStatImfile $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword detPendingNormStatImfile $options:0 pantaskState CRASH
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
    book setword detPendingNormStatImfile $options:0 pantaskState TIMEOUT
  end
end

# select images ready for copy 
# new entries are added to detPendingNormImfile
# compare the new list with the ones already selected
task	       detrend.norm.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/detrend.norm.log

  task.exec
    $run = dettool -tonormalize
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detPendingNormImfile_DB
      $run = $run -dbname $DB:$detPendingNormImfile_DB
      $detPendingNormImfile_DB ++
      if ($detPendingNormImfile_DB >= $DB:n) set detPendingNormImfile_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout detPendingNormImfile -key det_id:iteration:class_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook detPendingNormImfile
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup detPendingNormImfile
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
task	       detrend.norm.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 30

  task.exec
    book npages detPendingNormImfile -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in detPendingNormImfile
    book getpage detPendingNormImfile 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword detPendingNormImfile $pageName pantaskState RUN
    book getword detPendingNormImfile $pageName det_type  -var DET_TYPE 
    book getword detPendingNormImfile $pageName camera    -var CAMERA   
    book getword detPendingNormImfile $pageName uri       -var URI      
    book getword detPendingNormImfile $pageName det_id    -var DET_ID   
    book getword detPendingNormImfile $pageName iteration -var ITERATION     
    book getword detPendingNormImfile $pageName class_id  -var CLASS_ID 
    book getword detPendingNormImfile $pageName norm      -var NORM     
    book getword detPendingNormImfile $pageName workdir   -var WORKDIR_TEMPLATE
    book getword detPendingNormImfile $pageName dbname    -var DBNAME

    # specify choice of local or remote host based on camera and chip (class_id)
    set.host.for.camera $CAMERA $CLASS_ID

    # set workdir (interpolate host; see camera.pro for examples)
    set.workdir.by.camera $CAMERA $CLASS_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    ## generate outroot specific to this stack
    sprintf outroot "%s/%s.%s.%s/%s.%s.norm.%s.%s" $WORKDIR $CAMERA $DET_TYPE $DET_ID $CAMERA $DET_TYPE $DET_ID $ITERATION

    stdout $LOGDIR/detrend.norm.log
    stderr $LOGDIR/detrend.norm.log

    $run = detrend_norm_apply.pl --det_id $DET_ID --iteration $ITERATION --class_id $CLASS_ID --value $NORM --input_uri $URI --camera $CAMERA --det_type $DET_TYPE --outroot $outroot --redirect-output --verbose
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
    process_exit detPendingNormImfile $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword detPendingNormImfile $options:0 pantaskState CRASH
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
    book setword detPendingNormImfile $options:0 pantaskState TIMEOUT
  end
end

# select images ready for copy 
# new entries are added to detPendingNormExp
# compare the new list with the ones already selected
task	       detrend.normexp.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/detrend.normexp.log

  task.exec
    $run = dettool -tonormalizedexp
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detPendingNormExp_DB
      $run = $run -dbname $DB:$detPendingNormExp_DB
      $detPendingNormExp_DB ++
      if ($detPendingNormExp_DB >= $DB:n) set detPendingNormExp_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout detPendingNormExp -key det_id:iteration -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook detPendingNormExp
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup detPendingNormExp
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
task	       detrend.normexp.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 30

  task.exec
    book npages detPendingNormExp -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in detPendingNormExp
    book getpage detPendingNormExp 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword detPendingNormExp $pageName pantaskState RUN
    book getword detPendingNormExp $pageName det_id    -var DET_ID  
    book getword detPendingNormExp $pageName iteration -var ITERATION    
    book getword detPendingNormExp $pageName det_type  -var DET_TYPE
    book getword detPendingNormExp $pageName camera    -var CAMERA  
    book getword detPendingNormExp $pageName workdir   -var WORKDIR_TEMPLATE
    book getword detPendingNormExp $pageName dbname    -var DBNAME

    # specify choice of local or remote host based on camera and chip (class_id)
    set.host.for.camera $CAMERA FPA

    # set workdir (interpolate host; see camera.pro for examples)
    set.workdir.by.camera $CAMERA FPA $WORKDIR_TEMPLATE $default_host WORKDIR

    ## generate outroot specific to this exposure (& chip)
    sprintf outroot "%s/%s.%s.%s/%s.%s.normexp.%s.%s" $WORKDIR $CAMERA $DET_TYPE $DET_ID $CAMERA $DET_TYPE $DET_ID $ITERATION

    stdout $LOGDIR/detrend.normexp.log
    stderr $LOGDIR/detrend.normexp.log

    $run = detrend_norm_exp.pl --det_id $DET_ID --iteration $ITERATION --camera $CAMERA --det_type $DET_TYPE  --outroot $outroot
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
    process_exit detPendingNormExp $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword detPendingNormExp $options:0 pantaskState CRASH
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
    book setword detPendingNormExp $options:0 pantaskState TIMEOUT
  end
end

task detrend.norm.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec

    $run = dettool -revertnormalizedimfile -all-run 
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detPendingNormImfile_DB_revert
      $run = $run -dbname $DB:$detPendingNormImfile_DB_revert
      $detPendingNormImfile_DB_revert ++
      if ($detPendingNormImfile_DB_revert >= $DB:n) set detPendingNormImfile_DB_revert = 0
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

task detrend.normexp.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec

    $run = dettool -revertnormalizedexp -all-run 
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detPendingNormExp_DB_revert
      $run = $run -dbname $DB:$detPendingNormExp_DB_revert
      $detPendingNormExp_DB_revert ++
      if ($detPendingNormExp_DB_revert >= $DB:n) set detPendingNormExp_DB_revert = 0
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

task detrend.normstat.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
  
    $run = dettool -revertnormalizedstat -all-run 
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detPendingNormStatImfile_DB_revert
      $run = $run -dbname $DB:$detPendingNormStatImfile_DB_revert
      $detPendingNormStatImfile_DB_revert ++
      if ($detPendingNormStatImfile_DB_revert >= $DB:n) set detPendingNormStatImfile_DB_revert = 0
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
