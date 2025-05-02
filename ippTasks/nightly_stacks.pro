## nightly_stacks.pro : -*- sh -*-

check.globals

macro nightly.stacks.on
    ns.initday.on
    ns.detrends.off
    ns.dqstats.on
#    ns.sweetspot.on
#    ns.registration.on
#    ns.burntool.on
#    ns.chips.on
    ns.stacks.on
    ns.diffs.on
end

macro nightly.stacks.off
    ns.initday.off
    ns.detrends.off
    ns.dqstats.off
#    ns.sweetspot.off
#    ns.registration.off
#    ns.burntool.off
#    ns.chips.off
    ns.stacks.off
    ns.diffs.off
end

macro ns.on
    nightly.stacks.on
end

macro ns.off
    nightly.stacks.off
end

macro ns.initday.on
  task ns.initday.load
    active true
  end
end

macro ns.detrends.on
  task ns.detrends.load
    active true
  end
end

macro ns.dqstats.on
  task ns.dqstats.load
    active true
  end
end

# macro ns.sweetspot.on
#   task ns.sweetspot.load
#     active true
#   end
# end

# macro ns.registration.on
#   task ns.registration.load
#     active true
#   end
# end

# macro ns.burntool.on
#   task ns.burntool.load
#     active true
#   end
#   task ns.burntool.run
#     active true
#   end
# end

# macro ns.chips.on
#   task ns.chips.load
#     active true
#   end
#   task ns.chips.run
#     active true
#   end
# end

macro ns.stacks.on
  task ns.stacks.load
    active false
  end
  task ns.stacks.run
    active true
  end
  task ns.stacks.confirm
    active true
  end
end

macro ns.diffs.on
  task ns.diffs.load
    active false
  end
  task ns.diffs.run
    active true
  end
end

macro ns.initday.off
  task ns.initday.load
    active false
  end
end

macro ns.detrends.off
  task ns.detrends.load
    active false
  end
end

macro ns.dqstats.off
  task ns.dqstats.load
    active false
  end
end

# macro ns.sweetspot.off
#   task ns.sweetspot.load
#     active false
#   end
# end

# macro ns.registration.off
#   task ns.registration.load
#     active false
#   end
# end

# macro ns.burntool.off
#   task ns.burntool.load
#     active false
#   end
#   task ns.burntool.run
#     active false
#   end
# end

# macro ns.chips.off
#   task ns.chips.load
#     active false
#   end
#   task ns.chips.run
#     active false
#   end
# end

macro ns.stacks.off
  task ns.stacks.load
    active false
  end
  task ns.stacks.run
    active false
  end
end

macro ns.diffs.off
  task ns.diffs.load
    active false
  end
  task ns.diffs.run
    active false
  end
end

# $ns_regPAGE = 0
# $ns_burnPAGE = 0
# $ns_RburnPAGE = 0
# $ns_RburnCELL = 0
# $ns_chipPAGE = 0
# $ns_RchipPAGE = 0
$ns_stackPAGE = 0
$ns_RstackPAGE = 0
$ns_CstackPAGE = 0
$ns_diffPAGE = 0
$ns_RdiffPAGE = 0

book init nsData
# book init nsBurntool
#
# Macros to control the book.
#
macro ns.add.date
   book newpage nsData $1
   book setword nsData $1 nsState NEW
end

macro ns.show.dates
    book npages nsData -var Npages
    for i 0 $Npages
       book getpage nsData $i -var date
       book getword nsData $date nsState -var state
       echo $date $state
    end
#    book npages nsBurntool -var Npages
end

macro ns.del.date
    book delpage nsData $1
end

macro ns.set.date
    book setword nsData $1 nsState $2
end



#
# Start a new date to work on
#
task              ns.initday.load
  host            local
  periods         -poll $LOADPOLL
  periods         -exec $LOADEXEC
  periods         -timeout 30
  trange          0:00:00 1:00:00 -nmax 1
  npending        1

  task.exec
    stdout $LOGDIR/ns.initday.log
    stderr $LOGDIR/ns.initday.log
    $today = `date -u +%Y-%m-%d`
    book newpage nsData $today
    book setword nsData $today nsState NEW

