
## a basic test of memory allocation
## test of 'exec failure'

exec rm -f tmp.txt
verbose off
$Npass = 0
$Ntry = 0
if ($?VERBOSE == 0)
  $VERBOSE = 4
end

# a basic task which just runs 'echo'
task	       basic
  host         local

  periods      -poll 0.005
  periods      -exec 0.005
  periods      -timeout 2
  
  stdout tmp.txt
  stderr tmp.txt

  task.exec
    $Ntry ++
    if ($Ntry > -1) break

    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"
    echo "this is a line in the task exec code"

    command echo "$Npass : test line"
  end

  # success
  task.exit    0
    $Npass ++
  end

  # default exit status
  task.exit    default
    echo       "basic: exit status: $EXIT"
  end

  # operation times out?
  task.exit    timeout
    echo       "basic: timeout"
  end
end

macro memcheck.init
 list word -x "ps -p $PID -o rss"
 $startmem = $word:1
end
macro memcheck
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1
 if ($VERBOSE > 3)
   echo growth: {$endmem - $startmem}
 end
end

macro done
 stop
 date -var dtime -seconds -reftime $start
 if ($VERBOSE > 3)
  status
 end

 # this task generates no output files
 if ($VERBOSE > 3)
   echo "task test speed: {$dtime/$Ntry} sec/tasks"
 end
 ### this test depends on the value of periods -exec above
 if ($dtime/$Ntry > 0.010) 
   echo "tasks tests running too slow: {$dtime/$Ntry}"
 end

 memcheck

 if ($endmem - $startmem > 30)
   $PASS = 0
   echo "failed memcheck"
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end


memcheck.init
echo "starting memcheck"
echo "wait a few seconds and type 'done'"

run
date -var start -seconds
