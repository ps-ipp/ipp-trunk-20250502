## detrend.process.pro : globals and support macros : -*- sh -*-
## this file contains the tasks for running the detrend processing stage
## these tasks use the books detPendingProcessedImfile and detPendingProcessedExp

# test for required global variables
check.globals

book init detCleanupProcessed
book init detCleanupResid

book init detCleanupStackedImfile
book init detCleanupNormStatImfile
book init detCleanupNormImfile
book init detCleanupNormExp


macro detclean.reset
  book init detCleanupProcessed
  book init detCleanupResid

  book init detCleanupStackedImfile
  book init detCleanupNormStatImfile
  book init detCleanupNormImfile
  book init detCleanupNormExp
end

macro detclean.status
  book listbook detCleanupProcessed
  book listbook detCleanupResid

  book listbook detCleanupStackedImfile
  book listbook detCleanupNormStatImfile
  book listbook detCleanupNormImfile
  book listbook detCleanupNormExp

end

macro detclean.on
  task detrend.cleanup.processed.load
    active true
  end
  task detrend.cleanup.processed.run
    active true
  end

  task detrend.cleanup.resid.load
    active true
  end
  task detrend.cleanup.resid.run
    active true
  end

#  task detrend.cleanup.stack.load
#    active true
#  end
#  task detrend.cleanup.stack.run
#    active true
#  end
#  task detrend.cleanup.norm.load
#    active true
#  end
#  task detrend.cleanup.norm.run
#    active true
#  end
#  task detrend.cleanup.normexp.load
#    active true
#  end
#  task detrend.cleanup.normexp.run
#    active true
#  end
#  task detrend.cleanup.normstat.load
#    active true
#  end
#  task detrend.cleanup.normstat.run
#    active true
#  end
end

macro detclean.off
  task detrend.cleanup.processed.load
    active false
  end
  task detrend.cleanup.processed.run
    active false
  end

  task detrend.cleanup.resid.load
    active false
  end
  task detrend.cleanup.resid.run
    active false
  end

  task detrend.cleanup.stack.load
    active false
  end
  task detrend.cleanup.stack.run
    active false
  end
  task detrend.cleanup.norm.load
    active false
  end
  task detrend.cleanup.norm.run
    active false
  end
  task detrend.cleanup.normexp.load
    active false
  end
  task detrend.cleanup.normexp.run
    active false
  end
  task detrend.cleanup.normstat.load
    active false
  end
  task detrend.cleanup.normstat.run
    active false
  end

end


# these variables will cycle through the known database names
$detCleanupProcessed_DB = 0
$detCleanupResid_DB = 0

$detCleanupStackedImfile_DB = 0
$detCleanupNormStatImfile_DB = 0
$detCleanupNormImfile_DB = 0
$detCleanupNormExp_DB = 0

######## cleanup processed imfile/exp ########
task	       detrend.cleanup.processed.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1
  active       true

  stdout NULL
  stderr $LOGDIR/detrend.cleanup.processed.log

  task.exec
    $run = dettool -pendingcleanup_processedexp 
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detCleanupProcessed_DB
      $run = $run -dbname $DB:$detCleanupProcessed_DB
      $detCleanupProcessed_DB ++
      if ($detCleanupProcessed_DB >= $DB:n) set detCleanupProcessed_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout detCleanupProcessed -key det_id:exp_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook detCleanupProcessed
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup detCleanupProcessed
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

# run the ipp_cleanup.pl script on pending images
task	       detrend.cleanup.processed.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60
  active       true

  task.exec
    book npages detCleanupProcessed -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in detCleanupProcessed (pantaskState == INIT)
    book getpage detCleanupProcessed 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword detCleanupProcessed $pageName pantaskState RUN
    book getword detCleanupProcessed $pageName det_id   -var DET_ID   
    book getword detCleanupProcessed $pageName exp_id   -var EXP_ID   
    book getword detCleanupProcessed $pageName camera   -var CAMERA
    book getword detCleanupProcessed $pageName data_state    -var CLEANUP_MODE
    book getword detCleanupProcessed $pageName dbname   -var DBNAME

    # specify choice of local or remote host based on camera and diff (class_id)
    set.host.for.camera $CAMERA FPA

    stdout $LOGDIR/detrend.cleanup.processed.log
    stderr $LOGDIR/detrend.cleanup.processed.log

    # XXX is everything listed here needed?
    $run = ipp_cleanup.pl --stage detrend.processed --stage_id $DET_ID.$EXP_ID --camera $CAMERA --mode $CLEANUP_MODE --dbname $DBNAME
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
    process_exit detCleanupProcessed $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword detCleanupProcessed $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword detCleanupProcessed $options:0 pantaskState TIMEOUT
  end
end



