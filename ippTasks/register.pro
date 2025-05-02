## register.pro : tasks for image registration (insert into database) : -*- sh -*-
## this file contains the tasks for running the registration stage
## these tasks use the books regPendingImfile and regPendingExp

# test for required global variables
check.globals

book init regPendingImfile
book init regPendingExp
book init regPendingBurntoolImfile
book init regDates

macro register.reset
  book init regPendingImfile
  book init regPendingExp
  book init regPendingBurntoolImfile
  book init regDates
end

macro register.status
  book list
  book listbook regPendingImfile
  book listbook regPendingExp
  book listbook regPendingBurntoolImfile
  book listbook regDates
end

macro register.on
  task register.imfile.load
    active true
  end
  task register.imfile.run
    active true
  end
  task register.imfile.revert
    active false
  end
  task register.exp.revert
    active false
  end
  task register.exp.load
    active true
  end
  task register.exp.run
    active true
  end
  task register.burntool.load
    active true
  end
  task register.burntool.run
    active true
  end
end

macro register.off
  task register.imfile.load
    active false
  end
  task register.imfile.run
    active false
  end
  task register.imfile.revert
    active false
  end
  task register.exp.revert
    active false
  end
  task register.exp.load
    active false
  end
  task register.exp.run
    active false
  end
  task register.burntool.load
    active false
  end
  task register.burntool.run
    active false
  end
end

macro register.revert.on
  task register.imfile.revert
    active true
  end
  task register.exp.revert
    active true
  end
end

macro register.revert.off
  task register.imfile.revert
    active false
  end
  task register.exp.revert
    active false
  end
end

macro burntool.on
  task register.burntool.load
    active true
  end
  task register.burntool.run
    active true
  end
end

macro burntool.off
  task register.burntool.load
    active false
  end
  task register.burntool.run
    active false
  end
end

macro register.add.date
  if ($0 != 4) 
    echo "USAGE: register.add.date (YYYY-MM-DD) (DBNAME) (VALID_BURNTOOL)"
    break
  end

   book newpage regDates $1
   book setword regDates $1 nsState NEW
   book setword regDates $1 dbname $2
   book setword regDates $1 valid_burntool $3
end


# $valid_burntool_value = 14
$sunrise = 17:30:00
$sunset  = 03:30:00

macro define.sunset
  if ($0 != 2) 
    echo "USAGE: define.sunset DATE"
    break
  end

  $sunset = $1
end

macro set.sunset.time
  if ($0 != 2)
    echo "USAGE: set.sunset.time (time)"
    break
  end

   $sunset = $1
end

macro replace.survey.values
  if ($0 != 5)
    echo "USAGE: replace.survey.values (varname) (CAMERA) (FILTER) (DATE)"
    break
  end

  strsub $$1 @CAMERA@ $2 -var $1
  strsub $$1 @FILTER@ $3 -var $1
  strsub $$1 @DATE@   $4 -var $1
  if ($VERBOSE > 2)
        echo "adding $$1"
  end
end

