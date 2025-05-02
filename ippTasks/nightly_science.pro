## nightly_science.pro : -*- sh -*-

check.globals

macro nightly.science.on
    ns.initday.on
#    ns.detrends.off
    ns.dqstats.on
    ns.stacks.on
    ns.diffs.on
end

macro nightly.science.off
    ns.initday.off
#    ns.detrends.off
    ns.dqstats.off
    ns.stacks.off
    ns.diffs.off
end

macro ns.on
    nightly.science.on
end

macro ns.off
    nightly.science.off
end

macro ns.initday.on
  task ns.initday.load
    active true
  end
end


macro ns.dqstats.on
  task ns.dqstats.load
    active true
  end
end

macro ns.stacks.on
  task ns.stacks.run
    active true
  end
end

macro ns.diffs.on
  task ns.diffs.run
    active true
  end
end

macro ns.initday.off
  task ns.initday.load
    active false
  end
end

macro ns.dqstats.off
  task ns.dqstats.load
    active false
  end
end

macro ns.stacks.off
  task ns.stacks.run
    active false
  end
end

macro ns.diffs.off
  task ns.diffs.run
    active false
  end
end


$ns_stackPAGE = 0
$ns_RstackPAGE = 0
$ns_CstackPAGE = 0
$ns_diffPAGE = 0
$ns_RdiffPAGE = 0

book init nsData
book init nsDiffs
book init nsStacks

# Hard coded values 
$camera_map:gpc1 = "GPC1"
$camera_map:gpc2 = "GPC2"

# Macro to manage the camera maps
macro ns.add.camera_map
    if ($0 != 3)
	echo "USAGE: ns.add.camera_map (dbname) (camera)"
	break
    end

    $camera_map:$1 = $2
end

#
# Macros to control the book.
#
macro ns.add.date
  if ($0 != 4) 
    echo "USAGE: ns.add.date (YYYY-MM-DD) (dbname) (camera)"
    break
  end

   if ($?camera_map:$2) 
      if ($camera_map:$2 != "$3")
         echo "camera $3 does not match expected camera $camera_map:$2"
         break
      end
   else
      echo "dbname $2 does not have a camera_map entry"
      break
   end

   # build key from date and database name
   sprintf key "%s-%s" $1 $2
   book newpage nsData $key
   book setword nsData $key nsState NEW
   book setword nsData $key date $1
   book setword nsData $key dbname $2
   book setword nsData $key camera $3

   book newpage nsDiffs $key
   book setword nsDiffs $key date $1
   book setword nsDiffs $key nsDiffState TOWARP
   book setword nsDiffs $key nsObservingState UNKNOWN

   book newpage nsStacks $key
   book setword nsStacks $key date $1
   book setword nsStacks $key nsStackState TOWARP

end

macro ns.show.dates
    book npages nsData -var Npages
    for i 0 $Npages
       book getpage nsData $i -var key
       book getword nsData $key date -var date
       book getword nsData $key dbname -var dbname
       book getword nsData $key camera -var camera
       book getword nsData $key nsState -var state
       book getword nsStacks $key nsStackState -var Sstate
       book getword nsDiffs $key nsDiffState -var Dstate
       book getword nsDiffs $key nsObservingState -var Ostate
# I've moved dbname to end as czartool reads information from here, and it may be fixed length
       echo $key $date $state $Sstate $Dstate $Ostate $dbname $camera 

    end
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
$ns_init_DB = 0

task              ns.initday.load
  host            local
  periods         -poll $LOADPOLL
  periods         -exec $LOADEXEC
  periods         -timeout 300
  trange          0:00:00 1:00:00 -nmax 2
  npending        1

  task.exec
    stdout $LOGDIR/ns.initday.log
    stderr $LOGDIR/ns.initday.log
    $today = `date -u +%Y-%m-%d`
    # loop over all databases
