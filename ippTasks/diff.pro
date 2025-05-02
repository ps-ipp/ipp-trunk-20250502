## diff.pro : image difference analysis : -*- sh -*-

## This file contains panTasks definitions for performing the image differencing.
## After a difference (with associated diff_id) is defined, the difference is performed
## (tasks in diffSkyfile).

# test for required global variables
check.globals

### Initialise the books containing the tasks to do
book init diffSkyfile
#book init diffCleanup
book init diffPendingSummary

### Database lists
$diffSkycell_DB = 0
$diffAdvance_DB = 0
$diff_revert_DB = 0
$diffSummary_DB = 0
#$diffCleanup_DB = 0

### Check status of diffing tasks
macro diff.status
  book listbook diffSkyfile
#  book listbook diffCleanup
end

### Reset diffing tasks
macro diff.reset
  book init diffSkyfile
  book init diffPendingSummary
#  book init diffCleanup
end

### Turn diffing tasks on
macro diff.on
  task diff.skycell.load
    active true
  end
  task diff.skycell.run
    active true
  end
  task diff.advance
    active true
  end
  task diff.revert
    active true
  end
end

### Turn diffing tasks off
macro diff.off
  task diff.skycell.load
    active false
  end
  task diff.skycell.run
    active false
  end
  task diff.advance
    active false
  end
  task diff.revert
    active false
  end
end

macro diff.summary.on
  task diff.summary.load
    active true
  end
  task diff.summary.run
    active true
  end
end

macro diff.summary.off
  task diff.summary.load
    active false
  end
  task diff.summary.run
    active false
  end
end

macro diff.revert.on
  task diff.revert
    active true
  end
end

macro diff.revert.off
  task diff.revert
    active false
  end
end

# macro diff.cleanup.on
#   task diff.cleanup.load
#     active true
#   end
#   task diff.cleanup.run
#     active true
#   end
# end

# macro diff.cleanup.off
#   task diff.cleanup.load
#     active false
#   end
#   task diff.cleanup.run
#     active false
#   end
# end


### Load tasks for doing the differences
### Tasks are loaded into diffSkyfile.


task	       diff.skycell.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/diff.skycell.log

  task.exec
    if ($LABEL:n == 0) break
    $run = difftool -todiffskyfile
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$diffSkycell_DB
      $run = $run -dbname $DB:$diffSkycell_DB
      $diffSkycell_DB ++
      if ($diffSkycell_DB >= $DB:n) set diffSkycell_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    # increase the poll limit for warp over the default to
    # help it keep up with chip processing
    # NOTE : it is not a problem for difftool to have multiple 
    # -limit entries: the last one is used
    $run = $run -limit {$POLL_LIMIT * 2}
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout diffSkyfile -key diff_id:skycell_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook diffSkyfile
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup diffSkyfile
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

### Run tasks for calculating the diff overlaps
### Tasks are taken from diffSkyfile.
task	       diff.skycell.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC

    book npages diffSkyfile -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    if ($BURNTOOLING == 1) break

    # look for new images in diffSkyfile (pantaskState == INIT)
    book getpage diffSkyfile 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword diffSkyfile $pageName pantaskState RUN
    book getword diffSkyfile $pageName diff_id -var DIFF_ID
    book getword diffSkyfile $pageName diff_skyfile_id -var DIFF_SKYFILE_ID
    book getword diffSkyfile $pageName tess_id -var TESS_DIR
    book getword diffSkyfile $pageName skycell_id -var SKYCELL_ID
    book getword diffSkyfile $pageName camera -var CAMERA
    book getword diffSkyfile $pageName bothways -var BOTHWAYS
    book getword diffSkyfile $pageName workdir -var WORKDIR_TEMPLATE
    book getword diffSkyfile $pageName state -var RUN_STATE
    book getword diffSkyfile $pageName dbname -var DBNAME
    book getword diffSkyfile $pageName reduction -var REDUCTION
    book getword diffSkyfile $pageName diff_mode -var DIFF_MODE
    book getword diffSkyfile $pageName path_base -var PATH_BASE

    # set the host and workdir based on the skycell hash
    set.host.for.skycell $SKYCELL_ID
    set.workdir.by.skycell $SKYCELL_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    # XXX old code:
    # host anyhost
    # $WORKDIR = $WORKDIR_TEMPLATE
    if (($DIFF_MODE == 1)||("$DIFF_MODE" == "NULL")) 
	$DIFF_TAG = ""
    end
    if ($DIFF_MODE == 2)
	$DIFF_TAG = "WS."
    end 
    if ($DIFF_MODE == 3)
	$DIFF_TAG = "SW."
    end
    if ($DIFF_MODE == 4)
	$DIFF_TAG = "SS."
    end

    basename $TESS_DIR -var TESS_ID

    if ("$PATH_BASE" == "NULL")
        sprintf outroot "%s/%s/%s/%s.%s.%sdif.%s" $WORKDIR $TESS_ID $SKYCELL_ID $TESS_ID $SKYCELL_ID $DIFF_TAG $DIFF_ID
    else
        $outroot = $PATH_BASE
    end

    stdout $LOGDIR/diff.skycell.log
    stderr $LOGDIR/diff.skycell.log

    $run = diff_skycell.pl --threads @MAX_THREADS@ --diff_id $DIFF_ID --skycell_id $SKYCELL_ID --diff_skyfile_id $DIFF_SKYFILE_ID --outroot $outroot --redirect-output --run-state $RUN_STATE
    if ("$BOTHWAYS" == "T")
       $run = $run --inverse
    end
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
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit    default
    process_exit diffSkyfile $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword diffSkyfile $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword diffSkyfile $options:0 pantaskState TIMEOUT
  end