# we want to set the following arguments to register_exp.pl
# --label ($myLABEL) --dvodb ($myDVODB) --workdir ($myWORKDIR) 
macro set.survey.data
  if ($0 != 7)
    echo "USAGE: set.survey.data (surveyID) (camera) (filter) (datestr) (workdir) (cmdflags)"
    break
  end

  local surveyID myCAMERA myFILTER myDATE myUPDATE 

  $surveyID = $1
  $myCAMERA = $2
  $myFILTER = $3
  $myDATE   = $4

  $$6 = ""

  # we will find a more elegant solution for this later
  if ("$myFILTER" == "z.00000") return
  if ("$myFILTER" == "w.00000") return

  $myUPDATE1 = ""
  book getword surveys $surveyID label -var myLABEL
  if ("$myLABEL" != "NULL") 
    replace.survey.values myLABEL $myCAMERA $myFILTER $myDATE
    $myUPDATE1 = --label $myLABEL
  end

  $myUPDATE2 = ""
  book getword surveys $surveyID dvodb -var myDVODB
  if ("$myDVODB" != "NULL") 
    replace.survey.values myDVODB $myCAMERA $myFILTER $myDATE
    $myUPDATE2 = --dvodb $myDVODB
  end

  $myUPDATE3 = ""
  book getword surveys $surveyID end_stage -var myENDSTAGE
  if ("$myENDSTAGE" != "NULL") 
    replace.survey.values myENDSTAGE $myCAMERA $myFILTER $myDATE
    $myUPDATE3 = --end_stage $myENDSTAGE
  end

  $myUPDATE4 = ""
  book getword surveys $surveyID tess_id -var myTESSID
  if ("$myTESSID" != "NULL") 
    replace.survey.values myTESSID $myCAMERA $myFILTER $myDATE
    $myUPDATE4 = --tess_id $myTESSID
  end

  $$6  = $myUPDATE1 $myUPDATE2 $myUPDATE3 $myUPDATE4

  book getword surveys $surveyID worksubdir -var myWORKSUB
  if ("$myWORKSUB" != "NULL") 
    replace.survey.values myWORKSUB $myCAMERA $myFILTER $myDATE
    $$5 = $$5/$myWORKSUB
  end
end

macro load.surveys
  queueload tmp -x "cat $MODULES:0/surveys.mhpcc.config"
  ipptool2book tmp surveys -key survey
end

# these variables will cycle through the known database names
$regPendingImfile_DB = 0
$regPendingBurntoolImfile_DB = 0
$regRevertImfile_DB = 0
$regRevertExp_DB = 0
$regPendingExp_DB = 0
$reg_datePAGE = 0
# select images ready for register analysis
# new entries are added to regPendingImfile
# compare the new list with the ones already selected
task	       register.imfile.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  # silently drop stdout
  stdout NULL
  stderr $LOGDIR/register.imfile.load.log

  # select entries from the current DB; cycle to the next DB, if it exists
  # iff the DB list is not set, use the value defined in .ipprc
  task.exec
    $run = regtool -pendingimfile
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$regPendingImfile_DB
      $run = $run -dbname $DB:$regPendingImfile_DB
      $regPendingImfile_DB ++
      if ($regPendingImfile_DB >= $DB:n) set regPendingImfile_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit $EXIT_SUCCESS
    # convert 'stdout' to book format
    ipptool2book stdout regPendingImfile -key exp_id:tmp_class_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    book shuffle regPendingImfile
    if ($VERBOSE > 2)
      book listbook regPendingImfile
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup regPendingImfile
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

