## science.cleanup.pro : -*- sh -*-

macro science.cleanup.on
    chip.cleanup.on
    camera.cleanup.on
    fake.cleanup.on
    warp.cleanup.on
    diff.cleanup.on
    stack.cleanup.on
    chip.bg.cleanup.on
    warp.bg.cleanup.on
end

check.globals

# if set tell cleanup script to check for files from all components regardless of the data_state
$CHECK_ALL_COMPONENTS = 0
macro set.check.all.components
    $CHECK_ALL_COMPONENTS = 1
end
macro clear.check.all.components
    $CHECK_ALL_COMPONENTS = 0
end
macro get.check.all.components
    echo CHECK_ALL_COMPONENTS = $CHECK_ALL_COMPONENTS
end


## chip.cleanup.pro : -*- sh -*-

book init chipPendingCleanup

macro chip.cleanup.status
  book listbook chipPendingCleanup
end

macro chip.cleanup.reset
  book init chipPendingCleanup
end

macro chip.cleanup.on
  task chip.cleanup.load
    active true
  end
  task chip.cleanup.run
    active true
  end
end

macro chip.cleanup.off
  task chip.cleanup.load
    active false
  end
  task chip.cleanup.run
    active false
  end
end

# this variable will cycle through the known database names
$chip_cleanup_DB = 0

# select images ready for chip analysis
# new entries are added to chipPendingImfile
# skip already-present entries
task	       chip.cleanup.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1
  active       false

  stdout NULL
  stderr $LOGDIR/chip.cleanup.log

  task.exec
    if ($LABEL:n == 0) break
    $run = chiptool -pendingcleanuprun
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$chip_cleanup_DB
      $run = $run -dbname $DB:$chip_cleanup_DB
      $chip_cleanup_DB ++
      if ($chip_cleanup_DB >= $DB:n) set chip_cleanup_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout chipPendingCleanup -key chip_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook chipPendingCleanup
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup chipPendingCleanup
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
task	       chip.cleanup.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60
  active       false

  task.exec

    book npages chipPendingCleanup -var N
    if ($N == 0)
        periods -exec $RUNEXEC
        break
    end
    if ($NETWORK == 0) break
    
    # look for new images in chipPendingCleanup (pantaskState == INIT)
    book getpage chipPendingCleanup 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword chipPendingCleanup $pageName pantaskState RUN
    book getword chipPendingCleanup $pageName camera -var CAMERA
    book getword chipPendingCleanup $pageName state -var CLEANUP_MODE
    book getword chipPendingCleanup $pageName chip_id -var CHIP_ID
    book getword chipPendingCleanup $pageName exp_tag -var EXP_TAG
    book getword chipPendingCleanup $pageName workdir -var WORKDIR_TEMPLATE
    book getword chipPendingCleanup $pageName dbname -var DBNAME

    # specify choice of local or remote host based on camera and chip (in this case FPA)
    set.host.for.camera $CAMERA FPA

    set.workdir.by.camera $CAMERA FPA $WORKDIR_TEMPLATE $default_host WORKDIR

    sprintf LOGFILE "%s/%s/%s.ch.%s.cleanup.log" $WORKDIR $EXP_TAG $EXP_TAG $CHIP_ID

    stdout $LOGDIR/chip.cleanup.log
    stderr $LOGDIR/chip.cleanup.log

    $run = ipp_cleanup.pl --stage chip --stage_id $CHIP_ID --camera $CAMERA --mode $CLEANUP_MODE --logfile $LOGFILE
    if ($CHECK_ALL_COMPONENTS)
        $run = $run --check-all
    end
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit    default
    process_exit chipPendingCleanup $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword chipPendingCleanup $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword chipPendingCleanup $options:0 pantaskState TIMEOUT
  end
end

## camera.cleanup.pro

book init camPendingCleanup

macro camera.cleanup.status
  book listbook camPendingCleanup
end

macro camera.cleanup.reset
  book init camPendingCleanup
end

macro camera.cleanup.on
  task camera.cleanup.load
    active true
  end
  task camera.cleanup.run
    active true
  end
end

macro camera.cleanup.off
  task camera.cleanup.load
    active false
  end
  task camera.cleanup.run
    active false
  end
end

# this variable will cycle through the known database names
$camera_cleanup_DB = 0