end


# Advance exposures which have completed
task	       diff.advance
  host         local

  periods      -poll $LOADPOLL
#  periods      -exec $LOADEXEC
  periods      -exec 30
  periods      -timeout 60
  npending     1

  stdout NULL
  stderr $LOGDIR/diff.advance.log

  task.exec
    if ($LABEL:n == 0) break
    $run = difftool -advance
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$diffAdvance_DB
      $run = $run -dbname $DB:$diffAdvance_DB
      $diffAdvance_DB ++
      if ($diffAdvance_DB >= $DB:n) set diffAdvance_DB = 0
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



# # select images ready for diff analysis
# # new entries are added to diffPendingImfile
# # skip already-present entries
# task	       diff.cleanup.load
#   host         local

#   periods      -poll $LOADPOLL
#   periods      -exec $LOADEXEC
#   periods      -timeout 30
#   npending     1
#   active       false

#   stdout NULL
#   stderr $LOGDIR/diff.cleanup.log

#   task.exec
#     if ($LABEL:n == 0) break
#     $run = difftool -pendingcleanuprun
#     if ($DB:n == 0)
#       option DEFAULT
#     else
#       # save the DB name for the exit tasks
#       option $DB:$diffCleanup_DB
#       $run = $run -dbname $DB:$diffCleanup_DB
#       $diffCleanup_DB ++
#       if ($diffCleanup_DB >= $DB:n) set diffCleanup_DB = 0
#     end
#     add_poll_args run
#     add_poll_labels run
#     command $run
#   end

#   # success
#   task.exit    0
#     # convert 'stdout' to book format
#     ipptool2book stdout diffCleanup -key diff_id -uniq -setword dbname $options:0 -setword pantaskState INIT
#     if ($VERBOSE > 2)
#       book listbook diffCleanup
#     end

#     # delete existing entries in the appropriate pantaskStates
#     process_cleanup diffCleanup
#   end

#   # locked list
#   task.exit    default
#     showcommand failure
#   end

#   task.exit    crash
#     showcommand crash
#   end

#   # operation times out?
#   task.exit    timeout
#     showcommand timeout
#   end
# end

# # run the ipp_cleanup.pl script on pending images
# task	       diff.cleanup.run
#   periods      -poll $RUNPOLL
#   periods      -exec $RUNEXEC
#   periods      -timeout 60
#   active       false

#   task.exec
#     book npages diffCleanup -var N
#     if ($N == 0) break
#     if ($NETWORK == 0) break
    
#     # look for new images in diffCleanup (pantaskState == INIT)
#     book getpage diffCleanup 0 -var pageName -key pantaskState INIT
#     if ("$pageName" == "NULL") break

#     book setword diffCleanup $pageName pantaskState RUN
#     book getword diffCleanup $pageName camera -var CAMERA
#     book getword diffCleanup $pageName state -var CLEANUP_MODE
#     book getword diffCleanup $pageName diff_id -var DIFF_ID
#     book getword diffCleanup $pageName dbname -var DBNAME

#     # specify choice of local or remote host based on camera and diff (class_id)
#     set.host.for.camera $CAMERA FPA

#     stdout $LOGDIR/diff.cleanup.log
#     stderr $LOGDIR/diff.cleanup.log