# run the register_imfile.pl script on pending images
task	       register.imfile.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 30

  task.exec
    periods -exec $RUNEXEC

    book npages regPendingImfile -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in regPendingImfile
    book getpage regPendingImfile 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword regPendingImfile $pageName pantaskState RUN

    book getword regPendingImfile $pageName exp_id       -var EXP_ID
    book getword regPendingImfile $pageName tmp_exp_name -var TMP_EXP_NAME
    book getword regPendingImfile $pageName tmp_camera   -var TMP_CAMERA
    book getword regPendingImfile $pageName tmp_class_id -var TMP_CLASS_ID
    book getword regPendingImfile $pageName uri          -var URI
    book getword regPendingImfile $pageName bytes        -var BYTES
    book getword regPendingImfile $pageName md5sum       -var MD5SUM
    book getword regPendingImfile $pageName workdir      -var WORKDIR_TEMPLATE
    book getword regPendingImfile $pageName dbname       -var DBNAME
    book getword regPendingImfile $pageName summit_dateobs -var SUMMIT_DATEOBS

    # EXP_TAG is used to generate the unique, but human-readable, filenames
    sprintf EXP_TAG "%s.%s" $TMP_EXP_NAME $EXP_ID

    # specify choice of remote host
    set.host.for.camera $TMP_CAMERA $TMP_CLASS_ID 

    # set the WORKDIR variable
    set.workdir.by.camera $TMP_CAMERA $TMP_CLASS_ID $WORKDIR_TEMPLATE $default_host WORKDIR

    # notes on how this works:
    # -- raw workdir examples:
    # file://data/@HOST@.0/gpc1/20080130
    # neb:///@HOST@-vol0/gpc1/20080130 (need to supply volname?, or are we re-defining this each time?)
    # -- out workdir examples:
    # file://data/ipp004.0/gpc1/20080130
    # neb:///ipp004-vol0/gpc1/20080130

    ## generate outroot specific to this exposure (& chip)
    sprintf logfile "%s/%s/%s.reg.%s.log" $WORKDIR $EXP_TAG $EXP_TAG $TMP_CLASS_ID

    stdout $LOGDIR/register.imfile.run.log
    stderr $LOGDIR/register.imfile.run.log

    # XXX register_imfile.pl differs from the standard script : it does not have an 'outroot' argument, and it does not take '--redirect'
    $run = register_imfile.pl --exp_id $EXP_ID --tmp_class_id $TMP_CLASS_ID --tmp_exp_name $TMP_EXP_NAME --uri $URI --logfile $logfile --bytes $BYTES --md5sum $MD5SUM
    $run = $run --sunset $sunset --sunrise $sunrise --summit_dateobs $SUMMIT_DATEOBS
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
    process_exit regPendingImfile $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword regPendingImfile $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword regPendingImfile $options:0 pantaskState TIMEOUT
  end
end

task register.imfile.revert
  host         local
  periods      -poll 60.0
  periods      -exec 600.0
  periods      -timeout 120.0
  npending     1

  stdout       NULL
  stderr       $LOGDIR/register.imfile.revert.log

  task.exec
    $run = regtool -revertprocessedimfile
    if ($DB:n == 0) 
      option DEFAULT
    else
      option $DB:$regRevertImfile_DB
      $run = $run -dbname $DB:$regRevertImfile_DB
      $regRevertImfile_DB ++
      if ($regRevertImfile_DB >= $DB:n) set regRevertImfile_DB = 0
    end
    add_poll_args run
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

  # operation times out
  task.exit    timeout
    showcommand timeout
  end
end



task register.exp.revert
  host         local
  periods      -poll 60.0
  periods      -exec 600.0
  periods      -timeout 120.0
  npending     1

  stdout       NULL
  stderr       $LOGDIR/register.exp.revert.log

  task.exec
# Only revert fault 2, because other faults likely mean something horrible is happening.
    $run = regtool -revertprocessedexp -fault 2
    if ($DB:n == 0) 
      option DEFAULT
    else
      option $DB:$regRevertExp_DB
      $run = $run -dbname $DB:$regRevertExp_DB
      $regRevertExp_DB ++
      if ($regRevertExp_DB >= $DB:n) set regRevertExp_DB = 0
    end
#    add_poll_args run
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

  # operation times out
  task.exit    timeout
    showcommand timeout
  end
end


# select exposures ready for register_exp.pl
task	       register.exp.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/register.exp.load.log

  task.exec
    $run = regtool -pendingexp
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$regPendingExp_DB
      $run = $run -dbname $DB:$regPendingExp_DB
      $regPendingExp_DB ++
      if ($regPendingExp_DB >= $DB:n) set regPendingExp_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit $EXIT_SUCCESS
    # convert 'stdout' to book format
    ipptool2book stdout regPendingExp -key exp_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook regPendingExp
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup regPendingExp
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

