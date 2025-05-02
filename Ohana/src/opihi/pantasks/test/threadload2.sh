
$hostname = `hostname`

controller exit true
# controller host add $hostname -threads 0
controller host add $hostname -threads 3
controller host add $hostname -threads 3
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
    # output out.dat
    local i myvalue N
    queuesize stdout -var N
    for i 0 $N
      queuepop stdout -var myvalue
      # echo $myvalue
    end
    queuesize stderr -var N
    for i 0 $N
      queuepop stderr -var myvalue
      # echo $myvalue
    end
    # output stdout
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
    # output out.dat
    local i myvalue N
    queuesize stdout -var N
    for i 0 $N
      queuepop stdout -var myvalue
      # echo $myvalue
    end
    queuesize stderr -var N
    for i 0 $N
      queuepop stderr -var myvalue
      # echo $myvalue
    end
    # output stdout
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
    # output out.dat
    local i myvalue N
    queuesize stdout -var N
    for i 0 $N
      queuepop stdout -var myvalue
      # echo $myvalue
    end
    queuesize stderr -var N
    for i 0 $N
      queuepop stderr -var myvalue
      # echo $myvalue
    end
    # output stdout
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
