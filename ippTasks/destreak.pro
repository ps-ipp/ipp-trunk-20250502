## destreak.pro : support for the streak removal : -*- sh -*-

# test for required global variables
check.globals

$LOGSUBDIR = $LOGDIR/destreak
mkdir $LOGSUBDIR

### Initialise the books containing the tasks to do
book init magicToDS
book init magicDSToRevert

### indexes into Database lists
$magicToDS_DB = 0
$magicDSAdvance_DB = 0
$magicDSToRevert_DB = 0
$magicDSCompletedRevert_DB = 0
$magicDSToCleanup_DB = 0

#list of stages
$DS_STAGE:n = 0
list DS_STAGE -add "raw"
list DS_STAGE -add "chip"
list DS_STAGE -add "camera"
list DS_STAGE -add "warp"
list DS_STAGE -add "diff"

$magicDSStage = 0
$magicDSRevertStage = 0

macro destreak.show.stages
    echo $DS_STAGE:n stages enabled for destreak processing
    for i 0 $DS_STAGE:n
        echo $i $DS_STAGE:$i
    end
    echo current magicDSStage = $magicDSStage
    echo current magicDSRevertStage = $magicDSRevertStage
end

### Check status of tasks
macro destreak.status
    book listbook magicToDS
    book listbook magicDSToRevert
end

### Reset tasks
macro destreak.reset
    book init magicToDS
    book init magicDSToRevert
end

### Turn tasks on
macro destreak.on
    task destreak.load
        active true
    end
    task destreak.run
        active true
    end
    task destreak.advance
        active true
    end
end

macro destreak.revert.on
    task destreak.revert.load
        active true
    end
    task destreak.revert.run
        active true
    end
    task destreak.completed.revert
        active true
    end
end

### Turn tasks off
macro destreak.off
    task destreak.load
        active false
    end
    task destreak.run
        active false
    end
    task destreak.advance
        active false
    end
end

macro destreak.revert.off
    task destreak.revert.load
        active false
    end
    task destreak.revert.run
        active false
    end
    task destreak.completed.revert
        active false
    end
end

task	       destreak.load
  host         local

  periods      -poll $LOADPOLL
  # this query can take a long time (XXX: the long time should be fixed now)
  periods      -exec 10
  periods      -timeout 120
  npending     1

#  stdout NULL
#  stderr $LOGSUBDIR/destreak.load.log

  task.exec
    # we check twice in case an entry has been removed from DS_STAGE
    if ($magicDSStage >= $DS_STAGE:n) set magicDSStage = 0

    $run = magicdstool -todestreak -stage $DS_STAGE:$magicDSStage
    $magicDSStage ++
    if ($magicDSStage >= $DS_STAGE:n) set magicDSStage = 0

    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$magicToDS_DB
      $run = $run -dbname $DB:$magicToDS_DB

      # only bump database number after we have gone through all of the stages
      if ($magicDSStage == 0)
          $magicToDS_DB ++
          if ($magicToDS_DB >= $DB:n) set magicToDS_DB = 0
      end
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout magicToDS -key magic_ds_id:component -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook magicToDS
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup magicToDS
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

