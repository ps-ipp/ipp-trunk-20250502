## dist.pro : support for the production of distribution bundles : -*- sh -*-

# test for required global variables
check.globals

$LOGSUBDIR = $LOGDIR/dist
mkdir $LOGSUBDIR

### Initialise the books containing the tasks to do
book init distToProcess
book init distToAdvance

### Database lists
$distToProcess_DB = 0
$distToAdvance_DB = 0
$distQueue_DB = 0
$dist_revert_DB = 0

### list of stages
#list of stages
$DIST_STAGE:n = 0
list DIST_STAGE -add "raw"
list DIST_STAGE -add "chip"
list DIST_STAGE -add "camera"
list DIST_STAGE -add "fake"
list DIST_STAGE -add "warp"
list DIST_STAGE -add "diff"
list DIST_STAGE -add "stack"
list DIST_STAGE -add "SSdiff"
list DIST_STAGE -add "chip_bg"
list DIST_STAGE -add "warp_bg"
list DIST_STAGE -add "sky"
list DIST_STAGE -add "skycal"
list DIST_STAGE -add "ff"

$currentStage = 0

### Check status of tasks
macro dist.status
  book listbook distToProcess
  book listbook distToAdvance
end

### Reset tasks
macro dist.reset
  book init distToProcess
  book init distToAdvance
end

### Turn tasks on
macro dist.on
  task dist.process.load
    active true
  end
  task dist.process.run
    active true
  end
  task dist.advance.load
    active true
  end
  task dist.advance.run
    active true
  end
  task dist.revert
    active true
  end
end

macro dist.off
  task dist.process.load
    active false
  end
  task dist.process.run
    active false
  end
  task dist.advance.load
    active false
  end
  task dist.advance.run
    active false
  end
  task dist.revert
    active false
  end
end

macro dist.revert.on
  task dist.revert
    active true
  end
end

macro dist.revert.off
  task dist.revert
    active false
  end
end

# This is now only used if we are not using a nebulous outroot
macro get.host.for.component
    if ($0 != 3)
        echo "USAGE: get.host.for.component (componentID) (varname)"
        break
    end
    local component host varname
    $component = $1
    $varname = $2
    substr $component 0 3 COMP_HEAD
    if ("$COMP_HEAD" == "sky")
        book getword ipphosts dist_skycell count -var count
        local j myValue skyhash
        list word -splitbychar . $component
        $skyhash = 0
        for j 0 $word:n
            inthash $word:$j $count -var myValue
            $skyhash = $skyhash + $myValue
        end
        inthash $skyhash $count -var skyhash
        sprintf skyname "sky%02d" $skyhash
        book getword ipphosts dist_skycell $skyname -var host
    else
        book getword ipphosts dist_chip $component -var host
    end
    $$varname = $host
end

# This is now only used if we are not using a nebulous outroot
macro set.dist.workdir.by.component
    if ($0 != 5)
        echo "USAGE: set.workdir.by.component (stage_id) (componentID) (template) (varname)"
        break
    end
    local host stage_id component default template varname length start count selector component_id random_number

    $stage_id = $1
    $component = $2
    $template = $3
    $varname = $4
    if ("$template" == "NULL")
        echo "ERROR: WORKDIR template not set."
        break
    end
    book getword ipphosts distribution count -var count
    if ("$count" == "NULL")
        echo "ERROR: distribution hosts list is empty"
        break
    end
    $host = "foo"
    if ("$component" == "exposure") 
        # take last two letters of selector
        # treat it as an integer and use modulous of length of distribution hosts
        # to compute an index into the host table
        strlen $stage_id length
	if ($length >= 2) 
	    $start = $length - 2
	    substr $stage_id $start 2 index
	else
	    # stage_id < 10 caused a very annoying couple of hours for bills to debug
	    substr $stage_id 0 1 index
	end
        $component_id = $index % $count
        book getword ipphosts distribution $component_id -var myhost
    else
        get.host.for.component $component myhost
    end

    if ("$myhost" == "NULL")
        echo "ERROR: failed to find host for $component"
        break
    end

    strsub $template @HOST@ $myhost -var $varname

    #echo template is $template
    #echo outdir is $$varname
