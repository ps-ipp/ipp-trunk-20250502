## burntool.pro : -*- sh -*-
## This task is designed to only be used when re-burntooling data.  It is not necessary for standard registration.
check.globals

macro burntool.it.on
#    bt.initday.on
    bt.registration.on
    bt.burntool.on
    bt.chips.on
end

macro burntool.it.off
#    bt.initday.off
    bt.registration.off
    bt.burntool.off
    bt.chips.off
end

macro bt.on
    burntool.it.on
end

macro bt.off
    burntool.it.off
end

macro bt.initday.on
  task bt.initday.load
    active true
  end
end

macro bt.registration.on
  task bt.registration.load
    active true
  end
end

macro bt.burntool.on
  task bt.burntool.load
    active true
  end
  task bt.burntool.run
    active true
  end
end

macro bt.chips.on
  task bt.chips.load
    active true
  end
end

macro bt.initday.off
  task bt.initday.load
    active false
  end
end

macro bt.registration.off
  task bt.registration.load
    active false
  end
end

macro bt.burntool.off
  task bt.burntool.load
    active false
  end
  task bt.burntool.run
    active false
  end
end

macro bt.chips.off
  task bt.chips.load
    active false
  end
end

$bt_regPAGE = 0
$bt_burnPAGE = 0
$bt_RburnPAGE = 0
$bt_RburnCELL = 0
$bt_chipPAGE = 0
$bt_RchipPAGE = 0

book init btData
book init btBurntool
#
# Macros to control the book.
#
macro bt.add.date
   book newpage btData $1
   book setword btData $1 nsState NEW
end

macro bt.show.dates
    book npages btData -var Npages
    for i 0 $Npages
       book getpage btData $i -var date
       book getword btData $date nsState -var state
       echo $date $state
    end
    book npages btBurntool -var Npages
end

macro bt.del.date
    book delpage btData $1
end

macro bt.set.date
    book setword btData $1 nsState $2
end



#
# Start a new date to work on
#
task              bt.initday.load
  host            local
  periods         -poll $LOADPOLL
  periods         -exec $LOADEXEC
  periods         -timeout 30
  active          false
  npending        1

  task.exec
    stdout $LOGDIR/bt.initday.log
    stderr $LOGDIR/bt.initday.log
    $today = `date +%Y-%m-%d`
    book newpage btData $today
    book setword btData $today nsState NEW

    command echo $today
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

#
# Check to see if registration is complete
#
task              bt.registration.load
  host            local
  periods         -poll $LOADPOLL
  periods         -exec $LOADEXEC
  periods         -timeout 30
  active          false
  npending        1

  task.exec
    stdout NULL
    stderr $LOGDIR/bt.registration.log

    book getpage btData $bt_regPAGE -var date
    book getword btData $date nsState -var bt_STATE
    book npages btData -var Npages

    if ($VERBOSE > 5) 
       echo "bt.registration.load: " $bt_regPAGE $date $bt_STATE $Npages
    end

    $bt_regPAGE ++
    if ($bt_regPAGE >= $Npages) set bt_regPAGE = 0
    option $date

    if ("$bt_STATE" != "NEW") break
    $run = automate_stacks.pl --check_registration --date $date
    command $run
  end

  # success
  task.exit   0
#    convert 'stdout' to book format
    book delpage btData $options:0
    ipptool2book stdout btData -uniq -key date

    if ($VERBOSE > 2)
	book listbook btData
    end
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

#
# Check to see if the chips have been properly burntooled and are ready for queueing
#
task              bt.chips.load
  host            local
  periods         -poll $LOADPOLL
  periods         -exec $LOADEXEC
  periods         -timeout 300
  active          false
  npending        1

  task.exec
     stdout NULL
     stderr $LOGDIR/bt.chips.log

     book getpage btData $bt_chipPAGE -var date
     book getword btData $date nsState -var bt_STATE
     book npages btData -var Npages

     if ($VERBOSE > 5)
	echo "bt.chips.load: " $bt_chipPAGE $date $bt_STATE $Npages
     end

     $bt_chipPAGE ++
     if ($bt_chipPAGE >= $Npages) set bt_chipPAGE = 0
     option $date

     if (("$bt_STATE" != "REGISTERED")&&("$bt_STATE" != "BURNING")) break
     $run = automate_stacks.pl --check_chips --date $date

     if ("$bt_STATE" == "BURNING") 
	$run = $run --isburning
     end
     command $run
   end
  # success
  task.exit   0
