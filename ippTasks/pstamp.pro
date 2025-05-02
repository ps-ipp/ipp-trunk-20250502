# pstamp.pro : Postage Stamp Server tasks

check.globals

$LOGSUBDIR = $LOGDIR/pstamp
mkdir $LOGSUBDIR

# these variables wil cycle through the known database names

$pstampDS_DB = 0
$pstampReq_DB = 0
$pstampJob_DB = 0
$pstampFin_DB = 0
$pstampRev_DB = 0
$pstampRevReq_DB = 0
$pstampDep_DB = 0
$pstampRevDep_DB = 0
$pstampCleanup_DB = 0
$pstampStopFaulted_DB = 0
$pstampQCleanup_DB = 0

# limit number of requests in the queue to avoid blocking everything
# when jobs take an infinite amount of time to parse
$PSTAMP_PARSE_LIMIT = 20
macro set.parse.limit
    $PSTAMP_PARSE_LIMIT = $1
end
macro get.parse.limit
    echo parse limit : $PSTAMP_PARSE_LIMIT
end

# give up on dependents with fault_count >= $PSTAMP_MAX_FAULT_COUNT
$PSTAMP_MAX_FAULT_COUNT = 3
macro set.max.fault.count
    $PSTAMP_MAX_FAULT_COUNT = $1
end
macro get.max.fault.count
    echo maximum fault count: $PSTAMP_MAX_FAULT_COUNT
end
$POLL_DEP = 500

macro set.dependent.poll
    $POLL_DEP = $1
end
macro get.dependent.poll
    echo dependent poll limit: $POLL_DEP
end

# set PS_DBSERVER if postage stamp database host is not the same as the value for DBSERVER in site.config
# warning: no quotes around the two words. That causes the variable to get passed to pstamptool as one word
# and a tricky to debug problem ensues
# example:
# $PS_DBSERVER = -dbserver hostname

if ($?PS_DBSERVER == 0)
    $PS_DBSERVER = ""
end

macro pstamp.reset
    book init pstampRequest
    book init pstampJob
    book init pstampFinish
    book init pstampDependent
    book init pstampCleanup
end

pstamp.reset

macro pstamp.on
    task pstamp.request.find
        active true
    end
    task pstamp.request.load
        active true
    end
    task pstamp.request.run
        active true
    end

    task request.finish.load
        active true
    end
    task request.finish.run
        active true
    end

    task pstamp.job.load
        active true
    end
    task pstamp.job.run
        active true
    end
    task pstamp.dependent.load
        active true
    end
end


macro pstamp.off
    task pstamp.request.find
        active false
    end
    task pstamp.request.load
        active false
    end
    task pstamp.request.run
        active false
    end
    task request.finish.load
        active false
    end
    task request.finish.run
        active false
    end

    task pstamp.job.load
        active false
    end
    task pstamp.job.run
        active false
    end
    task pstamp.dependent.load
        active false
    end
    task pstamp.dependent.run
        active false
    end
end

macro pstamp.revert.on
    task pstamp.request.revert
        active true
    end
    task pstamp.job.revert
        active true
    end
    task pstamp.dependent.revert
        active true
    end
    task pstamp.stopfaulted
        active true
    end
end
macro pstamp.revert.off
    task pstamp.request.revert
        active false
    end
    task pstamp.job.revert
        active false
    end
    task pstamp.dependent.revert
        active false
    end
    task pstamp.stopfaulted
        active false
    end
end
macro pstamp.cleanup.on
    task pstamp.cleanup.load
        active true
    end
    task pstamp.cleanup.run
        active true
    end
    task pstamp.queue.cleanup
        active true
    end
end
macro pstamp.cleanup.off
    task pstamp.cleanup.load
        active false
    end
    task pstamp.cleanup.run
        active false
    end
    task pstamp.queue.cleanup
        active false
    end
end

macro pstamp.find.on
    task pstamp.request.find
        active true
    end