task	       destreak.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    stdout $LOGSUBDIR/destreak.run.log
    stderr $LOGSUBDIR/destreak.run.log

    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC

    if ($NETWORK == 0) break
    book npages magicToDS -var N
    if ($N == 0) break

    # look for new images (pantaskState == INIT)
    book getpage magicToDS 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword magicToDS $pageName pantaskState RUN
    book getword magicToDS $pageName exp_id -var EXP_ID
    book getword magicToDS $pageName magic_ds_id -var MAGIC_DS_ID
    book getword magicToDS $pageName state -var RUN_STATE
    book getword magicToDS $pageName camera -var CAMERA
    book getword magicToDS $pageName streaks_uri -var STREAKS
    book getword magicToDS $pageName streaks_path_base -var STREAKS_PATH_BASE
    book getword magicToDS $pageName inv_streaks_uri -var INV_STREAKS
    book getword magicToDS $pageName inv_streaks_path_base -var INV_STREAKS_PATH_BASE
    book getword magicToDS $pageName stage -var STAGE
    book getword magicToDS $pageName stage_id -var STAGE_ID
    book getword magicToDS $pageName component -var COMPONENT
    book getword magicToDS $pageName uri -var URI
    book getword magicToDS $pageName path_base -var PATH_BASE
    book getword magicToDS $pageName cam_path_base -var CAM_PATH_BASE
    book getword magicToDS $pageName cam_reduction -var CAM_REDUCTION
    book getword magicToDS $pageName outroot -var OUTROOT
    book getword magicToDS $pageName recoveryroot -var RECROOT
    book getword magicToDS $pageName re_place -var REPLACE
    book getword magicToDS $pageName magicked -var MAGICKED
    book getword magicToDS $pageName dbname -var DBNAME
    book getword magicToDS $pageName diff_tess_id -var DIFF_TESS_ID
    book getword magicToDS $pageName mismatched_tess -var MISMATCHED_TESS

    substr $COMPONENT 0 3 COMP_HEAD
    if ("$COMP_HEAD" == "sky")
        set.host.for.skycell $COMPONENT
        set.workdir.by.skycell $COMPONENT $OUTROOT $default_host WORKDIR
    else 
        # assume component is a class_id, if not we will default to anyhost
        set.host.for.camera $CAMERA $COMPONENT
        set.workdir.by.camera $CAMERA $COMPONENT $OUTROOT $default_host WORKDIR
    end

    sprintf logfile "%s/%s.mds.%s.%s.%s.log" $WORKDIR $EXP_ID $MAGIC_DS_ID $STAGE_ID $COMPONENT

    # TODO: do not add recoveryroot or replace if they are null or zero

    $run = magic_destreak.pl --magic_ds_id $MAGIC_DS_ID --camera $CAMERA --exp_id $EXP_ID --streaks_path_base $STREAKS_PATH_BASE --inv_streaks_path_base $INV_STREAKS_PATH_BASE --streaks $STREAKS --inv_streaks $INV_STREAKS --stage $STAGE --stage_id $STAGE_ID --component $COMPONENT --uri $URI --path_base $PATH_BASE --cam_path_base $CAM_PATH_BASE --cam_reduction $CAM_REDUCTION --outroot $WORKDIR --logfile $logfile --recoveryroot $RECROOT --replace $REPLACE --magicked $MAGICKED --run-state $RUN_STATE

    if ($MISMATCHED_TESS) 
        book getword magicToDS $pageName diff_tess_id -var DIFF_TESS_ID
        $run = $run --mismatched_tess --diff_tess_id $DIFF_TESS_ID
    end
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end

    # if we are ready to run, drop the retry timeout low so we fill up the queue
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit    0
    process_exit magicToDS $options:0 $JOB_STATUS
   end

  # locked list
  task.exit    default
    showcommand failure
    process_exit magicToDS $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword magicToDS $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword magicToDS $options:0 pantaskState TIMEOUT
  end
end

task	       destreak.advance
    # task to finish processing for magicDSRuns 
  host         local

  periods      -poll $LOADPOLL
  periods      -exec 30
  periods      -timeout 300
  npending     1

#  stdout NULL
#  stderr $LOGSUBDIR/destreak.advance.log

  task.exec
    $run = magicdstool -advancerun 
    if ($DB:n != 0)

      $run = $run -dbname $DB:$magicDSAdvance_DB

      $magicDSAdvance_DB ++
      if ($magicDSAdvance_DB >= $DB:n) set magicDSAdvance_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    # since this command can be expensive, reduce the limit
    $run = $run -limit 24
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

task	       destreak.revert.load
  host         local
  periods      -poll 5
  periods      -exec 120
  periods      -timeout 120
  npending     1
  active       false

  stdout NULL
  stderr $LOGSUBDIR/destreak.revert.log

  task.exec
    $run = magicdstool -torevert -stage $DS_STAGE:$magicDSRevertStage
    $magicDSRevertStage ++
    if ($magicDSRevertStage >= $DS_STAGE:n) set magicDSRevertStage = 0

    if ($DB:n == 0)
      option DEFAULT
    else

      # save the DB name for the exit tasks
      option $DB:$magicDSToRevert_DB
      $run = $run -dbname $DB:$magicDSToRevert_DB

      # only bump database number after we have gone through all of the stages
      if ($magicDSRevertStage == 0)
          $magicDSToRevert_DB ++
          if ($magicDSToRevert_DB >= $DB:n) set magicDSToRevert_DB = 0
      end
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout magicDSToRevert -key magic_ds_id:component -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook magicDSToRevert
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup magicDSToRevert
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

