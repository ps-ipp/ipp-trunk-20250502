## addstar.pro : globals and support macros : -*- sh -*-
## this file contains the tasks for running the addstar analysis stage
## these tasks use the book addPendingExp

# test for required global variables
check.globals


#There is a book for each task, because I dont use labels. 

if (not($?haveminidvodbBooks))
 book create MINIDVODB
 book create MINIDVODB_PREMERGE
 book create MINIDVODB_CREATE
 book create MINIDVODB_ACTIVE   
 $haveminidvodbBooks = TRUE
end

$MINIDVODB_DB = 0

$MINIDVODB_WAIT_DB = 0
$MINIDVODB_PREMERGE_DB = 0
$MINIDVODB_CREATE_DB = 0
$MINIDVODB_ACTIVE_DB = 0

book init minidvodbWaitlist
book init minidvodbPreMergelist
book init minidvodbCreatelist
book init minidvodbActivelist

macro minidvodb.create.status
  book listbook minidvodbCreatelist
end

macro minidvodb.create.reset
  book init minidvodbCreatelist
end

macro minidvodb.active.status
  book listbook minidvodbActivelist
end

macro minidvodb.active.reset
  book init minidvodbActivelist
end


macro minidvodb.wait.status
  book listbook minidvodbWaitlist
end

macro minidvodb.wait.reset
  book init minidvodbWaitlist
end

macro minidvodb.premerge.status
  book listbook minidvodbPreMergelist
end

macro minidvodb.premerge.reset
  book init minidvodbPreMergelist
end

#this is the create task, there is no load, it just runs
macro minidvodb.create.on
    task minidvodb.create
    active true
  end
end
macro minidvodb.create.off
   task minidvodb.create
    active false
  end
end
##these are the tasks that flip from new -> active
macro minidvodb.active.on
  task minidvodb.active.load
    active true
  end
  task minidvodb.active.run
    active true
  end  
end
macro minidvodb.active.off
  task minidvodb.active.load
    active false
  end
  task minidvodb.active.run
    active false
  end  
end

##these are the tasks that check to see if addRun processing is finished
##for a minidvodb in state wait
macro minidvodb.wait.on
  task minidvodb.wait.load
    active true
  end
  task minidvodb.wait.run
    active true
  end  
end
macro minidvodb.wait.off
  task minidvodb.wait.load
    active false
  end
  task minidvodb.wait.run
    active false
  end  
end

##these merge the dbs

macro minidvodb.premerge.on
  task minidvodb.premerge.load
    active true
  end
  task minidvodb.premerge.run
    active true
  end  
end
macro minidvodb.premerge.off
  task minidvodb.premerge.load
    active false
  end
  task minidvodb.premerge.run
    active false
  end  
end

## you get no choice - you add all of them in at the same time. you can always turn off the tasks you dont want to run.
macro add.minidvodb
  if ($0 != 8)
    echo "USAGE: add.minidvodb (minidvodb_group) (minidvodb) (dvodb) (interval days) (interval addruns) (camera) (minidvodb_host)"
    break
  end

  #wait - shoudl be renamed MINIDVODB_WAIT
  book newpage MINIDVODB $1
  book setword MINIDVODB $1 MINIDVODB_GROUP $1
  book setword MINIDVODB $1 DVODB $3
  book setword MINIDVODB $1 STATE PENDING
  #merge  
  book newpage MINIDVODB_PREMERGE $1
  book setword MINIDVODB_PREMERGE $1 MINIDVODB_GROUP $1
  book setword MINIDVODB_PREMERGE $1 DVODB $3
  book setword MINIDVODB_PREMERGE $1 CAMERA $6
  book setword MINIDVODB_PREMERGE $1 MINIDVODB_HOST $7
  book setword MINIDVODB_PREMERGE $1 STATE PENDING
  #active  
  book newpage MINIDVODB_ACTIVE $1
  book setword MINIDVODB_ACTIVE $1 MINIDVODB_GROUP $1
  book setword MINIDVODB_ACTIVE $1 DVODB $3
  book setword MINIDVODB_ACTIVE $1 STATE PENDING

  #create  note that camera should be GPC1 for it to work. I couldnt figure out how to easily get this out.
  book newpage MINIDVODB_CREATE $1
  book setword MINIDVODB_CREATE $1 MINIDVODB_GROUP $1
  book setword MINIDVODB_CREATE $1 MINIDVODB $2
  book setword MINIDVODB_CREATE $1 DVODB $3
  book setword MINIDVODB_CREATE $1 DVODB_DAYS $4
  book setword MINIDVODB_CREATE $1 DVODB_NUM $5
  book setword MINIDVODB_CREATE $1 CAMERA $6  
  book setword MINIDVODB_CREATE $1 MINIDVODB_HOST $7
  book setword MINIDVODB_CREATE $1 STATE PENDING  
