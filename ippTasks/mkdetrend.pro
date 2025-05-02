## mkdetrend.pro : autorun the detrend analysis : -*- sh -*-

# detRun  METADATA  
#    det_id           STR       3               
#    iteration        S32       0              
#    det_type         STR       SHUTTER         
#    mode             STR       master          
#    state            STR       run             
#    filelevel        STR       FPA             
# END

macro load.detRunDefs
  if ($0 != 2)
    echo "USAGE: load.detRunDefs (filename)"
    break
  end
  queueload detruns -x "cat $MODULES:0/$1"
  ipptool2book detruns detRunDef -key detRunName -uniq -setword pantaskState INIT
  queuedelete detruns
end

task detrun.launch
  host         local

  periods      -poll 1
  periods      -exec 5
  periods      -timeout 30
  active       true
  npending     1

  # each outstanding detrend run blocks the next one.  only launch the next 
  # if the current one is done
  task.exec
    local i j N found missed detRunName prereq prereqList prereqState cmdOptions dbname

    # how many detRunDefs are waiting to be started?
    book npages detRunDef -var N -key pantaskState INIT
    # echo "detrun.launch N: $N"
    if ($N == 0) break

    $found = 0

    # search the detRunDef book for an entry which is unstarted (state NULL) 
    # and for which the dependencies are done 
    for i 0 $N
      book getpage detRunDef $i -var detRunName -key pantaskState INIT
      # echo "detrun.launch detRunName: $detRunName"
      if ("$detRunName" == "NULL") break 
 
      book getword detRunDef $detRunName prereq -var prereqList
      if (("$prereqList" == "NULL") || ("$prereqList" == "NONE"))
        $found = 1
        last
      end

      list prereq -split $prereqList
      $missed = 0
      for j 0 $prereq:n
        # prereq must exist and be in state of DONE
        book getword detRunDef $prereq:$j pantaskState -var prereqState

        # echo "detrun.launch prereqState: $prereqState"
        # echo "detrun.launch detRunName: $detRunName"
        if ("$prereqState" != "DONE") 
          $missed = 1
          last
        end
      end
      if ($missed == 0)
        $found = 1
        last
      end

      if ($found == 0) 
        if ($VERBOSE >= 2)
          echo "$detRunName not ready"
        end
      else
        if ($VERBOSE >= 2)
          echo "$detRunName ready"
        end
      end
    end

    if ($found == 0) break

    # echo "detrun.launch detRunName: $detRunName"

    book getword detRunDef $detRunName options -var cmdOptions
    book getword detRunDef $detRunName dbname -var dbname
    book setword detRunDef $detRunName pantaskState INIT

    if ($VERBOSE >= 1)
      echo "starting detrend $detRunName :"
      echo "command dettool $cmdOptions -dbname $dbname"
    end

    options $dbname $detRunName
    command dettool $cmdOptions -dbname $dbname
  end

  task.exit $EXIT_SUCCESS
    local detRunID

    $dbname = $options:0
    $detRunName = $options:1

    # convert 'stdout' to book format
    ipptool2book stdout detRun -key det_id -uniq
    if ($VERBOSE > 2)
      book listbook detRun
    end

    # we should have launched only a single detRun
    book getpage detRun 0 -var detRunID
    if ("$detRunID" == "NULL") 
      echo "ERROR:detRun not defined"
      break
    end

    # we need to track the detID generated
    book setword detRunDef $detRunName detRunID $detRunID
    book setword detRunDef $detRunName pantaskState RUN

    # drop the detRun book after we've grabbed the state
    book delete detRun
  end
end

$detRunNcheck = 0

# loop over the outstanding detRuns and update state
task detrun.check
  host         local

  periods      -poll 1
  periods      -exec 5
  periods      -timeout 30
  active       true
  npending     1

  # each outstanding detrend run blocks the next one.  only launch the next 
  # if the current one is done
  task.exec
    local i N found detRunName detRunID dbname

    book npages detRunDef -var N -key pantaskState RUN
    if ($N == 0) break
    if ($detRunNcheck >= $N) 
      $detRunNcheck = 0
    end

    # try the next detRun entry which is in state RUN
    book getpage detRunDef $detRunNcheck -var detRunName -key pantaskState RUN
    $detRunNcheck ++

    if ("$detRunName" == "NULL")
      if ($VERBOSE >= 3)
        echo "no more active detruns (Ncheck = $detRunNcheck)"
      end
      break
    end 

    # info needed to check on current state
    book getword detRunDef $detRunName detRunID -var detRunID
    book getword detRunDef $detRunName dbname -var dbname

    if ($VERBOSE >= 2) 
      echo " checking on $detRunName"
      echo "command dettool -runs -det_id $detRunID -dbname $dbname"
    end

    # we need a dettool command which takes a specific det_id and returns
    # the current state
    options $dbname $detRunName
    command dettool -runs -det_id $detRunID -dbname $dbname
  end

  task.exit $EXIT_SUCCESS
    local detRunID state

    $dbname = $options:0
    $detRunName = $options:1

    # convert 'stdout' to book format
    ipptool2book stdout detRun -key det_id -uniq
    if ($VERBOSE > 2)
      book listbook detRun
    end

    # we should have launched only a single detRun
    book getpage detRun 0 -var detRunID
    if ("$detRunID" == "NULL") 
      echo "ERROR:detRun not defined"
      break
    end

    # what is the current state of the detRun?
    book getword detRun $detRunID state -var state

    # use any other state?
    if ("$state" == "stop")
      # update the state for this detRunName, drop detRun page
      book setword detRunDef $detRunName pantaskState DONE
      if ($VERBOSE > 1)
        echo "$detRunName is done"
      end
    else
      if ($VERBOSE > 1)
        echo "$detRunName is still running"
      end
    end

    # drop this book when completed
    book delete detRun
  end
end