task	       destreak.revert.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60
  active       false

  task.exec
    stdout $LOGSUBDIR/destreak.revert.run.log
    stderr $LOGSUBDIR/destreak.revert.run.log

    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC

    book npages magicDSToRevert -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images (pantaskState == INIT)
    book getpage magicDSToRevert 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword magicDSToRevert $pageName pantaskState RUN
    book getword magicDSToRevert $pageName exp_id -var EXP_ID
    book getword magicDSToRevert $pageName magic_ds_id -var MAGIC_DS_ID
    book getword magicDSToRevert $pageName camera -var CAMERA
    book getword magicDSToRevert $pageName stage -var STAGE
    book getword magicDSToRevert $pageName state -var RUN_STATE
    book getword magicDSToRevert $pageName stage_id -var STAGE_ID
    book getword magicDSToRevert $pageName component -var COMPONENT
    book getword magicDSToRevert $pageName path_base -var PATH_BASE
    book getword magicDSToRevert $pageName recovery_path_base -var RECOVERY_PATH_BASE
    book getword magicDSToRevert $pageName cam_path_base -var CAM_PATH_BASE
    book getword magicDSToRevert $pageName cam_reduction -var CAM_REDUCTION
    book getword magicDSToRevert $pageName outroot -var OUTROOT
    book getword magciDSToRevert $pageName bytes -var BYTES
    book getword magciDSToRevert $pageName md5sum -var md5sum
#    book getword magicDSToRevert $pageName recoveryroot -var RECROOT
    book getword magicDSToRevert $pageName re_place -var REPLACE
    book getword magicDSToRevert $pageName bothways -var BOTHWAYS
    book getword magicDSToRevert $pageName magicked -var MAGICKED
    book getword magicDSToRevert $pageName dbname -var DBNAME

    substr $COMPONENT 0 3 COMP_HEAD
    if ("$COMP_HEAD" == "sky")
        set.host.for.skycell $COMPONENT
        set.workdir.by.skycell $COMPONENT $OUTROOT $default_host WORKDIR
    else 
        # assume component is a class_id, if not we will default to anyhost
        set.host.for.camera $CAMERA $COMPONENT
        set.workdir.by.camera $CAMERA $COMPONENT $OUTROOT $default_host WORKDIR
    end

    if (("$RUN_STATE" == "goto_restored") && ("$STAGE" == "camera"))

        sprintf logfile "%s.dsrestore.log" $PATH_BASE

        $run = destreak_restore_camera.pl --magic_ds_id $MAGIC_DS_ID --camera $CAMERA  --cam_id $STAGE_ID --path_base $PATH_BASE --dbname $DBNAME --logfile $logfile

    else
        sprintf logfile "%s/%s.mds.revert.%s.%s.%s.log" $WORKDIR $EXP_ID $MAGIC_DS_ID $STAGE_ID $COMPONENT
        $run = magic_destreak_revert.pl --magic_ds_id $MAGIC_DS_ID --camera $CAMERA --stage $STAGE --stage_id $STAGE_ID --component $COMPONENT --path_base $PATH_BASE --cam_path_base $CAM_PATH_BASE --cam_reduction $CAM_REDUCTION --outroot $WORKDIR --logfile $logfile --replace $REPLACE --bothways $BOTHWAYS --magicked $MAGICKED --run-state $RUN_STATE --recovery_path_base $RECOVERY_PATH_BASE
    end
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    # if we are ready to run, drop the retry timeout low so we fill up the queue
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit    0
    process_exit magicDSToRevert $options:0 $JOB_STATUS
   end

  # locked list
  task.exit    default
    showcommand failure
    process_exit magicDSToRevert $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword magicDSToRevert $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword magicDSToRevert $options:0 pantaskState TIMEOUT
  end
end

task	       destreak.completed.revert
    # task to finish processing for magicDSRuns being reverted or restored
  host         local

  periods      -poll $LOADPOLL
  #periods      -exec $LOADEXEC
  periods      -exec 30
  periods      -timeout 20
  npending     1
  active       false

  stdout NULL
  stderr $LOGSUBDIR/destreak.completed.revert.log

  task.exec
    $run = magicdstool -completedrevert 
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$magicDSCompletedRevert_DB

      $run = $run -dbname $DB:$magicDSCompletedRevert_DB

      $magicDSCompletedRevert_DB ++
      if ($magicDSCompletedRevert_DB >= $DB:n) set magicDSCompletedRevert_DB = 0
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

