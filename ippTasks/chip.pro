## chip.pro : globals and support macros : -*- sh -*-
## this file contains the tasks for running the phase 0 stage
## these tasks use the book chipPendingImfile

# test for required global variables
check.globals

book init chipPendingImfile

macro chip.status
  book listbook chipPendingImfile
end

macro chip.reset
  book init chipPendingImfile
end

macro chip.on
  task chip.imfile.load
    active true
  end
  task chip.imfile.run
    active true
  end
  task chip.advanceexp
    active true
  end
  task chip.revert
    active true
  end
end

macro chip.off
  task chip.imfile.load
    active false
  end
  task chip.imfile.run
    active false
  end
  task chip.advanceexp
    active false
  end
  task chip.revert
    active false
  end
end

macro chip.revert.on
  task chip.revert
    active true
  end
end

macro chip.revert.off
  task chip.revert
    active false
  end
end

# this variable will cycle through the known database names
$chip_DB = 0
$chip_revert_DB = 0

# select images ready for chip analysis
# new entries are added to chipPendingImfile
# skip already-present entries
task	       chip.imfile.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/chip.imfile.log

  task.exec
    if ($LABEL:n == 0) break
    $run = chiptool -pendingimfile
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$chip_DB
      $run = $run -dbname $DB:$chip_DB
      $chip_DB ++
      if ($chip_DB >= $DB:n) set chip_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout chipPendingImfile -key chip_id:class_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook chipPendingImfile
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup chipPendingImfile
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

# run the chip_imfile.pl script on pending images
task	       chip.imfile.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC

    book npages chipPendingImfile -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    if ($BURNTOOLING == 1) break
    
    # look for new images in chipPendingImfile (pantaskState == INIT)
    book getpage chipPendingImfile 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword chipPendingImfile $pageName pantaskState RUN
    book getword chipPendingImfile $pageName camera -var CAMERA
    book getword chipPendingImfile $pageName exp_id -var EXP_ID
    book getword chipPendingImfile $pageName exp_tag -var EXP_TAG
    book getword chipPendingImfile $pageName raw_magicked -var RAW_MAGICKED
    book getword chipPendingImfile $pageName deburned -var DEBURNED
    book getword chipPendingImfile $pageName chip_id -var CHIP_ID
    book getword chipPendingImfile $pageName chip_imfile_id -var CHIP_IMFILE_ID
    book getword chipPendingImfile $pageName workdir -var WORKDIR_TEMPLATE
    book getword chipPendingImfile $pageName class_id -var CLASS_ID
    book getword chipPendingImfile $pageName uri -var URI
    book getword chipPendingImfile $pageName dbname -var DBNAME
    book getword chipPendingImfile $pageName reduction -var REDUCTION
    book getword chipPendingImfile $pageName state -var RUN_STATE
    book getword chipPendingImfile $pageName path_base -var PATH_BASE
    book getword chipPendingImfile $pageName update_mode -var UPDATE_MODE

    if ($RAW_MAGICKED > 0)
        $MAGICKED_ARG = "--magicked $RAW_MAGICKED"
    else 
        $MAGICKED_ARG = ""
    end

    # specify choice of local or remote host based on camera and chip (class_id)
    set.host.for.camera $CAMERA $CLASS_ID

    # set the WORKDIR variable
    set.workdir.by.camera $CAMERA $CLASS_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    # notes on how this works:
    # -- raw workdir examples:
    # file://data/@HOST@.0/gpc1/20080130
    # neb:///@HOST@-vol0/gpc1/20080130 (need to supply volname?, or are we re-defining this each time?)
    # -- out workdir examples:
    # file://data/ipp004.0/gpc1/20080130
    # neb:///ipp004-vol0/gpc1/20080130

    if ("$PATH_BASE" == "NULL") 
        ## generate outroot specific to this exposure (& chip)
        sprintf outroot "%s/%s/%s.ch.%s" $WORKDIR $EXP_TAG $EXP_TAG $CHIP_ID
    else
        $outroot = $PATH_BASE
    end

    stdout $LOGDIR/chip.imfile.log
    stderr $LOGDIR/chip.imfile.log

    $run = chip_imfile.pl --threads @MAX_THREADS@ --exp_id $EXP_ID --chip_id $CHIP_ID --chip_imfile_id $CHIP_IMFILE_ID --class_id $CLASS_ID --uri $URI --camera $CAMERA --run-state $RUN_STATE $MAGICKED_ARG --deburned $DEBURNED --outroot $outroot --redirect-output
    if ("$REDUCTION" != "NULL")
      $run = $run --reduction $REDUCTION
    end
    if ($UPDATE_MODE)
        $run = $run --update-mode $UPDATE_MODE
    end

    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    # if we are unable to run the 'exec', use a long retry time
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit    default
    process_exit chipPendingImfile $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    ### Getting a lot of chip crashes (no idea why), so remove verbosity for now
    #showcommand crash
    #echo "hostname: $JOB_HOSTNAME"

    # Set a fault code in the database
    exec chiptool -addprocessedimfile -dbname $DBNAME -chip_id $CHIP_ID -class_id $CLASS_ID -fault $EXIT_CRASH_ERR
    process_exit chipPendingImfile $options:0 $EXIT_CRASH_ERR
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword chipPendingImfile $options:0 pantaskState TIMEOUT
  end
end

# this variable will cycle through the known database names
$chip_advance_DB = 0

# advance exposures for which all imfiles have completed processing
# sets the exposure state to full and queues warp processing if requested
task	       chip.advanceexp
  host         local

  periods      -poll $LOADPOLL
#  periods      -exec $LOADEXEC
  periods      -exec 30
  periods      -timeout 60
  npending     1

  stdout NULL
  stderr $LOGDIR/chip.advanceexp.log

  task.exec
    if ($LABEL:n == 0) break
    $run = chiptool -advanceexp -limit 10
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$chip_advance_DB
      $run = $run -dbname $DB:$chip_advance_DB
      $chip_advance_DB ++
      if ($chip_advance_DB >= $DB:n) set chip_advance_DB = 0
    end
    add_poll_args run
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

task chip.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = chiptool -revertprocessedimfile
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$chip_revert_DB
      $run = $run -dbname $DB:$chip_revert_DB
      $chip_revert_DB ++
      if ($chip_revert_DB >= $DB:n) set chip_revert_DB = 0
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