#    command echo $today
   command automate_stacks.pl --clean_old --date $today
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
# Queue off any possible detrend verification runs
#
task              ns.detrends.load
  host            local
  periods         -poll $LOADPOLL
  periods         -exec $LOADEXEC
  periods         -timeout 30
  trange          23:00:00 23:59:59 -nmax 1
  npending        1

  task.exec
    stdout $LOGDIR/ns.detrends.log
    stdout $LOGDIR/ns.detrends.log
    $today = `date -u +%Y-%m-%d`

    command automate_stacks.pl --queue_detrends --date $today
  end
  
  task.exit       0
    # nothign to do here
  end
  # locked list
  task.exit       default
    showcommand failure
  end
  task.exit       crash
    showcommand crash
  end
  # operation times out
  task.exit       timeout
    showcommand timeout
  end
end

#
# Queue dqstats runs
#
task              ns.dqstats.load
  host            local
  periods         -poll 10
  periods         -exec $LOADEXEC
  periods         -timeout 600
  trange          22:00:00 23:59:59
  trange          00:00:00 10:00:00
  npending        1

  task.exec
    stdout $LOGDIR/ns.dqstats.log
    stderr $LOGDIR/ns.dqstats.log
    $today = `date -u +%Y-%m-%d`

    command automate_stacks.pl --queue_dqstats --date $today
  end

  task.exit       0
    # nothing to do here
  end
  # locked list
  task.exit       default
    showcommand failure
  end
  task.exit       crash
    showcommand crash
  end
  # operation times out
  task.exit       timeout
    showcommand timeout
  end
end

#
# Queue sweetspot runs
#
task              ns.sweetspot.load
  host            local
  periods         -poll 3600
  periods         -exec $LOADEXEC
  periods         -timeout 30
  trange          02:00:00 02:59:59 -nmax 1
  npending        1

  task.exec
    stdout $LOGDIR/ns.sweetspot.log
    stderr $LOGDIR/ns.sweetspot.log
    $today = `date -u +%Y-%m-%d`

    command automate_stacks.pl --queue_sweetspot --date $today
  end

  task.exit       0
    # nothing to do here
  end
  # locked list
  task.exit       default
    showcommand failure
  end
  task.exit       crash
    showcommand crash
  end
  # operation times out
  task.exit       timeout
    showcommand timeout
  end
end

#
# Check to see if registration is complete
#
# task              ns.registration.load
#   host            local
#   periods         -poll $LOADPOLL
#   periods         -exec $LOADEXEC
#   periods         -timeout 30
#   npending        1

#   task.exec
#     stdout NULL
#     stderr $LOGDIR/ns.registration.log

#     book getpage nsData $ns_regPAGE -var date
#     book getword nsData $date nsState -var ns_STATE
#     book npages nsData -var Npages

#     if ($VERBOSE > 5) 
#        echo "ns.registration.load: " $ns_regPAGE $date $ns_STATE $Npages
#     end

#     $ns_regPAGE ++
#     if ($ns_regPAGE >= $Npages) set ns_regPAGE = 0
#     option $date

#     if ("$ns_STATE" != "NEW") break
#     $run = automate_stacks.pl --check_registration --date $date
#     command $run
#   end

#   # success
#   task.exit   0
# #    convert 'stdout' to book format
#     book delpage nsData $options:0
#     ipptool2book stdout nsData -uniq -key date

#     if ($VERBOSE > 2)
# 	book listbook nsData
#     end
#   end

#   # locked list
#   task.exit    default
#     showcommand failure
#   end
#   task.exit    crash
#     showcommand crash
#   end
#   #operation times out?
#   task.exit    timeout
#     showcommand timeout
#   end
# end

# #
# # Check to see if the chips have been properly burntooled and are ready for queueing
# #
# task              ns.chips.load
#   host            local
#   periods         -poll $LOADPOLL
#   periods         -exec $LOADEXEC
#   periods         -timeout 300
#   npending        1

#   task.exec
#      stdout NULL
#      stderr $LOGDIR/ns.chips.log

#      book getpage nsData $ns_chipPAGE -var date
#      book getword nsData $date nsState -var ns_STATE
#      book npages nsData -var Npages

#      if ($VERBOSE > 5)
# 	echo "ns.chips.load: " $ns_chipPAGE $date $ns_STATE $Npages
#      end

#      $ns_chipPAGE ++
#      if ($ns_chipPAGE >= $Npages) set ns_chipPAGE = 0
#      option $date