end
macro pstamp.find.off
    task pstamp.request.find
        active false
    end
end
macro pstamp.parse.on
    task pstamp.request.load
        active true
    end
    task pstamp.request.run
        active true
    end
end
macro pstamp.parse.off
    task pstamp.request.load
        active false
    end
    task pstamp.request.run
        active false
    end
end
macro pstamp.job.on
    task pstamp.job.load
        active true
    end
    task pstamp.job.run
        active true
    end
end
macro pstamp.job.off
    task pstamp.job.load
        active false
    end
    task pstamp.job.run
        active false
    end
end
macro pstamp.finish.on
    task pstamp.finish.load
        active true
    end
    task pstamp.finish.run
        active true
    end
end
macro pstamp.finish.off
    task pstamp.finish.load
        active false
    end
    task pstamp.finish.run
        active false
    end
end

macro pstamp.dependent.on
    task pstamp.dependent.load
        active true
    end
    task pstamp.dependent.run
        active true
    end
end
macro pstamp.dependent.off
    task pstamp.dependent.load
        active false
    end
    task pstamp.dependent.run
        active false
    end
end


macro pstamp.status.on
    task pstamp.save.status
        active true
    end
end
macro pstamp.status.off
    task pstamp.save.status
        active false
    end
end

macro pstamp.status.set.exec
    task pstamp.save.status
        periods -exec $1
    end
end

echo DEFINING PLABEL stuff
# keep a separate list of labels for request parsing
if ($?PLABEL:n == 0)   	  set PLABEL:n = 0
set PLABEL:n = 0

macro add.parse.label
  if ($0 != 2)
    echo "USAGE: add.parse.label (label)"
    break
  end
  if ($?PLABEL:n == 0)
    list PLABEL -add $1
    return
  end

  local found
  $found = 0
  for i 0 $PLABEL:n
    if ($PLABEL:$i == $1) 
      $found = 1
      echo "$PLABEL:$i set"
      last
    end
  end
  
  if ($found == 0)
    list PLABEL -add $1
  end
end


macro del.parse.label
  if ($0 != 2)
    echo "USAGE: del.parse.label (label)"
    break
  end
  if ($?PLABEL:n == 0)
    return
  end

  list PLABEL -del $1
end

macro show.parse.labels
  if ($0 != 1)
    echo "USAGE: show.parse.labels"
    break
  end
  if ($?PLABEL:n == 0)
    echo "no labels defined"
  end
  if ($PLABEL:n == 0)
    echo "no labels defined"
  end

  local i
  for i 0 $PLABEL:n
    echo $PLABEL:$i
  end
end

macro add_parse_labels
    if ($0 != 2)
	echo "Must pass in the command of interest, and this function will supplement"
	stop
    end

    local command i

    $command = $$1

    # Only process the data with the specified label.
    for i 0 $PLABEL:n
      $command = $command -label $PLABEL:$i
    end

    $$1 = $command
end

echo DONE defineing plabel stuf

task pstamp.request.find
    host        local

    periods     -poll $LOADPOLL
    periods     -exec 10
    periods     -timeout 120
    npending    1

    task.exec
        stdout NULL
        stderr $LOGSUBDIR/pstamp.request.find.log
        if ($DB:n == 0)
            option DEFAULT
            command pstamp_queue_requests.pl --limit 5
        else 
            option $DB:$pstampDS_DB
            command pstamp_queue_requests.pl --limit 5 --dbname $DB:$pstampDS_DB --verbose
            $pstampDS_DB ++
            if ($pstampDS_DB >= $DB:n) set pstampDS_DB = 0
        end
    end

    task.exit $EXIT_SUCCESS
        # echo nothing to do
    end

    task.exit   default
        showcommand failure
    end

    task.exit   crash
        showcommand crash
    end

    task.exit   timeout
        showcommand timeout
    end
end

