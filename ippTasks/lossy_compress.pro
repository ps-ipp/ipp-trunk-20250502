## lossy_compress.pro : tasks to lossy compress old raw images that have negligible science value : -*- sh -*-
## use the books compPendingCompress, compPendingClean and compPendingExp

# test for required global variables
check.globals

book init compPendingCompress
book init compPendingClean
book init compPendingExp

macro lossycomp.reset
    book init compPendingCompress
    book init compPendingClean
    book init compPendingExp
end

macro lossycomp.status
    book list
    book listbook compPendingCompress
    book listbook compPendingClean
    book listbook compPendingExp
end

macro lossycomp.on
    task lossycomp.compress.load
	active true
    end
    task lossycomp.compress.run
	active true
    end
    task lossycomp.clean.load
	active true
    end
    task lossycomp.clean.run
	active true
    end
    task lossycomp.exp.finish
	active true
    end
end

macro lossycomp.off
    task lossycomp.compress.load
	active false
    end
    task lossycomp.compress.run
	active false
    end
    task lossycomp.clean.load
	active false
    end
    task lossycomp.clean.run
	active false
    end
    task lossycomp.exp.finish
	active false
    end
end

# these variables will cycle through the known database names
$compPendingCompress_DB = 0
$compPendingClean_DB = 0
$comp_finish_DB = 0

# select images ready to be compressed
task           lossycomp.compress.load
    host       local

    periods    -poll $LOADPOLL
    periods    -exec $LOADEXEC
    periods    -timeout 30
    npending   1

    stdout     NULL
    stderr     $LOGDIR/lossycomp.compress.load.log

    #select entries from the current DB, cycle to the next DB if possible
    task.exec
	$run = regtool -pendingcompressimfile -compress
	if ($DB:n == 0)
	    option DEFAULT
	else
	    # save the DB name for the exit tasks
	    option $DB:$compPendingCompress_DB
	    $run = $run -dbname $DB:$compPendingCompress_DB
	    $compPendingCompress_DB ++
	    if ($compPendingCompress_DB >= $DB:n) set compPendingCompress_DB = 0
	end
	add_poll_args run
	command $run
    end

    # success
    task.exit $EXIT_SUCCESS
	# convert 'stdout' to book format
	ipptool2book stdout compPendingCompress -key exp_id:class_id -uniq -setword dbname $options:0 -setword pantaskState INIT
	book shuffle compPendingCompress
	if ($VERBOSE > 2) 
	    book listbook compPendingCompress
	end
	
	# delete existing entries in the appropriate pantaskStates
	process_cleanup compPendingCompress
    end

    task.exit  default
	showcommand failure
    end
    task.exit  crash
	showcommand crash
    end
    task.exit  timeout
	showcommand timeout
    end
end

# run the lossy_compress_imfile.pl script on the pending images
task           lossycomp.compress.run
    periods    -poll $RUNPOLL
    periods    -exec $RUNEXEC
    periods    -timeout 30

    task.exec
        periods -exec $RUNEXEC

	book npages compPendingCompress -var N
	if ($N == 0) break
	if ($NETWORK == 0) break

	# look for new images
	book getpage compPendingCompress 0 -var pageName -key pantaskState INIT
	if ("$pageName" == "NULL") break

	book setword compPendingCompress $pageName pantaskState RUN
	book getword compPendingCompress $pageName exp_id        -var EXP_ID
	book getword compPendingCompress $pageName exp_name      -var EXP_NAME
	book getword compPendingCompress $pageName exp_tag       -var EXP_TAG
	book getword compPendingCompress $pageName tmp_class_id  -var TMP_CLASS_ID
	book getword compPendingCompress $pageName class_id      -var CLASS_ID
	book getword compPendingCompress $pageName camera        -var CAMERA
	book getword compPendingCompress $pageName uri           -var URI
	book getword compPendingCompress $pageName bytes         -var BYTES
	book getword compPendingCompress $pageName md5sum        -var MD5SUM
	book getword compPendingCompress $pageName workdir       -var WORKDIR
	book getword compPendingCompress $pageName data_state    -var STATE
	book getword compPendingCompress $pageName dbname        -var DBNAME

	# specify choice of remote host
	set.host.for.camera $CAMERA $CLASS_ID

	# set logfile name
	if ("$WORKDIR" == "NULL") 
	    sprintf logfile "compress_log/%s.%d.lossycomp.%s.log" $EXP_NAME $EXP_ID $TMP_CLASS_ID
	else 
	    sprintf logfile "%s/%s/%s.lossycomp.%s.log" $WORKDIR $EXP_TAG $EXP_TAG $TMP_CLASS_ID
	end

	stderr $LOGDIR/lossycomp.compress.run.log
	stdout $LOGDIR/lossycomp.compress.run.log

	$run = lossy_compress_imfile.pl --exp_id $EXP_ID --class_id $CLASS_ID --exp_name $EXP_NAME --uri $URI --camera $CAMERA --state $STATE  --logfile $logfile --bytes $BYTES --md5sum $MD5SUM
	add_standard_args run

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
	process_exit compPendingCompress $options:0 $JOB_STATUS
    end

    task.exit crash
	showcommand crash
	echo "hostname: $JOB_HOSTNAME"
	book setword compPendingCompress $options:0 pantaskState CRASH
    end
    task.exit timeout
	showcommand timeout
	book setword compPendingCompress $options:0 pantaskState TIMEOUT
    end