# run the register_exp.pl script on pending exposures
task	       register.exp.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 30

  task.exec
    book npages regPendingExp -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    # look for new images in regPendingExp (pantaskState == INIT)
    book getpage regPendingExp 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword regPendingExp $pageName pantaskState RUN
    book getword regPendingExp $pageName exp_id       -var EXP_ID
    book getword regPendingExp $pageName tmp_exp_name -var TMP_EXP_NAME
    book getword regPendingExp $pageName tmp_camera   -var TMP_CAMERA
    book getword regPendingExp $pageName workdir      -var WORKDIR_TEMPLATE
    book getword regPendingExp $pageName dbname       -var DBNAME
    book getword regPendingExp $pageName camera       -var CAMERA
    book getword regPendingExp $pageName filter       -var FILTER
    book getword regPendingExp $pageName dateobs      -var DATEOBS
    book getword regPendingExp $pageName obs_mode     -var OBS_MODE
    book getword regPendingExp $pageName obs_group    -var OBS_GROUP

    # EXP_TAG is used to generate the unique, but human-readable, filenames
    sprintf EXP_TAG "%s.%s" $TMP_EXP_NAME $EXP_ID

    # 2007-08-30T05:09:59Z
    strlen $DATEOBS length
    if ($length > 10)
      substr $DATEOBS 0 4 YEAR
      substr $DATEOBS 5 2 MONTH
      $datestr = "$YEAR\$MONTH"
    end

    # look up label, dvodb based on survey
    set.survey.data $OBS_MODE $CAMERA $FILTER $datestr WORKDIR_TEMPLATE CMDFLAGS

    # specify choice of remote host
    set.host.for.camera $TMP_CAMERA FPA

    # set the WORKDIR variable
    set.workdir.by.camera $TMP_CAMERA FPA $WORKDIR_TEMPLATE $default_host WORKDIR

    # notes on how this works:
    # -- raw workdir examples:
    # file://data/@HOST@.0/gpc1/20080130
    # neb:///@HOST@.0/gpc1/20080130
    # -- out workdir examples:
    # file://data/ipp004.0/gpc1/20080130
    # neb:///ipp004-vol0/gpc1/20080130
    ## generate output log based on filerule
    sprintf logfile "%s/%s/%s.reg.log" $WORKDIR $EXP_TAG $EXP_TAG

    stdout $LOGDIR/register.exp.run.log
    stderr $LOGDIR/register.exp.run.log

    $run = register_exp.pl --exp_id $EXP_ID --exp_tag $EXP_TAG --logfile $logfile $CMDFLAGS
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
    process_exit regPendingExp $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword regPendingExp $options:0 pantaskState CRASH
  end

  task.exit    crash
    showcommand crash
    book setword regPendingExp $options:0 pantaskState CRASH
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
    book setword regPendingExp $options:0 pantaskState TIMEOUT
  end
end


# select imfiles that can now be burntooled.
task       register.burntool.load
  host     local

  periods  -poll $LOADPOLL
  periods  -exec $LOADEXEC
  periods  -timeout 30
  npending 1

  # silently drop stdout
  stdout NULL
  stderr $LOGDIR/register.burntool.load.log
  
  # select entried from the current DB; cycle to the next DB, if it exists
  task.exec
     book npages regDates -var Npages
     if ($Npages == 0) 
       $today = `date -u +%Y-%m-%d`
       $dbname = $DB:0
       $valid_burntool = 14
     else 
       book getpage regDates $reg_datePAGE -var today
       book getword regDates $today dbname -var dbname
       book getword regDates $today valid_burntool -var valid_burntool
       $reg_datePAGE ++
       if ($reg_datePAGE >= $Npages) set reg_datePAGE = 0
     end

    $run = regtool -pendingburntoolimfile

# debugging purposes
#   $today = "2010-12-25"
    $dateobs_begin = $today\T$sunset
    $dateobs_end   = $today\T$sunrise
    $run = $run -dateobs_begin $dateobs_begin -dateobs_end $dateobs_end -valid_burntool $valid_burntool
    $run = $run -dbname $dbname
    option $dbname