#      if (("$ns_STATE" != "REGISTERED")&&("$ns_STATE" != "BURNING")) break
#      $run = automate_stacks.pl --check_chips --date $date

#      if ("$ns_STATE" == "BURNING") 
# 	$run = $run --isburning
#      end
#      command $run
#    end
#   # success
#   task.exit   0
# #   convert 'stdout' to book format
#     book delpage nsData $options:0
#     ipptool2book stdout nsData -uniq -key date

#     # remove the burntool page if we're done with it.
#     book getword nsData $options:0 nsState -var ns_STATE
#     if ("$ns_STATE" == "QUEUECHIPS") 
# 	book delpage nsBurntool $options:0
#     end

#     book getword nsData $options:0 nsNmacros -var ns_Nmacros
#     if ("$ns_Nmacros" != "NULL") 
# 	for i 0 $ns_Nmacros
# 	    sprintf macroName "ns%dMacro" $i
# 	    book getword nsData $options:0 $macroName -var macroCmd
# 	    $macroCmd
# 	end
#     end

#     if ($VERBOSE > 2)
# 	book listbook nsData
#     end
#   end

#   # locked list
#   task.exit    default
#     showcommand failure
#   end
#   task.exit    crash
#     showcommand crash
#   end
#   #operation times out?
#   task.exit    timeout
#     showcommand timeout
#   end
# end

# #
# # Check to see if the chips have been properly burntooled and are ready for queueing
# #
# task              ns.chips.run
#   host            local
#   periods         -poll $LOADPOLL
#   periods         -exec $LOADEXEC
#   periods         -timeout 120
#   npending        1

#   task.exec
#      stdout NULL
#      stderr $LOGDIR/ns.chips.log

#      book getpage nsData $ns_RchipPAGE -var date
#      book getword nsData $date nsState -var ns_STATE
#      book npages nsData -var Npages

#     if ($VERBOSE > 5) 
#        echo "ns.chips.run: " $ns_RchipPAGE $date $ns_STATE $Npages
#     end
     
#      $ns_RchipPAGE ++
#      if ($ns_RchipPAGE >= $Npages) set ns_RchipPAGE = 0
#      option $date

#      if ("$ns_STATE" != "QUEUECHIPS") break
#      $BURNTOOLING = 0
#      $run = automate_stacks.pl --queue_chips --date $date
#      command $run
#    end
#   # success
#   task.exit   0
# #    convert 'stdout' to book format
#     book delpage nsData $options:0
#     ipptool2book stdout nsData -uniq -key date

#     book getword nsData $options:0 nsNmacros -var ns_Nmacros
#     if ("$ns_Nmacros" != "NULL") 
# 	for i 0 $ns_Nmacros
# 	    sprintf macroName "ns%dMacro" $i
# 	    book getword nsData $options:0 $macroName -var macroCmd
# 	    $macroCmd
# 	end
#     end

#     if ($VERBOSE > 2)
# 	book listbook nsData
#     end
#   end

#   # locked list
#   task.exit    default
#     showcommand failure
#   end
#   task.exit    crash
#     showcommand crash
#   end
#   #operation times out?
#   task.exit    timeout
#     showcommand timeout
#   end
# end


#
# Check to see if the warps are finished
#
task              ns.stacks.load
  host            local
  periods         -poll $LOADPOLL
  periods         -exec $LOADEXEC
  periods         -timeout 120
  npending        1

  task.exec
     stdout NULL
     stderr $LOGDIR/ns.stacks.log

     book getpage nsData $ns_stackPAGE -var date
     book getword nsData $date nsState -var ns_STATE
     book npages nsData -var Npages

    if ($VERBOSE > 5) 
       echo "ns.stacks.load: " $ns_stackPAGE $date $ns_STATE $Npages
    end

     $ns_stackPAGE ++
     if ($ns_stackPAGE >= $Npages) set ns_stackPAGE = 0
     option $date

     if ("$ns_STATE" != "TOWARP") break
     $run = automate_stacks.pl --check_stacks --date $date
     command $run
   end

  # success
  task.exit   0