task pstamp.request.load
    host        local

    periods     -poll $LOADPOLL
    periods     -exec $LOADEXEC
    periods     -timeout 300
    npending    1

    task.exec
        stdout NULL
        stderr $LOGSUBDIR/pstamp.request.load.log
        $run = pstamptool -pendingreq
        if ($DB:n == 0)
            option DEFAULT
        else 
            option $DB:$pstampReq_DB
            $run = $run $PS_DBSERVER -dbname $DB:$pstampReq_DB
            $pstampReq_DB ++
            if ($pstampReq_DB >= $DB:n) set pstampReq_DB = 0
        end
        add_poll_args run
        # add_poll_labels run
        add_parse_labels run
        # limit number of requests in the queue to avoid blocking everything
        # when jobs take an infinite amount of time to parse
        #$run = $run -limit 10
        # except parsing is not label/prio based, high prio can get stuck behind low prio, so bump up to 20
        # -- should be a config option 
        $run = $run -limit $PSTAMP_PARSE_LIMIT 
        command $run
    end

    task.exit $EXIT_SUCCESS
        ipptool2book stdout pstampRequest -key req_id -uniq -setword dbname $options:0 -setword pantaskState INIT
        if ($VERBOSE > 2)
            echo starting request
            book listbook pstampRequest
        end

        process_cleanup pstampRequest
    end

    task.exit   default
        showcommand failure
    end

    task.exit   crash
        showcommand crash
    end

    task.exit   timeout
        showcommand timeout
    end
end

task pstamp.request.run
    periods     -poll $RUNPOLL
    periods     -exec $RUNEXEC
    periods     -timeout 300

    task.exec
        stdout NULL
        stderr $LOGSUBDIR/pstamp.request.run.log
        book npages pstampRequest -var N
        if ($N == 0) break
        
        book getpage pstampRequest 0 -var pageName -key pantaskState INIT
        if ("$pageName" == "NULL") break

        book setword pstampRequest $pageName pantaskState RUN
        book getword pstampRequest $pageName req_id -var REQ_ID
        book getword pstampRequest $pageName dbname -var DBNAME
        book getword pstampRequest $pageName uri -var URI
        book getword pstampRequest $pageName ds_outProduct -var PRODUCT
        book getword pstampRequest $pageName outdir -var OUTDIR
        book getword pstampRequest $pageName need_magic -var NEED_MAGIC
        book getword pstampRequest $pageName label -var LABEL

        host anyhost

        $run = pstamp_parser_run.pl --req_id $REQ_ID --uri $URI --product $PRODUCT --outdir $OUTDIR --label $LABEL --redirect-output

        if ($NEED_MAGIC != 0)
            $run = $run --need_magic
        end

        add_standard_args run
        options $pageName

        if ($VERBOSE > 1) 
            echo command $run
        end
        command $run
    end


    task.exit $EXIT_SUCCESS
        process_exit pstampRequest $options:0 $JOB_STATUS
    end

    task.exit default
        showcommand failure
        process_exit pstampRequest $options:0 $JOB_STATUS
    end

    task.exit crash
        showcommand crash
        book setword pstampRequest $options:0 pantaskState CRASH
    end

    task.exit timeout
        showcommand timeout
        book setword pstampRequest $options:0 pantaskState TIMEOUT
    end
end

task request.finish.load
    host        local

    periods     -poll $LOADPOLL
    periods     -exec $LOADEXEC
    periods     -timeout 300
    npending    1

    task.exec
        stdout NULL
        stderr $LOGSUBDIR/pstamp.finish.load.log
        $run = pstamptool  -completedreq
        if ($DB:n == 0)
            option DEFAULT
        else 
            option $DB:$pstampFin_DB
            $run = $run -dbname $DB:$pstampFin_DB $PS_DBSERVER
            $pstampFin_DB ++
            if ($pstampFin_DB >= $DB:n) set pstampFin_DB = 0
        end
        add_poll_args run
        add_poll_labels run
        # limit query for finished requests to small number
	# because we only run 4 at a time. Using the default queue
	# depth fouls up the priority orer
	# XXX: use a varaible
	$run = $run -limit 8
        command $run
    end

    task.exit $EXIT_SUCCESS
        ipptool2book stdout pstampFinish -key req_id -uniq -setword dbname $options:0 -setword pantaskState INIT
        if ($VERBOSE > 2)
            book listbook pstampFinish
        end

        process_cleanup pstampFinish
    end

    task.exit   default
        showcommand failure
    end

    task.exit   crash
        showcommand crash
    end

    task.exit   timeout
        showcommand timeout
    end
