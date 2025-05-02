## calibration.pro : globals and support macros : -*- sh -*-
## this file contains the tasks for running the calibration stage
## these tasks use the book 'calBook'

# test for required global variables
check.globals

$LOGSUBDIR = $LOGDIR/calibration
mkdir $LOGSUBDIR

book init calBook

macro calibration.reset
  book init calBook
end

macro calibration.status
  book listbook calBook
end

macro calibration.on
  task calibration.init
    active true
  end
end

macro calibration.off
  task calibration.init
    active false
  end
end

# these variables will cycle through the known ippdb database names
$calInit_DB = 0

# we have three steps here:
# 1) get the list of dvo databases:  caltool -dbs
# 2) define a new calibration run for each of the dvo dbs
#    caltool 

# create new calibration entries for the currently known DVO databases
# run this multiple times once an hour - on pass for each db
task	       calibration.load
  host         local

  # check the list of available dvo dbs regularly
  periods      -poll 10
  periods      -exec 900
  periods      -timeout 60

  # this is a strange construction; what is the purpose?
  if ($DB:n == 0) 
    npending $DB:n
  else
    npending 1
  end

  # define the command (does not depend on previous queries)
  if ($DB:n != 0)
    command caltool -dbs -active true
  else
    # save the DB name for the exit tasks
    # note that this DB name refers to the ippdb, not the dvodb
    option $DB:$calInit_DB
    command caltool -dbs -dbname $DB:$calInit_DB
    $calInit_DB ++
    if ($calInit_DB >= $DB:n) set calInit_DB = 0
  end

  # silently drop stdout
  stdout NULL
  stderr $LOGSUBDIR/calibration.log

  # success
  task.exit $EXIT_SUCCESS
    # convert 'stdout' to book format
    # XXX have ippTools report the dbname?
    ipptool2book stdout calBook -key dbname:cal_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook calBook
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup calBook
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

# create new calibration entries for the currently known DVO databases
# run this multiple times once an hour - one pass for each db
# can we include information that the db has been updated without a recent cal analysis?
task	       calibration.resort
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 60
  # trange     Hourly@00:00 Hourly@00:10
  # every hour on the hour
  npending     1

  # silently drop stdout
  stdout NULL
  stderr $LOGSUBDIR/calibration.log

  task.exec
    book npages calBook -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in calBook
    # the sequencing in this task set is by the pantasksState (see notes.txt)
    book getpage calBook 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    # get the current ST: 
    $RAs = 15*($ST - 1)
    $RAe = 15*($ST - 2)
    $DECs = -90.0
    $DECe = +90.0

    $REGION = "$RAs,$RAe:$DECs,$DECe"

    book setword calBook $pageName pantaskState     RUN
    book setword calBook $pageName region           $REGION

    # XXX probably need to set the output / log based on WORKDIR...
    book getword calBook $pageName cal_id      -var ID
    book getword calBook $pageName dvodb       -var DVODB
    book getword calBook $pageName dbname      -var DBNAME

    # specify choice of remote host
    # set a specific DVO host here 
    if ($PARALLEL)
      host anyhost
    else
      host local
    end

    # save the pageName for future reference below
    options $pageName

    # XXX do we modify relphot to loop over the filters?
    $run = calibrate_dvo.pl --cal_id $ID --dvodb $DVODB --region $REGION
    add_standard_args run

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    command $run
  end

  # default exit status
  task.exit default
    process_exit calBook $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword calBook $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword calBook $options:0 pantaskState TIMEOUT
  end
end
