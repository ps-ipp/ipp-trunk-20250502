## addstar.pro : globals and support macros : -*- sh -*-
## this file contains the tasks for running the addstar analysis stage
## these tasks use the book addPendingExp

# test for required global variables
check.globals

if ($?ADDSTAGES:n == 0)       set ADDSTAGES:n = 0
if ($?MULTIADDSTAGES:n == 0)  set MULTIADDSTAGES:n = 0

book init addPendingExp
book init addPendingMultiExp
    
macro addstar.status
  book listbook addPendingExp
end

macro addstar.multi.status
    book listbook addPendingMultiExp
end    
    
macro addstar.reset
  book init addPendingExp
end

macro addstar.multi.reset
  book init addPendingMultiExp
end	
    
macro addstar.on
  task addstar.exp.load
    active true
  end
  task addstar.exp.run
    active true
    end
end
macro addstar.multi.on
  task addstar.multiexp.load
    active true
  end
  task addstar.multiexp.run
    active true
  end    
end

macro addstar.off
  task addstar.exp.load
    active false
  end
  task addstar.exp.run
    active false
  end
end
macro addstar.multi.off
  task addstar.multiexp.load
    active false
  end
  task addstar.multiexp.run
    active false
  end
end

macro addstar.revert.off
  task addstar.revert.cam
    active false
  end
  task addstar.revert.stack
    active false
  end
  task addstar.revert.staticsky
    active false
  end
  task addstar.revert.skycal
    active false
  end
  task addstar.revert.diff
    active false
  end
  task addstar.revert.fullforce
    active false
  end
  task addstar.revert.fullforce_summary
    active false
  end
end

macro addstar.revert.on
  task addstar.revert.cam
    active true
  end
  task addstar.revert.stack
    active true
  end
  task addstar.revert.staticsky
    active true
  end
  task addstar.revert.skycal
    active true
  end
  task addstar.revert.diff
    active true
  end
  task addstar.revert.fullforce
    active true
  end
  task addstar.revert.fullforce_summary
    active true
  end
end


#addstar stages

macro add.addstages
  if ($0 != 2)
    echo "USAGE: add.addstages (addstages)"
    break
  end
  if ($?ADDSTAGES:n == 0)
    list ADDSTAGES -add $1
    return
  end

  local found
  $found = 0
  for i 0 $ADDSTAGES:n
    if ($ADDSTAGES:$i == $1) 
      $found = 1
      echo "$ADDSTAGES:$i set"
      last
    end
  end
  
  if ($found == 0)
    list ADDSTAGES -add $1
  end
end


    
macro del.addstages
  if ($0 != 2)
    echo "USAGE: del.addstages (addstages)"
    break
  end
  if ($?ADDSTAGES:n == 0)
    return
  end

  list ADDSTAGES -del $1
end

macro show.addstages
  if ($0 != 1)
    echo "USAGE: show.addstages"
    break
  end
  if ($?ADDSTAGES:n == 0)
    echo "no addstar stages defined"
  end
  if ($ADDSTAGES:n == 0)
    echo "no addstar stages defined"
  end

  local i
  for i 0 $ADDSTAGES:n
    echo $ADDSTAGES:$i
  end
end

macro add.multiaddstages
  if ($0 != 2)
    echo "USAGE: add.multiaddstages (multiaddstages)"
    break
  end
  if ($?MULTIADDSTAGES:n == 0)
    list MULTIADDSTAGES -add $1
    return
  end

  local found
  $found = 0
  for i 0 $MULTIADDSTAGES:n
    if ($MULTIADDSTAGES:$i == $1) 
      $found = 1
      echo "$MULTIADDSTAGES:$i set"
      last
    end
  end
  
  if ($found == 0)
    list MULTIADDSTAGES -add $1
  end
end


    
macro del.multiaddstages
  if ($0 != 2)
    echo "USAGE: del.multiaddstages (multiaddstages)"
    break
  end
  if ($?MULTIADDSTAGES:n == 0)
    return
  end

  list MULTIADDSTAGES -del $1