# select images ready for cam analysis
# new entries are added to camPendingImfile
# skip already-present entries
task	       camera.cleanup.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1
  active       false

  stdout NULL
  stderr $LOGDIR/camera.cleanup.log

  task.exec
    if ($LABEL:n == 0) break
    $run = camtool -pendingcleanuprun
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$camera_cleanup_DB
      $run = $run -dbname $DB:$camera_cleanup_DB
      $camera_cleanup_DB ++
      if ($camera_cleanup_DB >= $DB:n) set camera_cleanup_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout camPendingCleanup -key cam_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook camPendingCleanup
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup camPendingCleanup
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
task	       camera.cleanup.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60
  active       false

  task.exec
    book npages camPendingCleanup -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in camPendingCleanup (pantaskState == INIT)
    book getpage camPendingCleanup 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword camPendingCleanup $pageName pantaskState RUN
    book getword camPendingCleanup $pageName camera -var CAMERA
    book getword camPendingCleanup $pageName state  -var CLEANUP_MODE
    book getword camPendingCleanup $pageName cam_id -var CAM_ID
    book getword camPendingCleanup $pageName exp_tag -var EXP_TAG
    book getword camPendingCleanup $pageName workdir -var WORKDIR_TEMPLATE
    book getword camPendingCleanup $pageName dbname -var DBNAME

    # specify choice of local or remote host based on camera and cam (class_id)
    set.host.for.camera $CAMERA FPA

    # set the WORKDIR variable
    set.workdir.by.camera $CAMERA FPA $WORKDIR_TEMPLATE $default_host WORKDIR

    ## generate LOGFILE specific to this exposure (& cam_id)
    sprintf LOGFILE "%s/%s/%s.cm.%s.cleanup.log" $WORKDIR $EXP_TAG $EXP_TAG $CAM_ID

    stdout $LOGDIR/camera.cleanup.log
    stderr $LOGDIR/camera.cleanup.log

    $run = ipp_cleanup.pl --stage camera --stage_id $CAM_ID --camera $CAMERA --mode $CLEANUP_MODE --logfile $LOGFILE
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
    process_exit camPendingCleanup $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword camPendingCleanup $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword camPendingCleanup $options:0 pantaskState TIMEOUT
  end
end


## fake.cleanup.pro

book init fakePendingCleanup

macro fake.cleanup.status
  book listbook fakePendingCleanup
end

macro fake.cleanup.reset
  book init fakePendingCleanup
end

macro fake.cleanup.on
  task fake.cleanup.load
    active true
  end
  task fake.cleanup.run
    active true
  end
end

macro fake.cleanup.off
  task fake.cleanup.load
    active false
  end
  task fake.cleanup.run
    active false
  end
end

# this variable will cycle through the known database names
$fake_cleanup_DB = 0

# select images ready for fake analysis
# new entries are added to fakePendingImfile
# skip already-present entries
task	       fake.cleanup.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1
  active       false

  stdout NULL
  stderr $LOGDIR/fake.cleanup.log

  task.exec
    if ($LABEL:n == 0) break
    $run = faketool -pendingcleanuprun
    if ($DB:n == 0)
      option DEFAULT
      command 
    else
      # save the DB name for the exit tasks
      option $DB:$fake_cleanup_DB
      $run = $run -dbname $DB:$fake_cleanup_DB
      $fake_cleanup_DB ++
      if ($fake_cleanup_DB >= $DB:n) set fake_cleanup_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout fakePendingCleanup -key fake_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook fakePendingCleanup
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup fakePendingCleanup
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
task	       fake.cleanup.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60
  active       false

  task.exec
    book npages fakePendingCleanup -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in fakePendingCleanup (pantaskState == INIT)
    book getpage fakePendingCleanup 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword fakePendingCleanup $pageName pantaskState RUN
    book getword fakePendingCleanup $pageName camera -var CAMERA
    book getword fakePendingCleanup $pageName state -var CLEANUP_MODE
    book getword fakePendingCleanup $pageName fake_id -var FAKE_ID
    book getword fakePendingCleanup $pageName dbname -var DBNAME

    # specify choice of local or remote host based on camera and fake (class_id)
    set.host.for.camera $CAMERA FPA

    stdout $LOGDIR/fake.cleanup.log
    stderr $LOGDIR/fake.cleanup.log

    # XXX is everything listed here needed?
    $run = ipp_cleanup.pl --stage fake --stage_id $FAKE_ID --camera $CAMERA --mode $CLEANUP_MODE
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
    process_exit fakePendingCleanup $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword fakePendingCleanup $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword fakePendingCleanup $options:0 pantaskState TIMEOUT
  end
end


## warp.cleanup.pro


book init warpPendingCleanup

$warpCleanup_DB = 0

macro warp.cleanup.status
  book listbook warpPendingCleanup
end

macro warp.cleanup.reset
  book init warpPendingCleanup
end


