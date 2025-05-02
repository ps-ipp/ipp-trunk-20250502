## bg.regeneration.pro : tasks for regenerating the background model for warps and stacks : -*- sh -*-

check.globals

if ($?POLL_LIMIT_BGREG == 0) set POLL_LIMIT_BGREG = 40

macro set.bgreg.poll
  if ($0 != 2)
    echo "USAGE:set.bgreg.poll (value)"
    break;
  end

  $POLL_LIMIT_BGREG = $1
end
macro get.bgreg.poll
  echo $POLL_LIMIT_BGREG
end

## Initialize the books for the tasks to do
book init bgRegWarp
book init bgRegStack

## Database lists
$bgRegWarp_DB = 0
$bgRegStack_DB = 0

macro bgreg.warp.status
  book listbook bgRegWarp
end
macro bgreg.stack.status
  book listbook bgRegStack
end

## Reset tasks
macro bgreg.reset
  book init bgRegWarp
  book init bgRegStack
end

## Turn tasks on
macro bgreg.on
  task bgreg.warp.load
    active true
  end
  task bgreg.warp.run
    active true
  end
  task bgreg.stack.load
    active true
  end
  task bgreg.stack.run
    active true
  end
end
## or off
macro bgreg.off
  task bgreg.warp.load
    active false
  end
  task bgreg.warp.run
    active false
  end
  task bgreg.stack.load
    active false
  end
  task bgreg.stack.run
    active false
  end
end

# Load task for warp
task            bgreg.warp.load
  host          local

  periods       -poll $LOADPOLL
  periods       -poll $LOADEXEC
  periods       -timeout 30
  npending      1

  stdout        NULL
  stderr        $LOGDIR/bgreg.warp.load

  task.exec
    if ($LABEL:n == 0) break
    $run = warptool -warped -background_model 0
    if ($DB:n == 0) 
      option DEFAULT
    else
      #save the DB naem for the exit tasks
      option $DB:$bgRegWarp_DB
      $run = $run -dbname $DB:$bgRegWarp_DB
      $bgRegWarp_DB ++
      if ($bgRegWarp_DB >= $DB:n) set bgRegWarp_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    # change the limit?
    $run = $run -limit $POLL_LIMIT_BGREG
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout bgRegWarp -key warp_id:skycell_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook bgRegWarp
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup bgRegWarp
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

task           bgreg.warp.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    # if we can't exec, set the retry time long
    periods -exec $RUNEXEC

    book npages bgRegWarp -var N
    if ($N == 0) break
    if ($NETWORK == 0) break

    # look for new pages
    book getpage bgRegWarp 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword bgRegWarp $pageName pantaskState RUN
    book getword bgRegWarp $pageName warp_id -var STAGE_ID
    book getword bgRegWarp $pageName skycell_id -var SKYCELL_ID
    book getword bgRegWarp $pageName camera -var CAMERA
    book getword bgRegWarp $pageName dbname -var DBNAME

    set.host.for.skycell $SKYCELL_ID

    stdout $LOGDIR/bgreg.warp.log
    stderr $LOGDIR/bgreg.warp.log

    $run = regenerate_background.pl --stage warp --stage_id $STAGE_ID --skycell_id $SKYCELL_ID --camera $CAMERA

    add_standard_args run
    options $pageName

    if ($VERBOSE > 1)
      echo command $run
    end
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit        default
    process_exit bgRegWarp $options:0 $JOB_STATUS
  end

  # locked list
  task.exit        crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword bgRegWarp $options:0 pantasksState CRASH
  end

  # operation timed out
  task.exit        timeout
    showcommand timeout
    book setword bgRegWarp $options:0 pantaskState TIMEOUT
  end
end

# Load task for stack
task            bgreg.stack.load
  host          local

  periods       -poll $LOADPOLL
  periods       -poll $LOADEXEC
  periods       -timeout 30
  npending      1

  stdout        NULL
  stderr        $LOGDIR/bgreg.stack.load

  task.exec
    if ($LABEL:n == 0) break
    $run = stacktool -tobkg
    if ($DB:n == 0) 
      option DEFAULT
    else
      #save the DB naem for the exit tasks
      option $DB:$bgRegStack_DB
      $run = $run -dbname $DB:$bgRegStack_DB
      $bgRegStack_DB ++
      if ($bgRegStack_DB >= $DB:n) set bgRegStack_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    # change the limit?
    $run = $run -limit $POLL_LIMIT_BGREG
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout bgRegStack -key stack_id:skycell_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook bgRegStack
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup bgRegStack
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

task           bgreg.stack.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    # if we can't exec, set the retry time long
    periods -exec $RUNEXEC

    book npages bgRegStack -var N
    if ($N == 0) break
    if ($NETWORK == 0) break

    # look for new pages
    book getpage bgRegStack 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword bgRegStack $pageName pantaskState RUN
    book getword bgRegStack $pageName stack_id -var STAGE_ID
    book getword bgRegStack $pageName skycell_id -var SKYCELL_ID
    book getword bgRegStack $pageName camera -var CAMERA
    book getword bgRegStack $pageName dbname -var DBNAME
    set.host.for.skycell $SKYCELL_ID

    stdout $LOGDIR/bgreg.stack.log
    stderr $LOGDIR/bgreg.stack.log

    $run = regenerate_background.pl --stage stack --stage_id $STAGE_ID --skycell_id $SKYCELL_ID --camera $CAMERA

    add_standard_args run
    options $pageName

    if ($VERBOSE > 1)
      echo command $run
    end
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit        default
    process_exit bgRegStack $options:0 $JOB_STATUS
  end

  # locked list
  task.exit        crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword bgRegStack $options:0 pantasksState CRASH
  end

  # operation timed out
  task.exit        timeout
    showcommand timeout
    book setword bgRegStack $options:0 pantaskState TIMEOUT
  end
end


  
 
