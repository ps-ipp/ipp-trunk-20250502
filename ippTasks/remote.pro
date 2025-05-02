## remote.pro : -*- sh -*-

check.globals

# define chips

book init remotePrepCompRuns
book init remotePrepRuns
book init remoteExecRuns
book init remotePollRuns

$remote_label_iter = 0
$remote_label_warp = 0
$remote_stage_iter = 0

$remoteDefine_DB = 0
$remoteDefineWarp_DB = 0
$remotePrepComp_DB = 0
$remotePrep_DB = 0
$remoteExec_DB = 0
$remotePoll_DB = 0
$REMOTE_RECIPE = 0


list STAGES
  chip
  camera
#  warp
  stack
  staticsky
  diff
  ff
end

macro  remote.off
  task  remote.define
    active false
  end
  task  remote.define.warp
    active false
  end
  task  remote.prepcomp.load
    active false
  end
  task  remote.prepcomp.run
    active false
  end
  task  remote.prep.load
    active false
  end
  task  remote.prep.run
    active false
  end
  task  remote.exec.load
    active false
  end
  task  remote.exec.run
    active false
  end
  task  remote.poll.load
    active false
  end
  task  remote.poll.run
    active false
  end
end

macro  remote.on
  task  remote.define
    active true
  end
  task  remote.define.warp
    active true
  end
  task  remote.prepcomp.load
    active true
  end
  task  remote.prepcomp.run
    active true
  end
  task  remote.prep.load
    active true
  end
  task  remote.prep.run
    active true
  end
  task  remote.exec.load
    active true
  end
  task  remote.exec.run
    active true
  end
  task  remote.poll.load
    active true
  end
  task  remote.poll.run
    active true
  end
end

macro set.remote.recipe
  if ($0 != 2)
     echo "USAGE: set.remote.recipe (recipe_name)"
     break
  end
  $REMOTE_RECIPE = $1
end

task          remote.define
  host        local
  periods     -poll $LOADPOLL
  periods     -exec 300
  periods     -timeout 30
  active      true
  npending    1

  task.exec
    stdout NULL
    stderr $LOGDIR/remote.define.chip

    $label = $LABEL:$remote_label_iter

    $stage = $STAGES:$remote_stage_iter
    $remote_stage_iter ++

    # loop over all stages x all labels
    if ($remote_stage_iter >= $STAGES:n) 
       set remote_stage_iter = 0
       $remote_label_iter ++
       if ($remote_label_iter >= $LABEL:n) set remote_label_iter = 0
       echo $remote_stage_iter $remote_label_iter $label $stage
    end

    # min entry limit?
    if ("$stage" == "stack") 
	$run = remotetool -definebyquery -label $label -stage $stage -path_base neb://@HOST@.0/remote/$label -limit 50
    else 
	$run = remotetool -definebyquery -label $label -stage $stage -path_base neb://@HOST@.0/remote/$label -limit 500
    end

    if ($DB:n == 0)
      option DEFAULT
    else
      option $DB:$remoteDefine_DB
      $run = $run -dbname $DB:$remoteDefine_DB
      $remoteDefine_DB ++
      if ($remoteDefine_DB >= $DB:n) set remoteDefine_DB = 0
    end

    echo $run
    command $run
    
    end
    # success
    task.exit  0
  end
  # locked list                                                                                                                                    
  task.exit    default
    showcommand failure
  end
  task.exit    crash
    showcommand crash
  end
  #operation times out?                                                                                                                            
  task.exit    timeout
    showcommand timeout
  end
end

