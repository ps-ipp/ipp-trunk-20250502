## dqstats.pro : globals and support macros : -*- sh -*-
## this file contains the tasks for running the dqstats analysis stage
## these tasks use the book dqstatsPendingBundle

# test for required global variables
check.globals

book init dqstatsPendingBundle

macro dqstats.status
  book listbook dqstatsPendingBundle
end

macro dqstats.reset
  book init dqstatsPendingBundle
end

macro dqstats.on
  task dqstats.load
    active true
  end
  task dqstats.run
    active true
  end
  task dqstats.revert
    active true
  end
end

macro dqstats.off
  task dqstats.load
    active false
  end
  task dqstats.run
    active false
  end
  task dqstats.revert
    active false
  end
end

macro dqstats.revert.on
  task dqstats.revert
    active true
  end
end

macro dqstats.revert.off
  task dqstats.revert
    active true
  end
end

# this variable will cycle through the known database names
$dqstats_DB = 0
$dqstats_revert_DB = 0

# select images ready for dqstats analysis
# new entries are added to dqstatsPendingBundle
# skip already-present entries
task	       dqstats.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/dqstats.log

  task.exec
    if ($LABEL:n == 0) break
    $run = dqstatstool -pendingbundle -all 
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$dqstats_DB
      $run = $run -dbname $DB:$dqstats_DB
      $dqstats_DB ++
      if ($dqstats_DB >= $DB:n) set dqstats_DB = 0
    end
#    add_poll_args run
#    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout dqstatsPendingBundle -key dqstats_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook dqstatsPendingBundle
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup dqstatsPendingBundle
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

# run the dqstats script on pending exposures
task	       dqstats.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 600

  ## we want only a single outstanding dqstats job.  
  npending     1

  task.exec
    book npages dqstatsPendingBundle -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in dqstatsPendingBundle (pantaskState == INIT)
    book getpage dqstatsPendingBundle 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword dqstatsPendingBundle $pageName pantaskState RUN
    book getword dqstatsPendingBundle $pageName camera -var CAMERA
    book getword dqstatsPendingBundle $pageName dqstats_id -var DQSTATS_ID
    book getword dqstatsPendingBundle $pageName dbname -var DBNAME

    # specify choice of remote host based on camera and chip (class_id)
    # set.host.for.camera $CAMERA FPA

    # set the WORKDIR variable
#    set.workdir.by.camera $CAMERA FPA $WORKDIR_TEMPLATE $default_host WORKDIR

    # notes on how this works:
    # -- raw workdir examples:
    # file://data/@HOST@.0/gpc1/20080130
    # neb:///@HOST@-vol0/gpc1/20080130 (need to supply volname?, or are we re-defining this each time?)
    # -- out workdir examples:
    # file://data/ipp004.0/gpc1/20080130
    # neb:///ipp004-vol0/gpc1/20080130

    ## generate outroot specific to this exposure (& chip)
    # Uri's need to be standardized when the run is created.
#    sprintf outroot "%s/%s/%s.dqstats.%s" $WORKDIR $EXP_TAG $EXP_TAG $DQSTATS_ID

    stdout $LOGDIR/dqstats.log
    stderr $LOGDIR/dqstats.log

    $run = dqstats_bundle.pl --dqstats_id $DQSTATS_ID --camera $CAMERA --redirect-output
        
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    command $run
  end

  # success
  task.exit default
    process_exit dqstatsPendingBundle $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword dqstatsPendingBundle $options:0 pantaskState CRASH
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
    book setword dqstatsPendingBundle $options:0 pantaskState TIMEOUT
  end
end

task dqstats.revert
  host         local

  periods      -poll 5.0
  periods      -exec 60.0
  periods      -timeout 120.0
  npending     1
  active       false

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = dqstatstool -revertrun
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$dqstats_revert_DB
      $run = $run -dbname $DB:$dqstats_revert_DB
      $dqstats_revert_DB ++
      if ($dqstats_revert_DB >= $DB:n) set dqstats_revert_DB = 0
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
