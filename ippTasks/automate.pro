## automate.pro : autorun the detrend analysis : -*- sh -*-

# example automation stage
# automate METADATA
#  name       STR DARK
#  check      STR "detselect -search -inst SIMTEST -det_type BIAS -dbname eamtest"
#  ncheck     S32 1
#  launch     STR "dettool -definebyquery -workdir path://EAMWORK -inst SIMTEST -det_type DARK -select_exp_type DARK -dbname eamtest"
#  block      STR "dettool -runs -active -det_type DARK -dbname eamtest"
# END

# an entry in the 'automate' book goes through the following states:
# INIT (book loaded from input file)
# CHECK (stage prereqs being checked)
# READY (prereqs satisfield, ready to be launched)
# LAUNCH (action being performed)
# DONE (action finished)

macro automate.status

  ## Pull out the ones that are to be run regularly
  local npages
  book npages automate -var npages
  for i 0 $npages
    book getpage automate $i -var pageName
    echo $i : $pageName
    book getword automate $pageName pantaskState -var myState
    if ("$myState" != "NULL")
      echo $pageName : $myState
    end
  end
end

macro automate.reset
  ## probably should not always init
  book init automate
end

macro automate.load
  if ($0 != 4)
    echo "USAGE: automate.load (filename) (camera) (dbname)"
    break
  end

  file $1 isFound
  if ($isFound)
    queueload tmp -x "cat $1"
  else
    # search module path list for existing file
    queueload tmp -x "cat $MODULES:0/$1"
  end

  pwd -var cwd
  $username = `whoami`

  ## interpolate standard values
  queuesubstr tmp @CAMERA@ $2
  queuesubstr tmp @DBNAME@ $3
  queuesubstr tmp @CWD@ $cwd
  queuesubstr tmp @USER@ $username

  ipptool2book tmp automate -key name -uniq -setword pantaskState INIT.BLOCK
  queuedelete tmp

  ## Pull out the ones that are to be run regularly
  local npages
  book npages automate -var npages
  for i 0 $npages
    book getpage automate $i -var pageName
    if ("$pageName" != "NULL") 
      book getword automate $pageName regular -var regularCommand
      if ("$regularCommand" != "NULL")
         book setword automate $pageName pantaskState INIT.REGULAR
      end
    end
  end

end

macro automate.on
  task automate.block
    active true
  end
  task automate.check
    active true
  end
  task automate.launch
    active true
  end
  task automate.regular
    active true
  end
end

macro automate.off
  task automate.block
    active false
  end
  task automate.check
    active false
  end
  task automate.launch
    active false
  end
  task automate.regular
    active false
  end
end

$automate_Nblock = -1
task automate.block
  host         local

  periods      -poll 1
  periods      -exec 5
  periods      -timeout 30
  active       true
  npending     1

  task.exec
    local Npage pageName

    # how many pages are waiting to be started?
    book npages automate -var Npage -key pantaskState INIT.BLOCK
    if ($Npage == 0) 
      if ($VERBOSE >= 2) 
        echo "no entries in INIT.BLOCK state"
      end
      break
    end

    # cycle over the number of INIT.BLOCK pages: no point to loop over the others
    $automate_Nblock ++
    if ($automate_Nblock >= $Npage) set automate_Nblock = 0

    # search the automate book for an entry which is unstarted (state INIT.BLOCK) 
    book getpage automate $automate_Nblock -var pageName -key pantaskState INIT.BLOCK
    if ("$pageName" == "NULL") 
      if ($VERBOSE >= 2) 
        echo "entry $automate_Nblock not in INIT.BLOCK state"
      end
      break 
    end
 
    book getword automate $pageName block -var blockCommand
    if (("$blockCommand" == "NULL") || ("$blockCommand" == "NONE"))
      # if there is no block needed, we can immediate progress to the next stage (INIT.CHECK)
      book setword automate $pageName pantaskState INIT.CHECK
      if ($VERBOSE >= 2) 
        echo "$pageName is ready : INIT.CHECK"
      end
      break
    end

    book setword automate $pageName pantaskState RUN.BLOCK

    if ($VERBOSE >= 2)
      echo "starting automate block for $pageName"
      echo "command $blockCommand"
    end

    options $pageName
    command $blockCommand
  end

  task.exit $EXIT_SUCCESS
    local pageName Npage

    $pageName = $options:0

    # convert 'stdout' to book format
    # XXX to use other tests, we'll need to modify this 
    ipptool2book stdout tmpBlock
    if ($VERBOSE > 2)
      book listbook tmpBlock
    end

    # if the block test returns any valid pages (valid results), the block is set, don't move to check
    book npages tmpBlock -var Npage 
    if ($Npage == 0) 
      if ($VERBOSE >= 2)
        echo "$pageName is not blocked, ready for CHECK"
      end
      book setword automate $pageName pantaskState INIT.CHECK
    else
      if ($VERBOSE >= 2)
        echo "$pageName is blocked, not ready for CHECK"
      end
      book setword automate $pageName pantaskState INIT.BLOCK
    end

    # drop the detExt book after we've grabbed the state
    book delete tmpBlock
  end

  # all failures here (what state?)
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

