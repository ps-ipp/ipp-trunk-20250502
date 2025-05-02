
# a basic task which just runs 'sleep'
task	       newtask
  # command      sleep 1
  command      echo hi
  host         local

  periods      -poll 0.01
  periods      -exec 0.01
  periods      -timeout 20
  npending 5
  
  stdout tmp.txt
  stderr tmp.txt

  task.exec
    # echo "create command"
  end

  # success
  task.exit    0
    $Npass ++
    # echo done sleep
    # status
    # halt
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