#     # XXX is everything listed here needed?
#     $run = ipp_cleanup.pl --stage diff --stage_id $DIFF_ID --camera $CAMERA --mode $CLEANUP_MODE
#     add_standard_args run

#     # save the pageName for future reference below
#     options $pageName

#     # create the command line
#     if ($VERBOSE > 1)
#       echo command $run
#     end
#     command $run
#   end

#   # default exit status
#   task.exit    default
#     process_exit diffCleanup $options:0 $JOB_STATUS
#   end

#   task.exit    crash
#     showcommand crash
#     book setword diffCleanup $options:0 pantaskState CRASH
#   end

#   # operation timed out?
#   task.exit    timeout
#     showcommand timeout
#     book setword diffCleanup $options:0 pantaskState TIMEOUT
#   end
# end

task diff.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    # Only revert failures with fault=2 (SYS_ERROR), which tend to be
    # temporary filesystem problems.  Every other fault type is
    # interesting and should be kept for debugging (and so it does not
    # continue to occur).
    # XXX: actually we are getting enough jobs with fault 4 that work after reverting
    # that we are going to remove that cut for now
    # 
    #$run = difftool -revertdiffskyfile -fault 2
    $run = difftool -revertdiffskyfile
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$diff_revert_DB
      $run = $run -dbname $DB:$diff_revert_DB
      $diff_revert_DB ++
      if ($diff_revert_DB >= $DB:n) set diff_revert_DB = 0
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

### Load tasks for doing the diffs
### Tasks are loaded into diffPendingSkyCell.
task	       diff.summary.load
  host         local
  active       false

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 1200
  npending     1

  stdout NULL
  stderr $LOGDIR/diff.summary.log

  task.exec
    if ($LABEL:n == 0) break
    $run = difftool -tosummary
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$diffSummary_DB
      $run = $run -dbname $DB:$diffSummary_DB
      $diffSummary_DB ++
      if ($diffSummary_DB >= $DB:n) set diffSummary_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    # XXX change tess_id to tess_dir when db is updated
    ipptool2book stdout diffPendingSummary -key diff_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook diffPendingSummary
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup diffPendingSummary
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

### Run tasks for calculating the diff overlaps
### Tasks are taken from diffPendingSkyCell.
task	       diff.summary.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60
  active       false

  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC

    book npages diffPendingSummary -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in diffPendingSkyCell (pantaskState == INIT)
    book getpage diffPendingSummary 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword diffPendingSummary $pageName pantaskState RUN
    book getword diffPendingSummary $pageName diff_id -var DIFF_ID
    book getword diffPendingSummary $pageName camera -var CAMERA
    book getword diffPendingSummary $pageName workdir -var WORKDIR_TEMPLATE
    book getword diffPendingSummary $pageName dbname -var DBNAME
    book getword diffPendingSummary $pageName tess_id -var TESS_DIR
    book getword diffPendingSummary $pageName diff_mode -var DIFF_MODE
    book getword diffPendingSummary $pageName state -var RUN_STATE

    # set the host and workdir based on the skycell hash
    host anyhost
    strsub $WORKDIR_TEMPLATE @HOST@.0 $default_host -var WORKDIR
#    set.workdir.by.skycell $SKYCELL_ID $WORKDIR_TEMPLATE $default_host WORKDIR

#    if ("$PATH_BASE" == "NULL") 

    if (($DIFF_MODE == 1)||("$DIFF_MODE" == "NULL")) 
	$DIFF_TAG = ""
    end
    if ($DIFF_MODE == 2)
	$DIFF_TAG = "WS."
    end 
    if ($DIFF_MODE == 3)
	$DIFF_TAG = "SW."
    end
    if ($DIFF_MODE == 4)
	$DIFF_TAG = "SS."
    end

    basename $TESS_DIR -var TESS_ID

    ## generate outroot specific to this exposure
    sprintf outroot "%s/%s/%s.%sdif.%s.summary" $WORKDIR $TESS_ID $TESS_ID $DIFF_TAG $DIFF_ID


    stdout $LOGDIR/diff.summary.log
    stderr $LOGDIR/diff.summary.log

    $run = skycell_jpeg.pl --stage diff --stage_id $DIFF_ID --camera $CAMERA --outroot $outroot
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
    process_exit diffPendingSummary $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword diffPendingSummary $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword diffPendingSummary $options:0 pantaskState TIMEOUT
  end
end