end

macro del.minidvodb
  if ($0 != 2)
    echo "USAGE: del.minidvodb (minidvodb)"
    break
  end
  book delpage MINIDVODB $1
  book delpage MINIDVODB_PREMERGE $1
  book delpage MINIDVODB_CREATE $1  
  book delpage MINIDVODB_ACTIVE $1
end

macro show.minidvodb
  if ($0 != 1)
    echo "USAGE: show.minidvodb"
    break
  end
  echo "minidvodb wait"
  book listbook MINIDVODB
  echo "minidvodb premerge"
  book listbook MINIDVODB_PREMERGE
  echo "minidvodb create"
  book listbook MINIDVODB_CREATE 
  echo "minidvodb active"
  book listbook MINIDVODB_ACTIVE
end


$LOADEXEC_MDB = 60
$timeout_mdb  = 2
$LOADPOLL = 5
$RUNEXEC = 30


task           minidvodb.wait.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC_MDB
  periods      -timeout 300
  npending     1

  stdout NULL
  stderr $LOGDIR/minidvodb.wait.load.log

  task.exec
    book npages MINIDVODB -var N
    if ($N == 0)
      echo "No labels for processing minidvodb.wait.load"
      break
    endif

    book getpage MINIDVODB 0 -var minidvodb_group -key STATE NEW
    if ("$minidvodb_group" == "NULL")
      # All labels have been done --- reset
#      echo "Resetting labels"
      for i 0 $N
        book getpage MINIDVODB $i -var minidvodb_group
        book setword MINIDVODB $minidvodb_group STATE NEW
      end
      book getpage MINIDVODB 0 -var minidvodb_group -key STATE NEW

      # Select different database
      $MINIDVODB_DB ++
      if ($MINIDVODB_DB >= $DB:n) set MINIDVODB_DB = 0
    end
#using check as opposed to list because it sees if it is done with the addRun state yet.  
    book setword MINIDVODB $minidvodb_group STATE DONE
    $run = addtool -checkminidvodbrunaddrun -state waiting
    $run = $run -minidvodb_group $minidvodb_group
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$MINIDVODB_DB
      $run = $run -dbname $DB:$MINIDVODB_DB
      $MINIDVODB_DB ++
      if ($MINIDVODB_DB >= $DB:n) set MINIDVODB_DB = 0
    end
    add_poll_args run
   
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout minidvodbWaitlist -key minidvodb_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook minidvodbWaitlist
    end
    # delete existing entries in the appropriate pantaskStates
    process_cleanup minidvodbWaitlist
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




task           minidvodb.wait.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 10

  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC

    book npages minidvodbWaitlist -var N
    if ($N == 0) break
   
    # look for new images in minidvodbWaitlist (pantaskState == INIT)
    book getpage minidvodbWaitlist 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword minidvodbWaitlist $pageName pantaskState RUN
    book getword minidvodbWaitlist $pageName minidvodb_id -var MINIDVODB_ID
    book getword minidvodbWaitlist $pageName state -var STATE
    stdout $LOGDIR/minidvodb.wait.run.log
    stderr $LOGDIR/minidvodb.wait.run.log
    
    $run = addtool -updateminidvodbrun -minidvodb_id $MINIDVODB_ID -set_state to_be_merged  
    
   
if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$MINIDVODB_DB
      $run = $run -dbname $DB:$MINIDVODB_DB
      $MINIDVODB_DB ++
      if ($MINIDVODB_DB >= $DB:n) set MINIDVODB_DB = 0
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
    process_exit minidvodbWaitlist $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    echo "hostname: $JOB_HOSTNAME"
    process_exit minidvodbWaitlist $options:0 $EXIT_CRASH_ERR
  end

  # operation timed out?
    task.exit    timeout
	showcommand timeout
    book setword minidvodbWaitlist $options:0 pantaskState TIMEOUT
  end
end



task           minidvodb.premerge.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC_MDB
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/minidvodb.premerge.load.log

  task.exec
    book npages MINIDVODB_PREMERGE -var N
    if ($N == 0)
#      echo "No labels for processing"
      break
    endif

    book getpage MINIDVODB_PREMERGE 0 -var minidvodb_group -key STATE NEW
    if ("$minidvodb_group" == "NULL")
      # All labels have been done --- reset