end

task request.finish.run
    periods     -poll $RUNPOLL
    periods     -exec $RUNEXEC
    periods     -timeout 2400
    host        anyhost
    npending    4

    task.exec
        stdout NULL
        stderr $LOGSUBDIR/request.finish.run.log
        book npages pstampFinish -var N
        if ($N == 0) break
        
        book getpage pstampFinish 0 -var pageName -key pantaskState INIT
        if ("$pageName" == "NULL") break

        book setword pstampFinish $pageName pantaskState RUN
        book getword pstampFinish $pageName req_id -var REQ_ID
        book getword pstampFinish $pageName uri -var URI
        book getword pstampFinish $pageName dbname -var DBNAME
        book getword pstampFinish $pageName reqType -var REQ_TYPE
        book getword pstampFinish $pageName name -var REQ_NAME
        book getword pstampFinish $pageName outProduct -var PRODUCT
        book getword pstampFinish $pageName outdir -var OUTDIR

        $run = request_finish.pl --req_id $REQ_ID --req_type $REQ_TYPE --req_file $URI --req_name $REQ_NAME --product $PRODUCT --outdir $OUTDIR --redirect-output

        add_standard_args run
        options $pageName

        if ($VERBOSE > 1) 
            echo command $run
        end
        command $run
    end

    task.exit $EXIT_SUCCESS
        process_exit pstampFinish $options:0 $JOB_STATUS
    end

    task.exit default
        # echo request.finish.run task.exit status: $JOB_STATUS
        process_exit pstampFinish $options:0 $JOB_STATUS
        showcommand failure
    end

    task.exit crash
        showcommand crash
        book setword pstampFinish $options:0 pantaskState CRASH.run
    end

    task.exit timeout
        showcommand timeout
        book setword pstampFinish $options:0 pantaskState TIMEOUT.run
    end
end

task pstamp.job.load
    host        local

    periods     -poll $LOADPOLL
    periods     -exec $LOADEXEC
    periods     -timeout 30
    npending    1

    task.exec
        stdout NULL
        stderr $LOGSUBDIR/pstamp.job.load.log
        $run = pstamptool -pendingjob
        if ($DB:n == 0)
            option DEFAULT
        else
            option $DB:$pstampJob_DB
            $run = $run -dbname $DB:$pstampJob_DB $PS_DBSERVER
            $pstampJob_DB ++
            if ($pstampJob_DB >= $DB:n) set pstampJob_DB = 0
        end
        add_poll_args run
        add_poll_labels run 
        command $run
    end

    task.exit $EXIT_SUCCESS
        ipptool2book stdout pstampJob -key job_id -uniq -setword dbname $options:0 -setword pantaskState INIT

        book npages pstampJob -var N
        if ($VERBOSE > 2)
            book listbook pstampJob
        end

        # delete existing entries in the appropriate pantaskStates
        process_cleanup pstampJob
    end

    task.exit   crash
        showcommand crash
    end

    task.exit   timeout
        showcommand timeout
    end

    task.exit   default
        showcommand failure
    end

end