end

macro show.multiaddstages
  if ($0 != 1)
    echo "USAGE: show.multiaddstages"
    break
  end
  if ($?MULTIADDSTAGES:n == 0)
    echo "no addstar stages defined"
  end
  if ($MULTIADDSTAGES:n == 0)
    echo "no addstar stages defined"
  end

  local i
  for i 0 $MULTIADDSTAGES:n
    echo $MULTIADDSTAGES:$i
  end
end

macro stuff
   echo LOADEXEC_ADD: $LOADEXEC_ADD
   echo LOADPOLL_ADD: $LOADPOLL_ADD
   echo RUNEXEC_ADD:  $RUNEXEC_ADD
   echo RUNPOLL_ADD:  $RUNPOLL_ADD
end

# this variable will cycle through the known database names
$addstar_DB = 0 
$addstar_stages_DB = 0

$addstar_multi_DB = 0 
$addstar_multi_stages_DB = 0

# this may not work for more databases (addstar) will do that later)
$addstar_revert_DB_C = 0
$addstar_revert_DB_S = 0
$addstar_revert_DB_SS = 0
$addstar_revert_DB_SSM = 0
$addstar_revert_DB_SC = 0 
$addstar_revert_DB_FF = 0 
$addstar_revert_DB_FFS = 0
$addstar_revert_DB_DF = 0 

# loading time every N seconds
$LOADEXEC_ADD = 10
$LOADPOLL_ADD = 10
$RUNEXEC_ADD = 5
$RUNPOLL_ADD = 10

if ($?addstar_multiadd_limit == 0) set addstar_multiadd_limit = 0

macro set.multiadd.limit
  if ($0 != 2)
    echo "USAGE: set.multiadd.limit (Nentry)"
    break
  end

  $addstar_multiadd_limit = $1
end

macro get.multiadd.limit
  echo "addstar multi limit : $addstar_multiadd_limit"
end

# select images ready for addstar analysis
# new entries are added to addPendingExp
# skip already-present entries
task	       addstar.exp.load
  host         local

  periods      -poll $LOADPOLL_ADD
  periods      -exec $LOADEXEC_ADD
  periods      -timeout 300
  npending     1

  date -var rundate

  stdout NULL
  stderr $LOGDIR/addstar.exp.log

  task.exec
    #if no stages defined we cant run
    if ($ADDSTAGES:n == 0) echo "what" 
    if ($ADDSTAGES:n == 0) break
    $run = addtool -pendingexp -addrand

    book npages addPendingExp  -var addPendingExp_Npage
    if ($addPendingExp_Npage > 10000)
      $POLL_LIMIT = 1
      echo $addPendingExp_Npage $rundate
    end
    
    #option $ADDSTAGES:$addstar_stages_DB
    $run = $run -stage $ADDSTAGES:$addstar_stages_DB  
    option $DB:$addstar_DB
    $run = $run -dbname $DB:$addstar_DB
    $addstar_stages_DB ++
      if ($addstar_stages_DB >= $ADDSTAGES:n)
         set addstar_stages_DB = 0
         $addstar_DB ++
         if ($addstar_DB >= $DB:n) set addstar_DB = 0
         #go to next stage. if run out of stages, go to 0 stage and
         #cycle through dbname, if run out of dbnames, go to 0 dbname
      end
    add_poll_args run
    add_poll_labels run
    echo $run $rundate
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout addPendingExp -key add_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook addPendingExp
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup addPendingExp
  end

  # default exit status
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


