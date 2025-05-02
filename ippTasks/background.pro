## background.pro : globals and support macros : -*- sh -*-

check.globals

book init chipBackgroundRuns
book init warpBackgroundRuns

macro bg.status
  book listbook chipBackgroundRuns
  book listbook warpBackgroundRuns
end

macro bg.reset
  book init chipBackgroundRuns
  book init warpBackgroundRuns
end

macro bg.on
  task bg.chip.load
    active true
  end
  task bg.chip.run
    active true
  end
  task bg.chip.advance
    active true
  end
  task bg.chip.revert
    active true
  end
  task bg.warp.load
    active true
  end
  task bg.warp.run
    active true
  end
  task bg.warp.advance
    active true
  end
  task bg.warp.revert
    active true
  end
end

macro bg.off
  task bg.chip.load
    active false
  end
  task bg.chip.run
    active false
  end
  task bg.chip.advance
    active false
  end
  task bg.chip.revert
    active false
  end
  task bg.warp.load
    active false
  end
  task bg.warp.run
    active false
  end
  task bg.warp.advance
    active false
  end
  task bg.warp.revert
    active false
  end
end

macro bg.revert.on
  task bg.chip.revert
    active true
  end
  task bg.warp.revert
    active true
  end
end

macro bg.revert.off
  task bg.chip.revert
    active false
  end
  task bg.warp.revert
    active false
  end
end


# These variables cycle through the known database names
$bg_chip_load_DB = 0
$bg_chip_advance_DB = 0
$bg_chip_revert_DB = 0
$bg_warp_load_DB = 0
$bg_warp_advance_DB = 0
$bg_warp_revert_DB = 0

task	       bg.chip.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/bg.chip.load.log

  task.exec
    if ($LABEL:n == 0) break
    $run = bgtool -tochip
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$bg_chip_load_DB
      $run = $run -dbname $DB:$bg_chip_load_DB
      $bg_chip_load_DB ++
      if ($bg_chip_load_DB >= $DB:n) set bg_chip_load_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout chipBackgroundRuns -key chip_bg_id:class_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook chipBackgroundRuns
    end
    process_cleanup chipBackgroundRuns
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

task	       bg.chip.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    periods -exec $RUNEXEC

    book npages chipBackgroundRuns -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    if ($BURNTOOLING == 1) break
    
    book getpage chipBackgroundRuns 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword chipBackgroundRuns $pageName pantaskState RUN
    book getword chipBackgroundRuns $pageName camera -var CAMERA
    book getword chipBackgroundRuns $pageName chip_bg_id -var CHIP_BG_ID
    book getword chipBackgroundRuns $pageName class_id -var CLASS_ID
    book getword chipBackgroundRuns $pageName exp_tag -var EXP_TAG
    book getword chipBackgroundRuns $pageName workdir -var WORKDIR_TEMPLATE
    book getword chipBackgroundRuns $pageName dbname -var DBNAME
    book getword chipBackgroundRuns $pageName reduction -var REDUCTION

    set.host.for.camera $CAMERA $CLASS_ID
    set.workdir.by.camera $CAMERA $CLASS_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    sprintf outroot "%s/%s/%s.bgc.%s" $WORKDIR $EXP_TAG $EXP_TAG $CHIP_BG_ID
    stdout $LOGDIR/bg.chip.run.log
    stderr $LOGDIR/bg.chip.run.log

    $run = background_chip.pl --threads @MAX_THREADS@ --chip_bg_id $CHIP_BG_ID --class_id $CLASS_ID --camera $CAMERA --outroot $outroot --redirect-output
    if ("$REDUCTION" != "NULL")
      $run = $run --reduction $REDUCTION
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
    process_exit chipBackgroundRuns $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    exec bgtool -addchip -dbname $DBNAME -chip_bg_id $CHIP_BG_ID -class_id $CLASS_ID -fault $EXIT_CRASH_ERR
    process_exit chipBackgroundRuns $options:0 $EXIT_CRASH_ERR
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword chipBackgroundRuns $options:0 pantaskState TIMEOUT
  end
end

# advance exposures for which all imfiles have completed processing
# sets the exposure state to full and queues warp processing if requested
task	       bg.chip.advance
  host         local

  periods      -poll $LOADPOLL