#   convert 'stdout' to book format
    book delpage btData $options:0
    ipptool2book stdout btData -uniq -key date

    # remove the burntool page if we're done with it.
    book getword btData $options:0 nsState -var bt_STATE
    if ("$bt_STATE" == "QUEUECHIPS") 
	book delpage btBurntool $options:0
	book delpage btData $options:0
    end

    if ($VERBOSE > 2)
	book listbook btData
    end
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

#
# Generate a list of date ranges and chips that need to be processed with burntool
#
task              bt.burntool.load
  host            local
  periods         -poll $LOADPOLL
  periods         -exec $LOADEXEC
  periods         -timeout 30
  active          false
  npending        1

  task.exec
    stdout NULL
    stderr $LOGDIR/bt.burntool.log

    book getpage btData $bt_burnPAGE -var date
    book getword btData $date nsState -var bt_STATE
    book npages btData -var Npages

    if ($VERBOSE > 5) 
        echo "bt.burntool.load: " $bt_burnPAGE $date $bt_STATE $Npages
    end

    $bt_burnPAGE ++
    if ($bt_burnPAGE >= $Npages) set bt_burnPAGE = 0
    option $date
    
    if ("$bt_STATE" != "NEEDSBURNING") break

    $run = automate_stacks.pl --define_burntool --date $date
    command $run
  end
  # success
  task.exit   0
#    convert 'stdout' to book format
#    book delpage btBurntool $options:0
    ipptool2book stdout btBurntool -uniq -key date
    book setword btData $options:0 nsState "QUEUEBURNING"

    if ($VERBOSE > 2)
	book listbook btData
    end
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

#
# Magically run burntool on the data, based on the information stored in our book.
#
task              bt.burntool.run
  periods         -poll $RUNPOLL
  periods         -exec $RUNEXEC
  periods         -timeout 10800
  active          false
  npending        30
  task.exec
     periods -exec $RUNEXEC
     stdout NULL
     stderr $LOGDIR/bt.burntool.log

     book getpage btData $bt_RburnPAGE -var date
     book getword btData $date nsState -var bt_STATE
     book npages btData -var Npages

    if ($VERBOSE > 5) 
        echo "bt.burntool.run: " $bt_RburnPAGE $date $bt_STATE $Npages
    end

    $bt_RburnPAGE ++
    if ($bt_RburnPAGE >= $Npages) set bt_RburnPAGE = 0
    if (("$bt_STATE" != "QUEUEBURNING")&&("$bt_STATE" != "BURNING")) break
    $BURNTOOLING = 1
    # Find out where in the list of jobs we are
    book getword btBurntool $date btN -var btN
    book getword btBurntool $date btNCounter -var btNcounter

    if ($VERBOSE > 5) 
	echo "bt.burntool.run: Status: " $btNcounter $date
    end

    if ("$bt_STATE" == "QUEUEBURNING")
	$new_state = "QUEUEBURNING"
    end
    if ("$bt_STATE" == "BURNING")
	$new_state = "BURNING"
    end
    if ($btNcounter + 1 > $btN)
	$new_state = "BURNING"
    end
    if ($btNcounter > $btN) 
    	$btNcounter = 0
    end
    if ($VERBOSE > 5) 
	echo "bt.burntool.run: Status: " $btNcounter $new_state
    end
    
    # Increment the counter in the book for the next job.
    $counter_update = $btNcounter + 1
    book setword btBurntool $date btNCounter $counter_update
#    book setword btData $options:0 nsState $options:1
    book setword btData $date nsState $new_state

    # Get the current status of this job, and skip if it doesn't need to process.
    sprintf status_label     "bt%dStatus"  $btNcounter
    book getword btBurntool $date $status_label   -var status

    if ($VERBOSE > 5) 
	echo "bt.burntool.run: Status: " $btNcounter $status $date $status_label
    end
    if (("$status" == "FINISHED")||("$status" == "RUN")) break	
    
    # Continue loading information to process this job
    sprintf start_date_label "bt%dBegin"   $btNcounter
    sprintf end_date_label   "bt%dEnd"     $btNcounter
    sprintf class_count_label "bt%dClass"   $btNcounter

    book getword btBurntool $date $start_date_label -var start_date
    book getword btBurntool $date $end_date_label -var end_date
    book getword btBurntool $date $class_count_label -var chip_counter

    # Lookup class_id/host pairs
    list word -split $hostmatch:$chip_counter
    $class_id = $word:0
    $host = $word:1
    host $host