task          remote.define.warp
  host        local
  periods     -poll $LOADPOLL
  periods     -exec 900
  periods     -timeout 30
  active      true
  npending    1

  task.exec
    stdout NULL
    stderr $LOGDIR/remote.define.chip

    $label = $LABEL:$remote_label_warp

    if ($remote_label_warp >= $LABEL:n) set remote_label_warp = 0
    echo $remote_label_warp $label warp

    $run = remotetool -definebyquery -label $label -stage warp -path_base neb://@HOST@.0/remote/$label -limit 50
    if ($DB:n == 0)
      option DEFAULT
    else
      option $DB:$remoteDefineWarp_DB
      $run = $run -dbname $DB:$remoteDefineWarp_DB
      $remoteDefineWarp_DB ++
      if ($remoteDefineWarp_DB >= $DB:n) set remoteDefineWarp_DB = 0
    end

    echo $run
    command $run
    
  end

  # success
  task.exit  0
  end
  # locked list                                                                                                                                    
  task.exit    default
    showcommand failure
  end
  task.exit    crash
    showcommand crash
  end
  #operation times out?                                                                                                                            
  task.exit    timeout
    showcommand timeout
  end
end

task         remote.prepcomp.load
  host       local
  periods    -poll $LOADPOLL
  periods    -exec $LOADEXEC
  periods    -timeout 30
  active     true
  npending   1

  task.exec
    stdout NULL
    stderr $LOGDIR/remote.prepcomp.load

    $run = remotetool -listcomponent -state new

    if ($DB:n == 0)
      option DEFAULT
    else
      option $DB:$remotePrepComp_DB
      $run = $run -dbname $DB:$remotePrepComp_DB
      $remotePrepComp_DB ++
      if ($remotePrepComp_DB >= $DB:n) set remotePrepComp_DB = 0
    end

    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit  0
    ipptool2book stdout remotePrepCompRuns -uniq -key remote_id:stage_id -setword dbname $options:0 -setword pantaskState INIT
    process_cleanup remotePrepCompRuns

    if ($VERBOSE > 2)
      book listbook remotePrepCompRuns
    end
  end
  # locked list
  task.exit    default
    showcommand failure
  end
  task.exit    crash
    showcommand crash
  end
  #operation times out?
  task.exit    timeout
    showcommand timeout
  end
end

# run the prep component operations in parallel
task           remote.prepcomp.run
  host         anyhost
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 600
  active       true

  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC

    stdout $LOGDIR/remote.prepcomp.run
    stderr $LOGDIR/remote.prepcomp.run

    book npages remotePrepCompRuns -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    if ($REMOTE_RECIPE == 0) break

    book getpage remotePrepCompRuns 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword remotePrepCompRuns $pageName pantaskState RUN
    book getword remotePrepCompRuns $pageName remote_id -var REMOTE_ID
    book getword remotePrepCompRuns $pageName stage_id  -var STAGE_ID
    book getword remotePrepCompRuns $pageName stage     -var STAGE
    book getword remotePrepCompRuns $pageName run_path_base -var RUN_PATH_BASE
    book getword remotePrepCompRuns $pageName dbname    -var DBNAME

    sprintf outroot "%s/remote_%s.%s/stage_%s" $RUN_PATH_BASE $STAGE $REMOTE_ID $STAGE_ID

    if ("$STAGE" == "chip")
      $command = sc_prepare_chip.pl --chip_id $STAGE_ID
    end
    if ("$STAGE" == "camera")
      $command = sc_prepare_camera.pl --cam_id $STAGE_ID
    end
    if ("$STAGE" == "warp")
      $command = sc_prepare_warp.pl --warp_id $STAGE_ID
    end
    if ("$STAGE" == "stack")
      $command = sc_prepare_stack.pl --stack_id $STAGE_ID
    end
    if ("$STAGE" == "staticsky")
      $command = sc_prepare_staticsky.pl --sky_id $STAGE_ID
    end
    if ("$STAGE" == "diff")
      $command = sc_prepare_diff.pl --diff_id $STAGE_ID
    end
    if ("$STAGE" == "ff")
      $command = sc_prepare_ff.pl --ff_id $STAGE_ID
    end

# Common elements
    $command = $command --recipe $REMOTE_RECIPE --camera GPC1 --remote_id $REMOTE_ID --path_base $outroot --dbname $DBNAME

    options $pageName

    periods -exec 0.05
    command $command
  end

  # default exit status
  task.exit    default
    process_exit remotePrepCompRuns $options:0 $JOB_STATUS
  end
  # locked list
  task.exit    crash
    process_exit remotePrepCompRuns $options:0 $EXIT_CRASH_ERR
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword remotePrepCompRuns $options:0 pantaskState TIMEOUT
  end
