## flatcorr.pro : tasks to run flat-field correction : -*- sh -*-
## this file contains the tasks for running the flatcorr stage
## these tasks use the book 'flatcorrBook'

# a flat-field correction run is initiated (manually) with a command like:
# flatcorr -definebyquery -filter r ...
# the result is a flatcorrRun entry, a set of chipRun entries, a linking list in flatcorrChipList.  
# the chipRun entries are all set to stop at the "chip" stage.

# we also can define a run with:
# flatcorr -definerun 
# flatcorr -addchip ...

# as the chip analysis progresses, we need to occasionally migrate the
# completed chips to the camera stage by calling:
# flatcorr -advancecamera 

# as the camera analysis progresses, we need to occasionally migrate the
# completed exposures to the addstar stage by calling:
# flatcorr -advanceaddstar 

# we wait for the completed addstar jobs and search for
# completed processing analysis with:
# flatcorr -pendingprocess

# these define arguments to an analysis run.  successful completion of
# the analysis is marked with:
# flatcorr -addprocess
# and failures with:
# flatcorr -addprocess -fault N

# test for required global variables
check.globals

book init flatcorrBook

macro flatcorr.reset
  book init flatcorrBook
end

macro flatcorr.status
  book listbook flatcorrBook
end

macro flatcorr.on
  task flatcorr.advancecamera
    active true
  end
  task flatcorr.advanceaddstar
    active true
  end
  task flatcorr.load
    active true
  end
  task flatcorr.run
    active true
  end
end

macro flatcorr.off
  task flatcorr.advancecamera
    active false
  end
  task flatcorr.advanceaddstar
    active false
  end
  task flatcorr.load
    active false
  end
  task flatcorr.run
    active false
  end
end

macro flatcorr.proc.on
  task flatcorr.load
    active true
  end
  task flatcorr.run
    active true
  end
end

macro flatcorr.proc.off
  task flatcorr.load
    active false
  end
  task flatcorr.run
    active false
  end
end

# these variables will cycle through the known ippdb database names
$flatcorr_advancecamera_DB = 0
$flatcorr_advanceaddstar_DB = 0
$flatcorr_pendingprocess_DB = 0

# migrate complete flatcorr chips to the camera stage analysis
task	       flatcorr.advancecamera
  host         local

  # check the list of pending flatcorr runs
  periods      -poll 5
  periods      -exec 30
  periods      -timeout 60
  npending 1

  task.exec
    # define the command (does not depend on previous queries)
    if ($DB:n == 0)
      command flatcorr -advancecamera
    else
      # save the DB name for the exit tasks
      # note that this DB name refers to the ippdb, not the dvodb
      option $DB:$flatcorr_advancecamera_DB
      command flatcorr -advancecamera -dbname $DB:$flatcorr_advancecamera_DB
      $flatcorr_advancecamera_DB ++
      if ($flatcorr_advancecamera_DB >= $DB:n) set flatcorr_advancecamera_DB = 0
    end
  end

  # silently drop stdout
  stdout NULL
  stderr $LOGDIR/flatcorr.log

  # success (no action required)
  task.exit $EXIT_SUCCESS
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

# migrate complete flatcorr exposures to the addstar process
task	       flatcorr.advanceaddstar
  host         local

  # check the list of pending flatcorr runs
  periods      -poll 5
  periods      -exec 30
  periods      -timeout 60
  npending 1

  task.exec
    # define the command (does not depend on previous queries)
    if ($DB:n == 0)
      command flatcorr -advanceaddstar
    else
      # save the DB name for the exit tasks
      # note that this DB name refers to the ippdb, not the dvodb
      option $DB:$flatcorr_advanceaddstar_DB
      command flatcorr -advanceaddstar -dbname $DB:$flatcorr_advanceaddstar_DB
      $flatcorr_advanceaddstar_DB ++
      if ($flatcorr_advanceaddstar_DB >= $DB:n) set flatcorr_advanceaddstar_DB = 0
    end
  end

  # silently drop stdout
  stdout NULL
  stderr $LOGDIR/flatcorr.log

  # success (no action required)
  task.exit $EXIT_SUCCESS
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

# create new flatcorr entries for the currently known DVO databases
# run this multiple times once an hour - on pass for each db
task	       flatcorr.load
  host         local

  # check the list of pending flatcorr runs
  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 60
  npending 1

  # define the command (does not depend on previous queries)
  task.exec
    if ($DB:n == 0)
      $run = flatcorr -pendingprocess
    else 
      $run = flatcorr -pendingprocess
      # save the DB name for the exit tasks
      option $DB:$flatcorr_pendingprocess_DB
      $run = $run -dbname $DB:$flatcorr_pendingprocess_DB
      $flatcorr_pendingprocess_DB ++
      if ($flatcorr_pendingprocess_DB >= $DB:n) set flatcorr_pendingprocess_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # silently drop stdout
  stdout NULL
  stderr $LOGDIR/flatcorr.log

  # success
  task.exit $EXIT_SUCCESS
    ipptool2book stdout flatcorrBook -key corr_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook flatcorrBook
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup flatcorrBook
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

# check for flatcorr runs which are ready to go
task	       flatcorr.run
  host         local
  npending     1

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 3600

  # silently drop stdout
  stdout $LOGDIR/flatcorr.log
  stderr $LOGDIR/flatcorr.log

  task.exec
    book npages flatcorrBook -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new entries in flatcorrBook
    book getpage flatcorrBook 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword flatcorrBook $pageName pantaskState     RUN

    # XXX probably need to set the output / log based on WORKDIR...
    book getword flatcorrBook $pageName corr_id     -var CORR_ID
    book getword flatcorrBook $pageName det_type    -var DET_TYPE
    book getword flatcorrBook $pageName dvodb       -var DVODB
    book getword flatcorrBook $pageName camera      -var CAMERA
    book getword flatcorrBook $pageName region      -var REGION
    book getword flatcorrBook $pageName filter      -var FILTER
    book getword flatcorrBook $pageName make_corr   -var MAKE_CORRECTION
    book getword flatcorrBook $pageName dbname      -var DBNAME
    book getword flatcorrBook $pageName workdir     -var WORKDIR_TEMPLATE

    # save the pageName for future reference below
    options $pageName

    # see chip.pro for examples
    set.workdir.by.camera $CAMERA FPA $WORKDIR_TEMPLATE $default_host WORKDIR

    ## generate outroot specific to this exposure (& chip)
    sprintf outroot "%s/%s.flatcorr.%s" $WORKDIR $CAMERA $CORR_ID

    # XXX get these arguments right
    $run = flatcorr_proc.pl --corr_id $CORR_ID --det_type $DET_TYPE --dvodb $DVODB --camera $CAMERA --region $REGION --filter $FILTER --workdir $outroot
    if ("$MAKE_CORRECTION" == "T")
      $run = $run --make_correction
    end
    add_standard_args run

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    command $run
  end

  # default exit status
  task.exit default
    process_exit flatcorrBook $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword flatcorrBook $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword flatcorrBook $options:0 pantaskState TIMEOUT
  end
end
