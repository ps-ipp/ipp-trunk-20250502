
$hostname = `hostname`

controller exit true
# controller host add $hostname -threads 0
controller host add $hostname -threads 3

# a basic task which just runs 'sleep'
task	       basic
  command      ls -lRrt /tmp 
  host         anyhost

  periods      -poll 0.1
  periods      -exec 0.5
  periods      -timeout 20
  npending 10
  
  stdout tmp.txt
  stderr tmp.txt

  task.exec
    # echo "create command"
  end

  # success
  task.exit    0
    $Npass ++
    # echo done sleep
  end

  # default exit status
  task.exit    default
    echo       "basic: exit status: $JOB_STATUS"
  end

  # operation times out?
  task.exit    timeout
    echo       "basic: timeout"
  end
end

# a basic task which just runs 'sleep'
task	       bigger
  command      ls -lRrt /usr/bin
  host         anyhost

  periods      -poll 0.1
  periods      -exec 0.5
  periods      -timeout 20
  npending 10
  
  stdout tmp.txt
  stderr tmp.txt

  task.exec
    # echo "create command"
  end

  # success
  task.exit    0
    $Npass ++
    # echo done sleep
  end

  # default exit status
  task.exit    default
    echo       "basic: exit status: $JOB_STATUS"
  end

  # operation times out?
  task.exit    timeout
    echo       "basic: timeout"
  end
end

# a basic task which just runs 'sleep'
task	       biggest
  command      ls -lRrt /usr/lib
  host         anyhost

  periods      -poll 0.1
  periods      -exec 0.5
  periods      -timeout 20
  npending 10
  
  stdout tmp.txt
  stderr tmp.txt

  task.exec
    # echo "create command"
  end

  # success
  task.exit    0
    $Npass ++
    # echo done sleep
  end

  # default exit status
  task.exit    default
    echo       "basic: exit status: $JOB_STATUS"
  end

  # operation times out?
  task.exit    timeout
    echo       "basic: timeout"
  end
end
