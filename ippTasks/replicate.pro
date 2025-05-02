## replicate.pro : tasks for data replication : -*- sh -*-
## this file contains the tasks for maintaining duplicates as needed in Nebulous
## these tasks use the books replicatePending

# test for required global variables
check.globals

# load in the local configuration (NEB_USER, etc.)
module nebulous.site.pro

$LOGSUBDIR = $LOGDIR/replicate
mkdir $LOGSUBDIR

book init replicatePending

macro replicate.reset
  book init replicatePending
end

macro replicate.status
  book listbook replicatePending
end

macro replicate.on
  task replicate.load
    active true
  end
  task replicate.run
    active true
  end
end

macro replicate.off
  task replicate.load
    active false
  end
  task replicate.run
    active false
  end
end

macro set.host.for.replicate
  if ($0 != 2)
    echo "USAGE: set.host.for.replicate (hostname)"
    break
  end

  if (not($PARALLEL))
    host local
    return
  end

# parse volume name

  if ("$1" == "NULL")
    host anyhost
  else
    host $1
  end
end

# the replicate process interacts with only the single Nebulous server

# Each 'pendingreplicate' query is limited to a finite number of so_id values.  
# Each time we call replicate.load, we increment SO_ID_START by the range value.  
# If the pendingreplicate query exits with exit status 10, we start over at 0

$SO_ID_START = 0
$SO_ID_RANGE = 500000

# select Nebulous objects which desire additional copies
task	       replicate.load
  host         local

  # modify these after the tasks are tested
  periods      -poll 0.5
  periods      -exec 5
  periods      -timeout 1500
  npending     1

  # silently drop stdout
  stdout NULL
  stderr $LOGSUBDIR/replicate.log

  task.exec
      book npages replicatePending -var N
      if ($N > 2000)
        process_cleanup replicatePending
        break
      end      

      # command does not need to be dynamic, but having it so allows us to adjust the periods
      # so that we dont have to wait 10 minutes for things to start up
      # XXX smaller limited?  7500 will be a huge book of things to do...
      command neb-admin --host $NEB_HOST --db $NEB_DB --user $NEB_USER --pass $NEB_PASS --pendingreplicate --limit 500 --so_id_start $SO_ID_START --so_id_range $SO_ID_RANGE
      # periods      -exec 1800
  end

  # success : 0 -- we did not hit the limit, advance so_id counter
  task.exit 0
    # advance the so_id counter
    $SO_ID_START = $SO_ID_START + $SO_ID_RANGE

    # convert 'stdout' to book format
    ipptool2book stdout replicatePending -key key -uniq -setword pantaskState INIT

    if ($VERBOSE > 2)
      book listbook replicatePending
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup replicatePending
  end

  # success : 1 -- we DID hit the limit, do NOT advance so_id counter 
  task.exit 1
    # convert 'stdout' to book format
    ipptool2book stdout replicatePending -key key -uniq -setword pantaskState INIT

    if ($VERBOSE > 2)
      book listbook replicatePending
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup replicatePending
  end

  # out of so_id range, reset
  task.exit 10
    # advance the so_id counter
    $SO_ID_START = 0
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

# create the desired replicas
task	       replicate.run
  periods      -poll 0.5
  periods      -exec 5
  periods      -timeout 30

  task.exec
    periods -exec 5

    book npages replicatePending -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new objects in replicatePending
    book getpage replicatePending 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword replicatePending $pageName pantaskState RUN

    # XXX what values do I need to get back?
    book getword replicatePending $pageName key         -var KEY
    book getword replicatePending $pageName need_copies -var NEED_COPIES
    book getword replicatePending $pageName volume_name -var VOLUME_NAME
    book getword replicatePending $pageName volume_host -var VOLUME_HOST
    book getword replicatePending $pageName command     -var COMMAND

    set.host.for.replicate $VOLUME_HOST

    stdout NULL
    stderr $LOGSUBDIR/replicate.log

    # these operations do not require a database to be specified
    $run = $COMMAND $KEY

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
  task.exit default
    process_exit replicatePending $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword replicatePending $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword replicatePending $options:0 pantaskState TIMEOUT
  end
end