task pstamp.job.run
    periods     -poll $RUNPOLL
    periods     -exec $RUNEXEC
    periods     -timeout 1200

    task.exec
        periods -exec $RUNEXEC

        stdout NULL
        stderr $LOGSUBDIR/pstamp.job.run.log
        book npages pstampJob -var N
        if ($N == 0) break
        
        book getpage pstampJob 0 -var pageName -key pantaskState INIT
        if ("$pageName" == "NULL") break

        #echo pageName: $pageName

        book setword pstampJob $pageName pantaskState RUN
        book getword pstampJob $pageName job_id -var JOB_ID
        book getword pstampJob $pageName jobType -var JOB_TYPE
        book getword pstampJob $pageName rownum -var ROWNUM
        book getword pstampJob $pageName dbname -var DBNAME
        book getword pstampJob $pageName outputBase -var OUTPUT_BASE
        book getword pstampJob $pageName options -var OPTIONS

        if ($VERBOSE > 1) 
            book listpage pstampJob $pageName
        end

        host anyhost

        $run = pstamp_job_run.pl --job_id $JOB_ID --job_type $JOB_TYPE --rownum $ROWNUM --output_base $OUTPUT_BASE --options $OPTIONS --redirect-output 
        add_standard_args run

        options $pageName

        if ($VERBOSE > 1) 
            echo command $run
        end
        periods -exec 0.05
        command $run
    end


    task.exit $EXIT_SUCCESS
        if ($VERBOSE > 1)
            echo pstamp.job.run task.exit $JOB_ID status: $JOB_STATUS
        end
        process_exit pstampJob $options:0 $JOB_STATUS
    end
    task.exit default
        if ($VERBOSE > 1)
            echo pstamp.job.run task.exit $JOB_ID status: $JOB_STATUS
        end
#        showcommand failure
        process_exit pstampJob $options:0 $JOB_STATUS
    end

    task.exit crash
        echo pstamp.job.run task.crash $JOB_ID status: $JOB_STATUS
        process_exit pstampJob $options:0 $JOB_STATUS
        showcommand crash
        book setword pstampJob $options:0 pantaskState CRASH
    end

    task.exit timeout
        echo pstamp.job.run task.timeout $JOB_ID status: $JOB_STATUS
        process_exit pstampJob $options:0 $JOB_STATUS
        showcommand timeout
        book setword pstampJob $options:0 pantaskState TIMEOUT
    end
end

task pstamp.request.revert
    host        local

    periods     -poll $LOADPOLL
    periods     -exec 1200
    periods     -timeout 20
    npending    1

    task.exec
        stdout NULL
        stderr $LOGSUBDIR/pstamp.request.revert.log
        $run = pstamptool -revertreq
        if ($DB:n == 0)
            option DEFAULT
        else 
            $run = $run $PS_DBSERVER -dbname $DB:$pstampRevReq_DB
            $pstampRevReq_DB ++
            if ($pstampRevReq_DB >= $DB:n) set pstampRevReq_DB = 0
        end
        add_poll_args run
        add_poll_labels run
        command $run
    end

    task.exit $EXIT_SUCCESS
        # nothing to do
    end

    task.exit   default
        showcommand failure
    end

    task.exit   crash
        showcommand crash
    end

    task.exit   timeout
        showcommand timeout
    end
end
task pstamp.job.revert
    host        local

    periods     -poll $LOADPOLL
    periods     -exec 300
    periods     -timeout 300
    npending    1

    task.exec
        stdout NULL
        stderr $LOGSUBDIR/pstamp.job.revert.log
        $run = pstamptool -revertjob
        if ($DB:n == 0)
            option DEFAULT
        else 
            $run = $run $PS_DBSERVER -dbname $DB:$pstampRev_DB
            $pstampRev_DB ++
            if ($pstampRev_DB >= $DB:n) set pstampRev_DB = 0
        end
        add_poll_args run
        add_poll_labels run
        command $run
    end

    task.exit $EXIT_SUCCESS
        # nothing to do
    end

    task.exit   default
        showcommand failure
    end

    task.exit   crash
        showcommand crash
    end

    task.exit   timeout
        showcommand timeout
    end
end

task pstamp.dependent.load
    host        local

    periods     -poll $LOADPOLL
    # XXX: create a macro for this time
    periods     -exec 15