macro warp.cleanup.on
  task warp.cleanup.load
    active true
  end
  task warp.cleanup.run
    active true
  end
end
macro warp.cleanup.off
  task warp.cleanup.load
    active false
  end
  task warp.cleanup.run
    active false
  end
end

# select images ready for warp analysis
# new entries are added to warpPendingImfile
# skip already-present entries
task	       warp.cleanup.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1
  active       false

  stdout NULL
  stderr $LOGDIR/warp.cleanup.log

  task.exec
    if ($LABEL:n == 0) break
    $run = warptool -pendingcleanuprun
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$warpCleanup_DB
      $run = $run -dbname $DB:$warpCleanup_DB
      $warpCleanup_DB ++
      if ($warpCleanup_DB >= $DB:n) set warpCleanup_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout warpPendingCleanup -key warp_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook warpPendingCleanup
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup warpPendingCleanup
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
task	       warp.cleanup.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60
  active       false

  task.exec

    book npages warpPendingCleanup -var N
    if ($N == 0) 
        periods -exec $RUNEXEC
        break
    end
    if ($NETWORK == 0) break
    
    # look for new images in warpPendingCleanup (pantaskState == INIT)
    book getpage warpPendingCleanup 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword warpPendingCleanup $pageName pantaskState RUN
    book getword warpPendingCleanup $pageName camera -var CAMERA
    book getword warpPendingCleanup $pageName state -var CLEANUP_MODE
    book getword warpPendingCleanup $pageName warp_id -var WARP_ID
    book getword warpPendingCleanup $pageName exp_tag -var EXP_TAG
    book getword warpPendingCleanup $pageName workdir -var WORKDIR_TEMPLATE
    book getword warpPendingCleanup $pageName dbname -var DBNAME

    # specify choice of local or remote host based on camera and warp (class_id)
    set.host.for.camera $CAMERA FPA
    set.workdir.by.camera $CAMERA $WARP_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    sprintf LOGFILE "%s/%s/%s.wrp.%s.cleanup.log" $WORKDIR $EXP_TAG $EXP_TAG $WARP_ID

    stdout $LOGDIR/warp.cleanup.log
    stderr $LOGDIR/warp.cleanup.log

    $run = ipp_cleanup.pl --stage warp --stage_id $WARP_ID --camera $CAMERA --mode $CLEANUP_MODE --logfile $LOGFILE
    if ($CHECK_ALL_COMPONENTS)
        $run = $run --check-all
    end
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit    default
    process_exit warpPendingCleanup $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword warpPendingCleanup $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword warpPendingCleanup $options:0 pantaskState TIMEOUT
  end
end



## diff.cleanup.pro

book init diffCleanup

$diffCleanup_DB = 0

macro diff.cleanup.status
  book listbook diffCleanup
end

macro diff.cleanup.reset
  book init diffCleanup
end

macro diff.cleanup.on
  task diff.cleanup.load
    active true
  end
  task diff.cleanup.run
    active true
  end
end

macro diff.cleanup.off
  task diff.cleanup.load
    active false
  end
  task diff.cleanup.run
    active false
  end
end

# select images ready for diff analysis
# new entries are added to diffPendingImfile
# skip already-present entries
task	       diff.cleanup.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1
  active       false

  stdout NULL
  stderr $LOGDIR/diff.cleanup.log

  task.exec
    if ($LABEL:n == 0) break
    $run = difftool -pendingcleanuprun
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$diffCleanup_DB
      $run = $run -dbname $DB:$diffCleanup_DB
      $diffCleanup_DB ++
      if ($diffCleanup_DB >= $DB:n) set diffCleanup_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout diffCleanup -key diff_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook diffCleanup
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup diffCleanup
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
task	       diff.cleanup.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60
  active       false

  task.exec

    book npages diffCleanup -var N
    if ($N == 0)
        periods -exec $RUNEXEC
        break
    end
    if ($NETWORK == 0) break
    
    # look for new images in diffCleanup (pantaskState == INIT)
    book getpage diffCleanup 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword diffCleanup $pageName pantaskState RUN
    book getword diffCleanup $pageName camera -var CAMERA
    book getword diffCleanup $pageName state -var CLEANUP_MODE
    book getword diffCleanup $pageName diff_id -var DIFF_ID
    book getword diffCleanup $pageName tess_id -var TESS_ID
    book getword diffCleanup $pageName workdir -var WORKDIR_TEMPLATE
    book getword diffCleanup $pageName dbname -var DBNAME

    # specify choice of local or remote host based on camera and diff (class_id)
    set.host.for.camera $CAMERA FPA
    set.workdir.by.camera $CAMERA $DIFF_ID $WORKDIR_TEMPLATE $default_host WORKDIR
    sprintf LOGFILE "%s/%s/%s.dif.%s.cleanup.log" $WORKDIR $TESS_ID $TESS_ID $DIFF_ID


    stdout $LOGDIR/diff.cleanup.log
    stderr $LOGDIR/diff.cleanup.log

    $run = ipp_cleanup.pl --stage diff --stage_id $DIFF_ID --camera $CAMERA --mode $CLEANUP_MODE --logfile $LOGFILE
    if ($CHECK_ALL_COMPONENTS)
        $run = $run --check-all
    end
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit    default
    process_exit diffCleanup $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword diffCleanup $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword diffCleanup $options:0 pantaskState TIMEOUT
  end