#      echo "Resetting labels"
      for i 0 $N
        book getpage MINIDVODB_PREMERGE $i -var minidvodb_group
        book setword MINIDVODB_PREMERGE $minidvodb_group STATE NEW
      end
      book getpage MINIDVODB_PREMERGE 0 -var minidvodb_group -key STATE NEW

      # Select different database
      $MINIDVODB_DB ++
      if ($MINIDVODB_DB >= $DB:n) set MINIDVODB_DB = 0
    end
    #finds the minidvodbs in a state of 'to_be_merged' 
    book setword MINIDVODB_PREMERGE $minidvodb_group STATE DONE
    $run = addtool -listminidvodbrun -state to_be_merged -limit 1
    $run = $run -minidvodb_group $minidvodb_group
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$MINIDVODB_DB
      $run = $run -dbname $DB:$MINIDVODB_DB
      $MINIDVODB_DB ++
      if ($MINIDVODB_DB >= $DB:n) set MINIDVODB_DB = 0
    end
    add_poll_args run
   
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout minidvodbPreMergelist -key minidvodb_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook minidvodbPreMergelist
    end
    # delete existing entries in the appropriate pantaskStates
    process_cleanup minidvodbPreMergelist
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




task           minidvodb.premerge.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60000
  
  #we only want one running at a time

  npending     100

  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC

    book npages minidvodbPreMergelist -var N
    if ($N == 0) break
    
    # look for new images in minidvodbWaitlist (pantaskState == INIT)
    book getpage minidvodbPreMergelist 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword minidvodbPreMergelist $pageName pantaskState RUN
    book getword minidvodbPreMergelist $pageName minidvodb_id -var MINIDVODB_ID
    book getword minidvodbPreMergelist $pageName minidvodb_group -var MINIDVODB_GROUP
    book getword minidvodbPreMergelist $pageName minidvodb_path -var MINIDVODB_PATH
    book getword minidvodbPreMergelist $pageName minidvodb_host -var MINIDVODB_HOST
    book getword minidvodbPreMergelist $pageName camera -var CAMERA
    book getword minidvodbPreMergelist $pageName state -var STATE
    stdout $LOGDIR/minidvodb.premerge.run.log
    stderr $LOGDIR/minidvodb.premerge.run.log

    #still buggy

    host -required $MINIDVODB_HOST

    $run = minidvodb_premerge.pl --camera GPC1 --minidvodb $MINIDVODB_PATH --minidvodb_group $MINIDVODB_GROUP --minidvodb_id $MINIDVODB_ID --minidvodb_host $MINIDVODB_HOST
    
  if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$MINIDVODB_DB
      $run = $run --dbname $DB:$MINIDVODB_DB
      $MINIDVODB_DB ++
      if ($MINIDVODB_DB >= $DB:n) set MINIDVODB_DB = 0
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
    process_exit minidvodbPreMergelist $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
  
    echo "hostname: $JOB_HOSTNAME"

    # Set a fault code in the database
    
    process_exit minidvodbPreMergelist $options:0 $EXIT_CRASH_ERR
  end

  # operation timed out?
    task.exit    timeout
	showcommand timeout
    book setword minidvodbPreMergelist $options:0 pantaskState TIMEOUT
  end
end


#this is the complicated script - it creates the minidvodbRuns
#since none may exist by default, I decided not to have a load.task
#it calls a perl script, which creates a new one if:
#there is none in an active state (true if there is none at all)
#the current one is older than the dvodb_age
#there are none in new (we have a task that moves it to 'active')
#the current active one has > 30000 add_ids in it

#also confusing: it succeeds if it doesn't create it (if it doesn't want to create one)
#and it succeeds if it creates one. (otherwise there would be a lot of false failures,
# like in replication)
#it fails if it has problems with one of the addtool commands in the script.

task           minidvodb.create
#  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC_MDB
  periods      -timeout 240
#  npending     1

  stdout NULL
  stderr $LOGDIR/minidvodb.create.log

  task.exec
    
# periods      -poll 60
#wait a bit before trying again

    book npages MINIDVODB_CREATE -var N
    if ($N == 0)
      echo "No labels for processing minidvodb create"
      break
    endif

    book getpage MINIDVODB_CREATE 0 -var minidvodb_group -key STATE NEW
    if ("$minidvodb_group" == "NULL")
      # All labels have been done --- reset
