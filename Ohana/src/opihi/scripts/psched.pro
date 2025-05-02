
controller exit true
# controller host add kiawe
$Ntest = 0
# controller host add alala
# verbose on
pulse 1000
controller pulse 1000

macro load.machines
  if ($0 != 2)
    echo "load.machines (nmach)"
    break
  end

  for i 0 $1
    $n = $i + 1
    sprintf host "po%02d" $n
    controller host add $host
  end
end

task test
  command partest
  # polling period is no longer valid: we check for completed controller tasks
  # correction: still valid for local tasks
  periods -poll 0.20
  periods -exec 0.0001
  periods -timeout 10.0
  nmax 1024
#  nmax 100
  host anyhost

  # stdout / stderr lines on named queues
  task.exit 0
    # echo "task exit 0"
    queuedelete stdout
    queuedelete stderr
    date date
    queuepush done "$date"
    $Ntest ++
#   memory leaks
#   queuesize stdout -var Nstdout
#    for i 0 $Nstdout
#      queuepop stdout -var line
#      queuepush results "$line"
#    end
  end

  task.exit 1
    # echo "task exit 1"
    queuesize stdout
    queuesize stderr
  end

  task.exit crash
    echo "crashed job"
  end

  task.exit timeout
    output timeout.log
    echo $stdout
    output stdout
  end
end

# pulse == 100ms
# poll/exit = 0.2  : 29 sec / 100 jobs
# poll/exit = 0.1  : 20 sec / 100 jobs
# poll/exit = 0.05 : 17 sec / 100 jobs
# poll/exit = 0.01 : 18 sec / 100 jobs

# pulse == 10ms
# poll/exit = 0.2  : 20 sec / 100 jobs
# poll/exit = 0.10 : 12 sec / 100 jobs
# poll/exit = 0.05 : 12 sec / 100 jobs
# poll/exit = 0.01 :  9 sec / 100 jobs

# we are limited here by how quickly we can send data to the 
# controller.  this is limited by the occasional 'CheckSystem'
# loops, with ~40ms minimum.

# seems to be faster on po01 from kiawe (less interference?)

# pulse == 1ms, controller pulse == 1ms
# poll/exit = 0.01 :  3 sec / 100 jobs
# 2 mach, 3 sec
# 4 mach, 3 sec
# 8 mach, 3 sec

# 16 machines, 500 jobs, 13 sec: 26ms / job
# 32 machines, 1024 jobs, 26 sec: 26ms / job
# job harvesting rate is still the limitation.  Each job harvest requires:
#  - jobstack exit
#  - stdout
#  - stderr
#  - delete
#  - jobstack crash