#    periods     -exec $LOADEXEC
    periods     -timeout 300
    npending    1

    task.exec
        stdout NULL
        stderr $LOGSUBDIR/pstamp.dependent.load.log
        $run = pstamptool -pendingdependent
        if ($DB:n == 0)
            option DEFAULT
        else
            option $DB:$pstampDep_DB
            $run = $run -dbname $DB:$pstampDep_DB $PS_DBSERVER
            $pstampDep_DB ++
            if ($pstampDep_DB >= $DB:n) set pstampDep_DB = 0
        end
        add_poll_args run
        add_poll_labels run 
        command $run -limit $POLL_DEP
    end

    task.exit $EXIT_SUCCESS
        ipptool2book stdout pstampDependent -key dep_id -uniq -setword dbname $options:0 -setword pantaskState INIT

        book npages pstampDependent -var N
        if ($VERBOSE > 2)
            book listbook pstampDependent
        end

        # delete existing entries in the appropriate pantaskStates
        process_cleanup pstampDependent
    end

    task.exit   crash
        showcommand crash
    end

    task.exit   timeout
        showcommand timeout
    end

    task.exit   default
        showcommand failure
    end

end

task pstamp.dependent.run
    periods     -poll $RUNPOLL
    periods     -exec 0.5
    periods     -timeout 300

    task.exec
        # periods -exec $RUNEXEC

        book npages pstampDependent -var N
        if ($N == 0) break
        
        book getpage pstampDependent 0 -var pageName -key pantaskState INIT
        if ("$pageName" == "NULL") break

        book setword pstampDependent $pageName pantaskState RUN
        book getword pstampDependent $pageName dep_id     -var DEP_ID
        book getword pstampDependent $pageName state      -var STATE
        book getword pstampDependent $pageName stage      -var STAGE
        book getword pstampDependent $pageName stage_id   -var STAGE_ID
        book getword pstampDependent $pageName component  -var COMPONENT
        book getword pstampDependent $pageName imagedb    -var IMAGEDB
        book getword pstampDependent $pageName rlabel     -var RLABEL
        book getword pstampDependent $pageName label      -var LABEL
        book getword pstampDependent $pageName outdir     -var OUTDIR
        book getword pstampDependent $pageName need_magic -var NEED_MAGIC
        book getword pstampDependent $pageName fault_count -var FAULT_COUNT
        book getword pstampDependent $pageName dbname     -var DBNAME

        if ($VERBOSE > 1) 
            book listpage pstampDependent $pageName
        end

        host anyhost

        if ("$NEED_MAGIC" == "T")
            $NEED_MAGIC="--need_magic"
        else
            $NEED_MAGIC=""
        end

        $MYLOGFILE = $OUTDIR/checkdep.$DEP_ID.log
        stdout NULL
        stderr $MYLOGFILE

        $run = pstamp_checkdependent.pl --dep_id $DEP_ID --stage_id $STAGE_ID --stage $STAGE --component $COMPONENT --imagedb $IMAGEDB --rlabel $RLABEL --label $LABEL $NEED_MAGIC --fault_count $FAULT_COUNT --max_fault_count $PSTAMP_MAX_FAULT_COUNT --logfile $MYLOGFILE

        add_standard_args run

        options $pageName

        if ($VERBOSE > 1) 
            echo command $run
        end
#        periods -exec 0.05
        command $run
    end


    task.exit $EXIT_SUCCESS
        if ($VERBOSE > 1)
            echo pstamp.dependent.run task.exit $DEP_ID status: $JOB_STATUS
        end
        process_exit pstampDependent $options:0 $JOB_STATUS
    end
    task.exit default
        if ($VERBOSE > 1)
            echo pstamp.job.run task.exit $DEP_ID status: $JOB_STATUS
        end
