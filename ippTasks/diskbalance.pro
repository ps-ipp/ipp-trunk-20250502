## diskbalance.pro : tasks for data replication : -*- sh -*-
## this file contains the tasks for maintaining duplicates as needed in Nebulous
## these tasks use the books balancePending

# test for required global variables
check.globals

# load in the local configuration (NEB_USER, etc.)
module nebulous.site.pro

$LOGSUBDIR = $LOGDIR/balance
mkdir $LOGSUBDIR

book init balancePending
book init balanceControl


book init targetPending
book init targetControl

macro balance.reset
  book init balancePending
end

macro balance.status
  book listbook balancePending
end

macro balance.on
  task balance.load
    active true
  end
  task balance.run
    active true
  end
end

macro balance.off
  task balance.load
    active false
  end
  task balance.run
    active false
  end
end

macro target.reset
  book init targetPending
end

macro target.status
  book listbook targetPending
end

macro target.on
  task target.load
    active true
  end
  task target.run
    active true
  end
end

macro target.off
  task target.load
    active false
  end
  task target.run
    active false
  end
end
 
macro set.host.for.balance
  if ($0 != 2)
    echo "USAGE: set.host.for.balance (hostname)"
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

macro set.balance.numbers
  if ($0 != 3)
    echo "USAGE: set.balance.numbers (N_sources) (M_destinations)"
    break
  end

  book newpage balanceControl control
  book setword balanceControl control nSources $1
  book setword balanceControl control mDestinations $2
end

macro set.balance.range
  if ($0 != 3)
    echo "USAGE: set.balance.range (SO_ID_START) (SO_ID_RANGE)"
    break
  end

  $BALANCE_SO_ID_START = $1
  $BALANCE_SO_ID_RANGE = $2
end


macro show.balance.numbers
  book listpage balanceControl control
  echo "balance so_ids: $BALANCE_SO_ID_START $BALANCE_SO_ID_RANGE"
  echo "targetting:     $BALANCE_TARGET"
end

macro show.balancePending
  book npages balancePending -var N
  echo "$N pages in book"
  book listbook balancePending
end

macro set.balance.target
 if ($0 != 2) 
   echo "USAGE: set.balance.target (DESTINATION|SOURCE)"
   break
 end
 $BALANCE_TARGET = $1
end

macro set.target.range
  if ($0 != 3)
    echo "USAGE: set.target.range (SO_ID_START) (SO_ID_RANGE)"
    break
  end

  $TARGET_SO_ID_START = $1
  $TARGET_SO_ID_RANGE = $2
end


macro show.target.numbers
  echo "target so_ids: $TARGET_SO_ID_START $TARGET_SO_ID_RANGE"
end

macro show.targetPending
  book npages targetPending -var N
  echo "$N pages in book"
  book listbook targetPending
end

# the balance process interacts with only the single Nebulous server

# Each 'pendingbalance query is limited to a finite number of so_id values.
# Each time we call balance.load, we increment the so_id start by the range value
# If the pendingbalance query exits with exit status 10, we start over at 0

# This value will need to be incremented periodically.  It looks like we grow by about 5e7 so_ids per month.
$BALANCE_BIG_SO_ID   = 6e8
$BALANCE_SO_ID_START = int($BALANCE_BIG_SO_ID * rnd(0)) 
$BALANCE_SO_ID_RANGE = 500000
$BALANCE_LIMIT       = 500
$BALANCE_OFFSET      = 0
$BALANCE_TARGET      = SOURCE
# Select Nebulous objects which should be shuffled
task           balance.load
  host         local

  periods      -poll 60
  periods      -exec 60
  periods      -timeout 1500
  npending     1

  # logs
  stdout NULL
  stderr $LOGSUBDIR/balance.log
    
  task.exec
    book npages balancePending -var N
    if ($N > 2000)
      process_cleanup balancePending
      break
    end

    book getword balanceControl control nSources -var BALANCE_N
    book getword balanceControl control mDestinations -var BALANCE_M

    command neb-admin --host $NEB_HOST --db $NEB_DB --user $NEB_USER --pass $NEB_PASS --pendingbalance --limit $BALANCE_LIMIT --offset $BALANCE_OFFSET --so_id_start $BALANCE_SO_ID_START --so_id_range $BALANCE_SO_ID_RANGE  --balancesources $BALANCE_N --balancedestination $BALANCE_M

  end

   # success : 0 -- we did not hit the limit, advance so_id counter
  task.exit 0
    # advance the so_id counter
    $BALANCE_SO_ID_START = $BALANCE_SO_ID_START + $BALANCE_SO_ID_RANGE
    $BALANCE_OFFSET = 0
    # convert 'stdout' to book format
    ipptool2book stdout balancePending -key key -uniq -setword pantaskState INIT

    if ($VERBOSE > 2)
      book listbook balancePending
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup balancePending
  end

  # success : 1 -- we DID hit the limit, do NOT advance so_id counter 
  task.exit 1
    # convert 'stdout' to book format
    ipptool2book stdout balancePending -key key -uniq -setword pantaskState INIT
    $BALANCE_OFFSET = $BALANCE_OFFSET + $BALANCE_LIMIT
    if ($VERBOSE > 2)
      book listbook balancePending
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup balancePending
  end

  # out of so_id range, reset
  task.exit 10
    # advance the so_id counter
    $BALANCE_SO_ID_START = 0
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

