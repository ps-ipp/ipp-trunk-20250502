## lap.pro : -*- sh -*-

check.globals

$lapSeq:n = 0
$lap_N = 0
$lap_DB = 0

$lap_NewPage = 0
$lap_RunPage = 0
$lap_DonePage = 0

$LAP_QUEUE = NULL

book init lapNewRuns
book init lapRunRuns
book init lapFullRuns
book init lapDoneRuns

macro lap.show.books
    echo "lapNewRuns"
    book listbook lapNewRuns
    echo "lapRunRuns"
    book listbook lapRunRuns
    echo "lapDoneRuns"
    book listbook lapDoneRuns
    echo "lapFullRuns"
    book listbook lapFullRuns
end

macro lap.clear.books
    book init lapNewRuns
    book init lapRunRuns
    book init lapFullRuns
    book init lapDoneRuns
end

macro lap.on
    task lap.initial.load
      active true
    end
    task lap.initial.run
      active true
    end
    task lap.monitor.load
      active true
    end
    task lap.monitor.run
      active true
    end
end

macro lap.off
    task lap.initial.load
      active false
    end
    task lap.initial.run
      active false
    end
    task lap.monitor.load
      active false
    end
    task lap.monitor.run
      active false
    end
end

macro lap.debug.mode
    task lap.initial.load
      active true
    end
    task lap.initial.run
      active false
    end
    task lap.monitor.load
      active true
    end
    task lap.monitor.run
      active false
    end
end

macro lap.cleanup.on
    task lap.cleanup.load
      active true
    end
    task lap.cleanup.run
      active true
    end
end

macro lap.cleanup.off
    task lap.cleanup.load
      active false
    end
    task lap.cleanup.run
      active false
    end
end

macro lap.initial.on
    task lap.initial.load
      active true
    end
    task lap.initial.run
      active true
    end
end

macro lap.initial.off
    task lap.initial.load
      active false
    end
    task lap.initial.run
      active false
    end
end

macro lap.add.sequence
  if ($0 != 2) 
    echo "USAGE: lap.add.sequence (seq_id)"
    break
  end
  if ($?lapSeq:n == 0) 
    list lapSeq -add $1
    return
  end

  local found
  $found = 0
  for i 0 $lapSeq:n
    if ($lapSeq:$i == $1)
      $found = 1
      echo "$lapSeq:$i set"
      last
    end
  end

  if ($found == 0)
    list lapSeq -add $1
  end
end

macro lap.del.sequence
  if ($0 != 2)
    echo "USAGE: lap.del.sequence (seq_id)"
    break
  end
  if ($?lapSeq:n == 0)
    return
  end

  list lapSeq -del $1
end
 

macro lap.define.queue
  $LAP_QUEUE = $1
end


task           lap.initial.load
  host         local
  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  task.exec
    stdout NULL
    stderr $LOGDIR/lap.load.log

    $run = laptool -pendingrun -state new

    if ($lapSeq:n == 0)
      break
    else 
      option $lapSeq:$lap_N
      $run = $run -seq_id $lapSeq:$lap_N
      $lap_N ++
      if ($lap_N >= $lapSeq:n) set lap_N = 0
    end

    if ($DB:n == 0)
      option DEFAULT
    else
      option $DB:$lap_DB
      $run = $run -dbname $DB:$lap_DB
      $lap_DB ++
      if ($lap_DB >= $DB:n) set lap_DB = 0
    end

    command $run
  end
  # success
  task.exit  0
    ipptool2book stdout lapNewRuns -uniq -key lap_id -setword dbname $options:0 -setword pantaskState INIT

    process_cleanup lapNewRuns

    if ($VERBOSE > 2)
      book listbook lapNewRuns
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

task           lap.initial.run
  host         local
  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 3600
  active       false
# This can probably be increased and spread over hosts in the future.
  npending     1            

  task.exec
    stdout $LOGDIR/lap.initial.log
    stderr $LOGDIR/lap.initial.log

    book npages lapNewRuns -var N
    if ($N == 0) break
    if ($NETWORK == 0) break


    book getpage lapNewRuns 0 -var lapNewPageName -key pantaskState INIT

    $lap_NewPage ++
    if ($lap_NewPage >= $N) set lap_NewPage = 0

    if ("$lapNewPageName" == "NULL") break



    book setword lapNewRuns $lapNewPageName pantaskState RUN
    book getword lapNewRuns $lapNewPageName lap_id -var LAP_ID
    book getword lapNewRuns $lapNewPageName dbname -var DBNAME

    option $LAP_ID

    $run = lap_science.pl --chip_mode --dbname $DBNAME --lap_id $LAP_ID
    
    command $run

  end

  # success
  task.exit  0
    process_exit lapNewRuns $options:0 0
    if ($VERBOSE > 2)
      book listbook lapNewRuns
    end
  end
  # locked list
  task.exit    default
    process_exit lapNewRuns $options:0 0
    showcommand failure
  end
  task.exit    crash
    process_exit lapNewRuns $options:0 0
    showcommand crash
  end
  #operation times out?
  task.exit    timeout
    process_exit lapNewRuns $options:0 0
    showcommand timeout
  end
