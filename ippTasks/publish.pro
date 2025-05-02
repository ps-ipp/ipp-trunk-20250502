## publish.pro: -*- sh -*-

# test for required global variables
check.globals

book init publishRun

macro publish.status
  book listbook publishRun
end

macro publish.reset
  book init publishRun
end

macro publish.on
  task publish.load
    active true
  end
  task publish.run
    active true
  end
  task publish.revert
    active true
  end
end

macro publish.off
  task publish.load
    active false
  end
  task publish.run
    active false
  end
  task publish.revert
    active false
  end
end

macro publish.revert.on
  task publish.revert
    active true
  end
end

macro publish.revert.off
  task publish.revert
    active false
  end
end

macro show.publish.books
  book npages SURVEY_PUBLISH -var N
  for i 0 $N
    book getpage SURVEY_PUBLISH $i
  end
  echo "SURVEY_PUBLISH: $SURVEY_PUBLISH"
end

macro show.publish.book
 if ($0 != 2)
   echo "USAGE: show.publish.book (page)"
   break
 end

 book listpage SURVEY_PUBLISH $1
end  

# this variable will cycle through the known database names
$publish_load_DB = 0
$publish_revert_DB = 0

task	       publish.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/publish.load.log

  task.exec
    $run = pubtool -pending
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$publish_load_DB
      $run = $run -dbname $DB:$publish_load_DB
      $publish_load_DB ++
      if ($publish_load_DB >= $DB:n) set publish_load_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout publishRun -key pub_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook publishRun
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup publishRun
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

task	       publish.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 300

  task.exec
    book npages publishRun -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    book getpage publishRun 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword publishRun $pageName pantaskState RUN
    book getword publishRun $pageName pub_id -var PUB_ID
    book getword publishRun $pageName camera -var CAMERA
    book getword publishRun $pageName workdir -var WORKDIR_TEMPLATE
    book getword publishRun $pageName product -var PRODUCT
    book getword publishRun $pageName stage -var STAGE
    book getword publishRun $pageName stage_id -var STAGE_ID
    book getword publishRun $pageName dbname -var DBNAME
    book getword publishRun $pageName output_format -var OUTPUT_FORMAT
    book getword publishRun $pageName need_magic -var NEED_MAGIC

    stdout $LOGDIR/publish.run.log
    stderr $LOGDIR/publish.run.log

    host anyhost
    strsub $WORKDIR_TEMPLATE @HOST@ $default_host -var WORKDIR

    $run = publish_file.pl --pub_id $PUB_ID --camera $CAMERA --workdir $WORKDIR --product $PRODUCT --stage $STAGE --stage_id $STAGE_ID --output_format $OUTPUT_FORMAT --redirect-output

    if ("$NEED_MAGIC" == "T") 
        $run = $run --need-magic
    end

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
    process_exit publishRun $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword publishRun $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword publishRun $options:0 pantaskState TIMEOUT
  end
end


task publish.revert
  host         local

  periods      -poll 5.0
  periods      -exec 60.0
  periods      -timeout 120.0
  npending     1

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = pubtool -revert
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$publish_revert_DB
      $run = $run -dbname $DB:$publish_revert_DB
      $publish_revert_DB ++
      if ($publish_revert_DB >= $DB:n) set publish_revert_DB = 0
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