end

task         remote.prep.load
  host       local
  periods    -poll $LOADPOLL
  periods    -exec 30
  active     true
  npending   1

  task.exec
    stdout NULL
    stderr $LOGDIR/remote.prep.load

    $run = remotetool -doneprep -state new 

    if ($DB:n == 0)
      option DEFAULT
    else
      option $DB:$remotePrep_DB
      $run = $run -dbname $DB:$remotePrep_DB
      $remotePrep_DB ++
      if ($remotePrep_DB >= $DB:n) set remotePrep_DB = 0
    end

    add_poll_labels run
    command $run
  end

  # success
  task.exit  0
    ipptool2book stdout remotePrepRuns -uniq -key remote_id -setword dbname $options:0 -setword pantaskState INIT
    process_cleanup remotePrepRuns

    if ($VERBOSE > 2)
      book listbook remotePrepRuns
    end
  end
  # locked list
  task.exit    default
    showcommand failure
  end
  task.exit    crash
    showcommand crash
  end
  #operation times out?
  task.exit    timeout
    showcommand timeout
  end
end

task           remote.prep.run
  # this probably shouldn't be local
  host         local
  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 600000
  active       true
  npending     1

  task.exec
    stdout $LOGDIR/remote.prep.run
    stderr $LOGDIR/remote.prep.run

    book npages remotePrepRuns -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    if ($REMOTE_RECIPE == 0) break

    book getpage remotePrepRuns 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword remotePrepRuns $pageName pantaskState RUN
    book getword remotePrepRuns $pageName remote_id -var REMOTE_ID
    book getword remotePrepRuns $pageName stage     -var STAGE
    book getword remotePrepRuns $pageName path_base -var PATH_BASE
#   book getword remotePrepRuns $pageName label     -var LABEL
    book getword remotePrepRuns $pageName dbname    -var DBNAME

    sprintf outroot "%s/remote_%s.%s" $PATH_BASE $STAGE $REMOTE_ID

    $command = sc_prepare_run.pl --camera GPC1 --remote_id $REMOTE_ID --stage $STAGE --path_base $outroot --dbname $DBNAME --recipe $REMOTE_RECIPE

    options $pageName
    command $command
  end

  # default exit status
  task.exit    default
    process_exit remotePrepRuns $options:0 $JOB_STATUS
  end
  # locked list
  task.exit    crash
    process_exit remotePrepRuns $options:0 $EXIT_CRASH_ERR
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword remotePrepRuns $options:0 pantaskState TIMEOUT
  end
end

task         remote.exec.load
  host       local
  periods    -poll $LOADPOLL
  periods    -exec $LOADEXEC
  active     false
  npending   1

  task.exec
    stdout NULL
    stderr $LOGDIR/remote.exec.load

#    $end_date = `date +%Y-%m-%dT%H:%M:%S`
    $run = remotetool -listrun -state pending 
# -poll_end $end_date

    if ($DB:n == 0)
      option DEFAULT
    else
      option $DB:$remoteExec_DB
      $run = $run -dbname $DB:$remoteExec_DB
      $remoteExec_DB ++
      if ($remoteExec_DB >= $DB:n) set remoteExec_DB = 0
    end

    add_poll_labels run
    command $run
  end
  # success
  task.exit  0
    ipptool2book stdout remoteExecRuns -uniq -key remote_id -setword dbname $options:0 -setword pantaskState INIT
    process_cleanup remoteExecRuns

    if ($VERBOSE > 2)
      book listbook remoteExecRuns
    end
  end
  # locked list
  task.exit    default
    showcommand failure
  end
  task.exit    crash
    showcommand crash
  end
  #operation times out?
  task.exit    timeout
    showcommand timeout
  end
end

