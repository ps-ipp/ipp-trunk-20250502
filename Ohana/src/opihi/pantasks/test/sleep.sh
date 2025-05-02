
# a basic task which just runs 'sleep'
task	       basic
  command      sleep 10
  host         local

  periods      -poll 0.1
  periods      -exec 0.2
  periods      -timeout 20
  npending 5
  
  stdout tmp.txt
  stderr tmp.txt

  task.exec
    echo "create command"
  end

  # success
  task.exit    0
    $Npass ++
    echo done sleep
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
