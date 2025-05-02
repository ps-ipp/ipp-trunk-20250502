## fake.pro : globals and support macros : -*- sh -*-
## this file contains the tasks for running the phase 0 stage
## these tasks use the book fakePendingImfile

# test for required global variables
check.globals

book init fakePendingImfile

macro fake.status
  book listbook fakePendingImfile
end

macro fake.reset
  book init fakePendingImfile
end

macro fake.on
  task fake.imfile.load
    active true
  end
  task fake.imfile.run
    active true
  end
  task fake.advanceexp
    active true
  end
  task fake.revert
    active true
  end
end

macro fake.off
  task fake.imfile.load
    active false
  end
  task fake.imfile.run
    active false
  end
  task fake.advanceexp
    active false
  end
  task fake.revert
    active false
  end
end
macro fake.revert.off
    task fake.revert
        active false
    end
end
macro fake.revert.on
    task fake.revert
        active true
    end
end

# this variable will cycle through the known database names
$fakeImfile_DB = 0
$fake_revert_DB = 0

# select images ready for fake analysis
# new entries are added to fakePendingImfile
# skip already-present entries
task	       fake.imfile.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/fake.imfile.log

  task.exec
    if ($LABEL:n == 0) break
    $run = faketool -pendingimfile
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$fakeImfile_DB
      $run = $run -dbname $DB:$fakeImfile_DB
      $fakeImfile_DB ++
      if ($fakeImfile_DB >= $DB:n) set fakeImfile_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout fakePendingImfile -key fake_id:class_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook fakePendingImfile
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup fakePendingImfile
  end

  # locked list
  task.exit    default
    showcommand failure
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
  end
end

# run the fake_imfile.pl script on pending images
task	       fake.imfile.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC

    # fake is fast, so generate a bunch of commands in a row
    book npages fakePendingImfile -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in fakePendingImfile (pantaskState == INIT)
    book getpage fakePendingImfile 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword fakePendingImfile $pageName pantaskState RUN
    book getword fakePendingImfile $pageName camera -var CAMERA
    book getword fakePendingImfile $pageName exp_id -var EXP_ID
    book getword fakePendingImfile $pageName exp_tag -var EXP_TAG
    book getword fakePendingImfile $pageName fake_id -var FAKE_ID
    book getword fakePendingImfile $pageName workdir -var WORKDIR_TEMPLATE
    book getword fakePendingImfile $pageName class_id -var CLASS_ID
    book getword fakePendingImfile $pageName chip_path_base -var CHIPROOT
    book getword fakePendingImfile $pageName cam_path_base -var CAMROOT
    book getword fakePendingImfile $pageName dbname -var DBNAME
    book getword fakePendingImfile $pageName reduction -var REDUCTION

    # specify choice of local or remote host based on camera and fake (class_id)
    # XXX: this is not worth the trouble. use anyhost
    # set.host.for.camera $CAMERA $CLASS_ID
    host anyhost

    # set the WORKDIR variable
    set.workdir.by.camera $CAMERA $CLASS_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    ## generate outroot specific to this exposure (& chip)
    sprintf outroot "%s/%s/%s.fk.%s" $WORKDIR $EXP_TAG $EXP_TAG $FAKE_ID

    stderr $LOGDIR/fake.imfile.log
    stderr $LOGDIR/fake.imfile.log

    $run = fake_imfile.pl --exp_id $EXP_ID --fake_id $FAKE_ID --class_id $CLASS_ID --chiproot=$CHIPROOT --camroot=$CAMROOT --camera $CAMERA --outroot $outroot
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
    # if we are able to run the 'exec', use a short retry time
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit    default
    process_exit fakePendingImfile $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword fakePendingImfile $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword fakePendingImfile $options:0 pantaskState TIMEOUT
  end
end

# advance exposures for which all imfiles have completed processing
# sets the exposure state to full and queues warp processing if requested
$fake_advance_DB = 0
task	       fake.advanceexp
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -exec 30
  periods      -timeout 60
  npending     1

  stdout NULL
  stderr $LOGDIR/fake.advanceexp.log

  task.exec
    if ($LABEL:n == 0) break
    $run = faketool -advanceexp -limit 10
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$fake_advance_DB
      $run = $run -dbname $DB:$fake_advance_DB
      $fake_advance_DB ++
      if ($fake_advance_DB >= $DB:n) set fake_advance_DB = 0
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

task fake.revert
  host         local

  periods      -poll 60.0
  periods      -exec 1800.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = faketool -revertprocessedimfile
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$fake_revert_DB
      $run = $run -dbname $DB:$fake_revert_DB
      $fake_revert_DB ++
      if ($fake_revert_DB >= $DB:n) set fake_revert_DB = 0
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
