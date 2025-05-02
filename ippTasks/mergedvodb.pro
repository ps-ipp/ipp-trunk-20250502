## addstar.pro : globals and support macros : -*- sh -*-
## this file contains the tasks for running the addstar analysis stage
## these tasks use the book addPendingExp

# test for required global variables
check.globals


#There is a book for each task, because I dont use labels. 

if (not($?havemergedvodbBooks))
  book create MERGEDVODB_MERGE
  $havemergedvodbBooks = TRUE
end

$MERGEDVODB_DB = 0
$MERGEDVODB_MERGE_DB = 0

book init mergedvodbMergelist

macro mergedvodb.merge.status
  book listbook mergedvodbMergelist
end

macro mergedvodb.merge.reset
  book init mergedvodbMergelist
end

macro mergedvodb.merge.on
  task mergedvodb.merge.load
    active true
  end
  task mergedvodb.merge.run
    active true
  end  
end
macro mergedvodb.merge.off
  task mergedvodb.merge.load
    active false
  end
  task mergedvodb.merge.run
    active false
  end  
end

## you get no choice - you add all of them in at the same time. you can always turn off the tasks you dont want to run.
macro add.mergedvodb
  if ($0 != 2)
    echo "USAGE: add.mergedvodb (mergedvodb)"
    break
  end
    #merge  
  book newpage MERGEDVODB_MERGE $1
  book setword MERGEDVODB_MERGE $1 MERGEDVODB $1
  book setword MERGEDVODB_MERGE $1 STATE PENDING
end

macro del.mergedvodb
  if ($0 != 2)
    echo "USAGE: del.mergedvodb (mergedvodb)"
    break
  end
  book delpage MERGEDVODB_MERGE $1
end

macro show.mergedvodb
  if ($0 != 1)
    echo "USAGE: show.mergedvodb"
    break
  end
  echo "mergedvodb merge"
  book listbook MERGEDVODB_MERGE
  echo "n pages"
  book npages MERGEDVODB_MERGE -var N
  echo $N
end


task           mergedvodb.merge.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/mergedvodb.merge.load.log

  task.exec
    book npages MERGEDVODB_MERGE -var N
    if ($N == 0)
      echo "No labels for processing"
      break
    endif
    book getpage MERGEDVODB_MERGE 0 -var mergedvodb -key STATE NEW
    if ("$mergedvodb" == "NULL")
      # All labels have been done --- reset
      for i 0 $N
        book getpage MERGEDVODB_MERGE $i -var mergedvodb
        book setword MERGEDVODB_MERGE $mergedvodb STATE NEW
      end
      book getpage MERGEDVODB_MERGE 0 -var mergedvodb -key STATE NEW

      # Select different database
      $MERGEDVODB_DB ++
      if ($MERGEDVODB_DB >= $DB:n) set MERGEDVODB_DB = 0
    end
    book setword MERGEDVODB_MERGE $mergedvodb STATE DONE
    
    $run = mergetool -pendingmerge -limit 1
    $run = $run -mergedvodb $mergedvodb
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$MERGEDVODB_DB
      $run = $run -dbname $DB:$MERGEDVODB_DB
      $MERGEDVODB_DB ++
      if ($MERGEDVODB_DB >= $DB:n) set MERGEDVODB_DB = 0
    end
    #add_poll_args run
    #echo $run
    command $run
  end

  # success
  task.exit    0
    #echo "success"
    # convert 'stdout' to book format
    ipptool2book stdout mergedvodbMergelist -key merge_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook mergedvodbMergelist
    end
    # delete existing entries in the appropriate pantaskStates
    process_cleanup mergedvodbMergelist
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

task           mergedvodb.merge.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60000
  
  #we only want one running at a time

  host         local
  npending     1

  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC
    book npages mergedvodbMergelist -var N
    if ($N == 0) break
    # look for new images in minidvodbWaitlist (pantaskState == INIT)
    book getpage mergedvodbMergelist 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break
    book setword mergedvodbMergelist $pageName pantaskState RUN
    book getword mergedvodbMergelist $pageName merge_id -var MERGE_ID
    book getword mergedvodbMergelist $pageName mergedvodb -var MERGEDVODB
    book getword mergedvodbMergelist $pageName mergedvodb_path -var MERGEDVODB_PATH
    book getword mergedvodbMergelist $pageName minidvodb_path -var MINIDVODB_PATH
    book getword mergedvodbMergelist $pageName state -var STATE
    stdout $LOGDIR/mergedvodb.merge.run.log
    stderr $LOGDIR/mergedvodb.merge.run.log
    $run = mergedvodb_merge.pl --camera GPC1 
    $run = $run --mergedvodb_path $MERGEDVODB_PATH 
    $run = $run --minidvodb_path $MINIDVODB_PATH 
    $run = $run --merge_id $MERGE_ID --mergedvodb $MERGEDVODB
    echo $run
  if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$MERGEDVODB_DB
      $run = $run --dbname $DB:$MERGEDVODB_DB
      $MERGEDVODB_DB ++
      if ($MERGEDVODB_DB >= $DB:n) set MERGEDVODB_DB = 0
    end
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
    process_exit mergedvodbMergelist $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
  
    echo "hostname: $JOB_HOSTNAME"

    # Set a fault code in the database
    
    process_exit mergedvodbMergelist $options:0 $EXIT_CRASH_ERR
  end

  # operation timed out?
    task.exit    timeout
	showcommand timeout
    book setword mergedvodbMergelist $options:0 pantaskState TIMEOUT
  end
end