$automate_Ncheck = -1
task automate.check
  host         local

  periods      -poll 1
  periods      -exec 5
  periods      -timeout 30
  active       true
  npending     1

  task.exec
    local Npage pageName

    # how many pages are waiting to be started?
    book npages automate -var Npage -key pantaskState INIT.CHECK
    if ($Npage == 0) 
      if ($VERBOSE >= 2) 
        echo "no entries in INIT.CHECK state"
      end
      break
    end

    # cycle over the number of INIT.CHECK pages: no point to loop over the others
    $automate_Ncheck ++
    if ($automate_Ncheck >= $Npage) set automate_Ncheck = 0

    # search the automate book for an entry which is unstarted (state INIT.CHECK) 
    book getpage automate $automate_Ncheck -var pageName -key pantaskState INIT.CHECK
    if ("$pageName" == "NULL") 
      if ($VERBOSE >= 2) 
        echo "entry $automate_Ncheck not in INIT.CHECK state"
      end
      break 
    end
 
    book getword automate $pageName check -var checkCommand
    if (("$checkCommand" == "NULL") || ("$checkCommand" == "NONE"))
      # if there is no check needed, we can immediate progress to the next stage (INIT.LAUNCH)
      book setword automate $pageName pantaskState INIT.LAUNCH
      if ($VERBOSE >= 2) 
        echo "$pageName is ready : INIT.LAUNCH"
      end
      break
    end

    book setword automate $pageName pantaskState RUN.CHECK

    if ($VERBOSE >= 2)
      echo "starting automate check for $pageName"
      echo "command $checkCommand"
    end

    options $pageName
    command $checkCommand
  end

  task.exit $EXIT_SUCCESS
    local pageName Npage

    $pageName = $options:0

    # convert 'stdout' to book format
    # XXX to use other tests, we'll need to modify this 
    ipptool2book stdout tmpCheck
    if ($VERBOSE > 2)
      book listbook tmpCheck
    end

    book getword automate $pageName ncheck -var Ncheck
    if ("$Ncheck" == "NULL")
      $Ncheck = 1
    end 
    book npages tmpCheck -var Npage 
    if ($Npage < $Ncheck) 
      if ($VERBOSE >= 2) 
        echo "$pageName not ready for LAUNCH"
      end
      book setword automate $pageName pantaskState INIT.CHECK
    else
      if ($VERBOSE >= 2) 
        echo "$pageName is ready for LAUNCH"
      end
      book setword automate $pageName pantaskState INIT.LAUNCH

      if ($Ncheck == 1) 
        # XXX this is a somewhat hackish way to carry information from the 'check' to the 'launch'
        book getword tmpCheck page.000 det_id -var DET_ID
        book getword tmpCheck page.000 iteration -var ITERATION
        book setword automate $pageName det_id $DET_ID
        book setword automate $pageName iteration $ITERATION
      end
    end

    # drop the detExt book after we've grabbed the state
    book delete tmpCheck
  end

  # all failures here (what state?)
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