#    set.host.for.camera GPC1 $class_id
    $logfile = "/data/$host.0/burntool_logs/$class_id.$start_date.log"

    $run = ipp_apply_burntool.pl --class_id $class_id --dateobs_begin $start_date --dateobs_end $end_date --dbname gpc1 --logfile $logfile
	
    echo "bt.burntool.run: " $date $btN $btNcounter $start_date $end_date $chip_counter $class_id $host $logfile $run
    echo "bt.burntool.run: " $date $btN $btNcounter $chip_counter $new_state

    book setword btBurntool $date $status_label RUN
    option $date $new_state $status_label

    command $run
    periods -exec 0.05
#     command /bin/true
   end
  # success
  task.exit   0
#    convert 'stdout' to book format
    # Set data state based on if we're queueing or waiting
    # Set the job state for success.
    book setword btBurntool $options:0 $options:2 FINISHED
    if ($VERBOSE > 2)
	book listbook btData
    end
  end

  # locked list
  task.exit    default
    book setword btBurntool $options:0 $options:2 FAIL
    showcommand failure
  end
  task.exit    crash
    book setword btBurntool $options:0 $options:2 FAIL
    showcommand crash
  end
  #operation times out?
  task.exit    timeout
    book setword btBurntool $options:0 $option:2 FAIL
    showcommand timeout
  end
end


#
# This is all burntool related stuff.
#

macro burntool
  if ($0 != 3)
    echo "USAGE: burntool (dateobs_begin) (dateobs_end)"
    break
  end

  for i 0 $hostmatch:n
    job -host $word:1 ipp_apply_burntool.pl --class_id $class_id --dateobs_begin $1 --dateobs_end $2 --dbname gpc1 --logfile $logfile
  end
end
macro loadhosts
  for i 0 $allhosts:n
    controller host add $allhosts:$i
  end
end

# sorry this list is messy, it's supposed to be that way to try and keep to burntools from running on the same host at the same time.

list hostmatch
  XY01    ipp014
  XY03    ipp038
  XY05    ipp023
  XY10    ipp039
  XY12    ipp024
  XY15    ipp040
  XY17    ipp020
  XY21    ipp041
  XY23    ipp042
  XY25    ipp043
  XY27    ipp028
  XY31    ipp044
  XY33    ipp029
  XY35    ipp045
  XY37    ipp030
  XY41    ipp046
  XY43    ipp031
  XY45    ipp047
  XY47    ipp032
  XY51    ipp048
  XY53    ipp033
  XY55    ipp049
  XY57    ipp034
  XY61    ipp050
  XY63    ipp035
  XY65    ipp051
  XY67    ipp036
  XY72    ipp052
  XY74    ipp015
  XY76    ipp053
  XY02    ipp014
  XY04    ipp038
  XY06    ipp023
  XY11    ipp039
  XY13    ipp024
  XY14    ipp040
  XY16    ipp020
  XY20    ipp041
  XY22    ipp042
  XY24    ipp043 
  XY26    ipp028
  XY30    ipp044
  XY32    ipp029
  XY34    ipp045
  XY36    ipp030
  XY40    ipp046
  XY42    ipp031
  XY44    ipp047
  XY46    ipp032
  XY50    ipp048
  XY52    ipp033
  XY54    ipp049
  XY56    ipp034
  XY60    ipp050
  XY62    ipp035
  XY64    ipp051
  XY66    ipp036
  XY71    ipp052
  XY73    ipp015
  XY75    ipp053
end

list allhosts
  ipp043
  ipp014
  ipp015
  ipp023
  ipp024
  ipp020
  ipp028
  ipp029
  ipp030
  ipp031
  ipp032
  ipp033
  ipp034
  ipp035
  ipp036
  ipp038
  ipp039
  ipp040
  ipp041
  ipp042
  ipp044
  ipp045
  ipp046
  ipp047
  ipp048
  ipp049
  ipp050
  ipp051
  ipp052
  ipp053
end