#        showcommand failure
        process_exit pstampDependent $options:0 $JOB_STATUS
    end

    task.exit crash
        echo pstamp.job.run task.crash $DEP_ID status: $JOB_STATUS
        process_exit pstampDependent $options:0 $JOB_STATUS
        showcommand crash
        book setword pstampDependent $options:0 pantaskState CRASH
    end

    task.exit timeout
        echo pstamp.job.run task.timeout $DEP_ID status: $JOB_STATUS
        process_exit pstampDependent $options:0 $JOB_STATUS
        showcommand timeout
        book setword pstampDependent $options:0 pantaskState TIMEOUT
    end
end

task pstamp.cleanup.load
    host        local

    periods     -poll $LOADPOLL
    periods     -exec $LOADEXEC
    periods     -timeout 300
    npending    1

    task.exec
        stdout NULL
        stderr $LOGSUBDIR/pstamp.cleanup.load.log
        $run = pstamptool -pendingcleanup
        if ($DB:n == 0)
            option DEFAULT
        else 
            option $DB:$pstampCleanup_DB
            $run = $run $PS_DBSERVER -dbname $DB:$pstampCleanup_DB
            $pstampCleanup_DB ++
            if ($pstampCleanup_DB >= $DB:n) set pstampCleanup_DB = 0
        end
        add_poll_args run
        add_poll_labels run
        command $run
    end

    task.exit $EXIT_SUCCESS
        ipptool2book stdout pstampCleanup -key req_id -uniq -setword dbname $options:0 -setword pantaskState INIT
        if ($VERBOSE > 2)
            echo starting request
            book listbook pstampCleanup
        end

        process_cleanup pstampCleanup
    end

    task.exit   default
        showcommand failure
    end

    task.exit   crash
        showcommand crash
    end

    task.exit   timeout
        showcommand timeout
    end
end

task pstamp.cleanup.run
    periods     -poll $RUNPOLL
    periods     -exec $RUNEXEC
    periods     -timeout 300
    # since everything is on one file system keep npending low to avoid
    # overloading nfs

    npending    2

    task.exec
        periods -exec $RUNEXEC

        book npages pstampCleanup -var N
        if ($N == 0) break
        
        book getpage pstampCleanup 0 -var pageName -key pantaskState INIT
        if ("$pageName" == "NULL") break

        book setword pstampCleanup $pageName pantaskState RUN
        book getword pstampCleanup $pageName req_id -var REQ_ID
        book getword pstampCleanup $pageName dbname -var DBNAME
        book getword pstampCleanup $pageName name -var NAME
        book getword pstampCleanup $pageName outdir -var OUTDIR
        book getword pstampCleanup $pageName uri -var URI
        book getword pstampCleanup $pageName reqType -var REQTYPE
        book getword pstampCleanup $pageName outProduct -var PRODUCT

        # XXX: have the script set this up this
        #$MYLOGFILE=/data/ippc17.0/pstamp/work/logs/cleanup.$REQ_ID
        ## change to put log in pantasks_log directory, leaving setup similar to original in case need to go back
	## keeping stdout to NULL like other tasks and avoid trying to write to same file
	$MYLOGFILE=$LOGSUBDIR/pstamp.cleanup.run.log

        stdout NULL 
        stderr $MYLOGFILE

        host anyhost

        $run = pstamp_cleanup.pl --req_id $REQ_ID --uri $URI --product $PRODUCT --name $NAME --outdir $OUTDIR --reqType $REQTYPE

        add_standard_args run
        options $pageName

        if ($VERBOSE > 1) 
            echo command $run
        end
        periods -exec 0.05
        command $run
    end


    task.exit $EXIT_SUCCESS
        process_exit pstampCleanup $options:0 $JOB_STATUS
    end

    task.exit default
        showcommand failure
        process_exit pstampCleanup $options:0 $JOB_STATUS
    end

    task.exit crash
        showcommand crash
        book setword pstampCleanup $options:0 pantaskState CRASH
    end

    task.exit timeout
        showcommand timeout
        book setword pstampCleanup $options:0 pantaskState TIMEOUT
    end
end