#     if ($DB:n == 0)
#       option DEFAULT
#     else
#       # save the DB name for the exit tasks
#       option $DB:$regPendingBurntoolImfile_DB
#       $run = $run -dbname $DB:$regPendingBurntoolImfile_DB
#       if ("$dbname" != "$DB:$regPendingBurntoolImfile_DB")
#         break
#       end
#       $regPendingBurntoolImfile_DB ++
#       if ($regPendingBurntoolImfile_DB >= $DB:n) set regPendingBurntoolImfile_DB = 0
#     end

#    echo $run
    add_poll_args run
    command $run
  end

  # success
  task.exit $EXIT_SUCCESS
    # convert 'stdout' to book format
    ipptool2book stdout regPendingBurntoolImfile -key exp_id:class_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    book shuffle regPendingBurntoolImfile
    if ($VERBOSE > 2)
      book listbook regPendingBurntoolImfile
    end

    # delete existing entries in the appropriate pantasksStates
    process_cleanup regPendingBurntoolImfile
  end

  # locked list
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

# run the ipp_apply_burntool_single.pl script on the pending images
task          register.burntool.run
  periods     -poll $RUNPOLL
  periods     -exec $RUNEXEC
  periods     -timeout 30

  task.exec
    periods -exec $RUNEXEC

    book npages regPendingBurntoolImfile -var N
    if ($N == 0) break
    if ($NETWORK == 0) break

    # look for new images to burn in regPendingBurntoolImfile
    book getpage regPendingBurntoolImfile 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword regPendingBurntoolImfile $pageName pantaskState RUN

    book getword regPendingBurntoolImfile $pageName exp_id      -var EXP_ID
    book getword regPendingBurntoolImfile $pageName tmp_class_id -var TMP_CLASS_ID
    book getword regPendingBurntoolImfile $pageName class_id    -var CLASS_ID
    book getword regPendingBurntoolImfile $pageName uri         -var THIS_URI
    book getword regPendingBurntoolImfile $pageName previous_uri -var PREVIOUS_URI
    book getword regPendingBurntoolImfile $pageName camera      -var CAMERA
    book getword regPendingBurntoolImfile $pageName dbname      -var DBNAME
    set.host.for.camera $DBNAME $TMP_CLASS_ID

    stdout $LOGDIR/register.burntool.run.log
    stderr $LOGDIR/register.burntool.run.log

    $run = ipp_apply_burntool_single.pl --camera $CAMERA --exp_id $EXP_ID --class_id $CLASS_ID --this_uri $THIS_URI --continue 10
    if ("$PREVIOUS_URI" != "NULL")
      $run = $run --previous_uri $PREVIOUS_URI
    end

    add_standard_args run

    options $pageName

    if ($VERBOSE > 1) 
      echo command $run
    end
    periods -exec 0.05
    command $run
  end

  # default exit status
  task.exit default
    process_exit regPendingBurntoolImfile $options:0 $JOB_STATUS
  end

  # locked list
  task.exit crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword regPendingBurntoolImfile $options:0 pantaskState CRASH
  end

  # operation timed out
  task.exit  timeout
    showcommand timeout
    book setword regPendingBurntoolImfile $options:0 pantaskState TIMEOUT
  end
end

task   register.initday.load
  host            local
  periods         -poll $LOADPOLL
  periods         -exec $LOADEXEC
  periods         -timeout 30
  trange          0:00:00 1:00:00 -nmax 1
  npending        1

  task.exec
    $today = `date -u +%Y-%m-%d`
    book newpage regDates $today
    book setword regDates $today nsState NEW
    book setword regDates $today dbname $DB:0
    book setword regDates $today valid_burntool 14

    command true
  end

  task.exit       0
    # nothing to do here?
  end
  # locked list
  task.exit    default
    showcommand failure
  end
  task.exit    crash
    showcommand crash
  end
  #operation times out?
  task.exit    timeout
    showcommand timeout
  end
end

   
  
## XXX add a global path to output files  