#      echo "Resetting labels"
      for i 0 $N
        book getpage MINIDVODB_CREATE $i -var minidvodb_group
        book setword MINIDVODB_CREATE $minidvodb_group STATE NEW
      end
       book getpage MINIDVODB_CREATE 0 -var minidvodb_group -key STATE NEW
      # Select different database
      $MINIDVODB_DB ++
      if ($MINIDVODB_DB >= $DB:n) set MINIDVODB_DB = 0
    end
     
    book setword MINIDVODB_CREATE $minidvodb_group STATE DONE
    book getword MINIDVODB_CREATE $minidvodb_group MINIDVODB -var minidvodb 
    book getword MINIDVODB_CREATE $minidvodb_group DVODB -var dvodb 
    book getword MINIDVODB_CREATE $minidvodb_group DVODB_DAYS -var dvodb_days
    book getword MINIDVODB_CREATE $minidvodb_group DVODB_NUM -var dvodb_num
    book getword MINIDVODB_CREATE $minidvodb_group CAMERA -var camera
    book getword MINIDVODB_CREATE $minidvodb_group MINIDVODB_HOST -var minidvodbhost
    $run = minidvodb_createdb.pl --camera $camera --outroot $dvodb --dvodb $dvodb --minidvodb $minidvodb --minidvodb_group $minidvodb_group --interval $dvodb_days --num $dvodb_num --minidvodb_host $minidvodbhost
 
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$MINIDVODB_DB
      $run = $run --dbname $DB:$MINIDVODB_DB
      $MINIDVODB_DB ++
      if ($MINIDVODB_DB >= $DB:n) set MINIDVODB_DB = 0
    end
    command $run
  end

  # success
  task.exit    0
     if ($VERBOSE > 2)
      showcommand 
     end
   end

  # locked list

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



task           minidvodb.active.load
#  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC_MDB
  periods      -timeout 10
#  npending     1

  stdout NULL
  stderr $LOGDIR/minidvodb.active.load.log

  task.exec
    book npages MINIDVODB_ACTIVE -var N
    if ($N == 0)
      echo "No labels for processing active.load"
      break
    endif

    book getpage MINIDVODB_ACTIVE 0 -var minidvodb_group -key STATE NEW
    if ("$minidvodb_group" == "NULL")
      # All labels have been done --- reset
#      echo "Resetting labels"
      for i 0 $N
        book getpage MINIDVODB_ACTIVE $i -var minidvodb_group
        book setword MINIDVODB_ACTIVE $minidvodb_group STATE NEW
      end
      book getpage MINIDVODB_ACTIVE 0 -var minidvodb_group -key STATE NEW
      
      # Select different database
      $MINIDVODB_DB ++
      if ($MINIDVODB_DB >= $DB:n) set MINIDVODB_DB = 0
    end
    book setword MINIDVODB_ACTIVE $minidvodb_group STATE DONE
    
    $run = addtool -listminidvodbrun -state new
    $run = $run -minidvodb_group $minidvodb_group
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$MINIDVODB_DB
      $run = $run -dbname $DB:$MINIDVODB_DB
      $MINIDVODB_DB ++
      if ($MINIDVODB_DB >= $DB:n) set MINIDVODB_DB = 0
    end
    add_poll_args run
   
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout minidvodbActivelist -key minidvodb_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook minidvodbActivelist
    end
    # delete existing entries in the appropriate pantaskStates
    process_cleanup minidvodbActivelist
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




task           minidvodb.active.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 10

  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC

    book npages minidvodbActivelist -var N
    if ($N == 0) break
   
    # look for new images in minidvodbActivelist (pantaskState == INIT)
    book getpage minidvodbActivelist 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword minidvodbActivelist $pageName pantaskState RUN
    book getword minidvodbActivelist $pageName minidvodb_group -var MINIDVODB_GROUP
    stdout $LOGDIR/minidvodb.active.run.log
    stderr $LOGDIR/minidvodb.active.run.log

    $run = addtool -flipminidvodbrun -minidvodb_group $MINIDVODB_GROUP
    
   
if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$MINIDVODB_DB
      $run = $run -dbname $DB:$MINIDVODB_DB
      $MINIDVODB_DB ++
      if ($MINIDVODB_DB >= $DB:n) set MINIDVODB_DB = 0
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
    process_exit minidvodbActivelist $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    #showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    # Set a fault code in the database
    
    process_exit minidvodbActivelist $options:0 $EXIT_CRASH_ERR
  end

  # operation timed out?
    task.exit    timeout
	showcommand timeout
    book setword minidvodbActivelist $options:0 pantaskState TIMEOUT
  end
end