end



task           lap.monitor.load
  host         local
  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  task.exec
    stdout NULL
    stderr $LOGDIR/lap.load.log

    $run = laptool -pendingrun -state run

    if ($lapSeq:n == 0)
      break
    else 
      option $lapSeq:$lap_N
      $run = $run -seq_id $lapSeq:$lap_N
      $lap_N ++
      if ($lap_N >= $lapSeq:n) set lap_N = 0
    end

    if ($DB:n == 0)
      option DEFAULT
    else
      option $DB:$lap_DB
      $run = $run -dbname $DB:$lap_DB
      $lap_DB ++
      if ($lap_DB >= $DB:n) set lap_DB = 0
    end

    add_poll_labels run
    command $run
  end
  # success
  task.exit  0
    ipptool2book stdout lapRunRuns -uniq -key lap_id -setword dbname $options:0 -setword pantaskState INIT
    
    process_cleanup lapRunRuns

    if ($VERBOSE > 2)
      book listbook lapRunRuns
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

task           lap.monitor.run
  host         local
  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 3600
  active       false
# This can probably be increased and spread over hosts in the future.
  npending     4            

  task.exec
    stdout $LOGDIR/lap.monitor.log
    stderr $LOGDIR/lap.monitor.log

    book npages lapRunRuns -var N

    if ($N == 0) break
    if ($NETWORK == 0) break


    book getpage lapRunRuns 0 -var lapRunPageName -key pantaskState INIT

    $lap_RunPage ++
    if ($lap_RunPage >= $N) set lap_RunPage = 0

    if ("$lapRunPageName" == "NULL") break

    book setword lapRunRuns $lapRunPageName pantaskState RUN
    book getword lapRunRuns $lapRunPageName lap_id -var LAP_ID
    book getword lapRunRuns $lapRunPageName dbname -var DBNAME

    option $LAP_ID

    $run = lap_science.pl --monitor_mode --dbname $DBNAME --lap_id $LAP_ID

    command $run

  end

  # success
  task.exit  0
    process_exit lapRunRuns $options:0 0
    if ($VERBOSE > 2)

      book listbook lapRunRuns
    end
  end
  # locked list
  task.exit    default
    process_exit lapRunRuns $options:0 0
    showcommand failure
  end
  task.exit    crash
    process_exit lapRunRuns $options:0 0
    showcommand crash
  end
  #operation times out?
  task.exit    timeout
    process_exit lapRunRuns $options:0 0
    showcommand timeout
  end
end



task           lap.cleanup.load
  host         local
  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  active       false
  npending     1

  task.exec
    stdout NULL
    stderr $LOGDIR/lap.load.log

    $run = laptool -pendingrun -state full

    if ($lapSeq:n != 0)
      option $lapSeq:$lap_N
      $run = $run -seq_id $lapSeq:$lap_N
      $lap_N ++
      if ($lap_N >= $lapSeq:n) set lap_N = 0
    end

    if ($DB:n == 0)
      option DEFAULT
    else
      option $DB:$lap_DB
      $run = $run -dbname $DB:$lap_DB
      $lap_DB ++
      if ($lap_DB >= $DB:n) set lap_DB = 0
    end

    add_poll_labels run

    command $run
  end
  # success
  task.exit  0
    ipptool2book stdout lapDoneRuns -uniq -key lap_id  -setword dbname $options:0 -setword pantaskState INIT

    process_cleanup lapDoneRuns
    if ($VERBOSE > 2)
      book listbook lapRuns
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

task           lap.cleanup.run
  host         local
  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 3600
  active       false
# This can probably be increased and spread over hosts in the future.
  npending     1            

  task.exec
    stdout $LOGDIR/lap.cleanup.log
    stderr $LOGDIR/lap.cleanup.log

    book npages lapDoneRuns -var N
    if ($N == 0) break
    if ($NETWORK == 0) break


    book getpage lapDoneRuns 0 -var lapDonePageName -key pantaskState INIT

    $lap_DonePage ++
    if ($lap_DonePage >= $N) set lap_DonePage = 0

    if ("$lapDonePageName" == "NULL") break

    book setword lapDoneRuns $lapDonePageName pantaskState RUN
    book getword lapDoneRuns $lapDonePageName lap_id -var LAP_ID
    book getword lapDoneRuns $lapDonePageName dbname -var DBNAME

    option $LAP_ID
    $run = lap_science.pl --cleanup_mode --dbname $DBNAME --lap_id $LAP_ID
    if ("$LAP_QUEUE" != "NULL") 
      $run = $run --queue_list $LAP_QUEUE
    end
    

    command $run

  end

  # success
  task.exit  0
    process_exit lapDoneRuns $options:0 0
    if ($VERBOSE > 2)
      book listbook lapDoneRuns
    end
  end
  # locked list
  task.exit    default
    process_exit lapDoneRuns $options:0 0
    showcommand failure
  end
  task.exit    crash
    process_exit lapDoneRuns $options:0 0
    showcommand crash
  end
  #operation times out?
  task.exit    timeout
    process_exit lapDoneRuns $options:0 0
    showcommand timeout
  end
end