######## cleanup resid imfile/exp ########
task	       detrend.cleanup.resid.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1
  active       true

  stdout NULL
  stderr $LOGDIR/detrend.cleanup.resid.log

  task.exec
    $run = dettool -pendingcleanup_residexp
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detCleanupResid_DB
      $run = $run -dbname $DB:$detCleanupResid_DB
      $detCleanupResid_DB ++
      if ($detCleanupResid_DB >= $DB:n) set detCleanupResid_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout detCleanupResid -key det_id:iteration:exp_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook detCleanupResid
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup detCleanupResid
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

# run the ipp_cleanup.pl script on pending images
task	       detrend.cleanup.resid.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60
  active       true

  task.exec
    book npages detCleanupResid -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in detCleanupResid (pantaskState == INIT)
    book getpage detCleanupResid 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword detCleanupResid $pageName pantaskState RUN
    book getword detCleanupResid $pageName det_id   -var DET_ID   
    book getword detCleanupResid $pageName exp_id   -var EXP_ID   
    book getword detCleanupResid $pageName iteration -var ITERATION     
    book getword detCleanupResid $pageName camera -var CAMERA
    book getword detCleanupResid $pageName data_state -var CLEANUP_MODE
    book getword detCleanupResid $pageName dbname -var DBNAME

    # specify choice of local or remote host based on camera and diff (class_id)
    set.host.for.camera $CAMERA FPA

    stdout $LOGDIR/detrend.cleanup.resid.log
    stderr $LOGDIR/detrend.cleanup.resid.log

    # XXX is everything listed here needed?
    $run = ipp_cleanup.pl --stage detrend.resid --stage_id $DET_ID.$EXP_ID  --camera $CAMERA --mode $CLEANUP_MODE --dbname $DBNAME
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
    process_exit detCleanupResid $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword detCleanupResid $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword detCleanupResid $options:0 pantaskState TIMEOUT
  end
end

########## cleanup stack ###########
task	       detrend.cleanup.stack.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1
  active       false

  stdout NULL
  stderr $LOGDIR/detrend.cleanup.stack.log

  task.exec
    $run = dettool -pendingcleanup_stacked
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detCleanupStackedImfile_DB
      $run = $run -dbname $DB:$detCleanupStackedImfile_DB
      $detCleanupStackedImfile_DB ++
      if ($detCleanupStackedImfile_DB >= $DB:n) set detCleanupStackedImfile_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout detCleanupStackedImfile -key det_id:iteration:class_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook detCleanupStackedImfile
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup detCleanupStackedImfile
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


# run the ipp_cleanup.pl script on pending images
task	       detrend.cleanup.stack.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60
  active       true

  task.exec
    book npages detCleanupStackedImfile -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in detCleanupStackedImfile (pantaskState == INIT)
    book getpage detCleanupStackedImfile 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword detCleanupStackedImfile $pageName pantaskState RUN
    book getword detCleanupStackedImfile $pageName det_id   -var DET_ID   
    book getword detCleanupStackedImfile $pageName iteration -var ITERATION
    book getword detCleanupStackedImfile $pageName class_id -var CLASS_ID 
    book getword detCleanupStackedImfile $pageName camera -var CAMERA
    book getword detCleanupStackedImfile $pageName data_state -var CLEANUP_MODE
    book getword detCleanupStackedImfile $pageName dbname -var DBNAME

    # specify choice of local or remote host based on camera and chip (class_id)
    set.host.for.camera $CAMERA FPA

    stdout $LOGDIR/detrend.cleanup.stack.log
    stderr $LOGDIR/detrend.cleanup.stack.log

    # XXX is everything listed here needed?
    $run = ipp_cleanup.pl --stage detrend.stack.imfile --stage_id $DET_ID --camera $CAMERA --mode $CLEANUP_MODE
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
    process_exit detCleanupStackedImfile $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword detCleanupStackedImfile $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword detCleanupStackedImfile $options:0 pantaskState TIMEOUT
  end
end
 

########## cleanup normstat ###########
task	       detrend.cleanup.normstat.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1
  active       false

  stdout NULL
  stderr $LOGDIR/detrend.cleanup.normstat.log

  task.exec
    $run = dettool -pendingcleanup_normalizedstat
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detCleanupNormStatImfile_DB
      $run = $run -dbname $DB:$detCleanupNormStatImfile_DB
      $detCleanupNormStatImfile_DB ++
      if ($detCleanupNormStatImfile_DB >= $DB:n) set detCleanupNormStatImfile_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout detCleanupNormStatImfile -key det_id:iteration -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook detCleanupNormStatImfile
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup detCleanupNormStatImfile
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