$automate_Nlaunch = 0
task automate.launch
  host         local

  periods      -poll 1
  periods      -exec 5
  periods      -timeout 30
  active       true
  npending     1

  task.exec
    local Npage pageName

    # how many pages are waiting to be started?
    book npages automate -var Npage -key pantaskState INIT.LAUNCH
    if ($Npage == 0) 
      if ($VERBOSE >= 2)
        echo "no entries in INIT.LAUNCH state"
      end
      break
    end

    # cycle over the number of INIT.LAUNCH pages: no point to loop over the others
    $automate_Nlaunch ++
    if ($automate_Nlaunch >= $Npage) set automate_Nlaunch = 0

    # search the automate book for an entry which is unstarted (state INIT.LAUNCH) 
    book getpage automate $automate_Nlaunch -var pageName -key pantaskState INIT.LAUNCH
    if ("$pageName" == "NULL") 
      if ($VERBOSE >= 2)
        echo "entry $automate_Nlaunch not in INIT.LAUNCH state"
      end
      break 
    end
 
    book getword automate $pageName launch -var launchCommand
    if (("$launchCommand" == "NULL") || ("$launchCommand" == "NONE"))
      # if there is no launch needed, we can immediate progress to the next stage (INIT.LAUNCH)
      book setword automate $pageName pantaskState INIT.LAUNCH
      if ($VERBOSE >= 2)
        echo "$pageName is ready : INIT.LAUNCH"
      end
      break
    end

    # modify the launch command to replace certain elements from the page
    book getword automate $pageName det_id -var DET_ID
    if ("$DET_ID" != "NULL") 
      echo '$launchCommand'
      strsub "$launchCommand" @det_id@ $DET_ID -var launchCommand
    end

    book getword automate $pageName iteration -var ITERATION
    if ("$ITERATION" != "NULL") 
      echo '$launchCommand'
      strsub "$launchCommand" @iteration@ $ITERATION -var launchCommand
    end

    echo '$launchCommand'

    book setword automate $pageName pantaskState RUN.LAUNCH

    if ($VERBOSE >= 2)
      echo "starting automate launch for $pageName"
      echo "command $launchCommand"
    end

    options $pageName
    command $launchCommand
  end

  task.exit $EXIT_SUCCESS
    local pageName Npage

    $pageName = $options:0

    book setword automate $pageName pantaskState DONE.LAUNCH
  end

  # all failures here (what state?)
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


$automate_Nregular = 0
task automate.regular
  host         local

  periods      -poll 1
  periods      -exec 5
  periods      -timeout 30
  active       true
  npending     1

  task.exec
    local Npage pageName

    # how many pages are waiting to be started?
    book npages automate -var Npage -key pantaskState INIT.REGULAR
    if ($Npage == 0) 
      if ($VERBOSE >= 2)
        echo "no entries in INIT.REGULAR state"
      end
      break
    end

    # cycle over the number of INIT.REGULAR pages: no point to loop over the others
    $automate_Nregular ++
    if ($automate_Nregular >= $Npage) set automate_Nregular = 0

    # search the automate book for an entry which is unstarted (state INIT.REGULAR) 
    book getpage automate $automate_Nregular -var pageName -key pantaskState INIT.REGULAR
    if ("$pageName" == "NULL") 
      if ($VERBOSE >= 2)
        echo "entry $automate_Nregular not in INIT.REGULAR state"
      end
      break 
    end
 
    book getword automate $pageName regular -var regularCommand
    if (("$regularCommand" == "NULL") || ("$regularCommand" == "NONE"))
      if ($VERBOSE >= 2)
        echo "Warning: $pageName has no regular command"
      end
      break
    end

    book setword automate $pageName pantaskState RUN.REGULAR

    if ($VERBOSE >= 2)
      echo "starting automate regular for $pageName"
      echo "command $regularCommand"
    end

    options $pageName
    command $regularCommand
  end

  task.exit $EXIT_SUCCESS
    local pageName Npage

    $pageName = $options:0

    book setword automate $pageName pantaskState INIT.REGULAR
  end

  # all failures here (what state?)
  task.exit    default
    book setword automate $pageName pantaskState FAIL.REGULAR
    showcommand failure
  end

  task.exit    crash
    book setword automate $pageName pantaskState INIT.REGULAR
    showcommand crash
  end

  # operation times out?
  task.exit    timeout
    book setword automate $pageName pantaskState INIT.REGULAR
    showcommand timeout
  end
end

