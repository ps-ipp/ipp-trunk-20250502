## addstar.pro : globals and support macros : -*- sh -*-
## this file contains the tasks for running the addstar analysis stage
## these tasks use the book addPendingExp

# test for required global variables
check.globals


#There is a book for each task, because I don't use labels. 

if (not($?haveminidvodbCopyBooks))
 book create MINIDVODB_COPY
 book create MINIDVODB_HOST
 $haveminidvodbCopyBooks = TRUE
end

$MINIDVODBCOPY_DB = 0
$MINIDVODB_HOSTS_DB = 0

book init minidvodbCopylist

macro minidvodb.copy.status
  book listbook minidvodbCopylist
end

macro minidvodb.copy.reset
  book init minidvodbCopylist
end

#this is the task that manages the copying of minidvodbs to another machine
macro minidvodb.copy.on
    task minidvodb.copy.load
    active true
  end
    task minidvodb.copy.run
    active true
  end
    task minidvodb.copy.queue
    active true
  end
end

macro minidvodb.copy.queue.on
    task minidvodb.copy.queue
    active true
  end
end

macro minidvodb.copy.queue.off   
    task minidvodb.copy.queue
    active false
   end
end

macro minidvodb.copy.load.on
    task minidvodb.copy.load
    active true
    end
end

macro minidvodb.copy.load.off
    task minidvodb.copy.load
    active false
    end
end

macro minidvodb.copy.run.on
    task minidvodb.copy.run
    active true
    end
end

macro minidvodb.copy.run.off
    task minidvodb.copy.run
    active false
    end
end

macro minidvodb.copy.off
   task minidvodb.copy.load
    active false
  end
   task minidvodb.copy.run
    active false
  end
   task minidvodb.copy.queue
    active false
  end
end


## you get no choice - you add all of them in at the same time. you can always turn off the tasks you don't want to run.
macro add.minidvodb.copy
  if ($0 != 4)
    echo "USAGE: add.minidvodb.copy (minidvodb_group) (host) (minidvodbcopy_path)"
    break
  end
  $hostandgroup = $2.$1
  #wait - shoudl be renamed MINIDVODB_WAIT
  book newpage MINIDVODB_COPY $hostandgroup
  book setword MINIDVODB_COPY $hostandgroup MINIDVODBHOSTANDGROUP $hostandgroup
  book setword MINIDVODB_COPY $hostandgroup MINIDVODB_GROUP $1
  book setword MINIDVODB_COPY $hostandgroup HOST $2
  book setword MINIDVODB_COPY $hostandgroup MINIDVODB_COPY_PATH $3
  book setword MINIDVODB_COPY $hostandgroup STATE PENDING
  book setword MINIDVODB_COPY $hostandgroup QUEUE_STATE PENDING
end 

macro del.minidvodb.copy
  if ($0 != 3)
    echo "USAGE: del.minidvodb.copy (host) (minidvodb_group)"
    break
  end
  book delpage MINIDVODB_COPY $1.$2
end

macro show.minidvodb.copy
  if ($0 != 1)
    echo "USAGE: show.minidvodb.copy"
    break
  end
  echo "minidvodb copy"
  book listbook MINIDVODB_COPY
end

macro add.minidvodb.host
  if ($0 != 2)
    echo "USAGE: add.minidvodb.host (host)"
    break
  end
    #wait - shoudl be renamed MINIDVODB_WAIT
  book newpage MINIDVODB_HOST $1
  book setword MINIDVODB_HOST $1 HOST $1
  
end 

macro del.minidvodb.host
  if ($0 != 2)
    echo "USAGE: del.minidvodb.host (host)"
    break
  end
  book delpage MINIDVODB_HOST $1
end

macro show.minidvodb.host
  if ($0 != 1)
    echo "USAGE: show.minidvodb.host"
    break
  end
  echo "minidvodb hosts"
  book listbook MINIDVODB_HOST
end

task           minidvodb.copy.queue
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 300
  npending     1

  stdout NULL
  stderr $LOGDIR/minidvodb.copy.queue.log

  task.exec
    book npages MINIDVODB_COPY -var N
    if ($N == 0)
#      echo "No labels for processing"
      break
    endif

    book getpage MINIDVODB_COPY 0 -var minidvodbhostandgroup -key QUEUE_STATE NEW
    if ("$minidvodbhostandgroup" == "NULL")
      # All labels have been done --- reset
#      echo "Resetting labels"
      for i 0 $N
        book getpage MINIDVODB_COPY $i -var minidvodbhostandgroup
        book setword MINIDVODB_COPY $minidvodbhostandgroup QUEUE_STATE NEW
      end
      book getpage MINIDVODB_COPY 0 -var minidvodbhostandgroup -key QUEUE_STATE NEW
      book getword MINIDVODB_COPY $minidvodbhostandgroup HOST -var host 
      book getword MINIDVODB_COPY $minidvodbhostandgroup MINIDVODB_GROUP -var minidvodb_group   
      book getword MINIDVODB_COPY $minidvodbhostandgroup MINIDVODB_COPY_PATH -var minidvodb_copy_path
      

   #   echo $minidvodb_group $minidvodb_copy_path 
   #   book getword MINIDVODB_COPY 0 minidvodb_copy_path -var MINIDVODB_COPY_PATH
      #this needs work - it is setting everything to null
      #also, I want the database (I think?)

      # Select different database
      $MINIDVODB_COPY_DB ++
      if ($MINIDVODB_COPY_DB >= $DB:n) set MINIDVODB_COPY_DB = 0
    end
