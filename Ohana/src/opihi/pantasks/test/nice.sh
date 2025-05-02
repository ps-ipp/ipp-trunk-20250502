
# a basic task which just runs 'sleep 10'
task	       nicetask_local
  command      sleep 10
  host         local
  nice         10

  periods      -poll 0.1
  periods      -exec 1.0
  periods      -timeout 20
  active       true
  npending 5
  
  stdout tmp.txt
  stderr tmp.txt

  task.exec
    echo "nicetask_local start"
  end

  # success
  task.exit    0
    echo "nicetask_local stop"
  end

  # default exit status
  task.exit    default
    echo       "failure: exit status: $EXIT"
  end

  # operation times out?
  task.exit    timeout
    echo       "timeout"
  end
end

# a basic task which just runs 'sleep 10'
task	       meantask_local
  command      sleep 10
  host         local

  periods      -poll 0.1
  periods      -exec 1.0
  periods      -timeout 20
  active       true
  npending 5
  
  stdout tmp.txt
  stderr tmp.txt

  task.exec
    echo "meantask_local start"
  end

  # success
  task.exit    0
    echo "meantask_local stop"
  end

  # default exit status
  task.exit    default
    echo       "failure: exit status: $EXIT"
  end

  # operation times out?
  task.exit    timeout
    echo       "timeout"
  end
end