end

## stack.cleanup.pro

book init stackCleanup

$stackCleanup_DB = 0

macro stack.cleanup.status
  book listbook stackCleanup
end

macro stack.cleanup.reset
  book init stackCleanup
end

macro stack.cleanup.on
  task stack.cleanup.load
    active true
  end
  task stack.cleanup.run
    active true
  end
end

macro stack.cleanup.off
  task stack.cleanup.load
    active false
  end
  task stack.cleanup.run
    active false
  end
end

# select images ready for stack analysis
# new entries are added to stackCleanup
# skip already-present entries
task	       stack.cleanup.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1
  active       false

  stdout NULL
  stderr $LOGDIR/stack.cleanup.log

  task.exec
    if ($LABEL:n == 0) break
    $run = stacktool -pendingcleanuprun
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$stackCleanup_DB
      $run = $run -dbname $DB:$stackCleanup_DB
      $stackCleanup_DB ++
      if ($stackCleanup_DB >= $DB:n) set stackCleanup_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout stackCleanup -key stack_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook stackCleanup
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup stackCleanup
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
task	       stack.cleanup.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60
  active       false

  task.exec
    book npages stackCleanup -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in stackCleanup (pantaskState == INIT)
    book getpage stackCleanup 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword stackCleanup $pageName pantaskState RUN
    book getword stackCleanup $pageName camera -var CAMERA
    book getword stackCleanup $pageName state -var CLEANUP_MODE
    book getword stackCleanup $pageName stack_id -var STACK_ID
    book getword stackCleanup $pageName path_base -var PATH_BASE
    book getword stackCleanup $pageName dbname -var DBNAME

    # specify choice of local or remote host based on camera and stack (class_id)
    set.host.for.camera $CAMERA FPA

    stdout $LOGDIR/stack.cleanup.log
    stderr $LOGDIR/stack.cleanup.log

    sprintf LOGFILE "%s.cleanup.log" $PATH_BASE
    $run = ipp_cleanup.pl --stage stack --stage_id $STACK_ID --camera $CAMERA --mode $CLEANUP_MODE --logfile $LOGFILE
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
    process_exit stackCleanup $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword stackCleanup $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword stackCleanup $options:0 pantaskState TIMEOUT
  end
end


book init chipBGPendingCleanup

macro chip.bg.cleanup.status
  book listbook chipBGPendingCleanup
end

macro chip.bg.cleanup.reset
  book init chipBGPendingCleanup
end

macro chip.bg.cleanup.on
  task chip.bg.cleanup.load
    active true
  end
  task chip.bg.cleanup.run
    active true
  end
end

macro chip.bg.cleanup.off
  task chip.bg.cleanup.load
    active false
  end
  task chip.bg.cleanup.run
    active false
  end
end

# this variable will cycle through the known database names
$chip_bg_cleanup_DB = 0