# select images ready for addstar analysis
# new entries are added to addPendingExp
# skip already-present entries
task	       addstar.multiexp.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC_ADD
  periods      -timeout 300
  npending     1

  stdout NULL
  stderr $LOGDIR/addstar.multiexp.log

  task.exec
    #if no stages defined we cant run
    if ($MULTIADDSTAGES:n == 0) echo "no stages for addstar.multi, use add.addstagesmulti" 
 #   if ($MULTIADDSTAGES:n == 0) break
    $run = addtool -pendingexp
    
    #option $ADDSTAGES:$addstar_stages_DB
    $run = $run -stage $MULTIADDSTAGES:$addstar_multi_stages_DB -multiadd  
    option $DB:$addstar_multi_DB
    $run = $run -dbname $DB:$addstar_multi_DB
    $addstar_multi_stages_DB ++
      if ($addstar_multi_stages_DB >= $MULTIADDSTAGES:n)
         set addstar_multi_stages_DB = 0
         $addstar_multi_DB ++
         if ($addstar_multi_DB >= $DB:n) set addstar_multi_DB = 0
         #go to next stage. if run out of stages, go to 0 stage and
         #cycle through dbname, if run out of dbnames, go to 0 dbname
      end
    add_poll_args run
    add_poll_labels run
    echo $run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout addPendingMultiExp -key add_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook addPendingMultiExp
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup addPendingMultiExp
  end

  # default exit status
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
    
# run the addstar script on pending exposures
task	       addstar.exp.run
  periods      -poll $RUNPOLL_ADD
  periods      -exec $RUNEXEC_ADD
  periods      -timeout 1200

  ## we want only a single outstanding addstar job.  