end


task	       dist.process.load
  host         local

  periods      -exec 20
  periods      -poll $LOADPOLL
  periods      -timeout 300
  npending     1


  task.exec
     # stdout NULL
     # stderr $LOGSUBDIR/dist.process.load.log

    # check current stage in case it is out of range because entries
    # have been removed
    if ($currentStage >= $DIST_STAGE:n) set currentStage = 0

    $run = disttool -pendingcomponent -stage $DIST_STAGE:$currentStage
    $currentStage ++
    if ($currentStage >= $DIST_STAGE:n) set currentStage = 0

    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$distToProcess_DB
      $run = $run -dbname $DB:$distToProcess_DB

      # only increment the database number after we have gone through all of
      # the stages
      if ($currentStage == 0)
          $distToProcess_DB ++
          if ($distToProcess_DB >= $DB:n) set distToProcess_DB = 0
      end
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout distToProcess -key dist_id:component -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook distToProcess
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup distToProcess
  end

  # locked list
  task.exit    default
    showcommand failure
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
  end
end

task	       dist.process.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    periods -exec 10

    book npages distToProcess -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
   
    # look for new components to process (pantaskState == INIT)
    book getpage distToProcess 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

#    echo running $pageName

    book setword distToProcess $pageName pantaskState RUN
    book getword distToProcess $pageName dist_id -var DIST_ID
    book getword distToProcess $pageName camera -var CAMERA
    book getword distToProcess $pageName stage -var DIST_STAGE
    book getword distToProcess $pageName stage_id -var DIST_STAGE_ID
    book getword distToProcess $pageName clean -var CLEAN
    book getword distToProcess $pageName component -var COMPONENT
    book getword distToProcess $pageName path_base -var PATH_BASE
    book getword distToProcess $pageName chip_path_base -var CHIP_PATH_BASE
    book getword distToProcess $pageName state -var STATE
    book getword distToProcess $pageName data_state -var DATA_STATE
    book getword distToProcess $pageName quality -var QUALITY
    book getword distToProcess $pageName no_magic -var NO_MAGIC
    book getword distToProcess $pageName magicked -var MAGICKED
    book getword distToProcess $pageName alt_path_base -var ALT_PATH_BASE
    book getword distToProcess $pageName outdir -var OUTDIR_TEMPLATE
    book getword distToProcess $pageName exp_type -var EXP_TYPE
    book getword distToProcess $pageName dbname -var DBNAME

    $EXTRA_ARGS = ""
    if ("$CLEAN" == "T")
        $EXTRA_ARGS = --clean
    end
    # magicked is output as integer due to the union in the sql
    if ($MAGICKED)
        $EXTRA_ARGS = $EXTRA_ARGS --magicked
    end
    # is this right for stack and fake?
    if ("$NO_MAGIC" == "T")
        $EXTRA_ARGS = $EXTRA_ARGS --no_magic
    end
    if ($QUALITY)
        $EXTRA_ARGS = $EXTRA_ARGS --poor_quality
    end
    if ("$ALT_PATH_BASE" != "NULL")
        $EXTRA_ARGS = $EXTRA_ARGS --alt_path_base $ALT_PATH_BASE
    end

    substr $COMPONENT 0 3 COMP_HEAD
    if ("$COMP_HEAD" == "sky")
        set.host.for.skycell $COMPONENT
        set.workdir.by.skycell $COMPONENT $OUTDIR_TEMPLATE $default_host OUTDIR
    else 
        # assume component is a class_id, if  it is not we will default to
        # host "anyhost"
        # and volume "any"
        set.host.for.camera $CAMERA $COMPONENT
        set.workdir.by.camera $CAMERA $COMPONENT $OUTDIR_TEMPLATE $default_host OUTDIR
    end

    if ("$OUTDIR" == "NULL")
        echo ERROR failed to set workdir for $COMPONENT
        break
    end

    sprintf logfile "%s/dist.%s.%s.log" $OUTDIR $DIST_ID $COMPONENT

    $run = dist_component.pl --dist_id $DIST_ID --camera $CAMERA --stage $DIST_STAGE --stage_id $DIST_STAGE_ID --component $COMPONENT --exp_type $EXP_TYPE --path_base $PATH_BASE --chip_path_base $CHIP_PATH_BASE --state $STATE --data_state $DATA_STATE $EXTRA_ARGS --outdir $OUTDIR --logfile $logfile

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
  task.exit    default
    process_exit distToProcess $options:0 $JOB_STATUS
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword distToProcess $options:0 pantaskState TIMEOUT
  end