#    convert 'stdout' to book format
    book delpage nsData $options:0
    ipptool2book stdout nsData -uniq -key date

    book getword nsData $options:0 nsNmacros -var ns_Nmacros
    if ("$ns_Nmacros" != "NULL") 
	for i 0 $ns_Nmacros
	    sprintf macroName "ns%dMacro" $i
	    book getword nsData $options:0 $macroName -var macroCmd
	    $macroCmd
	end
    end

    if ($VERBOSE > 2)
	book listbook nsData
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
# Check to see if the warps are finished and so we can make stacks.
#

task              ns.stacks.run
  host            local
  periods         -poll $LOADPOLL
  periods         -exec $LOADEXEC
  periods         -timeout 900 # this task can be quite slow
  npending        1

  task.exec
     stdout NULL
     stderr $LOGDIR/ns.stacks.log

     book getpage nsData $ns_RstackPAGE -var date
     book getword nsData $date nsState -var ns_STATE
     book npages nsData -var Npages

    if ($VERBOSE > 5) 
       echo "ns.stacks.run: " $ns_RstackPAGE $date $ns_STATE $Npages
    end

     $ns_RstackPAGE ++
     if ($ns_RstackPAGE >= $Npages) set ns_RstackPAGE = 0
     option $date

     if (("$ns_STATE" != "QUEUESTACKS")&&("$ns_STATE" != "TOWARP")&&("$ns_STATE" != "FORCETOWARP")) break
     $run = automate_stacks.pl --queue_stacks --date $date
     command $run
   end
  # success
  task.exit   0
#    convert 'stdout' to book format
    book delpage nsData $options:0
# We've queued up stacking. We're done with this date, so don't reload the page.
    ipptool2book stdout nsData -uniq -key date

    book getword nsData $options:0 nsNmacros -var ns_Nmacros
    if ("$ns_Nmacros" != "NULL") 
	for i 0 $ns_Nmacros
	    sprintf macroName "ns%dMacro" $i
	    book getword nsData $options:0 $macroName -var macroCmd
	    $macroCmd
	end
    end

    if ($VERBOSE > 2)
	book listbook nsData
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
# Confirm that all the stacks that can be built have been built, or at least attempted.
#
task              ns.stacks.confirm
  host            local
  periods         -poll $LOADPOLL
  periods         -exec $LOADEXEC
  periods         -timeout 120
  npending        1

  task.exec
     stdout NULL
     stderr $LOGDIR/ns.stacks.log

     book getpage nsData $ns_CstackPAGE -var date
     book getword nsData $date nsState -var ns_STATE
     book npages nsData -var Npages

     if ($VERBOSE > 5)
	echo "ns.stacks.confirm: " $ns_CstackPAGE $date $ns_STATE $Npages
     end

     $ns_CstackPAGE ++
     if ($ns_CstackPAGE >= $Npages) set ns_CstackPAGE = 0
     option $date

     if ("$ns_STATE" != "STACKING") break
     $run = automate_stacks.pl --confirm_stacks --date $date
     command $run
   end
  # success
  task.exit   0
#    convert 'stdout' to book format
    book delpage nsData $options:0
    ipptool2book stdout nsData -uniq -key date

    book getword nsData $options:0 nsNmacros -var ns_Nmacros
    if ("$ns_Nmacros" != "NULL") 
	for i 0 $ns_Nmacros
	    sprintf macroName "ns%dMacro" $i
	    book getword nsData $options:0 $macroName -var macroCmd
	    $macroCmd
	end
    end

    if ($VERBOSE > 2)
	book listbook nsData
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

# #
# # Generate a list of date ranges and chips that need to be processed with burntool
# #
# task              ns.burntool.load
#   host            local
#   periods         -poll $LOADPOLL
#   periods         -exec $LOADEXEC
#   periods         -timeout 30
#   npending        1

#   task.exec
#     stdout NULL
#     stderr $LOGDIR/ns.burntool.log

#     book getpage nsData $ns_burnPAGE -var date
#     book getword nsData $date nsState -var ns_STATE
#     book npages nsData -var Npages

#     if ($VERBOSE > 5) 
#         echo "ns.burntool.load: " $ns_burnPAGE $date $ns_STATE $Npages
#     end

#     $ns_burnPAGE ++
#     if ($ns_burnPAGE >= $Npages) set ns_burnPAGE = 0
#     option $date
    
#     if ("$ns_STATE" != "NEEDSBURNING") break

