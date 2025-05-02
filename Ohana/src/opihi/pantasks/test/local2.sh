
## a basic test of memory allocation

exec rm -f tmp.txt
verbose off
$Npass = 0

# a basic task which just runs 'echo'
task	       basic
  command      echo "a test line"
  host         local

  periods      -poll 0.1
  periods      -exec 0.2
  periods      -timeout 2
  
  stdout tmp.txt
  stderr tmp.txt

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
 echo growth: {$endmem - $startmem}
end

macro done
 stop
 date -var dtime -seconds -reftime $start
 if ($VERBOSE > 3)
  status
  exec wc -l tmp.txt
 end

 $answer = `wc -l tmp.txt`
 list word -split $answer
 if ($word:0 != $Npass) 
   echo "missing lines in tmp.txt: $word:0"
 end
 if ($Npass/$dtime < 4.2) 
   echo "tasks running too slow: {$Npass/$dtime}"
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