task	       chip.bg.cleanup.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1
  active       false

  stdout NULL
  stderr $LOGDIR/chip.bg.cleanup.log

  task.exec
    if ($LABEL:n == 0) break
    $run = bgtool -pendingcleanupchiprun
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$chip_bg_cleanup_DB
      $run = $run -dbname $DB:$chip_bg_cleanup_DB
      $chip_bg_cleanup_DB ++
      if ($chip_bg_cleanup_DB >= $DB:n) set chip_bg_cleanup_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout chipBGPendingCleanup -key chip_bg_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook chipBGPendingCleanup
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup chipBGPendingCleanup
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
task	       chip.bg.cleanup.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 300
  active       false

  task.exec

    book npages chipBGPendingCleanup -var N
    if ($N == 0)
        periods -exec $RUNEXEC
        break
    end
    if ($NETWORK == 0) break
    
    # look for new images in chipBGPendingCleanup (pantaskState == INIT)
    book getpage chipBGPendingCleanup 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword chipBGPendingCleanup $pageName pantaskState RUN
    book getword chipBGPendingCleanup $pageName camera -var CAMERA
    book getword chipBGPendingCleanup $pageName state -var CLEANUP_MODE
    book getword chipBGPendingCleanup $pageName chip_bg_id -var CHIP_BG_ID
    book getword chipBGPendingCleanup $pageName workdir -var WORKDIR_TEMPLATE
    book getword chipBGPendingCleanup $pageName exp_tag -var EXP_TAG
    book getword chipBGPendingCleanup $pageName dbname -var DBNAME

    # specify choice of local or remote host based on camera and chip (class_id)
    set.host.for.camera $CAMERA FPA
    set.workdir.by.camera $CAMERA FPA $WORKDIR_TEMPLATE $default_host WORKDIR
    sprintf logfile "%s/%s/%s.bgc.%s.cleanup.log" $WORKDIR $EXP_TAG $EXP_TAG $CHIP_BG_ID

    stdout $LOGDIR/chip.bg.cleanup.log
    stderr $LOGDIR/chip.bg.cleanup.log

    # XXX is everything listed here needed?
    $run = ipp_cleanup.pl --stage chip_bg --stage_id $CHIP_BG_ID --camera $CAMERA --mode $CLEANUP_MODE --logfile $logfile
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit    default
    process_exit chipBGPendingCleanup $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword chipBGPendingCleanup $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword chipBGPendingCleanup $options:0 pantaskState TIMEOUT
  end
end

book init warpBGPendingCleanup

macro warp.bg.cleanup.status
  book listbook warpBGPendingCleanup
end

macro warp.bg.cleanup.reset
  book init warpBGPendingCleanup
end

macro warp.bg.cleanup.on
  task warp.bg.cleanup.load
    active true
  end
  task warp.bg.cleanup.run
    active true
  end
end

macro warp.bg.cleanup.off
  task warp.bg.cleanup.load
    active false
  end
  task warp.bg.cleanup.run
    active false
  end
end

# this variable will cycle through the known database names
$warp_bg_cleanup_DB = 0

task	       warp.bg.cleanup.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 300
  npending     1
  active       false

  stdout NULL
  stderr $LOGDIR/warp.bg.cleanup.log

  task.exec
    if ($LABEL:n == 0) break
    $run = bgtool -pendingcleanupwarprun
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$warp_bg_cleanup_DB
      $run = $run -dbname $DB:$warp_bg_cleanup_DB
      $warp_bg_cleanup_DB ++
      if ($warp_bg_cleanup_DB >= $DB:n) set warp_bg_cleanup_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout warpBGPendingCleanup -key warp_bg_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook warpBGPendingCleanup
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup warpBGPendingCleanup
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
task	       warp.bg.cleanup.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 300
  active       false

  task.exec
    book npages warpBGPendingCleanup -var N
    if ($N == 0)
        periods -exec $RUNEXEC
        break
    end
    if ($NETWORK == 0) break
    
    # look for new images in warpBGPendingCleanup (pantaskState == INIT)
    book getpage warpBGPendingCleanup 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword warpBGPendingCleanup $pageName pantaskState RUN
    book getword warpBGPendingCleanup $pageName camera -var CAMERA
    book getword warpBGPendingCleanup $pageName state -var CLEANUP_MODE
    book getword warpBGPendingCleanup $pageName warp_bg_id -var WARP_BG_ID
    book getword warpBGPendingCleanup $pageName workdir -var WORKDIR_TEMPLATE
    book getword warpBGPendingCleanup $pageName exp_tag -var EXP_TAG
    book getword warpBGPendingCleanup $pageName dbname -var DBNAME

    # specify choice of local or remote host based on camera and warp 
    set.host.for.camera $CAMERA FPA
    set.workdir.by.camera $CAMERA $WARP_BG_ID $WORKDIR_TEMPLATE $default_host WORKDIR
    sprintf logfile "%s/%s/%s.bgw.%s.cleanup.log" $WORKDIR $EXP_TAG $EXP_TAG $WARP_BG_ID

    stdout $LOGDIR/warp.bg.cleanup.log
    stderr $LOGDIR/warp.bg.cleanup.log

    # XXX is everything listed here needed?
    $run = ipp_cleanup.pl --stage warp_bg --stage_id $WARP_BG_ID --camera $CAMERA --mode $CLEANUP_MODE --logfile $logfile
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit    default
    process_exit warpBGPendingCleanup $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword warpBGPendingCleanup $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword warpBGPendingCleanup $options:0 pantaskState TIMEOUT
  end
end