#    for i 0 $DB:n
        $thisDB = $DB:$ns_init_DB

        sprintf key "%s-%s" $today $thisDB
        book newpage nsData $key
        book setword nsData $key date $today
        book setword nsData $key nsState NEW
        book setword nsData $key dbname $thisDB 
	book setword nsData $key camera $camera_map:$thisDB

        book newpage nsStacks $key
        book setword nsStacks $key date $today
        book setword nsStacks $key nsStackState TOWARP
        book newpage nsDiffs $key
        book setword nsDiffs $key date $today
        book setword nsDiffs $key nsDiffState TOWARP

	$ns_init_DB ++
	if ($ns_init_DB >= $DB:n) set ns_init_DB = 0	

       #command echo $today
       $run = nightly_science.pl --clean_old --date $today --dbname $thisDB --camera $camera_map:$thisDB
       command $run
#    end
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
# Queue dqstats runs
#

# CZW: 2015-05-05 One day we will stop making these, but that day is not today.  
#      According to Ken, we do not need to implement this for gpc2, as nothing is 
#      written on the OTIS/scheduler side to accept these products.  We should 
#      continue to generate these for gpc1, as the machinery does exist to check
#      for them, and not generating them may cause that to fail (even if nothing
#      is done with the results).

task              ns.dqstats.load
  host            local
  periods         -poll 20
  periods         -exec $LOADEXEC
  periods         -timeout 300
  trange          22:00:00 23:59:59
  trange          00:00:00 10:00:00
  npending        1

  task.exec
    stdout $LOGDIR/ns.dqstats.log
    stderr $LOGDIR/ns.dqstats.log
    $today = `date -u +%Y-%m-%d`

    $run = nightly_science.pl --queue_dqstats --date $today --dbname gpc1 --camera GPC1
#     add_standard_args $run
     command $run

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
# Check to see if the warps are finished and so we can make stacks.
#

task              ns.stacks.run
  host            local
  periods         -poll $LOADPOLL
  periods         -exec $LOADEXEC
  periods         -timeout 1000
  npending        1

  task.exec
     stdout NULL
     stderr $LOGDIR/ns.stacks.log

     book getpage nsData $ns_RstackPAGE -var key
     book getword nsStacks $key date -var date
     book getword nsStacks $key nsStackState -var ns_STATE
     book getword nsDiffs $key nsDiffState -var ns_diff_STATE
     book getword nsData $key dbname -var DBNAME
     book getword nsData $key camera -var CAMERA
     book npages nsData -var Npages

    if ($VERBOSE > 5) 
       echo "ns.stacks.run: " $ns_RstackPAGE $key $date $ns_STATE $Npages
    end

     $ns_RstackPAGE ++
     if ($ns_RstackPAGE >= $Npages) set ns_RstackPAGE = 0
     option $key

     if (("$ns_STATE" != "STACKING")&&("$ns_STATE" != "TOWARP")&&("$ns_STATE" != "FORCETOWARP")) break
     $run = nightly_science.pl --queue_stacks --date $date --dbname $DBNAME --camera $CAMERA
#     add_standard_args $run
     command $run
   end
  # success
  task.exit   0
#    convert 'stdout' to book format
    book delpage nsStacks $options:0
# We've queued up stacking. We're done with this date, so don't reload the page.
    ipptool2book stdout nsStacks -uniq -key key

    book getword nsStacks $options:0 nsNmacros -var ns_Nmacros
    if ("$ns_Nmacros" != "NULL") 
	for i 0 $ns_Nmacros
	    sprintf macroName "ns%dMacro" $i
	    book getword nsStacks $options:0 $macroName -var macroCmd
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
# Check to see if the warps are finished and so we can make diffs.
#

task              ns.diffs.run
  host            local
  periods         -poll $LOADPOLL
  periods         -exec $LOADEXEC
  periods         -timeout 900
  npending        1

  task.exec
     stdout NULL
     stderr $LOGDIR/ns.diffs.log

     book getpage nsData $ns_RdiffPAGE -var key
     book getword nsDiffs $key date -var date
     book getword nsDiffs $key nsDiffState -var ns_STATE
     book getword nsStacks $key nsStackState -var ns_stack_STATE
     book getword nsData $key dbname -var DBNAME
     book getword nsData $key camera -var CAMERA
     book npages nsData -var Npages

    if ($VERBOSE > 5) 
       echo "ns.diffs.run: " $ns_RdiffPAGE $date $ns_STATE $Npages
    end

     $ns_RdiffPAGE ++
     if ($ns_RdiffPAGE >= $Npages) set ns_RdiffPAGE = 0
     option $key

     if (("$ns_STATE" != "DIFFING")&&("$ns_STATE" != "QUEUEDIFFS")&&("$ns_STATE" != "TOWARP")&&("$ns_STATE" != "FORCETOWARP")) break
     $run = nightly_science.pl --queue_diffs --date $date --dbname $DBNAME --camera $CAMERA