task           remote.exec.run
  # this probably shouldn't be local
  host         local
  periods      -poll $LOADPOLL
  periods      -exec 30
  periods      -timeout 6000000
  active       false
  npending     3

  task.exec
    stdout $LOGDIR/remote.exec.run
    stderr $LOGDIR/remote.exec.run

    book npages remoteExecRuns -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    if ($REMOTE_RECIPE == 0) break

    book getpage remoteExecRuns 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword remoteExecRuns $pageName pantaskState RUN
    book getword remoteExecRuns $pageName remote_id -var REMOTE_ID
    book getword remoteExecRuns $pageName stage     -var STAGE
    book getword remoteExecRuns $pageName path_base -var PATH_BASE
#    book getword remoteExecRuns $pageName label     -var LABEL
    book getword remoteExecRuns $pageName job_id    -var JOB_ID
    book getword remoteExecRuns $pageName dbname    -var DBNAME

    sprintf outroot "%s/remote_%s.%s" $PATH_BASE $STAGE $REMOTE_ID

    $command = sc_remote_exec.pl --remote_id $REMOTE_ID --path_base $outroot --verbose --dbname $DBNAME --camera GPC1 --recipe $REMOTE_RECIPE

    options $pageName
    command $command
  end

  # default exit status
  task.exit    default
    process_exit remoteExecRuns $options:0 $JOB_STATUS
  end
  # locked list
  task.exit    crash
    process_exit remoteExecRuns $options:0 $EXIT_CRASH_ERR
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword remoteExecRuns $options:0 pantaskState TIMEOUT
  end
end

task         remote.poll.load
  host       local
  periods    -poll $LOADPOLL
  periods    -exec 300
  active       false
  npending   1

  task.exec
    stdout NULL
    stderr $LOGDIR/remote.poll.load

    # $end_date = `date +%Y-%m-%dT%H:%M:%S -d '1 hour ago'`
    # $run = remotetool -listrun -state run -poll_end $end_date
    $run = remotetool -listrun -state run

    if ($DB:n == 0)
      option DEFAULT
    else
      option $DB:$remotePoll_DB
      $run = $run -dbname $DB:$remotePoll_DB
      $remotePoll_DB ++
      if ($remotePoll_DB >= $DB:n) set remotePoll_DB = 0
    end

    add_poll_labels run
    command $run
  end

  # success
  task.exit  0
    ipptool2book stdout remotePollRuns -uniq -key remote_id -setword dbname $options:0 -setword pantaskState INIT
    process_cleanup remotePollRuns

    if ($VERBOSE > 2)
      book listbook remotePollRuns
    end
  end
  # locked list
  task.exit    default
    showcommand failure
  end
  task.exit    crash
    showcommand crash
  end
  #operation times out?
  task.exit    timeout
    showcommand timeout
  end
end

task           remote.poll.run
  # this probably shouldn't be local
  host         local
  periods      -poll $LOADPOLL
  periods      -exec 30
  periods      -timeout 6000000
  active       false
  npending     10

  task.exec
    stdout $LOGDIR/remote.poll.run
    stderr $LOGDIR/remote.poll.run

    book npages remotePollRuns -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    if ($REMOTE_RECIPE == 0) break

    book getpage remotePollRuns 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword remotePollRuns $pageName pantaskState RUN
    book getword remotePollRuns $pageName remote_id -var REMOTE_ID
    book getword remotePollRuns $pageName stage     -var STAGE
    book getword remotePollRuns $pageName path_base -var PATH_BASE
#    book getword remotePollRuns $pageName label     -var LABEL
    book getword remotePollRuns $pageName job_id    -var JOB_ID
    book getword remotePollRuns $pageName dbname    -var DBNAME

    sprintf outroot "%s/remote_%s.%s" $PATH_BASE $STAGE $REMOTE_ID

    # This can't have an invalid job_id
    if ($JOB_ID == -1) break
    $command = sc_remote_poll.pl --remote_id $REMOTE_ID --path_base $outroot --verbose --dbname $DBNAME --camera GPC1 --job_id $JOB_ID --recipe $REMOTE_RECIPE

    options $pageName
    command $command
  end

  # default exit status
  task.exit    default
    process_exit remotePollRuns $options:0 $JOB_STATUS
  end
  # locked list
  task.exit    crash
    process_exit remotePollRuns $options:0 $EXIT_CRASH_ERR
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword remotePollRuns $options:0 pantaskState TIMEOUT
  end
end