#  host         local
  npending     1000

  task.exec
    # if we are unable to run the 'exec', use a long retry time
    periods -exec $RUNEXEC_ADD

    book npages addPendingExp -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in addPendingExp (pantaskState == INIT)
    book getpage addPendingExp 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword addPendingExp $pageName pantaskState RUN
    book getword addPendingExp $pageName camera -var CAMERA
    book getword addPendingExp $pageName exp_tag -var EXP_TAG
    book getword addPendingExp $pageName add_id -var ADD_ID
    book getword addPendingExp $pageName stage_extra1 -var STAGE_EXTRA1
    book getword addPendingExp $pageName stageroot -var STAGEROOT
    book getword addPendingExp $pageName stage -var STAGE
    book getword addPendingExp $pageName stage_id -var STAGE_ID
    book getword addPendingExp $pageName workdir -var WORKDIR_TEMPLATE
    book getword addPendingExp $pageName reduction -var REDUCTION
    book getword addPendingExp $pageName dvodb  -var DVODB
    book getword addPendingExp $pageName minidvodb  -var MINIDVODB
    book getword addPendingExp $pageName minidvodb_name  -var MINIDVODB_NAME
    book getword addPendingExp $pageName minidvodb_group  -var MINIDVODB_GROUP
    book getword addPendingExp $pageName image_only -var IMAGE_ONLY
    book getword addPendingExp $pageName dbname -var DBNAME
    book getword addPendingExp $pageName addrun_host  -var ADDRUN_HOST

    host -required $ADDRUN_HOST

    # specify choice of remote host based on camera and chip (class_id)
    # set.host.for.camera $CAMERA FPA

    # set the WORKDIR variable
    set.workdir.by.camera $CAMERA FPA $WORKDIR_TEMPLATE $default_host WORKDIR

    # notes on how this works:
    # -- raw workdir examples:
    # file://data/@HOST@.0/gpc1/20080130
    # neb:///@HOST@-vol0/gpc1/20080130 (need to supply volname?, or are we re-defining this each time?)
    # -- out workdir examples:
    # file://data/ipp004.0/gpc1/20080130
    # neb:///ipp004-vol0/gpc1/20080130

    ## generate outroot specific to this exposure (& chip)

    if ("$STAGE" == "cam")
	sprintf outroot "%s/%s/%s.add.%s" $WORKDIR $EXP_TAG $EXP_TAG $ADD_ID
    end
    if ("$STAGE" == "staticsky")
	sprintf outroot "%s.add.%s" $STAGEROOT $ADD_ID
    end
    if ("$STAGE" == "stack")
	sprintf outroot "%s.add.%s" $STAGEROOT $ADD_ID
    end
    if ("$STAGE" == "skycal")
        sprintf outroot "%s.add.%s" $STAGEROOT $ADD_ID
    end
    if ("$STAGE" == "diff")
        sprintf outroot "%s.add.%s" $STAGEROOT $ADD_ID
    end
    if ("$STAGE" == "fullforce")
        sprintf outroot "%s.add.%s" $STAGEROOT $ADD_ID
    end
    if ("$STAGE" == "fullforce_summary")
	sprintf outroot "%s.add.%s" $STAGEROOT $ADD_ID
    end

  

    stdout $LOGDIR/addstar.exp.log
    stderr $LOGDIR/addstar.exp.log

    $run = addstar_run.pl --add_id $ADD_ID --camera $CAMERA --dvodb $DVODB --stage $STAGE --stageroot $STAGEROOT --outroot $outroot --redirect-output --addrun_host $ADDRUN_HOST
    if ("$REDUCTION" != "NULL")
      $run = $run --reduction $REDUCTION
    end
    if ("$STAGE" == "staticsky")
      $run = $run --stage_extra1 $STAGE_EXTRA1  --stage_id $STAGE_ID
    end
    if ("$STAGE" == "cam") 
      $run = $run --stage_id $STAGE_ID
    end
    if ("$STAGE" == "skycal")
      $run = $run --stage_id $STAGE_ID
    end
    if ("$STAGE" == "diff")
      #$run = $run --stage_id $STAGE_ID --stage_extra1 $STAGE_EXTRA1
	$run = $run --use_diff_inv --stage_id $STAGE_ID --stage_extra1 $STAGE_EXTRA1 
    end
    if ("$STAGE" == "fullforce")
      $run = $run --stage_id $STAGE_ID --stage_extra1 $STAGE_EXTRA1
    end
    if ("$STAGE" == "fullforce_summary")
    # This shouldnt need a stage_extra1, as there are no subcomponents to the ff summary.
      $run = $run --stage_id $STAGE_ID
    end


    if ("$IMAGE_ONLY" == "T")
      $run = $run --image-only
    end
    if ("$MINIDVODB" == "T")
    $run = $run --minidvodb
    $run = $run --minidvodb_group $MINIDVODB_GROUP
	if (("$MINIDVODB_NAME" != "NULL") && ("$MINIDVODB_NAME" != "(null)"))
           $run = $run --minidvodb_name $MINIDVODB_NAME 
    #have addstar_run.pl grab the 'active' name if it is NULL
	end
    end 
    
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    periods -exec 0.1
    command $run
  end

  # success
  task.exit default
    process_exit addPendingExp $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword addPendingExp $options:0 pantaskState CRASH
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
    book setword addPendingExp $options:0 pantaskState TIMEOUT
  end