end
# select images ready to be compressed
task           lossycomp.clean.load
    host       local

    periods    -poll $LOADPOLL
    periods    -exec $LOADEXEC
    periods    -timeout 30
    npending   1

    stdout     NULL
    stderr     $LOGDIR/lossycomp.clean.load.log

    #select entries from the current DB, cycle to the next DB if possible
    task.exec
	$run = regtool -pendingcompressimfile -clean
	if ($DB:n == 0)
	    option DEFAULT
	else
	    # save the DB name for the exit tasks
	    option $DB:$compPendingClean_DB
	    $run = $run -dbname $DB:$compPendingClean_DB
	    $compPendingClean_DB ++
	    if ($compPendingClean_DB >= $DB:n) set compPendingClean_DB = 0
	end
	add_poll_args run
	command $run
    end

    # success
    task.exit $EXIT_SUCCESS
	# convert 'stdout' to book format
	ipptool2book stdout compPendingClean -key exp_id:class_id -uniq -setword dbname $options:0 -setword pantaskState INIT
	book shuffle compPendingClean
	if ($VERBOSE > 2) 
	    book listbook compPendingClean
	end
	
	# delete existing entries in the appropriate pantaskStates
	process_cleanup compPendingClean
    end

    task.exit  default
	showcommand failure
    end
    task.exit  crash
	showcommand crash
    end
    task.exit  timeout
	showcommand timeout
    end
end

# run the lossy_compress_imfile.pl script on the pending images
task           lossycomp.clean.run
    periods    -poll $RUNPOLL
    periods    -exec $RUNEXEC
    periods    -timeout 30

    task.exec
        periods -exec $RUNEXEC

	book npages compPendingClean -var N
	if ($N == 0) break
	if ($NETWORK == 0) break

	# look for new images
	book getpage compPendingClean 0 -var pageName -key pantaskState INIT
	if ("$pageName" == "NULL") break

	book setword compPendingClean $pageName pantaskState RUN
	book getword compPendingClean $pageName exp_id        -var EXP_ID
	book getword compPendingClean $pageName exp_name      -var EXP_NAME
	book getword compPendingClean $pageName exp_tag       -var EXP_TAG
	book getword compPendingClean $pageName tmp_class_id  -var TMP_CLASS_ID
	book getword compPendingClean $pageName class_id      -var CLASS_ID
	book getword compPendingClean $pageName camera        -var CAMERA
	book getword compPendingClean $pageName uri           -var URI
	book getword compPendingClean $pageName bytes         -var BYTES
	book getword compPendingClean $pageName md5sum        -var MD5SUM
	book getword compPendingClean $pageName workdir       -var WORKDIR
	book getword compPendingClean $pageName data_state    -var STATE
	book getword compPendingClean $pageName dbname        -var DBNAME

	# specify choice of remote host
	set.host.for.camera $CAMERA $CLASS_ID

	# set logfile name
	if ("$WORKDIR" == "NULL") 
	    sprintf logfile "compress_log/%s.%d.lossycomp.%s.log" $EXP_NAME $EXP_ID $TMP_CLASS_ID
	else 
	    sprintf logfile "%s/%s/%s.lossycomp.%s.log" $WORKDIR $EXP_TAG $EXP_TAG $TMP_CLASS_ID
	end

	stderr $LOGDIR/lossycomp.clean.run.log
	stdout $LOGDIR/lossycomp.clean.run.log

	$run = lossy_compress_imfile.pl --exp_id $EXP_ID --class_id $CLASS_ID --exp_name $EXP_NAME --uri $URI --camera $CAMERA --state $STATE  --logfile $logfile --bytes $BYTES --md5sum $MD5SUM
	add_standard_args run

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
	process_exit compPendingClean $options:0 $JOB_STATUS
    end

    task.exit crash
	showcommand crash
	echo "hostname: $JOB_HOSTNAME"
	book setword compPendingClean $options:0 pantaskState CRASH
    end
    task.exit timeout
	showcommand timeout
	book setword compPendingClean $options:0 pantaskState TIMEOUT
    end
end

# select exposures ready for lossy_compress_exp.pl
task         lossycomp.exp.finish
    host     local
    periods  -poll $LOADPOLL
    periods  -exec 30
    periods  -timeout 30
    npending 1

    stdout   NULL
    stderr   $LOGDIR/lossycomp.exp.load.log

    task.exec
	$run = regtool -finishcompressexp -limit 10
	if ($DB:n == 0) 
	    option DEFAULT
	else
	    # save the DB name for the exit tasks
	    option $DB:$comp_finish_DB
	    $run = $run -dbname $DB:$comp_finish_DB
	    $comp_finish_DB ++
	    if ($comp_finish_DB >= $DB:n) set comp_finish_DB = 0
	end
	add_poll_args run
	command $run
    end

    # success
    task.exit $EXIT_SUCCESS
    end

    task.exit  default
	showcommand failure
    end
    task.exit  crash
	showcommand crash
    end
    task.exit  timeout
	showcommand timeout
    end
end