#  periods      -exec $LOADEXEC
  periods      -exec 30
  periods      -timeout 60
  npending     1

  stdout NULL
  stderr $LOGDIR/bg.chip.advance.log

  task.exec
    if ($LABEL:n == 0) break
    $run = bgtool -advancechip
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$bg_chip_advance_DB
      $run = $run -dbname $DB:$bg_chip_advance_DB
      $bg_chip_advance_DB ++
      if ($bg_chip_advance_DB >= $DB:n) set bg_chip_advance_DB = 0
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

task bg.chip.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGDIR/bg.chip.revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = bgtool -revertchip
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$bg_chip_revert_DB
      $run = $run -dbname $DB:$bg_chip_revert_DB
      $bg_chip_revert_DB ++
      if ($bg_chip_revert_DB >= $DB:n) set bg_chip_revert_DB = 0
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

task	       bg.warp.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/bg.warp.load.log

  task.exec
    if ($LABEL:n == 0) break
    $run = bgtool -towarp
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$bg_warp_load_DB
      $run = $run -dbname $DB:$bg_warp_load_DB
      $bg_warp_load_DB ++
      if ($bg_warp_load_DB >= $DB:n) set bg_warp_load_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout warpBackgroundRuns -key warp_bg_id:skycell_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook warpBackgroundRuns
    end
    process_cleanup warpBackgroundRuns
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

task	       bg.warp.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    periods -exec $RUNEXEC

    book npages warpBackgroundRuns -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    if ($BURNTOOLING == 1) break
    
    book getpage warpBackgroundRuns 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword warpBackgroundRuns $pageName pantaskState RUN
    book getword warpBackgroundRuns $pageName camera -var CAMERA
    book getword warpBackgroundRuns $pageName warp_bg_id -var WARP_BG_ID
    book getword warpBackgroundRuns $pageName skycell_id -var SKYCELL_ID
    book getword warpBackgroundRuns $pageName tess_id -var TESS_DIR
    book getword warpBackgroundRuns $pageName exp_tag -var EXP_TAG
    book getword warpBackgroundRuns $pageName workdir -var WORKDIR_TEMPLATE
    book getword warpBackgroundRuns $pageName dbname -var DBNAME
    book getword warpBackgroundRuns $pageName reduction -var REDUCTION

    set.host.for.skycell $SKYCELL_ID
    set.workdir.by.skycell $SKYCELL_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    sprintf outroot "%s/%s/%s.bgw.%s.%s" $WORKDIR $EXP_TAG $EXP_TAG $WARP_BG_ID $SKYCELL_ID
    stdout $LOGDIR/bg.warp.run.log
    stderr $LOGDIR/bg.warp.run.log

    $run = background_warp.pl --threads @MAX_THREADS@ --warp_bg_id $WARP_BG_ID --skycell_id $SKYCELL_ID --tess_dir $TESS_DIR --camera $CAMERA --outroot $outroot --redirect-output
    if ("$REDUCTION" != "NULL")
      $run = $run --reduction $REDUCTION
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
    process_exit warpBackgroundRuns $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    exec bgtool -addwarp -dbname $DBNAME -warp_bg_id $WARP_BG_ID -skycell_id $SKYCELL_ID -fault $EXIT_CRASH_ERR
    process_exit warpBackgroundRuns $options:0 $EXIT_CRASH_ERR
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword warpBackgroundRuns $options:0 pantaskState TIMEOUT
  end
end

# advance exposures for which all imfiles have completed processing
# sets the exposure state to full and queues warp processing if requested
task	       bg.warp.advance
  host         local

  periods      -poll $LOADPOLL
  periods      -exec 30
  periods      -timeout 60
  npending     1

  stdout NULL
  stderr $LOGDIR/bg.warp.advance.log

  task.exec
    if ($LABEL:n == 0) break
    $run = bgtool -advancewarp
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$bg_warp_advance_DB
      $run = $run -dbname $DB:$bg_warp_advance_DB
      $bg_warp_advance_DB ++
      if ($bg_warp_advance_DB >= $DB:n) set bg_warp_advance_DB = 0
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

task bg.warp.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGDIR/bg.warp.revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = bgtool -revertwarp
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$bg_warp_revert_DB
      $run = $run -dbname $DB:$bg_warp_revert_DB
      $bg_warp_revert_DB ++
      if ($bg_warp_revert_DB >= $DB:n) set bg_warp_revert_DB = 0
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