end


    
# run the addstar script on pending exposures
task	       addstar.multiexp.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 3600
  # addstar MD fields with 250 exp takes about 2400 sec to load

  ## we want only a single outstanding addstar job.  
  host         local
  npending     1

  task.exec
    book npages addPendingMultiExp -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in addPendingExp (pantaskState == INIT)
    book getpage addPendingMultiExp 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword addPendingMultiExp $pageName pantaskState RUN
    book getword addPendingMultiExp $pageName camera -var CAMERA
    book getword addPendingMultiExp $pageName exp_tag -var EXP_TAG
    book getword addPendingMultiExp $pageName add_id -var ADD_ID
    book getword addPendingMultiExp $pageName stage_extra1 -var STAGE_EXTRA1
    book getword addPendingMultiExp $pageName stageroot -var STAGEROOT
    book getword addPendingMultiExp $pageName stage -var STAGE
    book getword addPendingMultiExp $pageName stage_id -var STAGE_ID
    book getword addPendingMultiExp $pageName workdir -var WORKDIR_TEMPLATE
    book getword addPendingMultiExp $pageName reduction -var REDUCTION
    book getword addPendingMultiExp $pageName dvodb  -var DVODB
    book getword addPendingMultiExp $pageName minidvodb  -var MINIDVODB
    book getword addPendingMultiExp $pageName minidvodb_name  -var MINIDVODB_NAME
    book getword addPendingMultiExp $pageName minidvodb_group  -var MINIDVODB_GROUP
    book getword addPendingMultiExp $pageName image_only -var IMAGE_ONLY
    book getword addPendingMultiExp $pageName dbname -var DBNAME
    book getword addPendingMultiExp $pageName label -var LABEL

    # specify choice of remote host based on camera and chip (class_id)
    # set.host.for.camera $CAMERA FPA

    # set the WORKDIR variable
    set.workdir.by.camera $CAMERA FPA $WORKDIR_TEMPLATE $default_host WORKDIR

    # notes on how this works:
    # -- raw workdir examples:
    # file://data/@HOST@.0/gpc1/20080130
    # neb:///@HOST@-vol0/gpc1/20080130 (need to supply volname?, or are we re-defining this each time?)
    # -- out workdir examples:
    # file://data/ipp004.0/gpc1/20080130
    # neb:///ipp004-vol0/gpc1/20080130

    ## generate outroot specific to this exposure (& chip)

    if ("$STAGE" == "cam")
	sprintf outroot "%s/%s/%s.add.%s" $WORKDIR $EXP_TAG $EXP_TAG $ADD_ID
    end
    if ("$STAGE" == "staticsky")
	sprintf outroot "%s.add.%s" $STAGEROOT $ADD_ID
    end
    if ("$STAGE" == "stack")
	sprintf outroot "%s.add.%s" $STAGEROOT $ADD_ID
    end
    if ("$STAGE" == "skycal")
        sprintf outroot "%s.add.%s" $STAGEROOT $ADD_ID
    end
    if ("$STAGE" == "diff")
        sprintf outroot "%s.add.%s" $STAGEROOT $ADD_ID
    end
    if ("$STAGE" == "fullforce")
        sprintf outroot "%s.add.%s" $STAGEROOT $ADD_ID
    end
    if ("$STAGE" == "fullforce_summary")
	sprintf outroot "%s.add.%s" $STAGEROOT $ADD_ID
    end

  

    stdout $LOGDIR/addstar.multiexp.log
    stderr $LOGDIR/addstar.multiexp.log

    $run = addstar_multi_run.pl --stage_id $STAGE_ID --label $LABEL --camera $CAMERA --dvodb $DVODB --stage $STAGE --stageroot $STAGEROOT --outroot $outroot --redirect-output
    if ("$REDUCTION" != "NULL")
      $run = $run --reduction $REDUCTION
    end
    if ("$STAGE" == "staticsky")
      $run = $run --stage_extra1 $STAGE_EXTRA1  --stage_id $STAGE_ID
    end
    if ("$STAGE" == "cam") 
      $run = $run --stage_id $STAGE_ID
    end
   if ("$STAGE" == "skycal")
      $run = $run --stage_id $STAGE_ID
    end
   #if ("$STAGE" == "diff")
   #   $run = $run --stage_id $STAGE_ID --stage_extra1 $STAGE_EXTRA1
   # end
   #if ("$STAGE" == "fullforce")
   #   $run = $run --stage_id $STAGE_ID --stage_extra1 $STAGE_EXTRA1
   # end

    if ($addstar_multiadd_limit > 0) 
      $run = $run --limit $addstar_multiadd_limit
    end

    if ("$IMAGE_ONLY" == "T")
      $run = $run --image-only
    end

    if ("$MINIDVODB" == "T")
    $run = $run --minidvodb
    $run = $run --minidvodb_group $MINIDVODB_GROUP
	if (("$MINIDVODB_NAME" != "NULL") && ("$MINIDVODB_NAME" != "(null)"))
           $run = $run --minidvodb_name $MINIDVODB_NAME 
    #have addstar_run.pl grab the 'active' name if it is NULL
	end
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

  # success
  task.exit default
    process_exit addPendingMultiExp $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword addPendingMultiExp $options:0 pantaskState CRASH
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
    book setword addPendingMultiExp $options:0 pantaskState TIMEOUT
  end
