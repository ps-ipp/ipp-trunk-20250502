## replicate.pro : tasks for data replication : -*- sh -*-

# test for required global variables
check.globals

# load in the local configuration (NEB_USER, etc.)
# module nebulous.site.pro

$LOGSUBDIR = $LOGDIR/rawcheck
mkdir $LOGSUBDIR

book init rawcheckPending

macro rawcheck.reset
  book init checkPending
end

macro rawcheck.status
  book listbook rawcheckPending
end

macro rawcheck.on
  task rawcheck.load
    active true
 end
  task rawcheck.run
    active true
 end
end

macro rawcheck.off
  task rawcheck.load
    active false
 end
  task rawcheck.run
    active false
 end
end

# this variable will cycle through the known database names
$rawcheck_DB = 0


$DATEOBS_MIN  = 1970-01-01T00:00:00

macro rawcheck.set.date
  if ($0 != 2) 
    echo "USAGE: rawcheck.set.date (YYYY-MM-DD)"
    break
  end

  $DATEOBS_MIN = "$1"
end

macro rawcheck.show.date

  echo "DATE: "
  echo $DATEOBS_MIN
end


task           rawcheck.load
  host         local

  # modify these after the tasks are tested
  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 60
  npending     1

  # silently drop stdout
  stdout NULL
  stderr $LOGSUBDIR/rawcheck.log

  task.exec
      book npages rawcheckPending -var N
      if ($N > 200)
        process_cleanup rawcheckPending
        break
     end      
     if ($N > 1) 
        book getpage rawcheckPending N -var pageName
        book getword rawcheckPending $pageName dateobs -var DATEOBS_MIN
     end
     

     $run = regtool -processedexp -state full -dateobs_begin $DATEOBS_MIN 

     if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$rawcheck_DB
      $run = $run -dbname $DB:$rawcheck_DB
      $rawcheck_DB ++
      if ($rawcheck_DB >= $DB:n) set rawcheck_DB = 0

    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout rawcheckPending -key exp_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook rawcheckPending
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup rawcheckPending
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


task        rawcheck.run
  periods      -poll 0.5
  periods      -exec 5
  periods      -timeout 1200
  host         anyhost


  task.exec
    periods -exec 5

    book npages rawcheckPending -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new objects in rawcheckPending
    book getpage rawcheckPending 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword rawcheckPending $pageName pantaskState RUN

    # XXX what values do I need to get back?
    book getword rawcheckPending $pageName exp_id         -var EXP_ID
    book getword rawcheckPending $pageName dbname         -var DBNAME

#    stdout NULL
    stdout $LOGSUBDIR/rawcheck.log
    stderr $LOGSUBDIR/rawcheck.log

    # these operations do not require a database to be specified
    $run = rawcheck.pl --dbname $DBNAME --exp_id $EXP_ID

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
    process_exit rawcheckPending $options:0 $JOB_STATUS
  end

  task.exit    crash
    showcommand crash
    book setword rawcheckPending $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword rawcheckPending $options:0 pantaskState TIMEOUT
  end
end