task pstamp.dependent.revert
    host        local

    periods     -poll $LOADPOLL
    periods     -exec 900
    periods     -timeout 300
    npending    1

    task.exec
        stdout NULL
        stderr $LOGSUBDIR/pstamp.dependent.revert.log
        $run = pstamptool -revertdependent
        if ($DB:n == 0)
            option DEFAULT
        else 
            $run = $run $PS_DBSERVER -dbname $DB:$pstampRevDep_DB
            $pstampRevDep_DB ++
            if ($pstampRevDep_DB >= $DB:n) set pstampRevDep_DB = 0
        end
        add_poll_args run
        add_poll_labels run
        command $run
    end

    task.exit $EXIT_SUCCESS
        # nothing to do
    end

    task.exit   default
        showcommand failure
    end

    task.exit   crash
        showcommand crash
    end

    task.exit   timeout
        showcommand timeout
    end
end

task pstamp.stopfaulted
    host        local

    periods     -poll $LOADPOLL
    periods     -exec 30
    periods     -timeout 300
    npending    1

    task.exec
        stdout $LOGSUBDIR/pstamp.stopfaulted.log
        stderr $LOGSUBDIR/pstamp.stopfaulted.log
        $run = pstampstopfaulted -fault_count $PSTAMP_MAX_FAULT_COUNT
        if ($DB:n == 0)
            option DEFAULT
        else 
            $run = $run $PS_DBSERVER -dbname $DB:$pstampStopFaulted_DB
            $pstampStopFaulted_DB ++
            if ($pstampStopFaulted_DB >= $DB:n) set pstampStopFaulted_DB = 0
        end
        add_poll_args run
        add_poll_labels run
        command $run
    end

    task.exit $EXIT_SUCCESS
        # nothing to do
    end

    task.exit   default
        showcommand failure
    end

    task.exit   crash
        showcommand crash
    end

    task.exit   timeout
        showcommand timeout
    end
end

task pstamp.save.status
    host        local

    periods     -poll $LOADPOLL
    periods     -exec 60
    periods     -timeout 120
    npending    1

    task.exec
        stdout NULL
        stderr $LOGSUBDIR/pstamp.save.status.log

        $run = pstamp_save_server_status.pl --update-link
        command $run
    end

    task.exit $EXIT_SUCCESS
        # echo nothing to do
    end

    task.exit   default
        showcommand failure
    end

    task.exit   crash
        showcommand crash
    end

    task.exit   timeout
        showcommand timeout
    end
end

task pstamp.queue.cleanup
    host        local

    periods     -poll $RUNPOLL
    periods     -exec 600
    periods     -timeout 120
    npending    1

    task.exec
        stdout NULL
        stderr $LOGSUBDIR/pstamp.queue.cleanup.log

        $run = pstamp_queue_cleanup.pl
        command $run
    end

    task.exit $EXIT_SUCCESS
        # echo nothing to do
    end

    task.exit   default
        showcommand failure
    end

    task.exit   crash
        showcommand crash
    end

    task.exit   timeout
        showcommand timeout
    end
end

task pstamp.queue.update.cleanup
    host        local
    trange      22:00:00 23:59:59 -nmax 2
    periods     -poll $RUNPOLL
    periods     -exec 3600
    periods     -timeout 120
    npending    1

    task.exec
        stdout NULL
        stderr $LOGSUBDIR/pstamp.queue.update.cleanup.log

        $DBNAME = $DB:$pstampQCleanup_DB
        $pstampQCleanup_DB++
        if ($pstampQCleanup_DB >= $DB:n) set pstampQCleanup_DB = 0

        $run = pstamp_queue_update_cleanup.pl  -$PS_DBSERVER --imagedb gpc1 --label ps_ud%
        add_standard_args run
        echo $run
        command $run
    end

    task.exit $EXIT_SUCCESS
        # echo nothing to do
    end

    task.exit   default
        showcommand failure
    end

    task.exit   crash
        showcommand crash
    end

    task.exit   timeout
        showcommand timeout
    end
end