end
    
task addstar.revert.cam
  host         local

  periods      -poll 5.0
  periods      -exec 60.0
  periods      -timeout 1200
  npending     1
  active        false

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = addtool -revertprocessedexp -stage cam
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$addstar_revert_DB_C
      $run = $run -dbname $DB:$addstar_revert_DB_C
      $addstar_revert_DB_C ++
      if ($addstar_revert_DB_C >= $DB:n) set addstar_revert_DB_C = 0
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
task addstar.revert.stack
  host         local

  periods      -poll 5.0
  periods      -exec 60.0
  periods      -timeout 1200
  npending     1
  active        false

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = addtool -revertprocessedexp -stage stack
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$addstar_revert_DB_S
      $run = $run -dbname $DB:$addstar_revert_DB_S
      $addstar_revert_DB_S ++
      if ($addstar_revert_DB_S >= $DB:n) set addstar_revert_DB_S = 0
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

task addstar.revert.staticsky
  host         local

  periods      -poll 5.0
  periods      -exec 60.0
  periods      -timeout 1200
  npending     1
  active        false

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = addtool -revertprocessedexp -stage staticsky
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$addstar_revert_DB_SSM
      $run = $run -dbname $DB:$addstar_revert_DB_SSM
      $addstar_revert_DB_SSM ++
      if ($addstar_revert_DB_SSM >= $DB:n) set addstar_revert_DB_SSM = 0
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

task addstar.revert.skycal
  host         local

  periods      -poll 5.0
  periods      -exec 60.0
  periods      -timeout 1200
  npending     1
  active        false

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = addtool -revertprocessedexp -stage skycal
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks                                                                   
      option $DB:$addstar_revert_DB_SC
      $run = $run -dbname $DB:$addstar_revert_DB_SC
      $addstar_revert_DB_SC ++
      if ($addstar_revert_DB_SC >= $DB:n) set addstar_revert_DB_SC = 0
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


task addstar.revert.diff
  host         local

  periods      -poll 5.0
  periods      -exec 60.0
  periods      -timeout 1200
  npending     1
  active        false

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = addtool -revertprocessedexp -stage diff
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks                                                                   
      option $DB:$addstar_revert_DB_DF
      $run = $run -dbname $DB:$addstar_revert_DB_DF
      $addstar_revert_DB_DF ++
      if ($addstar_revert_DB_DF >= $DB:n) set addstar_revert_DB_DF = 0
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


task addstar.revert.fullforce
  host         local

  periods      -poll 5.0
  periods      -exec 60.0
  periods      -timeout 1200
  npending     1
  active        false

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = addtool -revertprocessedexp -stage fullforce
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks                                                                   
      option $DB:$addstar_revert_DB_FF
      $run = $run -dbname $DB:$addstar_revert_DB_FF
      $addstar_revert_DB_FF ++
      if ($addstar_revert_DB_FF >= $DB:n) set addstar_revert_DB_FF = 0
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

task addstar.revert.fullforce_summary
  host         local

  periods      -poll 5.0
  periods      -exec 60.0
  periods      -timeout 1200
  npending     1
  active        false

  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = addtool -revertprocessedexp -stage fullforce_summary
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks                                                                   
      option $DB:$addstar_revert_DB_FFS
      $run = $run -dbname $DB:$addstar_revert_DB_FFS
      $addstar_revert_DB_FFS ++
      if ($addstar_revert_DB_FFS >= $DB:n) set addstar_revert_DB_FFS = 0
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