# task to execute shuffles
task            balance.run
  periods       -poll 0.5
  periods       -exec 5
  periods       -timeout 5
  npending      200 

  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec 5

    book npages balancePending -var N
    if ($NETWORK == 0) break
    if ($N == 0) break

    # look for new objects in balancePending
    book getpage balancePending 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword balancePending $pageName pantaskState RUN

    book getword balancePending $pageName key              -var KEY
    book getword balancePending $pageName source_name      -var SOURCE
    book getword balancePending $pageName source_host      -var SOURCE_HOST
    book getword balancePending $pageName destination_name -var DESTINATION
    book getword balancePending $pageName destination_host -var DEST_HOST
    # Fix this with multihost restriction when possible.
    if ("$BALANCE_TARGET" == "DESTINATION")
       set.host.for.balance $DEST_HOST
    else 
       set.host.for.balance $SOURCE_HOST
    end
    

    stdout NULL
    stderr $LOGSUBDIR/balance.log

    $run = neb-shift --volume $DESTINATION $KEY $SOURCE
    
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
  task.exit     default
    process_exit balancePending $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword balancePending $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword balancePending $options:0 pantaskState TIMEOUT
  end
end


# This value will need to be incremented periodically.  It looks like we grow by about 5e7 so_ids per month.
$TARGET_BIG_SO_ID   = 6e8
$TARGET_SO_ID_START = int($TARGET_BIG_SO_ID * rnd(0)) 
$TARGET_SO_ID_RANGE = 500000
$TARGET_LIMIT       = 500
$TARGET_OFFSET      = 0
$TARGET_TARGET      = SOURCE
# Select Nebulous objects which should be shuffled
task           target.load
  host         local

  periods      -poll 60
  periods      -exec 60
  periods      -timeout 1500
  npending     1

  # logs
  stdout NULL
  stderr $LOGSUBDIR/target.log
    
  task.exec
    book npages targetPending -var N
    if ($N > 2000)
      process_cleanup targetPending
      break
    end

    command neb-admin --host $NEB_HOST --db $NEB_DB --user $NEB_USER --pass $NEB_PASS --pendingtarget --limit $TARGET_LIMIT --offset $TARGET_OFFSET --so_id_start $TARGET_SO_ID_START --so_id_range $TARGET_SO_ID_RANGE  
  end

   # success : 0 -- we did not hit the limit, advance so_id counter
  task.exit 0
    # advance the so_id counter
    $TARGET_SO_ID_START = $TARGET_SO_ID_START + $TARGET_SO_ID_RANGE
    $TARGET_OFFSET = 0
    # convert 'stdout' to book format
    ipptool2book stdout targetPending -key key -uniq -setword pantaskState INIT

    if ($VERBOSE > 2)
      book listbook targetPending
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup targetPending
  end

  # success : 1 -- we DID hit the limit, do NOT advance so_id counter 
  task.exit 1
    # convert 'stdout' to book format
    ipptool2book stdout targetPending -key key -uniq -setword pantaskState INIT
    $TARGET_OFFSET = $TARGET_OFFSET + $TARGET_LIMIT
    if ($VERBOSE > 2)
      book listbook targetPending
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup targetPending
  end

  # out of so_id range, reset
  task.exit 10
    # advance the so_id counter
    $TARGET_SO_ID_START = 0
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

# task to execute shuffles
task            target.run
  periods       -poll 0.5
  periods       -exec 5
  periods       -timeout 5
  npending      200 

  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec 5

    book npages targetPending -var N
    if ($NETWORK == 0) break
    if ($N == 0) break

    # look for new objects in targetPending
    book getpage targetPending 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword targetPending $pageName pantaskState RUN

    book getword targetPending $pageName key              -var KEY
    book getword targetPending $pageName source_name      -var SOURCE
    book getword targetPending $pageName source_host      -var SOURCE_HOST
    book getword targetPending $pageName destination_name -var DESTINATION
    book getword targetPending $pageName destination_host -var DEST_HOST
    book getword targetPending $pageName destination_uri  -var DEST_URI
    book getword targetPending $pageName ins_id           -var INS_ID

    # Fix this with multihost restriction when possible.
    if ("$TARGET_TARGET" == "DESTINATION")
       set.host.for.balance $DEST_HOST
    else 
       set.host.for.balance $SOURCE_HOST
    end
    

    stdout NULL
    stderr $LOGSUBDIR/target.log

    $run = neb-insedit --key $KEY --ins_id $INS_ID --uri $DEST_URI --volume $DESTINATION 
    $run = $run --db $NEB_DB --host $NEB_HOST --user $NEB_USER --pass $NEB_PASS
    $run = $run --neb_host http://ippc04/nebulous
    
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
  task.exit     default
    process_exit targetPending $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword targetPending $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword targetPending $options:0 pantaskState TIMEOUT
  end
end