# run the ipp_cleanup.pl script on pending images
task	       detrend.cleanup.normstat.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60
  active       false

  task.exec
    book npages detCleanupNormStatImfile -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in detCleanupNormStatImfile (pantaskState == INIT)
    book getpage detCleanupNormStatImfile 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword detCleanupNormStatImfile $pageName pantaskState RUN
    book getword detCleanupNormStatImfile $pageName det_id   -var DET_ID   
    book getword detCleanupNormStatImfile $pageName iteration -var ITERATION
    book getword detCleanupNormStatImfile $pageName camera -var CAMERA
    book getword detCleanupNormStatImfile $pageName data_state -var CLEANUP_MODE
    book getword detCleanupNormStatImfile $pageName dbname -var DBNAME

    # specify choice of local or remote host based on camera and chip (class_id)
    set.host.for.camera $CAMERA FPA

    stdout $LOGDIR/detrend.cleanup.normstat.log
    stderr $LOGDIR/detrend.cleanup.normstat.log

    # XXX is everything listed here needed?
    $run = ipp_cleanup.pl --stage detrend.normstat.imfile --stage_id $DET_ID --camera $CAMERA --mode $CLEANUP_MODE
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
    process_exit detCleanupNormStatImfile $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword detCleanupNormStatImfile $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword detCleanupNormStatImfile $options:0 pantaskState TIMEOUT
  end
end
 
########## cleanup norm (normalized.imfile) ###########
task	       detrend.cleanup.norm.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1
  active       false

  stdout NULL
  stderr $LOGDIR/detrend.cleanup.norm.log

  task.exec
    $run = dettool -pendingcleanup_normalizedimfile
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detCleanupNormImfile_DB
      $run = $run -dbname $DB:$detCleanupNormImfile_DB
      $detCleanupNormImfile_DB ++
      if ($detCleanupNormImfile_DB >= $DB:n) set detCleanupNormImfile_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout detCleanupNormImfile -key det_id:iteration:class_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook detCleanupNormImfile
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup detCleanupNormImfile
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

# run the ipp_cleanup.pl script on pending images
task	       detrend.cleanup.norm.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60
  active       false

  task.exec
    book npages detCleanupNormImfile -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in detCleanupNormImfile (pantaskState == INIT)
    book getpage detCleanupNormImfile 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword detCleanupNormImfile $pageName pantaskState RUN
    book getword detCleanupNormImfile $pageName det_id   -var DET_ID   
    book getword detCleanupNormImfile $pageName iteration -var ITERATION
    book getword detCleanupNormImfile $pageName class_id -var CLASS_ID 
    book getword detCleanupNormImfile $pageName camera -var CAMERA
    book getword detCleanupNormImfile $pageName data_state -var CLEANUP_MODE
    book getword detCleanupNormImfile $pageName dbname -var DBNAME

    # specify choice of local or remote host based on camera and chip (class_id)
    set.host.for.camera $CAMERA FPA

    stdout $LOGDIR/detrend.cleanup.norm.log
    stderr $LOGDIR/detrend.cleanup.norm.log

    # XXX is everything listed here needed?
    $run = ipp_cleanup.pl --stage detrend.norm.imfile --stage_id $DET_ID --camera $CAMERA --mode $CLEANUP_MODE
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
    process_exit detCleanupNormImfile $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword detCleanupNormImfile $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword detCleanupNormImfile $options:0 pantaskState TIMEOUT
  end
end
 
########## cleanup normexp ###########
task	       detrend.cleanup.normexp.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1
  active       false

  stdout NULL
  stderr $LOGDIR/detrend.normexp.cleanup.log

  task.exec
    $run = dettool -pendingcleanup_normalizedexp
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detCleanupNormExp_DB
      $run = $run -dbname $DB:$detCleanupNormExp_DB
      $detCleanupNormExp_DB ++
      if ($detCleanupNormExp_DB >= $DB:n) set detCleanupNormExp_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout detCleanupNormExp -key det_id:iteration -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook detCleanupNormExp
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup detCleanupNormExp
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

# run the ipp_cleanup.pl script on pending images
task	       detrend.cleanup.normexp.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60
  active       false

  task.exec
    book npages detCleanupNormExp -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in detCleanupNormExp (pantaskState == INIT)
    book getpage detCleanupNormExp 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword detCleanupNormExp $pageName pantaskState RUN
    book getword detCleanupNormExp $pageName det_id   -var DET_ID   
    book getword detCleanupNormExp $pageName iteration -var ITERATION
    book getword detCleanupNormExp $pageName camera -var CAMERA
    book getword detCleanupNormExp $pageName data_state -var CLEANUP_MODE
    book getword detCleanupNormExp $pageName dbname -var DBNAME

    # specify choice of local or remote host based on camera and chip (class_id)
    set.host.for.camera $CAMERA FPA

    stdout $LOGDIR/detrend.cleanup.normexp.log
    stderr $LOGDIR/detrend.cleanup.normexp.log

    # XXX is everything listed here needed?
    $run = ipp_cleanup.pl --stage detrend.norm.exp --stage_id $DET_ID --camera $CAMERA --mode $CLEANUP_MODE
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
    process_exit detCleanupNormExp $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword detCleanupNormExp $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword detCleanupNormExp $options:0 pantaskState TIMEOUT
  end
end
 