end

task	       dist.advance.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec 30
  periods      -timeout 3000
  npending     1

  task.exec
  #  stderr $LOGSUBDIR/dist.advance.load.log

    $run = disttool -toadvance
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$distToAdvance_DB
      $run = $run -dbname $DB:$distToAdvance_DB
      $distToAdvance_DB ++
      if ($distToAdvance_DB >= $DB:n) set distToAdvance_DB = 0
    end
    add_poll_args run
    add_poll_labels run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout distToAdvance -key dist_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook distToAdvance
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup distToAdvance
  end

  # locked list
  task.exit    default
    showcommand failure
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
  end
end


task	       dist.advance.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    book npages distToAdvance -var N
    if ($NETWORK == 0) break
    if ($N == 0) 
    	periods -exec 10 
	break
    end
    periods -exec .05
    
    # look for new components to process (pantaskState == INIT)
    book getpage distToAdvance 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    # XXX the sql for disttool -toadvance does not include the camera:
    # hack around this for now
    $CAMERA = GPC1

    book setword distToAdvance $pageName pantaskState RUN
    book getword distToAdvance $pageName dist_id -var DIST_ID
    book getword distToAdvance $pageName stage   -var STAGE
    book getword distToAdvance $pageName stage_id -var STAGE_ID
    book getword distToAdvance $pageName outdir -var OUTDIR_TEMPLATE
    book getword distToAdvance $pageName clean -var CLEAN
    book getword distToAdvance $pageName dbname -var DBNAME
    $EXTRA_ARGS = ""
    if ("$CLEAN" == "T")
        $EXTRA_ARGS = --clean
    end

    substr $OUTDIR_TEMPLATE 0 3 SCHEME
    if ("$SCHEME" == "neb")
        # XXX This does not work if workdir is not in nebulous
        # and $default_host == "any" because /data/any does not exist
        # use "dummy" as component. Result will be 'any'
        # set.workdir.by.camera $CAMERA dummy $OUTDIR_TEMPLATE $default_host OUTDIR
        set.workdir.by.camera GPC1 dummy $OUTDIR_TEMPLATE $default_host OUTDIR
    else 
        # using $DIST_ID as the "component" works fine here since we only look
        # at the last two digits. But make sure that there 2 digits
        $fake_component = $STAGE_ID + 10
        set.dist.workdir.by.component $fake_component "exposure" $OUTDIR_TEMPLATE OUTDIR 
    end
 
    if ("$OUTDIR" == "NULL")
        echo ERROR failed to set workdir for $DIST_ID
        break
    end
    host anyhost

    sprintf logfile "%s/dist.advance.%s.log" $OUTDIR $DIST_ID

    $run = dist_advancerun.pl --dist_id $DIST_ID --stage $STAGE --stage_id $STAGE_ID --camera $CAMERA --outdir $OUTDIR $EXTRA_ARGS --logfile $logfile
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    command $run
  end

  # default exit status
  task.exit    default
    process_exit distToAdvance $options:0 $JOB_STATUS
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword distToAdvance $options:0 pantaskState TIMEOUT
  end
end

task dist.revert
  host         local

  periods      -poll 5.0
  periods      -exec 60.0
  periods      -timeout 120.0
  npending     1
  active false
  
  stdout NULL
  stderr $LOGDIR/revert.log

  task.exec
    if ($LABEL:n == 0) break
    $run = disttool -revertcomponent
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$dist_revert_DB
      $run = $run -dbname $DB:$dist_revert_DB
      $dist_revert_DB ++
      if ($dist_revert_DB >= $DB:n) set dist_revert_DB = 0
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