#     $run = automate_stacks.pl --define_burntool --date $date
#     command $run
#   end
#   # success
#   task.exit   0
# #    convert 'stdout' to book format
# #    book delpage nsBurntool $options:0
#     ipptool2book stdout nsBurntool -uniq -key date
#     book setword nsData $options:0 nsState "QUEUEBURNING"

#     book getword nsData $options:0 nsNmacros -var ns_Nmacros
#     if ("$ns_Nmacros" != "NULL") 
# 	for i 0 $ns_Nmacros
# 	    sprintf macroName "ns%dMacro" $i
# 	    book getword nsData $options:0 $macroName -var macroCmd
# 	    $macroCmd
# 	end
#     end

#     if ($VERBOSE > 2)
# 	book listbook nsData
#     end
#   end

#   # locked list
#   task.exit    default
#     showcommand failure
#   end
#   task.exit    crash
#     showcommand crash
#   end
#   #operation times out?
#   task.exit    timeout
#     showcommand timeout
#   end
# end

# #
# # Magically run burntool on the data, based on the information stored in our book.
# #
# task              ns.burntool.run
#   periods         -poll $LOADPOLL
#   periods         -exec $LOADEXEC
#   periods         -timeout 10800
#   npending        30
#   task.exec
#      stdout NULL
#      stderr $LOGDIR/ns.burntool.log

#      book getpage nsData $ns_RburnPAGE -var date
#      book getword nsData $date nsState -var ns_STATE
#      book npages nsData -var Npages

#     if ($VERBOSE > 5) 
#         echo "ns.burntool.run: " $ns_RburnPAGE $date $ns_STATE $Npages
#     end

#     $ns_RburnPAGE ++
#     if ($ns_RburnPAGE >= $Npages) set ns_RburnPAGE = 0
#     if (("$ns_STATE" != "QUEUEBURNING")&&("$ns_STATE" != "BURNING")) break
#     $BURNTOOLING = 1
#     # Find out where in the list of jobs we are
#     book getword nsBurntool $date btN -var btN
#     book getword nsBurntool $date btNCounter -var btNcounter

#     if ($VERBOSE > 5) 
# 	echo "ns.burntool.run: Status: " $btNcounter $date
#     end

#     if ("$ns_STATE" == "QUEUEBURNING")
# 	$new_state = "QUEUEBURNING"
#     end
#     if ("$ns_STATE" == "BURNING")
# 	$new_state = "BURNING"
#     end
#     if ($btNcounter + 1 > $btN)
# 	$new_state = "BURNING"
#     end
#     if ($btNcounter > $btN) 
#     	$btNcounter = 0
#     end
#     if ($VERBOSE > 5) 
# 	echo "ns.burntool.run: Status: " $btNcounter $new_state
#     end
    
#     # Increment the counter in the book for the next job.
#     $counter_update = $btNcounter + 1
#     book setword nsBurntool $date btNCounter $counter_update
# #    book setword nsData $options:0 nsState $options:1
#     book setword nsData $date nsState $new_state

#     # Get the current status of this job, and skip if it doesn't need to process.
#     sprintf status_label     "bt%dStatus"  $btNcounter
#     book getword nsBurntool $date $status_label   -var status

#     if ($VERBOSE > 5) 
# 	echo "ns.burntool.run: Status: " $btNcounter $status $date $status_label
#     end
#     if (("$status" == "FINISHED")||("$status" == "RUN")) break	
    
#     # Continue loading information to process this job
#     sprintf start_date_label "bt%dBegin"   $btNcounter
#     sprintf end_date_label   "bt%dEnd"     $btNcounter
#     sprintf class_count_label "bt%dClass"   $btNcounter

#     book getword nsBurntool $date $start_date_label -var start_date
#     book getword nsBurntool $date $end_date_label -var end_date
#     book getword nsBurntool $date $class_count_label -var chip_counter

#     # Lookup class_id/host pairs
#     list word -split $hostmatch:$chip_counter
#     $class_id = $word:0
#     $host = $word:1
#     host $host
# #    set.host.for.camera GPC1 $class_id
#     $logfile = "/data/$host.0/burntool_logs/$class_id.$start_date.log"

#     $run = ipp_apply_burntool.pl --class_id $class_id --dateobs_begin $start_date --dateobs_end $end_date --dbname gpc1 --logfile $logfile
	