#     add_standard_args $run
     command $run
   end
  # success
  task.exit   0
#    convert 'stdout' to book format
    book delpage nsDiffs $options:0
# We've queued up diffing. We're done with this date, so don't reload the page.
    ipptool2book stdout nsDiffs -uniq -key key

    book getword nsDiffs $options:0 nsNmacros -var ns_Nmacros
    if ("$ns_Nmacros" != "NULL") 
	for i 0 $ns_Nmacros
	    sprintf macroName "ns%dMacro" $i
	    book getword nsDiffs $options:0 $macroName -var macroCmd
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



# This is the graveyard of tasks that we've defined in the past, but either never use, or never let go active.

# if (0)


# macro ns.detrends.on
#   task ns.detrends.load
#     active true
#   end
# end


# macro ns.detrends.off
#   task ns.detrends.load
#     active false
#   end
# end

# macro ns.stacks.confirm.on
#   task ns.stacks.confirm
#     active true
#   end
# end

# macro ns.stacks.confirm.off
#   task ns.stacks.confirm
#     active false
#   end
# end





# #
# # Queue off any possible detrend verification runs
# #
# task              ns.detrends.load
#   host            local
#   periods         -poll $LOADPOLL
#   periods         -exec $LOADEXEC
#   periods         -timeout 30
#   trange          23:00:00 23:59:59 -nmax 1
#   npending        1

#   task.exec
#     stdout $LOGDIR/ns.detrends.log
#     stdout $LOGDIR/ns.detrends.log
#     $today = `date -u +%Y-%m-%d`

#     $run = nightly_science.pl --queue_detrends --date $today --dbname $DB:0
# #     add_standard_args $run
#      command $run
#   end
  
#   task.exit       0
#     # nothign to do here
#   end
#   # locked list
#   task.exit       default
#     showcommand failure
#   end
#   task.exit       crash
#     showcommand crash
#   end
#   # operation times out
#   task.exit       timeout
#     showcommand timeout
#   end
# end


# #
# # Check to see if the warps are finished
# #
# task              ns.stacks.load
#   active          false
#   host            local
#   periods         -poll $LOADPOLL
#   periods         -exec $LOADEXEC
#   periods         -timeout 120
#   npending        1

#   task.exec
#      stdout NULL
#      stderr $LOGDIR/ns.stacks.log

#      book getpage nsData $ns_stackPAGE -var key
#      book getword nsStacks $key date -var date
#      book getword nsStacks $key nsStackState -var ns_STATE
#      book getword nsDiffs $key nsDiffState -var ns_diff_STATE
#      book getword nsData $key dbname -var DBNAME
#      book npages nsData -var Npages

#     if ($VERBOSE > 5) 
#        echo "ns.stacks.load: " $ns_stackPAGE $date $DBNAME $ns_STATE $Npages
#     end

#      $ns_stackPAGE ++
#      if ($ns_stackPAGE >= $Npages) set ns_stackPAGE = 0
#      option $key

#      if ("$ns_STATE" != "TOWARP") break

#      $run = nightly_science.pl --check_stacks --date $date --dbname $DBNAME
# #     add_standard_args $run
#      command $run

#    end

#   # success
#   task.exit   0
# #    convert 'stdout' to book format
#     book delpage nsStacks $options:0
#     ipptool2book stdout nsStacks -uniq -key date

#     book getword nsStacks $options:0 nsNmacros -var ns_Nmacros
#     if ("$ns_Nmacros" != "NULL") 
# 	for i 0 $ns_Nmacros
# 	    sprintf macroName "ns%dMacro" $i
# 	    book getword nsStacks $options:0 $macroName -var macroCmd
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
# # Confirm that all the stacks that can be built have been built, or at least attempted.
# #
# task              ns.stacks.confirm
#   active          false
#   host            local
#   periods         -poll $LOADPOLL
#   periods         -exec $LOADEXEC
#   periods         -timeout 120
#   npending        1