#using check as opposed to list because it sees if it is done with the addRun state yet.  
    book setword MINIDVODB_COPY $minidvodbhostandgroup QUEUE_STATE DONE
    
    book npages MINIDVODB_HOST -var NUM
    if ($N ==0)
         break
    endif

    book getpage MINIDVODB_HOST $host -var host2 -key HOST $host
    if ($host2 != $host)
        break
    end

    #echo $host $host2

    
    $run = minidvodbtool -definebyquery -set_destination_host $host -set_minidvodb_rsync_path $minidvodb_copy_path
    $run = $run -minidvodb_group $minidvodb_group
       if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$MINIDVODB_COPY_DB
      $run = $run -dbname $DB:$MINIDVODB_COPY_DB
      $MINIDVODB_COPY_DB ++
      if ($MINIDVODB_COPY_DB >= $DB:n) set MINIDVODB_COPY_DB = 0
    end
    #add_poll_args run
    #echo $run
    command $run
  end
  # success
  task.exit    0
    # convert 'stdout' to book format
    #ipptool2book stdout minidvodCopylist -key minidvodbcopy_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    #if ($VERBOSE > 2)
    #  book listbook minidvodbCopylist
    #end
    # delete existing entries in the appropriate pantaskStates
    #process_cleanup minidvodbCopylist
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










task           minidvodb.copy.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/minidvodb.copy.load.log

  task.exec
    book npages MINIDVODB_COPY -var N
    if ($N == 0)
#      echo "No labels for processing"
      break
    endif

    book getpage MINIDVODB_COPY 0 -var minidvodbhostandgroup -key STATE NEW
    if ("$minidvodbhostandgroup" == "NULL")
      # All labels have been done --- reset
#      echo "Resetting labels"
      for i 0 $N
        book getpage MINIDVODB_COPY $i -var minidvodbhostandgroup
        book setword MINIDVODB_COPY $minidvodbhostandgroup STATE NEW
      end
      book getpage MINIDVODB_COPY 0 -var minidvodbhostandgroup -key STATE NEW
      book getword MINIDVODB_COPY $minidvodbhostandgroup HOST -var host 
      book getword MINIDVODB_COPY $minidvodbhostandgroup MINIDVODB_GROUP -var minidvodb_group   

      
   #   book getword MINIDVODB_COPY 0 minidvodb_copy_path -var MINIDVODB_COPY_PATH
      #this needs work - it is setting everything to null
      #also, I want the database (I think?)

      # Select different database
      $MINIDVODB_COPY_DB ++
      if ($MINIDVODB_COPY_DB >= $DB:n) set MINIDVODB_COPY_DB = 0
    end
#using check as opposed to list because it sees if it is done with the addRun state yet.  
    book setword MINIDVODB_COPY $minidvodbhostandgroup STATE DONE
    
    book npages MINIDVODB_HOST -var NUM
    if ($N ==0)
	 break
    endif

    book getpage MINIDVODB_HOST $host -var host2 -key HOST $host
    if ($host2 != $host)
	break
    end

    #echo $host $host2

    
    $run = minidvodbtool -listminidvodbcopy -destination_host $host
    $run = $run -minidvodb_group $minidvodb_group -pending
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$MINIDVODB_COPY_DB
      $run = $run -dbname $DB:$MINIDVODB_COPY_DB
      $MINIDVODB_COPY_DB ++
      if ($MINIDVODB_COPY_DB >= $DB:n) set MINIDVODB_COPY_DB = 0
    end
    add_poll_args run
    #echo $run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout minidvodbCopylist -key minidvodbcopy_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook minidvodbCopylist
    end
    # delete existing entries in the appropriate pantaskStates
    process_cleanup minidvodbCopylist
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




task           minidvodb.copy.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 17280 
 #if rsync is slow I want to make sure it doesn't time out

 ## we want only a single outstanding rsync job.  
  host         local
  npending     1


  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC

    book npages minidvodbCopylist -var N
    if ($N == 0) break
   
    # look for new images in minidvodbWaitlist (pantaskState == INIT)
    book getpage minidvodbCopylist 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break
    echo "got here"
    book setword minidvodbCopylist $pageName pantaskState RUN
    book getword minidvodbCopylist $pageName minidvodbcopy_id -var MINIDVODB_COPY_ID
    book getword minidvodbCopylist $pageName minidvodb_path -var MINIDVODBRUN_PATH
    book getword minidvodbCopylist $pageName minidvodb_id -var MINIDVODB_ID
    book getword minidvodbCopylist $pageName minidvodb_rsync_path -var MINIDVODB_RSYNC_PATH
    book getword minidvodbCopylist $pageName destination_host -var DESTINATION_HOST
    book getword minidvodbCopylist $pageName state -var STATE
    stdout $LOGDIR/minidvodb.copy.run.log
    stderr $LOGDIR/minidvodb.copy.run.log
    
    $run = minidvodb_copy.pl --minidvodbcopy_id $MINIDVODB_COPY_ID --minidvodb_id $MINIDVODB_ID  
# and more things
    $run = $run --minidvodbrun_path $MINIDVODBRUN_PATH --minidvodb_rsync_path $MINIDVODB_RSYNC_PATH
    $run = $run --destination_host $DESTINATION_HOST
   
if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$MINIDVODB_COPY_DB
      $run = $run --dbname $DB:$MINIDVODB_COPY_DB
      $MINIDVODB_COPY_DB ++
      if ($MINIDVODB_COPY_DB >= $DB:n) set MINIDVODB_COPY_DB = 0
    end
    # save the pageName for future reference below
    options $pageName
    echo command $run
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
    process_exit minidvodbCopylist $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    echo "hostname: $JOB_HOSTNAME"
    process_exit minidvodbCopylist $options:0 $EXIT_CRASH_ERR
  end

  # operation timed out?
    task.exit    timeout
	showcommand timeout
    book setword minidvodbCopylist $options:0 pantaskState TIMEOUT
  end
end

