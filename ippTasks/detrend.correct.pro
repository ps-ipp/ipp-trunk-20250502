## detrend.correct.pro : globals and support macros : -*- sh -*-
## this file contains the tasks for running the detrend correction stage
## these tasks use the books detPendingCorrectImfile

# test for required global variables
check.globals

book init detPendingCorrectImfile

macro detcorr.reset
  book init detPendingCorrectImfile
end

macro detcorr.status
  echo detPendingCorrectImfile
  book listbook detPendingCorrectImfile
end

macro detcorr.on
  task detrend.correct.load
    active true
  end
  task detrend.correct.run
    active true
  end
end

macro detcorr.off
  task detrend.correct.load
    active false
  end
  task detrend.correct.run
    active false
  end
end


# these variables will cycle through the known database names
$detPendingCorrectImfile_DB = 0

# select images ready for copy 
# new entries are added to detPendingCorrectImfile
# compare the new list with the ones already selected
task	       detrend.correct.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/detcorr.load.log

  task.exec
    $run = dettool -tocorrectimfile
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$detPendingCorrectImfile_DB
      $run = $run -dbname $DB:$detPendingCorrectImfile_DB
      $detPendingCorrectImfile_DB ++
      if ($detPendingCorrectImfile_DB >= $DB:n) set detPendingCorrectImfile_DB = 0
    end
    # add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout detPendingCorrectImfile -key det_id:class_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook detPendingCorrectImfile
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup detPendingCorrectImfile
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

# run detrend_correct_imfile.pl on pending images
task	       detrend.correct.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 30

  task.exec
    book npages detPendingCorrectImfile -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in detPendingCorrectImfile
    book getpage detPendingCorrectImfile 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword detPendingCorrectImfile $pageName pantaskState RUN
    book getword detPendingCorrectImfile $pageName det_id    -var DET_ID   
    book getword detPendingCorrectImfile $pageName class_id  -var CLASS_ID 
    book getword detPendingCorrectImfile $pageName det_type  -var DET_TYPE
    book getword detPendingCorrectImfile $pageName camera    -var CAMERA
    book getword detPendingCorrectImfile $pageName uri       -var URI      
    book getword detPendingCorrectImfile $pageName workdir   -var WORKDIR_TEMPLATE
    book getword detPendingCorrectImfile $pageName dbname    -var DBNAME

    # specify choice of local or remote host based on camera and chip (class_id)
    set.host.for.camera $CAMERA $CLASS_ID

    # set the WORKDIR variable
    set.workdir.by.camera $CAMERA $CLASS_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    ## generate outroot specific to this exposure (& chip)
    sprintf outroot "%s/%s.%s.%s/%s.%s.%s" $WORKDIR $CAMERA $DET_TYPE $DET_ID $CAMERA $DET_TYPE $DET_ID

    stdout $LOGDIR/detcorr.run.log
    stderr $LOGDIR/detcorr.run.log

    $run = detrend_correct_imfile.pl --det_id $DET_ID --class_id $CLASS_ID --det_type $DET_TYPE --input_uri $URI --camera $CAMERA --outroot $outroot --redirect-output --verbose
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
    process_exit detPendingCorrectImfile $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword detPendingCorrectImfile $options:0 pantaskState CRASH
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
    book setword detPendingCorrectImfile $options:0 pantaskState TIMEOUT
 end
end