#   task.exec
#      stdout NULL
#      stderr $LOGDIR/ns.stacks.log

#      book getpage nsData $ns_CstackPAGE -var key
#      book getword nsStacks $key date -var date
#      book getword nsStacks $key nsStackState -var ns_STATE
#      book getword nsData $key dbname -var DBNAME
#      book npages nsData -var Npages

#      if ($VERBOSE > 5)
# 	echo "ns.stacks.confirm: " $ns_CstackPAGE $key $date $DBNAME $ns_STATE $Npages
#      end

#      $ns_CstackPAGE ++
#      if ($ns_CstackPAGE >= $Npages) set ns_CstackPAGE = 0
#      option $key

#      if ("$ns_STATE" != "STACKING") break
#      $run = nightly_science.pl --confirm_stacks --date $date --dbname $DBNAME
# #     add_standard_args $run
#      command $run
#    end
#   # success
#   task.exit   0
# #    convert 'stdout' to book format
#     book delpage nsStacks $options:0
#     ipptool2book stdout nsStacks -uniq -key date

#     book getword nsStacks $options:0 nsNmacros -var ns_Nmacros
#     if ("$ns_Nmacros" != "NULL") 
# 	for i 0 $ns_Nmacros
# 	    sprintf macroName "ns%dMacro" $i
# 	    book getword nsStacks $options:0 $macroName -var macroCmd
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
# # Check to see if the warps are finished
# #
# task              ns.diffs.load
#   active          false
#   host            local
#   periods         -poll $LOADPOLL
#   periods         -exec $LOADEXEC
#   periods         -timeout 120
#   npending        1

#   task.exec
#      stdout NULL
#      stderr $LOGDIR/ns.diffs.log

#      book getpage nsData $ns_diffPAGE -var key
#      book getword nsDiffs $key date -var date
#      book getword nsDiffs $key nsDiffState -var ns_STATE
#      book getword nsStacks $key nsStackState -var ns_stack_STATE
#      book getword nsData $key dbname -var DBNAME
#      book npages nsData -var Npages

#     if ($VERBOSE > 5) 
#        echo "ns.diffs.load: " $ns_diffPAGE $key $date $DBNAME $ns_STATE $Npages
#     end

#      $ns_diffPAGE ++
#      if ($ns_diffPAGE >= $Npages) set ns_diffPAGE = 0
#      option $key

#      if ("$ns_STATE" != "TOWARP") break
#      $run = nightly_science.pl --check_diffs --date $date --dbname $DBNAME
# #     add_standard_args $run
#      command $run
#    end

#   # success
#   task.exit   0
# #    convert 'stdout' to book format
#     book delpage nsDiffs $options:0
#     ipptool2book stdout nsDiffs -uniq -key date

#     book getword nsDiffs $options:0 nsNmacros -var ns_Nmacros
#     if ("$ns_Nmacros" != "NULL") 
# 	for i 0 $ns_Nmacros
# 	    sprintf macroName "ns%dMacro" $i
# 	    book getword nsDiffs $options:0 $macroName -var macroCmd
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


# # END GRAVEYARD 
# end


# #
# # Queue sweetspot runs
# #
# task              ns.sweetspot.load
#   host            local
#   periods         -poll 3600
#   periods         -exec $LOADEXEC
#   periods         -timeout 30
#   trange          02:00:00 02:59:59 -nmax 1
#   npending        1

#   task.exec
#     stdout $LOGDIR/ns.sweetspot.log
#     stderr $LOGDIR/ns.sweetspot.log
#     $today = `date -u +%Y-%m-%d`

#     command nightly_science.pl --queue_sweetspot --date $today
#   end

#   task.exit       0
#     # nothing to do here
#   end
#   # locked list
#   task.exit       default
#     showcommand failure
#   end
#   task.exit       crash
#     showcommand crash
#   end
#   # operation times out
#   task.exit       timeout
#     showcommand timeout
#   end
# end