#     echo "ns.burntool.run: " $date $btN $btNcounter $start_date $end_date $chip_counter $class_id $host $logfile $run
#     echo "ns.burntool.run: " $date $btN $btNcounter $chip_counter $new_state

#     book setword nsBurntool $date $status_label RUN
#     option $date $new_state $status_label

#     command $run
# #     command /bin/true
#    end
#   # success
#   task.exit   0
# #    convert 'stdout' to book format
#     # Set data state based on if we're queueing or waiting
#     # Set the job state for success.
#     book setword nsBurntool $options:0 $options:2 FINISHED

#     book getword nsData $options:0 nsNmacros -var ns_Nmacros
#     if ("$ns_Nmacros" != "NULL") 
# 	for i 0 $ns_Nmacros
# 	    sprintf macroName "ns%dMacro" $i
# 	    book getword nsData $options:0 $macroName -var macroCmd
# 	    $macroCmd
# 	end
#     end

#     if ($VERBOSE > 2)
# 	book listbook nsData
#     end
#   end

#   # locked list
#   task.exit    default
#     book setword nsBurntool $options:0 $options:2 FAIL
#     showcommand failure
#   end
#   task.exit    crash
#     book setword nsBurntool $options:0 $options:2 FAIL
#     showcommand crash
#   end
#   #operation times out?
#   task.exit    timeout
#     book setword nsBurntool $options:0 $options:2 FAIL
#     showcommand timeout
#   end
# end


# #
# # This is all burntool related stuff.
# #

# macro burntool
#   if ($0 != 3)
#     echo "USAGE: burntool (dateobs_begin) (dateobs_end)"
#     break
#   end

#   for i 0 $hostmatch:n
#     job -host $word:1 ipp_apply_burntool.pl --class_id $class_id --dateobs_begin $1 --dateobs_end $2 --dbname gpc1 --logfile $logfile
#   end
# end
# macro loadhosts
#   for i 0 $allhosts:n
#     controller host add $allhosts:$i
#   end
# end

# # sorry this list is messy, it's supposed to be that way to try and keep to burntools from running on the same host at the same time.

# list hostmatch
#   XY01    ipp014
#   XY03    ipp038
#   XY05    ipp023
#   XY10    ipp039
#   XY12    ipp024
#   XY15    ipp040
#   XY17    ipp020
#   XY21    ipp041
#   XY23    ipp042
#   XY25    ipp043
#   XY27    ipp028
#   XY31    ipp044
#   XY33    ipp029
#   XY35    ipp045
#   XY37    ipp030
#   XY41    ipp046
#   XY43    ipp031
#   XY45    ipp047
#   XY47    ipp032
#   XY51    ipp048
#   XY53    ipp033
#   XY55    ipp049
#   XY57    ipp034
#   XY61    ipp050
#   XY63    ipp035
#   XY65    ipp051
#   XY67    ipp036
#   XY72    ipp052
#   XY74    ipp015
#   XY76    ipp053
#   XY02    ipp014
#   XY04    ipp038
#   XY06    ipp023
#   XY11    ipp039
#   XY13    ipp024
#   XY14    ipp040
#   XY16    ipp020
#   XY20    ipp041
#   XY22    ipp042
#   XY24    ipp043 
#   XY26    ipp028
#   XY30    ipp044
#   XY32    ipp029
#   XY34    ipp045
#   XY36    ipp030
#   XY40    ipp046
#   XY42    ipp031
#   XY44    ipp047
#   XY46    ipp032
#   XY50    ipp048
#   XY52    ipp033
#   XY54    ipp049
#   XY56    ipp034
#   XY60    ipp050
#   XY62    ipp035
#   XY64    ipp051
#   XY66    ipp036
#   XY71    ipp052
#   XY73    ipp015
#   XY75    ipp053
# end

# list allhosts
#   ipp043
#   ipp014
#   ipp015
#   ipp023
#   ipp024
#   ipp020
#   ipp028
#   ipp029
#   ipp030
#   ipp031
#   ipp032
#   ipp033
#   ipp034
#   ipp035
#   ipp036
#   ipp038
#   ipp039
#   ipp040
#   ipp041
#   ipp042
#   ipp044
#   ipp045
#   ipp046
#   ipp047
#   ipp048
#   ipp049
#   ipp050
#   ipp051
#   ipp052
#   ipp053
# end
